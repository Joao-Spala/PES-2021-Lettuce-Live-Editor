#include "win32_mem.hpp"

#include <tlhelp32.h>
#include <algorithm>
#include <array>
#include <cctype>

namespace pes {

namespace {

constexpr DWORD kProcessAccess =
    PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION;

constexpr std::array<const wchar_t*, 5> kSupportedExes = {
    L"PES2021.exe",
    L"FL_2026.exe",
    L"FL_2025.exe",
    L"FL_2024.exe",
    L"FL_2023.exe",
};

std::string wide_to_utf8(const wchar_t* ws) {
    if (!ws || !ws[0]) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, ws, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) return {};
    std::string out(static_cast<size_t>(len - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws, -1, out.data(), len, nullptr, nullptr);
    return out;
}

bool find_game_process(uint32_t& out_pid, std::wstring& out_wexe) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE || !snap) return false;
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(PROCESSENTRY32W);
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            for (const wchar_t* target : kSupportedExes) {
                if (_wcsicmp(pe.szExeFile, target) == 0) {
                    out_pid = pe.th32ProcessID;
                    out_wexe = pe.szExeFile;
                    found = true;
                    break;
                }
            }
            if (found) break;
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

bool get_module_info(uint32_t pid, const std::wstring& mod_name, uint64_t& out_base, uint32_t& out_size) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE || !snap) return false;
    MODULEENTRY32W me{};
    me.dwSize = sizeof(MODULEENTRY32W);
    bool found = false;
    if (Module32FirstW(snap, &me)) {
        do {
            if (_wcsicmp(me.szModule, mod_name.c_str()) == 0) {
                out_base = reinterpret_cast<uint64_t>(me.modBaseAddr);
                out_size = static_cast<uint32_t>(me.modBaseSize);
                found = true;
                break;
            }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return found;
}

int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

} // namespace

bool Win32Memory::is_alive() const {
    if (!handle || !pid) return false;
    DWORD code = 0;
    if (!GetExitCodeProcess(handle, &code)) return false;
    return code == STILL_ACTIVE;
}

bool Win32Memory::attached() {
    if (!handle || !pid) return false;
    if (!is_alive()) {
        detach();
        return false;
    }
    return true;
}

bool Win32Memory::attach() {
    if (attached()) return true;
    access_denied = false;
    uint32_t found_pid = 0;
    std::wstring wexe;
    if (!find_game_process(found_pid, wexe)) return false;

    HANDLE h = OpenProcess(kProcessAccess, FALSE, found_pid);
    if (!h) {
        if (GetLastError() == ERROR_ACCESS_DENIED) access_denied = true;
        return false;
    }

    uint64_t base = 0;
    uint32_t sz = 0;
    if (!get_module_info(found_pid, wexe, base, sz) || base == 0) {
        CloseHandle(h);
        return false;
    }

    handle = h;
    pid = found_pid;
    exe_name = wide_to_utf8(wexe.c_str());
    module_base = base;
    module_size = sz;
    return true;
}

void Win32Memory::detach() {
    if (handle) {
        CloseHandle(handle);
        handle = nullptr;
    }
    pid = 0;
    exe_name.clear();
    module_base = 0;
    module_size = 0;
}

void Win32Memory::flush_icache(uint64_t addr, size_t size) const {
    if (handle && addr > 0x10000ULL && size > 0) {
        FlushInstructionCache(handle, reinterpret_cast<LPCVOID>(addr), size);
    }
}

bool Win32Memory::write_code_bytes(uint64_t addr, const void* data, size_t size) const {
    if (!handle || addr <= 0x10000ULL || !data || size == 0) return false;
    DWORD old_prot = 0;
    BOOL ok_p = VirtualProtectEx(
        handle,
        reinterpret_cast<LPVOID>(addr),
        size,
        PAGE_EXECUTE_READWRITE,
        &old_prot
    );
    bool ok_w = write_from(addr, data, size);
    if (ok_w) {
        FlushInstructionCache(handle, reinterpret_cast<LPCVOID>(addr), size);
    }
    if (ok_p) {
        DWORD dummy = 0;
        VirtualProtectEx(handle, reinterpret_cast<LPVOID>(addr), size, old_prot, &dummy);
    }
    return ok_w;
}

uint64_t Win32Memory::alloc_near(uint64_t target_addr, size_t size) const {
    if (!handle || target_addr <= 0x10000ULL || size == 0) return 0;
    const uint64_t base_aligned = target_addr & ~0xFFFFULL;
    constexpr uint64_t step = 0x10000ULL;
    constexpr uint64_t max_dist = 0x70000000ULL;
    for (uint64_t delta = step; delta < max_dist; delta += step) {
        const uint64_t cands[2] = {
            (base_aligned > delta) ? (base_aligned - delta) : 0ULL,
            base_aligned + delta
        };
        for (uint64_t cand : cands) {
            if (cand <= 0x10000ULL) continue;
            LPVOID ptr = VirtualAllocEx(
                handle,
                reinterpret_cast<LPVOID>(cand),
                size,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE
            );
            if (ptr) {
                uint64_t p = reinterpret_cast<uint64_t>(ptr);
                uint64_t diff = (p >= target_addr) ? (p - target_addr) : (target_addr - p);
                if (diff <= 0x7F000000ULL) {
                    return p;
                }
                free_mem(p);
            }
        }
    }
    return 0;
}

bool Win32Memory::free_mem(uint64_t addr) const {
    if (!handle || !addr) return false;
    return VirtualFreeEx(handle, reinterpret_cast<LPVOID>(addr), 0, MEM_RELEASE) != 0;
}

std::string Win32Memory::read_str(uint64_t addr, size_t max_len) const {
    if (max_len == 0) return {};
    std::string buf(max_len, '\0');
    if (!read_into(addr, buf.data(), max_len)) return {};
    size_t nul = buf.find('\0');
    if (nul != std::string::npos) buf.resize(nul);
    while (!buf.empty() && std::isspace(static_cast<unsigned char>(buf.back()))) buf.pop_back();
    size_t lead = 0;
    while (lead < buf.size() && std::isspace(static_cast<unsigned char>(buf[lead]))) ++lead;
    if (lead > 0) buf.erase(0, lead);
    return buf;
}

bool Win32Memory::write_str(uint64_t addr, std::string_view text, size_t max_len) const {
    if (max_len <= 1) return false;
    const size_t limit = max_len - 1;
    std::vector<uint8_t> out(max_len, 0);
    // Copy UTF-8 codepoints cleanly without ever splitting a multi-byte sequence
    size_t i = 0;
    size_t dst = 0;
    while (i < text.size()) {
        uint8_t lead = static_cast<uint8_t>(text[i]);
        size_t clen = 1;
        if ((lead & 0x80u) == 0) clen = 1;
        else if ((lead & 0xE0u) == 0xC0u) clen = 2;
        else if ((lead & 0xF0u) == 0xE0u) clen = 3;
        else if ((lead & 0xF8u) == 0xF0u) clen = 4;
        if (i + clen > text.size()) break;
        if (dst + clen > limit) break;
        std::memcpy(out.data() + dst, text.data() + i, clen);
        dst += clen;
        i += clen;
    }
    return write_from(addr, out.data(), max_len);
}

uint32_t Win32Memory::get_bits(uint64_t addr, int bit_start, int bit_len) const {
    uint32_t v = 0;
    if (!read_into(addr, &v, sizeof(v))) return 0;
    uint32_t mask = (bit_len >= 32) ? 0xFFFFFFFFu : ((1u << bit_len) - 1u);
    return (v >> bit_start) & mask;
}

bool Win32Memory::set_bits(uint64_t addr, int bit_start, int bit_len, uint32_t new_val) const {
    uint32_t v = 0;
    if (!read_into(addr, &v, sizeof(v))) return false;
    uint32_t mask = ((bit_len >= 32) ? 0xFFFFFFFFu : ((1u << bit_len) - 1u)) << bit_start;
    v = (v & ~mask) | ((new_val << bit_start) & mask);
    return write_u32(addr, v);
}

uint64_t Win32Memory::aob_scan_region(uint64_t start_addr, uint64_t end_addr, std::string_view pattern) const {
    if (end_addr <= start_addr) return 0;
    const uint64_t total = end_addr - start_addr;
    if (total > 0x4000000ULL) return 0;

    std::vector<int16_t> pat;
    pat.reserve(64);
    size_t pos = 0;
    while (pos < pattern.size()) {
        while (pos < pattern.size() && std::isspace(static_cast<unsigned char>(pattern[pos]))) ++pos;
        if (pos >= pattern.size()) break;
        size_t end = pos;
        while (end < pattern.size() && !std::isspace(static_cast<unsigned char>(pattern[end]))) ++end;
        std::string_view tok = pattern.substr(pos, end - pos);
        if (tok == "?" || tok == "??") {
            pat.push_back(-1);
        } else if (tok.size() == 2) {
            int hi = hex_nibble(tok[0]);
            int lo = hex_nibble(tok[1]);
            if (hi < 0 || lo < 0) return 0;
            pat.push_back(static_cast<int16_t>((hi << 4) | lo));
        } else {
            return 0;
        }
        pos = end;
    }
    if (pat.empty()) return 0;

    // Pick first fixed byte as anchor for fast memchr skipping
    size_t anchor_idx = 0;
    while (anchor_idx < pat.size() && pat[anchor_idx] < 0) ++anchor_idx;
    const size_t pat_len = pat.size();

    auto match_buf = [&](const uint8_t* data, size_t len) -> int64_t {
        if (len < pat_len) return -1;
        if (anchor_idx == pat_len) return 0; // all wildcards
        const uint8_t anchor_val = static_cast<uint8_t>(pat[anchor_idx]);
        const size_t max_start = len - pat_len;
        size_t search_pos = anchor_idx;
        while (search_pos <= max_start + anchor_idx) {
            const uint8_t* hit = static_cast<const uint8_t*>(
                std::memchr(data + search_pos, anchor_val, (max_start + anchor_idx) - search_pos + 1)
            );
            if (!hit) return -1;
            const size_t start = static_cast<size_t>(hit - data) - anchor_idx;
            bool ok = true;
            for (size_t k = 0; k < pat_len; ++k) {
                if (pat[k] >= 0 && data[start + k] != static_cast<uint8_t>(pat[k])) {
                    ok = false;
                    break;
                }
            }
            if (ok) return static_cast<int64_t>(start);
            search_pos = start + anchor_idx + 1;
        }
        return -1;
    };

    const size_t chunk_sz = (total <= 0x40000ULL) ? static_cast<size_t>(total) : 0x100000u;
    std::vector<uint8_t> buf(chunk_sz);
    uint64_t cur = start_addr;
    while (cur < end_addr) {
        size_t sz = static_cast<size_t>(std::min<uint64_t>(chunk_sz, end_addr - cur));
        if (read_into(cur, buf.data(), sz)) {
            int64_t found = match_buf(buf.data(), sz);
            if (found >= 0) return cur + static_cast<uint64_t>(found);
        }
        if (sz <= pat_len) break;
        cur += (sz - pat_len);
    }
    return 0;
}

} // namespace pes
