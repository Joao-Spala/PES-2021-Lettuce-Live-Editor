#include "self_update.hpp"

#include "../app_version.hpp"
#include "update_check.hpp"

#include <windows.h>
#include <bcrypt.h>
#include <winhttp.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>

namespace pes::selfupdate {

namespace {

std::wstring widen_ascii(std::string_view s) {
    return std::wstring(s.begin(), s.end());
}

bool is_dotted_version(std::string_view s) {
    if (s.empty() || s.size() > 31 || !std::isdigit(static_cast<unsigned char>(s[0]))) return false;
    for (unsigned char c : s) {
        if (!std::isdigit(c) && c != '.') return false;
    }
    return true;
}

int hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool write_whole_file(const std::wstring& path, const std::string& data) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    size_t done = 0;
    bool ok = true;
    while (ok && done < data.size()) {
        const DWORD chunk = static_cast<DWORD>(std::min<size_t>(data.size() - done, 1u << 24));
        DWORD written = 0;
        ok = WriteFile(h, data.data() + done, chunk, &written, nullptr) && written == chunk;
        done += written;
    }
    ok = ok && FlushFileBuffers(h);
    CloseHandle(h);
    if (!ok) DeleteFileW(path.c_str());
    return ok;
}

bool read_whole_file(const std::wstring& path, std::string& out) {
    out.clear();
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER sz{};
    bool ok = GetFileSizeEx(h, &sz) && sz.QuadPart >= 0 && static_cast<uint64_t>(sz.QuadPart) <= kMaxExeBytes;
    if (ok) {
        out.resize(static_cast<size_t>(sz.QuadPart));
        size_t done = 0;
        while (ok && done < out.size()) {
            DWORD got = 0;
            const DWORD chunk = static_cast<DWORD>(std::min<size_t>(out.size() - done, 1u << 24));
            ok = ReadFile(h, out.data() + done, chunk, &got, nullptr) && got == chunk;
            done += got;
        }
    }
    CloseHandle(h);
    if (!ok) out.clear();
    return ok;
}

bool file_exists(const std::wstring& p) {
    const DWORD a = GetFileAttributesW(p.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

// "<64 hex> <tamanho>" depois de "chave = "
bool parse_hash_line(std::string_view v, uint8_t hash[32], uint64_t& size) {
    while (!v.empty() && v.front() == ' ') v.remove_prefix(1);
    if (v.size() < 66 || !parse_hex(v.substr(0, 64), hash, 32) || v[64] != ' ') return false;
    std::string_view num = v.substr(65);
    while (!num.empty() && (num.back() == ' ' || num.back() == '\r')) num.remove_suffix(1);
    if (num.empty() || num.size() > 12) return false;
    uint64_t n = 0;
    for (unsigned char c : num) {
        if (!std::isdigit(c)) return false;
        n = n * 10 + static_cast<uint64_t>(c - '0');
    }
    if (n == 0 || n > kMaxExeBytes) return false;
    size = n;
    return true;
}

bool looks_like_pe64(const std::string& b) {
    if (b.size() < 0x200 || b[0] != 'M' || b[1] != 'Z') return false;
    uint32_t nt = 0;
    std::memcpy(&nt, b.data() + 0x3C, 4);
    if (nt < 0x40 || static_cast<size_t>(nt) + 0x18 > b.size()) return false;
    if (std::memcmp(b.data() + nt, "PE\0\0", 4) != 0) return false;
    uint16_t machine = 0;
    std::memcpy(&machine, b.data() + nt + 4, 2);
    return machine == 0x8664;
}

} // namespace

Source github_source() {
    Source s;
    const std::wstring repo = widen_ascii(kUpdateRepo);
    s.api_path = L"/repos/" + repo + L"/releases/latest";
    s.dl_prefix = L"/" + repo + L"/releases/download/";
    return s;
}

bool http_get(const Source& src, bool api, const std::wstring& path, const std::string& user_agent, size_t max_bytes,
              std::string& body, unsigned long* http_status) {
    body.clear();
    if (http_status) *http_status = 0;
    HINTERNET ses = WinHttpOpen(widen_ascii(user_agent).c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                                WINHTTP_NO_PROXY_BYPASS, 0);
    if (!ses) return false;
    // resolver, conectar, enviar, receber (ms). O exe tem uns 3 MB: o receber cobre a transferencia inteira por leitura.
    WinHttpSetTimeouts(ses, 5000, 8000, 15000, 30000);
    bool ok = false;
    HINTERNET con = WinHttpConnect(ses, (api ? src.api_host : src.dl_host).c_str(), src.port, 0);
    HINTERNET req = con ? WinHttpOpenRequest(con, L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                             src.secure ? WINHTTP_FLAG_SECURE : 0)
                        : nullptr;
    if (req) {
        DWORD opt = WINHTTP_DISABLE_COOKIES | WINHTTP_DISABLE_AUTHENTICATION;
        WinHttpSetOption(req, WINHTTP_OPTION_DISABLE_FEATURE, &opt, sizeof(opt));
        DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
        WinHttpSetOption(req, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy));
        const wchar_t* hdr = L"Accept: application/vnd.github+json, application/octet-stream\r\n";
        if (WinHttpSendRequest(req, hdr, static_cast<DWORD>(-1L), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(req, nullptr)) {
            DWORD status = 0, sz = sizeof(status);
            WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz,
                                WINHTTP_NO_HEADER_INDEX);
            if (http_status) *http_status = status;
            if (status == 200) {
                ok = true;
                for (;;) {
                    DWORD avail = 0;
                    if (!WinHttpQueryDataAvailable(req, &avail)) { ok = false; break; }
                    if (avail == 0) break;
                    if (body.size() + avail > max_bytes) { ok = false; break; }
                    const size_t old = body.size();
                    body.resize(old + avail);
                    DWORD got = 0;
                    if (!WinHttpReadData(req, body.data() + old, avail, &got)) { ok = false; break; }
                    body.resize(old + got);
                    if (got == 0) break;
                }
            }
        }
        WinHttpCloseHandle(req);
    }
    if (con) WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
    if (!ok) body.clear();
    return ok;
}

bool sha256(const void* data, size_t len, uint8_t out[32]) {
    BCRYPT_ALG_HANDLE alg = nullptr;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return false;
    BCRYPT_HASH_HANDLE h = nullptr;
    bool ok = BCryptCreateHash(alg, &h, nullptr, 0, nullptr, 0, 0) == 0;
    const uint8_t* p = static_cast<const uint8_t*>(data);
    size_t left = len;
    while (ok && left > 0) {
        const ULONG chunk = static_cast<ULONG>(left > 0x40000000 ? 0x40000000 : left);
        ok = BCryptHashData(h, const_cast<PUCHAR>(p), chunk, 0) == 0;
        p += chunk;
        left -= chunk;
    }
    if (ok) ok = BCryptFinishHash(h, out, 32, 0) == 0;
    if (h) BCryptDestroyHash(h);
    BCryptCloseAlgorithmProvider(alg, 0);
    return ok;
}

bool parse_hex(std::string_view text, uint8_t* out, size_t out_len) {
    size_t a = 0, b = text.size();
    while (a < b && std::isspace(static_cast<unsigned char>(text[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(text[b - 1]))) --b;
    if (b - a != out_len * 2) return false;
    for (size_t i = 0; i < out_len; ++i) {
        const int hi = hex_nibble(text[a + i * 2]);
        const int lo = hex_nibble(text[a + i * 2 + 1]);
        if (hi < 0 || lo < 0) return false;
        out[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    return true;
}

bool verify_signature(const void* msg, size_t len, const uint8_t sig[64], const uint8_t pub_xy[64]) {
    uint8_t digest[32];
    if (!sha256(msg, len, digest)) return false;
    struct { BCRYPT_ECCKEY_BLOB hdr; uint8_t xy[64]; } blob{};
    blob.hdr.dwMagic = BCRYPT_ECDSA_PUBLIC_P256_MAGIC;
    blob.hdr.cbKey = 32;
    std::memcpy(blob.xy, pub_xy, 64);
    BCRYPT_ALG_HANDLE alg = nullptr;
    if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_ECDSA_P256_ALGORITHM, nullptr, 0) != 0) return false;
    BCRYPT_KEY_HANDLE key = nullptr;
    bool ok = BCryptImportKeyPair(alg, nullptr, BCRYPT_ECCPUBLIC_BLOB, &key, reinterpret_cast<PUCHAR>(&blob), sizeof(blob), 0) == 0;
    if (ok) {
        ok = BCryptVerifySignature(key, nullptr, digest, sizeof(digest), const_cast<PUCHAR>(sig), 64, 0) == 0;
        BCryptDestroyKey(key);
    }
    BCryptCloseAlgorithmProvider(alg, 0);
    return ok;
}

bool parse_manifest(std::string_view text, Manifest& out) {
    out = Manifest{};
    if (text.empty() || text.size() > kMaxManifestBytes) return false;
    bool ver = false, exe = false;
    int line_no = 0;
    size_t pos = 0;
    while (pos < text.size()) {
        size_t nl = text.find('\n', pos);
        if (nl == std::string_view::npos) nl = text.size();
        std::string_view line = text.substr(pos, nl - pos);
        pos = nl + 1;
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (line.empty()) continue;
        if (line_no++ == 0) {
            if (line != kManifestHeader) return false;
            continue;
        }
        const size_t eq = line.find(" = ");
        if (eq == std::string_view::npos) return false;
        const std::string_view k = line.substr(0, eq);
        const std::string_view v = line.substr(eq + 3);
        if (k == "version") {
            if (ver || !is_dotted_version(v)) return false;
            out.version = std::string(v);
            ver = true;
        } else if (k == "exe") {
            if (exe || !parse_hash_line(v, out.exe_hash, out.exe_size)) return false;
            exe = true;
        } else {
            return false;   // chave que esta versao do app nao conhece: formato de manifesto mais novo
        }
    }
    return line_no > 0 && ver && exe;
}

bool validate_update(const std::string& manifest_text, const std::string& sig_hex, const std::string& tag,
                     const std::string& exe_bytes, const std::string& current_version, const uint8_t pub_xy[64],
                     Manifest& out, const char** why) {
    const char* dummy = nullptr;
    if (!why) why = &dummy;
    uint8_t sig[64];
    if (!parse_hex(sig_hex, sig, sizeof(sig))) { *why = "malformed signature"; return false; }
    if (!verify_signature(manifest_text.data(), manifest_text.size(), sig, pub_xy)) { *why = "signature does not verify"; return false; }
    if (!parse_manifest(manifest_text, out)) { *why = "malformed manifest"; return false; }
    std::string_view t = tag;
    if (!t.empty() && (t.front() == 'v' || t.front() == 'V')) t.remove_prefix(1);
    if (out.version != t) { *why = "manifest version differs from the release tag"; return false; }
    const update::Version mv = update::parse_version(out.version);
    const update::Version cv = update::parse_version(current_version);
    if (!mv.valid || !cv.valid || update::compare_versions(mv, cv) <= 0) { *why = "not newer than the running version"; return false; }
    uint8_t h[32];
    if (exe_bytes.size() != out.exe_size || !sha256(exe_bytes.data(), exe_bytes.size(), h) || std::memcmp(h, out.exe_hash, 32) != 0) {
        *why = "executable differs from the signed manifest";
        return false;
    }
    if (!looks_like_pe64(exe_bytes)) { *why = "not a 64-bit executable"; return false; }
    *why = "";
    return true;
}

bool apply_update(const std::wstring& exe_path, const std::string& new_exe, unsigned long* error) {
    const std::wstring neu = exe_path + L".new", old = exe_path + L".old";
    auto fail = [&](DWORD e) {
        DeleteFileW(neu.c_str());
        if (error) *error = e;
        SetLastError(e);
        return false;
    };
    if (!file_exists(exe_path)) return fail(ERROR_FILE_NOT_FOUND);
    DeleteFileW(neu.c_str());
    // Um .old em uso e a imagem de uma atualizacao anterior que ainda esta rodando: nao pode ser apagado, e nada e trocado
    if (GetFileAttributesW(old.c_str()) != INVALID_FILE_ATTRIBUTES && !DeleteFileW(old.c_str())) return fail(GetLastError());
    if (!write_whole_file(neu, new_exe)) return fail(GetLastError());
    std::string back;
    if (!read_whole_file(neu, back) || back != new_exe) return fail(ERROR_CRC);
    // Renomear a imagem em execucao e permitido, apagar ou sobrescrever nao
    if (!MoveFileExW(exe_path.c_str(), old.c_str(), MOVEFILE_WRITE_THROUGH)) return fail(GetLastError());
    if (!MoveFileExW(neu.c_str(), exe_path.c_str(), MOVEFILE_WRITE_THROUGH)) {
        const DWORD e = GetLastError();
        MoveFileExW(old.c_str(), exe_path.c_str(), MOVEFILE_WRITE_THROUGH);   // volta o original
        return fail(e);
    }
    if (error) *error = 0;
    return true;
}

void finish_previous_update(const std::wstring& exe_path) {
    if (exe_path.empty()) return;
    DeleteFileW((exe_path + L".new").c_str());
    DeleteFileW((exe_path + L".old").c_str());   // falha enquanto a versao antiga ainda estiver fechando: fica para a proxima
}

std::wstring current_exe_path() {
    std::wstring buf(512, L'\0');
    for (int i = 0; i < 6; ++i) {
        const DWORD n = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
        if (n == 0) return {};
        if (n < buf.size() - 1) {
            buf.resize(n);
            return buf;
        }
        buf.resize(buf.size() * 2);
    }
    return {};
}

bool relaunch(const std::wstring& exe_path) {
    if (exe_path.empty()) return false;
    std::wstring cmd = L"\"" + exe_path + L"\" --after-update " + std::to_wstring(GetCurrentProcessId());
    std::wstring dir = exe_path;
    const size_t cut = dir.find_last_of(L"\\/");
    if (cut != std::wstring::npos) dir.resize(cut);
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(exe_path.c_str(), cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, cut == std::wstring::npos ? nullptr : dir.c_str(),
                        &si, &pi)) {
        return false;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

unsigned long parse_after_update_pid(const wchar_t* cmdline) {
    if (!cmdline) return 0;
    const wchar_t* p = wcsstr(cmdline, L"--after-update");
    if (!p) return 0;
    p += wcslen(L"--after-update");
    while (*p == L' ' || *p == L'=') ++p;
    unsigned long long pid = 0;
    int digits = 0;
    while (*p >= L'0' && *p <= L'9') {
        if (++digits > 12) return 0;
        pid = pid * 10 + static_cast<unsigned long long>(*p - L'0');
        ++p;
    }
    return pid > 0xFFFFFFFFull ? 0 : static_cast<unsigned long>(pid);
}

void wait_for_exit(unsigned long pid, unsigned long timeout_ms) {
    if (pid == 0 || pid == GetCurrentProcessId()) return;
    HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!h) return;   // ja encerrou
    WaitForSingleObject(h, timeout_ms);
    CloseHandle(h);
}

} // namespace pes::selfupdate
