#include "hooks.hpp"

#include <cstring>
#include <stdexcept>
#include <unordered_map>

namespace pes {

namespace {

int32_t check_rel32(int64_t rel) {
    if (rel < -0x80000000LL || rel > 0x7FFFFFFFLL) {
        throw std::overflow_error("rel32 out of range");
    }
    return static_cast<int32_t>(rel);
}

class CodeBuf {
public:
    explicit CodeBuf(uint64_t base_addr) : base(base_addr) { buf.reserve(160); }

    uint64_t cur_addr() const { return base + buf.size(); }

    void emit(std::initializer_list<uint8_t> bytes) {
        buf.insert(buf.end(), bytes.begin(), bytes.end());
    }

    void label(const char* name) {
        labels[name] = buf.size();
    }

    void emit_rip(std::initializer_list<uint8_t> prefix, uint64_t target_abs, std::initializer_list<uint8_t> imm_tail = {}) {
        buf.insert(buf.end(), prefix.begin(), prefix.end());
        uint64_t next_rip = cur_addr() + 4 + imm_tail.size();
        int32_t rel = check_rel32(static_cast<int64_t>(target_abs) - static_cast<int64_t>(next_rip));
        uint8_t raw[4];
        std::memcpy(raw, &rel, 4);
        buf.insert(buf.end(), raw, raw + 4);
        buf.insert(buf.end(), imm_tail.begin(), imm_tail.end());
    }

    void jmp_abs(uint64_t target_abs) {
        buf.push_back(0xE9);
        int32_t rel = check_rel32(static_cast<int64_t>(target_abs) - static_cast<int64_t>(cur_addr() + 4));
        uint8_t raw[4];
        std::memcpy(raw, &rel, 4);
        buf.insert(buf.end(), raw, raw + 4);
    }

    void jcc8(uint8_t opcode, const char* lbl) {
        buf.push_back(opcode);
        fixups_rel8.emplace_back(buf.size(), lbl);
        buf.push_back(0x00);
    }

    void jcc32(std::initializer_list<uint8_t> opcodes, const char* lbl) {
        buf.insert(buf.end(), opcodes.begin(), opcodes.end());
        fixups_rel32.emplace_back(buf.size(), lbl);
        for (int i = 0; i < 4; ++i) buf.push_back(0x00);
    }

    std::vector<uint8_t> finalize() {
        for (const auto& [pos, name] : fixups_rel8) {
            int64_t rel = static_cast<int64_t>(labels.at(name)) - static_cast<int64_t>(pos + 1);
            if (rel < -128 || rel > 127) throw std::overflow_error("rel8 overflow");
            buf[pos] = static_cast<uint8_t>(rel & 0xFF);
        }
        for (const auto& [pos, name] : fixups_rel32) {
            int32_t rel = check_rel32(static_cast<int64_t>(labels.at(name)) - static_cast<int64_t>(pos + 4));
            std::memcpy(&buf[pos], &rel, 4);
        }
        return buf;
    }

private:
    uint64_t base;
    std::vector<uint8_t> buf;
    std::unordered_map<std::string, size_t> labels;
    std::vector<std::pair<size_t, std::string>> fixups_rel8;
    std::vector<std::pair<size_t, std::string>> fixups_rel32;
};

struct StaleEntry {
    uint32_t rva;
    int32_t check_off;
    uint8_t check_byte;
    uint8_t orig[10];
    uint8_t orig_len;
    int16_t stale_byte; // -1 means 0xE9
};

bool iequals_pes2021(const std::string& s) {
    return _stricmp(s.c_str(), "PES2021.exe") == 0;
}

} // namespace

bool HookManager::init_state() {
    if (state_base && mem.attached()) return true;
    if (!mem.attached()) return false;
    uint64_t ptr = mem.alloc_near(mem.module_base, 0x2000);
    if (!ptr) return false;
    std::vector<uint8_t> zeros(0x2000, 0);
    if (!mem.write_from(ptr, zeros.data(), zeros.size())) {
        mem.free_mem(ptr);
        return false;
    }
    state_base = ptr;
    cave_cursor = ptr + 0x500;
    patches.clear();
    active_hooks.fill(false);
    return true;
}

uint64_t HookManager::alloc_cave(const std::vector<uint8_t>& code) {
    uint64_t addr = (cave_cursor + 15ULL) & ~15ULL;
    if (addr + code.size() > state_base + 0x2000ULL) return 0;
    if (!mem.write_from(addr, code.data(), code.size())) return 0;
    mem.flush_icache(addr, code.size());
    cave_cursor = addr + code.size();
    return addr;
}

bool HookManager::install_jmp(int hook_id, uint64_t site, uint64_t cave_addr, size_t orig_len) {
    if (!cave_addr || orig_len < 5) return false;
    std::vector<uint8_t> orig(orig_len);
    if (!mem.read_into(site, orig.data(), orig_len)) return false;
    // Anti-double-hook guard: never overwrite an existing JMP and corrupt original backup
    if (orig[0] == 0xE9) return false;

    int64_t rel = static_cast<int64_t>(cave_addr) - static_cast<int64_t>(site + 5);
    if (rel < -0x80000000LL || rel > 0x7FFFFFFFLL) return false;

    std::vector<uint8_t> patch(orig_len, 0x90);
    patch[0] = 0xE9;
    int32_t rel32 = static_cast<int32_t>(rel);
    std::memcpy(&patch[1], &rel32, 4);

    if (!mem.write_code_bytes(site, patch.data(), patch.size())) return false;
    patches[hook_id].push_back({site, std::move(orig)});
    return true;
}

uint64_t HookManager::scan(uint32_t rva_start, uint32_t rva_end, std::string_view pattern) const {
    return mem.aob_scan_region(mem.module_base + rva_start, mem.module_base + rva_end, pattern);
}

void HookManager::restore_stale_hook_bytes(int hook_num) {
    if (!iequals_pes2021(mem.exe_name) || !mem.attached() || !mem.module_base) return;

    static const std::unordered_map<int, std::vector<StaleEntry>> kStale = {
        {1, {
            {0xC6ED40, -19, 0xCB, {0x0F, 0x10, 0x02, 0x0F, 0x11, 0x01}, 6, -1},
            {0x8E24D7,  -7, 0x48, {0x48, 0x8B, 0x40, 0x2C, 0x48, 0x89, 0x02}, 7, -1},
            {0xDDCA5C,   7, 0x89, {0x0F, 0xB6, 0x80, 0x50, 0x01, 0x00, 0x00}, 7, -1},
            {0xF27462,   7, 0x89, {0x0F, 0xB6, 0x80, 0x50, 0x01, 0x00, 0x00}, 7, -1},
        }},
        {3, {
            {0x12E14A0,  6, 0x0F, {0x45, 0x8B, 0xC8, 0x83, 0xFA, 0x6C}, 6, -1},
        }},
        {4, {
            {0xBBFE79,  -8, 0x0F, {0x15}, 1, 0x7F},
        }},
        {5, {
            {0x20F9FD6,  7, 0x88, {0x49, 0x8B, 0xF8, 0x44, 0x0F, 0xB6, 0xE2}, 7, -1},
            {0xA3C787,   8, 0xF3, {0x8B, 0x44, 0x24, 0x40, 0x89, 0x44, 0x24, 0x40}, 8, -1},
        }},
        {6, {
            {0xA58EAC,   9, 0x48, {0x6B, 0x45, 0x10, 0x64, 0x48, 0x8B, 0x5C, 0x24, 0x50}, 9, -1},
        }},
    };

    auto it = kStale.find(hook_num);
    if (it == kStale.end()) return;
    for (const auto& e : it->second) {
        uint64_t addr = mem.module_base + e.rva;
        uint8_t b0 = mem.read_u8(addr);
        bool is_stale = (e.stale_byte >= 0) ? (b0 == static_cast<uint8_t>(e.stale_byte)) : (b0 == 0xE9);
        if (is_stale && mem.read_u8(addr + e.check_off) == e.check_byte) {
            mem.write_code_bytes(addr, e.orig, e.orig_len);
        }
    }
}

bool HookManager::verify_hook_alive(int hook_num) const {
    if (!active_hooks[hook_num] || !state_base || !mem.attached()) return false;
    auto it = patches.find(hook_num);
    if (it == patches.end() || it->second.empty()) return false;
    for (const auto& rec : it->second) {
        if (rec.orig.size() < 5) continue;
        uint8_t cur[5]{};
        if (!mem.read_into(rec.site, cur, 5)) return false;
        if (cur[0] != 0xE9) return false;
        int32_t rel = 0;
        std::memcpy(&rel, cur + 1, 4);
        uint64_t target = static_cast<uint64_t>(static_cast<int64_t>(rec.site + 5) + rel);
        if (target < state_base || target >= state_base + 0x2000ULL) return false;
    }
    return true;
}

bool HookManager::enable_hook_1_player() {
    if (active_hooks[1] && verify_hook_alive(1)) return true;
    active_hooks[1] = false;
    patches.erase(1);
    if (!init_state()) return false;
    restore_stale_hook_bytes(1);
    // Keep cave allocation bounded if re-hooking after external stomp
    if (cave_cursor > state_base + 0x1600ULL) {
        cave_cursor = state_base + 0x500ULL;
    }
    const uint64_t ptr_player = sym_addr(HookSym::ptrPlayer);
    int installed = 0;

    try {
        if (uint64_t s1 = scan(0xC6DC00, 0xC6FE00, "CB B8 02 00 00 00 0F 1F 40 00 66 0F 1F 84 00 00 00 00 00 0F 10 02 0F 11 01")) {
            uint64_t site = s1 + 0x13;
            uint64_t cave_addr = (cave_cursor + 15ULL) & ~15ULL;
            CodeBuf cb(cave_addr);
            cb.emit({0x83, 0xF8, 0x02});
            cb.jcc8(0x75, "code");
            cb.emit_rip({0x48, 0x89, 0x15}, ptr_player);
            cb.label("code");
            cb.emit({0x0F, 0x10, 0x02, 0x0F, 0x11, 0x01});
            cb.jmp_abs(site + 6);
            if (install_jmp(1, site, alloc_cave(cb.finalize()), 6)) ++installed;
        }
        if (uint64_t s2 = scan(0x8E1400, 0x8E3500, "48 8B 40 2C 48 89 02")) {
            uint64_t cave_addr = (cave_cursor + 15ULL) & ~15ULL;
            CodeBuf cb(cave_addr);
            cb.emit_rip({0x48, 0x89, 0x05}, ptr_player);
            cb.emit({0x48, 0x8B, 0x40, 0x2C, 0x48, 0x89, 0x02});
            cb.jmp_abs(s2 + 7);
            if (install_jmp(1, s2, alloc_cave(cb.finalize()), 7)) ++installed;
        }
        if (uint64_t s3 = scan(0xDDB900, 0xDDDB00, "0F B6 80 50 01 00 00 89 87 98 00 00 00")) {
            uint64_t cave_addr = (cave_cursor + 15ULL) & ~15ULL;
            CodeBuf cb(cave_addr);
            cb.emit({0x48, 0x85, 0xC0});
            cb.jcc8(0x74, "code");
            cb.emit_rip({0x48, 0x89, 0x05}, ptr_player);
            cb.label("code");
            cb.emit({0x0F, 0xB6, 0x80, 0x50, 0x01, 0x00, 0x00});
            cb.jmp_abs(s3 + 7);
            if (install_jmp(1, s3, alloc_cave(cb.finalize()), 7)) ++installed;
        }
        if (uint64_t s4 = scan(0xF26400, 0xF28500, "0F B6 80 50 01 00 00 89 45 67")) {
            uint64_t cave_addr = (cave_cursor + 15ULL) & ~15ULL;
            CodeBuf cb(cave_addr);
            cb.emit({0x48, 0x85, 0xC0});
            cb.jcc8(0x74, "code");
            cb.emit_rip({0x48, 0x89, 0x05}, ptr_player);
            cb.label("code");
            cb.emit({0x0F, 0xB6, 0x80, 0x50, 0x01, 0x00, 0x00});
            cb.jmp_abs(s4 + 7);
            if (install_jmp(1, s4, alloc_cave(cb.finalize()), 7)) ++installed;
        }
    } catch (...) {
        return false;
    }

    active_hooks[1] = installed > 0;
    return active_hooks[1];
}

bool HookManager::enable_hook_3_training() {
    if (active_hooks[3]) return true;
    if (!init_state()) return false;
    restore_stale_hook_bytes(3);
    uint64_t s = scan(0x12E0400, 0x12E2500, "45 8B C8 83 FA 6C 0F 8D 1E 01 00 00");
    if (!s) return false;
    try {
        uint64_t cfg = sym_addr(HookSym::cfgTrainingMultMode);
        uint64_t cave_addr = (cave_cursor + 15ULL) & ~15ULL;
        CodeBuf cb(cave_addr);
        cb.emit_rip({0x44, 0x8B, 0x0D}, cfg);
        cb.emit({0x45, 0x85, 0xC9});
        cb.jcc8(0x74, "orig");
        cb.emit({0x45, 0x85, 0xC0});
        cb.jcc8(0x7E, "orig");

        cb.emit({0x41, 0x83, 0xF9, 0x01});
        cb.jcc8(0x75, "m2");
        cb.emit({0x45, 0x6B, 0xC0, 0x02});
        cb.jcc8(0xEB, "clamp");

        cb.label("m2");
        cb.emit({0x41, 0x83, 0xF9, 0x02});
        cb.jcc8(0x75, "m3");
        cb.emit({0x45, 0x6B, 0xC0, 0x05});
        cb.jcc8(0xEB, "clamp");

        cb.label("m3");
        cb.emit({0x41, 0x83, 0xF9, 0x03});
        cb.jcc8(0x75, "m4");
        cb.emit({0x45, 0x6B, 0xC0, 0x0A});
        cb.jcc8(0xEB, "clamp");

        cb.label("m4");
        cb.emit({0x41, 0x83, 0xF9, 0x04});
        cb.jcc8(0x75, "m5");
        cb.emit({0x45, 0x6B, 0xC0, 0x32});
        cb.jcc8(0xEB, "clamp");

        cb.label("m5");
        cb.emit({0x41, 0xB8, 0x10, 0x27, 0x00, 0x00});

        cb.label("clamp");
        cb.emit({0x41, 0x81, 0xF8, 0x10, 0x27, 0x00, 0x00});
        cb.jcc8(0x76, "orig"); // jbe (unsigned <= 10000)
        cb.emit({0x41, 0xB8, 0x10, 0x27, 0x00, 0x00});

        cb.label("orig");
        cb.emit({0x45, 0x8B, 0xC8, 0x83, 0xFA, 0x6C});
        cb.jmp_abs(s + 6);

        bool ok = install_jmp(3, s, alloc_cave(cb.finalize()), 6);
        active_hooks[3] = ok;
        return ok;
    } catch (...) {
        return false;
    }
}

bool HookManager::enable_hook_4_bal_cap() {
    if (active_hooks[4]) return true;
    if (!init_state()) return false;
    restore_stale_hook_bytes(4);
    uint64_t s = scan(0xBBEE00, 0xBC0F00, "0F BE 41 0A 2B C2 83 F8 15 77 09 48 63 C3");
    if (!s) return false;
    uint64_t site = s + 8;
    uint8_t orig = 0;
    if (!mem.read_into(site, &orig, 1)) return false;
    uint8_t patched = 0x7F;
    if (!mem.write_code_bytes(site, &patched, 1)) return false;
    patches[4].push_back({site, {orig}});
    active_hooks[4] = true;
    return true;
}

bool HookManager::enable_hook_5_match() {
    if (active_hooks[5]) return true;
    if (!init_state()) return false;
    restore_stale_hook_bytes(5);
    bool ok_clk = false;

    try {
        if (uint64_t s_hud = scan(0x20F8F00, 0x20FB000, "49 8B F8 44 0F B6 E2 88 55 B7")) {
            uint64_t cave_addr = (cave_cursor + 15ULL) & ~15ULL;
            CodeBuf cb(cave_addr);
            cb.emit_rip({0x48, 0x89, 0x0D}, sym_addr(HookSym::ptrMatchHud));
            cb.emit({0x50, 0x48, 0x8B, 0x41, 0x38});
            cb.emit_rip({0x48, 0x89, 0x05}, sym_addr(HookSym::ptrMatchCore));
            cb.emit({0x58, 0x49, 0x8B, 0xF8, 0x44, 0x0F, 0xB6, 0xE2});
            cb.jmp_abs(s_hud + 7);
            install_jmp(5, s_hud, alloc_cave(cb.finalize()), 7);
        }

        if (uint64_t s_clk = scan(0xA3B700, 0xA3D800, "8B 44 24 40 89 44 24 40 F3 0F 10 44 24 40 48 83 C4 30 5B C3")) {
            uint64_t cave_addr = (cave_cursor + 15ULL) & ~15ULL;
            CodeBuf cb(cave_addr);
            cb.emit({0x8B, 0x44, 0x24, 0x40, 0x51});
            cb.emit_rip({0x48, 0x8B, 0x0D}, sym_addr(HookSym::ptrMatchState));
            cb.emit({0x48, 0x85, 0xC9});
            cb.jcc8(0x74, "done");
            cb.emit({0x48, 0x8D, 0x89, 0x68, 0x11, 0x00, 0x00, 0x48, 0x39, 0xCB});
            cb.jcc8(0x75, "done");
            cb.emit_rip({0x89, 0x05}, sym_addr(HookSym::valProtectedMatchTicks));
            cb.emit_rip({0x83, 0x3D}, sym_addr(HookSym::cfgFreezeClock), {0x01});
            cb.jcc8(0x75, "done");
            cb.emit_rip({0xF3, 0x0F, 0x10, 0x05}, sym_addr(HookSym::valFrozenTotalTicks));
            cb.emit_rip({0xF3, 0x0F, 0x5C, 0x05}, sym_addr(HookSym::valProtectedMatchTicks));
            cb.emit({0xF3, 0x0F, 0x11, 0x43, 0x0C});
            cb.label("done");
            cb.emit({0x59, 0x89, 0x44, 0x24, 0x40});
            cb.jmp_abs(s_clk + 8);
            ok_clk = install_jmp(5, s_clk, alloc_cave(cb.finalize()), 8);
        }
    } catch (...) {
        ok_clk = false;
    }

    if (!ok_clk) {
        auto it = patches.find(5);
        if (it != patches.end()) {
            for (const auto& rec : it->second) {
                mem.write_code_bytes(rec.site, rec.orig.data(), rec.orig.size());
            }
            patches.erase(it);
        }
    }

    active_hooks[5] = ok_clk;
    return ok_clk;
}

bool HookManager::enable_hook_6_stamina() {
    if (active_hooks[6]) return true;
    if (!init_state()) return false;
    restore_stale_hook_bytes(6);
    uint64_t s = scan(0xA57E00, 0xA59F00, "6B 45 10 64 48 8B 5C 24 50");
    if (!s) return false;

    try {
        uint64_t cave_addr = (cave_cursor + 15ULL) & ~15ULL;
        CodeBuf cb(cave_addr);
        uint64_t cfg = sym_addr(HookSym::cfgStaminaMode);
        uint64_t mgr = sym_addr(HookSym::ptrMatchMgr);

        cb.emit_rip({0x83, 0x3D}, cfg, {0x00});
        cb.jcc32({0x0F, 0x84}, "orig");
        cb.emit_rip({0x48, 0x8B, 0x0D}, mgr);
        cb.emit({0x48, 0x85, 0xC9});
        cb.jcc32({0x0F, 0x84}, "orig");
        cb.emit({0x31, 0xD2});

        cb.label("loop");
        cb.emit({0x48, 0x89, 0xD0, 0x48, 0x05, 0xF3, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x04, 0x80, 0x48, 0x39, 0x1C, 0xC1});
        cb.jcc8(0x74, "found");
        cb.emit({0xFF, 0xC2, 0x83, 0xFA, 0x16});
        cb.jcc8(0x72, "loop");
        cb.jcc32({0xE9}, "orig");

        cb.label("found");
        cb.emit_rip({0x8B, 0x05}, cfg);
        cb.emit({0x83, 0xFA, 0x0B});
        cb.jcc8(0x73, "away");

        cb.emit({0x83, 0xF8, 0x01});
        cb.jcc8(0x74, "give100");
        cb.emit({0x83, 0xF8, 0x03});
        cb.jcc8(0x74, "give100");
        cb.emit({0x83, 0xF8, 0x04});
        cb.jcc8(0x74, "give100");
        cb.emit({0x83, 0xF8, 0x05});
        cb.jcc8(0x74, "drain");
        cb.jcc8(0xEB, "orig");

        cb.label("away");
        cb.emit({0x83, 0xF8, 0x02});
        cb.jcc8(0x74, "give100");
        cb.emit({0x83, 0xF8, 0x03});
        cb.jcc8(0x74, "give100");
        cb.emit({0x83, 0xF8, 0x05});
        cb.jcc8(0x74, "give100");
        cb.emit({0x83, 0xF8, 0x04});
        cb.jcc8(0x74, "drain");
        cb.jcc8(0xEB, "orig");

        cb.label("give100");
        cb.emit({0x48, 0x8B, 0x5C, 0x24, 0x50, 0x48, 0x8B, 0x7C, 0x24, 0x58, 0xB8, 0x64, 0x00, 0x00, 0x00, 0x48, 0x83, 0xC4, 0x30, 0x5D, 0xC3});

        cb.label("drain");
        cb.emit({0x48, 0x8B, 0x5C, 0x24, 0x50, 0x48, 0x8B, 0x7C, 0x24, 0x58, 0xB8, 0x05, 0x00, 0x00, 0x00, 0x48, 0x83, 0xC4, 0x30, 0x5D, 0xC3});

        cb.label("orig");
        cb.emit({0x6B, 0x45, 0x10, 0x64, 0x48, 0x8B, 0x5C, 0x24, 0x50});
        cb.jmp_abs(s + 9);

        bool ok = install_jmp(6, s, alloc_cave(cb.finalize()), 9);
        active_hooks[6] = ok;
        return ok;
    } catch (...) {
        return false;
    }
}

void HookManager::disable_all() {
    if (mem.attached()) {
        bool all_restored = true;
        for (const auto& [hid, list] : patches) {
            for (const auto& rec : list) {
                bool mine = true;
                if (rec.orig.size() >= 5 && state_base) {
                    uint8_t cur[5]{};
                    if (mem.read_into(rec.site, cur, 5) && cur[0] == 0xE9) {
                        int32_t rel = 0;
                        std::memcpy(&rel, cur + 1, 4);
                        uint64_t target = static_cast<uint64_t>(static_cast<int64_t>(rec.site + 5) + rel);
                        if (target < state_base || target >= state_base + 0x2000ULL) {
                            mine = false; // another instance currently owns this JMP
                        }
                    }
                }
                if (mine && !mem.write_code_bytes(rec.site, rec.orig.data(), rec.orig.size())) {
                    all_restored = false;
                }
            }
        }
        if (state_base && all_restored) {
            // Allow any in-flight game thread inside a cave to return before freeing
            Sleep(80);
            mem.free_mem(state_base);
        }
    }
    patches.clear();
    active_hooks.fill(false);
    state_base = 0;
    cave_cursor = 0;
}

} // namespace pes
