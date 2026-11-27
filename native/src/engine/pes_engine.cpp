#include "pes_engine.hpp"
#include "app_dirs.hpp"
#include "backup_retention.hpp"
#include "skill_presets.hpp"
#include <unordered_set>
#include <filesystem>

#include "../../third_party/json/json.hpp"

#include <tlhelp32.h>
#include <shlobj.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>

namespace pes {

static std::filesystem::path fs_path_from_utf8(std::string_view s);

namespace {

constexpr int kUndoMaxDepth = 10;

bool detect_legacy_workbench_running() {
    static double last_check = -10.0;
    static bool cached = false;
    using clk = std::chrono::steady_clock;
    double now = std::chrono::duration<double>(clk::now().time_since_epoch()).count();
    if (now - last_check < 3.0) return cached;
    last_check = now;
    cached = false;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE || !snap) return false;
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"PES 2021 Workbench.exe") == 0) {
                cached = true;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return cached;
}

std::string extract_cstr(const uint8_t* data, size_t max_len) {
    size_t len = 0;
    while (len < max_len && data[len] != 0) ++len;
    while (len > 0 && std::isspace(static_cast<unsigned char>(data[len - 1]))) --len;
    size_t start = 0;
    while (start < len && std::isspace(static_cast<unsigned char>(data[start]))) ++start;
    return std::string(reinterpret_cast<const char*>(data + start), len - start);
}

// Fast UTF-8 Latin-1/Latin-Extended accent stripper -> ASCII lowercase
char fold_latin_codepoint(uint32_t cp) {
    if (cp < 128) return static_cast<char>(std::tolower(static_cast<unsigned char>(cp)));
    switch (cp) {
        case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5:
        case 0xE0: case 0xE1: case 0xE2: case 0xE3: case 0xE4: case 0xE5:
        case 0x100: case 0x101: case 0x102: case 0x103: case 0x104: case 0x105: return 'a';
        case 0xC7: case 0xE7: case 0x106: case 0x107: case 0x10C: case 0x10D: return 'c';
        case 0xD0: case 0xF0: case 0x10E: case 0x10F: case 0x110: case 0x111: return 'd';
        case 0xC8: case 0xC9: case 0xCA: case 0xCB:
        case 0xE8: case 0xE9: case 0xEA: case 0xEB:
        case 0x112: case 0x113: case 0x118: case 0x119: case 0x11A: case 0x11B: return 'e';
        case 0x11E: case 0x11F: return 'g';
        case 0xCC: case 0xCD: case 0xCE: case 0xCF:
        case 0xEC: case 0xED: case 0xEE: case 0xEF:
        case 0x130: case 0x131: return 'i';
        case 0x141: case 0x142: return 'l';
        case 0xD1: case 0xF1: case 0x143: case 0x144: case 0x147: case 0x148: return 'n';
        case 0xD2: case 0xD3: case 0xD4: case 0xD5: case 0xD6: case 0xD8:
        case 0xF2: case 0xF3: case 0xF4: case 0xF5: case 0xF6: case 0xF8:
        case 0x150: case 0x151: return 'o';
        case 0x158: case 0x159: return 'r';
        case 0x15A: case 0x15B: case 0x15E: case 0x15F: case 0x160: case 0x161: return 's';
        case 0x162: case 0x163: case 0x164: case 0x165: return 't';
        case 0xD9: case 0xDA: case 0xDB: case 0xDC:
        case 0xF9: case 0xFA: case 0xFB: case 0xFC:
        case 0x16E: case 0x16F: case 0x170: case 0x171: return 'u';
        case 0xDD: case 0xFD: case 0xFF: return 'y';
        case 0x179: case 0x17A: case 0x17B: case 0x17C: case 0x17D: case 0x17E: return 'z';
        default: return 0;
    }
}

bool hex_to_bytes(std::string_view hex, std::vector<uint8_t>& out) {
    if (hex.size() % 2 != 0) return false;
    out.resize(hex.size() / 2);
    auto nib = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i < out.size(); ++i) {
        int hi = nib(hex[i * 2]);
        int lo = nib(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) return false;
        out[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    return true;
}

void set_bits_in_buf(uint8_t* buf, size_t len, size_t off, int bit, int width, uint32_t val) {
    if (!buf || width <= 0 || width > 32 || bit < 0 || bit >= 32) return;
    if (off + 4 > len) {
        // Fallback byte-level for tail offsets near end of buffer
        uint32_t cur = 0;
        size_t avail = (off < len) ? std::min<size_t>(4, len - off) : 0;
        if (avail == 0) return;
        std::memcpy(&cur, buf + off, avail);
        uint32_t mask = (width == 32) ? 0xFFFFFFFFu : ((1u << width) - 1u);
        cur = (cur & ~(mask << bit)) | ((val & mask) << bit);
        std::memcpy(buf + off, &cur, avail);
        return;
    }
    uint32_t cur = 0;
    std::memcpy(&cur, buf + off, 4);
    uint32_t mask = (width == 32) ? 0xFFFFFFFFu : ((1u << width) - 1u);
    cur = (cur & ~(mask << bit)) | ((val & mask) << bit);
    std::memcpy(buf + off, &cur, 4);
}

void write_cstr_in_buf(uint8_t* buf, size_t len, size_t off, size_t max_bytes, std::string_view text) {
    if (!buf || off >= len || max_bytes == 0) return;
    size_t cap = std::min(max_bytes, len - off);
    std::memset(buf + off, 0, cap);
    if (cap <= 1) return;
    size_t n = std::min(text.size(), cap - 1);
    if (n > 0) std::memcpy(buf + off, text.data(), n);
}

std::string cur_iso_datetime() {
    std::time_t t = std::time(nullptr);
    std::tm tm_buf{};
    localtime_s(&tm_buf, &t);
    char b[32];
    std::snprintf(b, sizeof(b), "%04d-%02d-%02d %02d:%02d:%02d",
                  tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
                  tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
    return b;
}

PackPlayerData extract_pack_player_from_buf(const uint8_t* buf, std::string_view club_name) {
    PackPlayerData p;
    auto [ovr, reg_pos] = calc_ovr_from_buf(buf, data::PLAYER_RECORD_SIZE);
    std::memcpy(&p.pid, buf + 0x30, 4);
    p.name = extract_cstr(buf + 0x38, 0x2E);
    if (p.name.empty()) p.name = "?";
    p.name_norm = norm_text_utf8(p.name);
    p.shirt = extract_cstr(buf + 0x75, 0x12);
    p.club = std::string(club_name);
    p.ovr = ovr;
    p.reg_pos = reg_pos;
    uint16_t nat_raw = 0;
    std::memcpy(&nat_raw, buf + 0x144, 2);
    p.nationality = nat_raw & 0x1FF;
    p.age = buf[0x1C] & 0x3F;
    p.height = buf[0x00];
    p.weight = buf[0x01];
    p.foot = (buf[0x03] >> 7) & 1;
    p.playing_style = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x1D, 3, 5));
    p.wf_usage = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x23, 0, 2)) + 1;
    p.wf_acc = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x22, 4, 2)) + 1;
    p.form_cons = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x17, 4, 3)) + 1;
    p.inj_res = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x22, 6, 2)) + 1;
    p.stamina = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x146, 0, 7));
    p.arrow = std::min(4, static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x147, 0, 3)));
    p.blink = get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x146, 7, 1) != 0;
    p.injury = buf[0x148];
    uint16_t cyr = 0;
    std::memcpy(&cyr, buf + 0x138, 2);
    p.contract_yr = cyr;
    std::memcpy(&p.salary_raw, buf + 0x15C, 4);
    std::memcpy(&p.market_raw, buf + 0x174, 4);
    p.has_release_fee = get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x147, 6, 1) == 1;
    uint32_t d13c = 0;
    std::memcpy(&d13c, buf + 0x13C, 4);
    p.affection = static_cast<int>((d13c >> 18) & 0x7Fu);
    p.transfer_listed = get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x14A, 1, 1) == 1;
    p.loan_listed = get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x14A, 2, 1) == 1;
    p.team_role = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x150, 0, 5));
    p.team_role_lvl = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x143, 6, 2));

    p.stats.fill(40);
    for (const auto& s : data::STAT_DEFS) {
        p.stats[s.fid] = get_fid_from_buf(buf, s.fid);
    }
    for (int pi = 0; pi < 13; ++pi) {
        p.pos_apt[pi] = get_apt_from_buf(buf, pi);
    }
    for (size_t i = 0; i < data::PLAYER_SKILLS.size(); ++i) {
        const auto& sk = data::PLAYER_SKILLS[i];
        p.skills[i] = get_bits_buf(buf, data::PLAYER_RECORD_SIZE, sk.offset, sk.bit, 1) == 1;
    }
    for (size_t i = 0; i < data::COM_AI_STYLES.size(); ++i) {
        const auto& ai = data::COM_AI_STYLES[i];
        p.com_ai[i] = get_bits_buf(buf, data::PLAYER_RECORD_SIZE, ai.offset, ai.bit, 1) == 1;
    }
    return p;
}

void apply_pack_player_to_buf(
    uint8_t* dst,
    const uint8_t* src,
    const PackPlayerData& pp,
    const PackApplyOptions& opts,
    EconSyncMode econ_mode,
    int* out_stat_changes)
{
    std::memcpy(dst, src, data::PLAYER_RECORD_SIZE);
    int stat_chg = 0;

    if (opts.apply_stats_pos) {
        for (const auto& s : data::STAT_DEFS) {
            int ov = get_fid_from_buf(src, s.fid);
            int nv = std::clamp(pp.stats[s.fid], 40, 99);
            if (ov != nv) ++stat_chg;
            set_fid_in_buf(dst, s.fid, nv);
        }
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x07, 4, 4, static_cast<uint32_t>(std::clamp(pp.reg_pos, 0, 12)));
        for (int pi = 0; pi < 13; ++pi) {
            auto loc = data::POS_APT_LOC[pi];
            set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, loc.off, loc.bit, 2, static_cast<uint32_t>(std::clamp(pp.pos_apt[pi], 0, 2)));
        }
        // Guarantee registered position has Grade A
        auto rloc = data::POS_APT_LOC[std::clamp(pp.reg_pos, 0, 12)];
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, rloc.off, rloc.bit, 2, 2u);
    }

    if (opts.apply_skills_style) {
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x1D, 3, 5, static_cast<uint32_t>(std::clamp(pp.playing_style, 0, 21)));
        for (size_t i = 0; i < data::PLAYER_SKILLS.size(); ++i) {
            const auto& sk = data::PLAYER_SKILLS[i];
            set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, sk.offset, sk.bit, 1, pp.skills[i] ? 1u : 0u);
        }
        for (size_t i = 0; i < data::COM_AI_STYLES.size(); ++i) {
            const auto& ai = data::COM_AI_STYLES[i];
            set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, ai.offset, ai.bit, 1, pp.com_ai[i] ? 1u : 0u);
        }
    }

    if (opts.apply_traits) {
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x23, 0, 2, static_cast<uint32_t>(std::clamp(pp.wf_usage - 1, 0, 3)));
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x22, 4, 2, static_cast<uint32_t>(std::clamp(pp.wf_acc - 1, 0, 3)));
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x17, 4, 3, static_cast<uint32_t>(std::clamp(pp.form_cons - 1, 0, 7)));
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x22, 6, 2, static_cast<uint32_t>(std::clamp(pp.inj_res - 1, 0, 2)));
    }

    if (opts.apply_team_role) {
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x150, 0, 5, static_cast<uint32_t>(std::clamp(pp.team_role, 0, 22)));
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x143, 6, 2, static_cast<uint32_t>(std::clamp(pp.team_role_lvl, 0, 3)));
    }

    if (opts.apply_bio) {
        if (!pp.name.empty() && pp.name != "?") write_cstr_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x38, 46, pp.name);
        if (!pp.shirt.empty()) {
            std::string sh = pp.shirt;
            for (char& ch : sh) if (static_cast<unsigned char>(ch) < 128) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            write_cstr_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x75, 18, sh);
        }
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x1C, 0, 6, static_cast<uint32_t>(std::clamp(pp.age, 15, 50)));
        dst[0x00] = static_cast<uint8_t>(std::clamp(pp.height, 145, 215));
        dst[0x01] = static_cast<uint8_t>(std::clamp(pp.weight, 45, 125));
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x03, 7, 1, pp.foot ? 1u : 0u);
        uint16_t nat_cur = 0;
        std::memcpy(&nat_cur, dst + 0x144, 2);
        nat_cur = static_cast<uint16_t>((nat_cur & ~0x1FFu) | (static_cast<uint32_t>(pp.nationality) & 0x1FFu));
        std::memcpy(dst + 0x144, &nat_cur, 2);
    }

    if (opts.apply_condition) {
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x146, 0, 7, static_cast<uint32_t>(std::clamp(pp.stamina, 0, 100)));
        int a = std::clamp(pp.arrow, 0, 4);
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x147, 0, 3, static_cast<uint32_t>(a));
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x146, 7, 1, pp.blink ? 1u : 0u);
        dst[0x148] = static_cast<uint8_t>(std::clamp(pp.injury, 0, 255));
    }

    auto [old_ovr, old_pos] = calc_ovr_from_buf(src, data::PLAYER_RECORD_SIZE);
    auto [new_ovr, new_pos] = calc_ovr_from_buf(dst, data::PLAYER_RECORD_SIZE);
    int old_age = src[0x1C] & 0x3F;
    int new_age = dst[0x1C] & 0x3F;
    uint16_t old_nat_raw = 0, new_nat_raw = 0;
    std::memcpy(&old_nat_raw, src + 0x144, 2);
    std::memcpy(&new_nat_raw, dst + 0x144, 2);
    int old_nat = old_nat_raw & 0x1FF;
    int new_nat = new_nat_raw & 0x1FF;

    if (opts.apply_pack_economy) {
        if (pp.contract_yr >= 2019 && pp.contract_yr <= 2150) {
            uint16_t cy = static_cast<uint16_t>(pp.contract_yr);
            std::memcpy(dst + 0x138, &cy, 2);
        }
        if (pp.market_raw > 0) {
            std::memcpy(dst + 0x174, &pp.market_raw, 4);
            dst[0x178] = static_cast<uint8_t>(std::clamp(new_age, 15, 63));
            dst[0x179] = static_cast<uint8_t>(std::clamp(new_ovr, 40, 99));
        }
        if (pp.salary_raw > 0) {
            std::memcpy(dst + 0x15C, &pp.salary_raw, 4);
        }
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x147, 6, 1, pp.has_release_fee ? 1u : 0u);
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x13C, 18, 7, static_cast<uint32_t>(std::clamp(pp.affection, 0, 100)));
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x14A, 1, 1, pp.transfer_listed ? 1u : 0u);
        set_bits_in_buf(dst, data::PLAYER_RECORD_SIZE, 0x14A, 2, 1, pp.loan_listed ? 1u : 0u);
    } else if (opts.auto_recal_economy) {
        auto calc_sec = [](const uint8_t* b) -> double {
            auto [po, rp] = calc_ovr_from_buf(b, data::PLAYER_RECORD_SIZE);
            std::array<int, 13> ovs{}, aps{};
            for (int pi = 0; pi < 13; ++pi) {
                ovs[pi] = calc_ovr_from_buf(b, data::PLAYER_RECORD_SIZE, pi, false).first;
                aps[pi] = get_apt_from_buf(b, pi);
            }
            return econ::secondary_versatility_mult(rp, po, ovs, aps);
        };
        double old_sec = calc_sec(src);
        double new_sec = calc_sec(dst);
        if (new_ovr != old_ovr || new_pos != old_pos || new_age != old_age || new_nat != old_nat ||
            std::fabs(new_sec - old_sec) > 1e-4) {
            EconSyncMode eff_mode = (econ_mode == EconSyncMode::Off) ? EconSyncMode::All : econ_mode;
            int32_t cur_mkt = 0, cur_sal = 0;
            std::memcpy(&cur_mkt, src + 0x174, 4);
            std::memcpy(&cur_sal, src + 0x15C, 4);
            if (eff_mode == EconSyncMode::All || eff_mode == EconSyncMode::MarketAndRelease) {
                int32_t nm = econ::scale_economy_value(cur_mkt, old_ovr, old_age, old_pos, old_nat,
                                                       new_ovr, new_age, new_pos, new_nat, false,
                                                       old_sec, new_sec);
                std::memcpy(dst + 0x174, &nm, 4);
                dst[0x178] = static_cast<uint8_t>(std::clamp(new_age, 15, 63));
                dst[0x179] = static_cast<uint8_t>(std::clamp(new_ovr, 40, 99));
            }
            if (eff_mode == EconSyncMode::All || eff_mode == EconSyncMode::SalaryOnly) {
                int32_t ns = econ::scale_economy_value(cur_sal, old_ovr, old_age, old_pos, old_nat,
                                                       new_ovr, new_age, new_pos, new_nat, true,
                                                       old_sec, new_sec);
                std::memcpy(dst + 0x15C, &ns, 4);
            }
        }
    }

    // Always preserve target save's internal career handles & club pointers intact
    std::memcpy(dst + 0x2C, src + 0x2C, 12);
    std::memcpy(dst + 0x12C, src + 0x12C, 8);
    std::memcpy(dst + 0x160, src + 0x160, 8);

    if (out_stat_changes) *out_stat_changes = stat_chg;
}

} // namespace

std::string norm_text_utf8(std::string_view s) {
    std::string out;
    out.reserve(s.size());
    size_t i = 0;
    while (i < s.size()) {
        uint8_t c = static_cast<uint8_t>(s[i]);
        if (c < 0x80u) {
            out.push_back(static_cast<char>(std::tolower(c)));
            ++i;
            continue;
        }
        uint32_t cp = 0;
        size_t adv = 1;
        if ((c & 0xE0u) == 0xC0u && i + 1 < s.size()) {
            cp = ((c & 0x1Fu) << 6) | (static_cast<uint8_t>(s[i + 1]) & 0x3Fu);
            adv = 2;
        } else if ((c & 0xF0u) == 0xE0u && i + 2 < s.size()) {
            cp = ((c & 0x0Fu) << 12) | ((static_cast<uint8_t>(s[i + 1]) & 0x3Fu) << 6) | (static_cast<uint8_t>(s[i + 2]) & 0x3Fu);
            adv = 3;
        } else if ((c & 0xF8u) == 0xF0u && i + 3 < s.size()) {
            cp = ((c & 0x07u) << 18) | ((static_cast<uint8_t>(s[i + 1]) & 0x3Fu) << 12) |
                 ((static_cast<uint8_t>(s[i + 2]) & 0x3Fu) << 6) | (static_cast<uint8_t>(s[i + 3]) & 0x3Fu);
            adv = 4;
        }
        char folded = fold_latin_codepoint(cp);
        if (folded != 0) out.push_back(folded);
        i += adv;
    }
    return out;
}

std::optional<int32_t> parse_human_money_to_raw(std::string_view text) {
    std::string s;
    s.reserve(text.size());
    for (size_t i = 0; i < text.size(); ) {
        uint8_t c = static_cast<uint8_t>(text[i]);
        // Skip UTF-8 Euro (E2 82 AC) and Pound (C2 A3)
        if (c == 0xE2 && i + 2 < text.size() && static_cast<uint8_t>(text[i + 1]) == 0x82 && static_cast<uint8_t>(text[i + 2]) == 0xAC) {
            i += 3;
            continue;
        }
        if (c == 0xC2 && i + 1 < text.size() && static_cast<uint8_t>(text[i + 1]) == 0xA3) {
            i += 2;
            continue;
        }
        char up = static_cast<char>(std::toupper(c));
        if (up != '$' && up != ' ' && up != '\t' && up != '\r' && up != '\n') {
            s.push_back(up);
        }
        ++i;
    }
    for (const char* tok : {"R$", "EUR", "BRL", "USD", "GBP", "R"}) {
        // Strip R only if followed by digit/currency or prefix
        if (std::strcmp(tok, "R") == 0) {
            if (!s.empty() && s[0] == 'R') s.erase(0, 1);
            continue;
        }
        size_t pos = 0;
        size_t tlen = std::strlen(tok);
        while ((pos = s.find(tok, pos)) != std::string::npos) {
            s.erase(pos, tlen);
        }
    }
    if (s.empty()) return std::nullopt;

    double mult = 1.0;
    if (s.back() == 'B') { mult = 1e9; s.pop_back(); }
    else if (s.back() == 'M') { mult = 1e6; s.pop_back(); }
    else if (s.back() == 'K') { mult = 1e3; s.pop_back(); }
    if (s.empty()) return std::nullopt;

    if (mult > 1.0) {
        std::replace(s.begin(), s.end(), ',', '.');
    } else {
        auto dot_cnt = std::count(s.begin(), s.end(), '.');
        auto dot_pos = s.find('.');
        if (dot_cnt > 1 || (dot_cnt == 1 && dot_pos != std::string::npos && (s.size() - dot_pos - 1) == 3)) {
            s.erase(std::remove(s.begin(), s.end(), '.'), s.end());
        }
        auto com_cnt = std::count(s.begin(), s.end(), ',');
        auto com_pos = s.find(',');
        if (com_cnt > 1 || (com_cnt == 1 && com_pos != std::string::npos && (s.size() - com_pos - 1) == 3)) {
            s.erase(std::remove(s.begin(), s.end(), ','), s.end());
        }
        std::replace(s.begin(), s.end(), ',', '.');
    }

    char* endp = nullptr;
    double val = std::strtod(s.c_str(), &endp);
    if (!endp || *endp != '\0' || !std::isfinite(val)) return std::nullopt;
    double raw_d = std::round((val * mult) / 100.0);
    if (raw_d < 0.0) raw_d = 0.0;
    if (raw_d > 2147483647.0) raw_d = 2147483647.0;
    return static_cast<int32_t>(raw_d);
}

PESEngine::PESEngine() : hooks_(mem_) {}

double PESEngine::now_sec() {
    using clk = std::chrono::steady_clock;
    return std::chrono::duration<double>(clk::now().time_since_epoch()).count();
}

bool PESEngine::is_sane_player_buf(const uint8_t* buf) {
    uint32_t pid = 0;
    std::memcpy(&pid, buf + 0x30, 4);
    if (pid == 0 || pid == 0xFFFFFFFFu) return false;
    int age = buf[0x1C] & 0x3F;
    if (age < 10 || age > 63) return false;
    int reg_pos = (buf[0x07] >> 4) & 0x0F;
    if (reg_pos > 12) return false;
    return true;
}

bool PESEngine::ensure_attached() {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (shutting_down_.load(std::memory_order_acquire)) return false;

    if (mem_.attached() && mem_.pid == last_pid_ && hooks_.state_base) {
        if (!hooks_.active_hooks[1] || !hooks_.verify_hook_alive(1)) {
            hooks_.enable_hook_1_player();
        }
        return true;
    }
    if (!mem_.attach()) {
        reset_session();
        return false;
    }
    // Processo escolhido a mao que nao e um jogo (modulo pequeno e nome desconhecido): nao aloca nem grava nada nele
    {
        const std::wstring wname = [&] {
            int need = MultiByteToWideChar(CP_UTF8, 0, mem_.exe_name.data(), static_cast<int>(mem_.exe_name.size()), nullptr, 0);
            std::wstring w(static_cast<size_t>(std::max(need, 0)), L'x');
            if (need > 0) MultiByteToWideChar(CP_UTF8, 0, mem_.exe_name.data(), static_cast<int>(mem_.exe_name.size()), w.data(), need);
            return w;
        }();
        if (!is_supported_game_exe(wname.c_str()) && mem_.module_size < kMinGameModuleBytes) {
            unsupported_exe_ = mem_.exe_name;
            mem_.detach();
            reset_session();
            return false;
        }
        unsupported_exe_.clear();
    }
    if (mem_.pid != last_pid_) {
        reset_session();
        last_pid_ = mem_.pid;
    }
    if (!hooks_.init_state()) return false;
    discover_singletons();
    hooks_.enable_hook_1_player();
    uint64_t c = 0, p = 0, m = 0;
    sync_pointers(c, p, m);
    return true;
}

void PESEngine::set_target_process(const std::string& exe_utf8, uint32_t pid) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (shutting_down_.load(std::memory_order_acquire)) return;
    int need = MultiByteToWideChar(CP_UTF8, 0, exe_utf8.data(), static_cast<int>(exe_utf8.size()), nullptr, 0);
    std::wstring w(static_cast<size_t>(std::max(need, 0)), L'x');
    if (need > 0) MultiByteToWideChar(CP_UTF8, 0, exe_utf8.data(), static_cast<int>(exe_utf8.size()), w.data(), need);
    const bool changed = (_wcsicmp(w.c_str(), mem_.target_exe.c_str()) != 0) || pid != mem_.target_pid;
    if (changed && mem_.attached()) hooks_.disable_all();   // devolve o codigo original ao processo que vai ser largado
    mem_.set_target(w, pid);
    if (changed) {
        vlock_ = ValueLockState{};   // a trava pertence ao processo anterior
        unsupported_exe_.clear();
        last_pid_ = 0;
        reset_session();
    }
}

void PESEngine::reset_session() {
    addr_career_owner_ = 0;
    addr_match_mgr_ = 0;
    last_career_ = 0;
    last_written_career_ = UINT64_MAX;
    last_written_mmgr_ = UINT64_MAX;
    last_written_mstate_ = UINT64_MAX;
    last_in_match_ = false;
    hook5_auto_attempted_ = false;
    reset_career_caches();
}

void PESEngine::reset_career_caches() {
    squad_cache_ = {};
    wage_cache_career_ = 0;
    wage_cache_time_ = 0.0;
    manual_player_addr_ = 0;
    last_hook_player_ = 0;
    club_name_cache_.clear();
    undo_stack_.clear();
    last_batch_undo_.clear();
    last_batch_label_.clear();
    econ_anchors_.clear();
    // Outro save: snapshots de olheiros e linha do tempo apontam para memoria do save anterior
    scout_undo_.clear();
    journal_.clear();
    journal_batch_open_ = false;
    ++journal_rev_;
}

void PESEngine::shutdown() {
    bool expected = false;
    if (!shutting_down_.compare_exchange_strong(expected, true)) return;
    std::lock_guard<std::recursive_mutex> g(lock_);
    vlock_ = ValueLockState{};
    hooks_.disable_all();
    mem_.detach();
}

void PESEngine::discover_singletons() {
    uint64_t base = mem_.module_base;
    uint64_t fast_career = base + 0x3714240ULL;
    uint64_t fast_match = base + 0x3702208ULL;
    if (mem_.read_u64(fast_career) > 0x10000ULL) {
        addr_career_owner_ = fast_career;
        addr_match_mgr_ = fast_match;
        return;
    }

    if (uint64_t call_site = mem_.aob_scan_region(
            base + 0x1000ULL, base + 0x2540000ULL,
            "E8 ?? ?? ?? ?? 48 8B 48 48 48 8D 81 E0 CB 6E 01")) {
        int32_t rel_call = mem_.read_i32(call_site + 1);
        uint64_t fn_getter = static_cast<uint64_t>(static_cast<int64_t>(call_site + 5) + rel_call);
        uint8_t hdr[7]{};
        if (mem_.read_into(fn_getter, hdr, 7) && hdr[0] == 0x48 && hdr[1] == 0x8B && hdr[2] == 0x05) {
            int32_t disp = 0;
            std::memcpy(&disp, hdr + 3, 4);
            addr_career_owner_ = static_cast<uint64_t>(static_cast<int64_t>(fn_getter + 7) + disp);
        }
    }
    if (!addr_career_owner_) addr_career_owner_ = fast_career;

    if (uint64_t match_site = mem_.aob_scan_region(
            base + 0x1000ULL, base + 0x2540000ULL,
            "48 8B 05 ?? ?? ?? ?? 4C 8B E2 45 33 FF 48 8B B0 70 02 00 00")) {
        int32_t disp = mem_.read_i32(match_site + 3);
        addr_match_mgr_ = static_cast<uint64_t>(static_cast<int64_t>(match_site + 7) + disp);
    } else {
        addr_match_mgr_ = fast_match;
    }
}

void PESEngine::sync_pointers(uint64_t& out_career, uint64_t& out_player, uint64_t& out_match_state, HookStateBlock* out_state) {
    out_career = 0;
    out_player = 0;
    out_match_state = 0;
    if (!mem_.attached() || !hooks_.state_base) return;

    uint64_t career = 0;
    if (addr_career_owner_) {
        uint64_t owner = mem_.read_u64(addr_career_owner_);
        if (owner > 0x10000ULL) career = mem_.read_u64(owner + 0x48);
    }

    if (career != last_career_) {
        last_career_ = career;
        reset_career_caches();
        mem_.write_u64(hooks_.sym_addr(HookSym::ptrPlayer), 0);
    }
    // Only write ptrCareer when changed (0 writes/sec in steady state)
    if (career != last_written_career_) {
        mem_.write_u64(hooks_.sym_addr(HookSym::ptrCareer), career);
        last_written_career_ = career;
    }

    uint64_t match_mgr = 0;
    uint64_t match_state = 0;
    uint32_t match_period_raw = 0xFFFFFFFFu;
    if (addr_match_mgr_) {
        match_mgr = mem_.read_u64(addr_match_mgr_);
        if (match_mgr > 0x10000ULL) {
            uint64_t cand = mem_.read_u64(match_mgr + 0x270);
            if (cand > 0x10000ULL) {
                uint32_t p = mem_.read_u32(cand + 0x1168ULL);
                if (p <= 8u) {
                    match_state = cand;
                    match_period_raw = p;
                }
            }
        }
    }
    if (match_mgr != last_written_mmgr_) {
        mem_.write_u64(hooks_.sym_addr(HookSym::ptrMatchMgr), match_mgr);
        last_written_mmgr_ = match_mgr;
    }
    if (match_state != last_written_mstate_) {
        mem_.write_u64(hooks_.sym_addr(HookSym::ptrMatchState), match_state);
        last_written_mstate_ = match_state;
    }

    bool in_match = (match_state > 0x10000ULL && match_period_raw <= 8u);
    if (in_match && !hooks_.active_hooks[5] && !hook5_auto_attempted_) {
        // Auto-arm Match HUD & Clock Tap once per process session as soon as a live match starts
        // so valProtectedMatchTicks and ptrMatchHud are captured before the user pauses/Alt-Tabs.
        hook5_auto_attempted_ = true;
        hooks_.enable_hook_5_match();
    }
    if (last_in_match_ && !in_match) {
        mem_.write_u32(hooks_.sym_addr(HookSym::cfgFreezeClock), 0);
        mem_.write_f32(hooks_.sym_addr(HookSym::valFrozenTotalTicks), 0.0f);
        mem_.write_f32(hooks_.sym_addr(HookSym::valProtectedMatchTicks), 0.0f);
        mem_.write_u64(hooks_.sym_addr(HookSym::ptrMatchHud), 0);
        mem_.write_u64(hooks_.sym_addr(HookSym::ptrMatchCore), 0);
    }
    last_in_match_ = in_match;

    // Single 160-byte bulk read for the entire hook state block!
    HookStateBlock sb{};
    hooks_.read_state_block(sb);
    if (out_state) *out_state = sb;

    if (career > 0x10000ULL && sb.cfgLockBALFocusPoints == 1) {
        hooks_.enable_hook_4_bal_cap();
        mem_.write_u8(career + 0x016ECBEAULL, bal_lock_target_);
    }

    uint64_t hook_p = sb.ptrPlayer;
    if (hook_p > 0x10000ULL && hook_p != last_hook_player_) {
        last_hook_player_ = hook_p;
        // Validate captured pointer before promoting it, so transient copy buffers
        // never wipe out a valid Game Plan / Squad player selection.
        uint8_t probe[data::PLAYER_RECORD_SIZE]{};
        if (mem_.read_into(hook_p, probe, sizeof(probe)) && is_sane_player_buf(probe)) {
            manual_player_addr_ = hook_p;
        }
    }

    out_career = career;
    out_player = (manual_player_addr_ > 0x10000ULL) ? manual_player_addr_ : 0ULL;
    out_match_state = match_state;
}

bool PESEngine::select_player(uint64_t addr) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached() || addr <= 0x10000ULL) return false;
    manual_player_addr_ = addr;
    last_hook_player_ = addr;
    mem_.write_u64(hooks_.sym_addr(HookSym::ptrPlayer), addr);
    return true;
}

void PESEngine::push_undo(uint64_t p_addr, const char* action_code, const uint8_t* existing_buf) {
    UndoEntry e;
    e.action = action_code ? action_code : "edit";
    if (existing_buf) {
        std::memcpy(e.bytes.data(), existing_buf, data::PLAYER_RECORD_SIZE);
    } else if (!mem_.read_into(p_addr, e.bytes.data(), data::PLAYER_RECORD_SIZE)) {
        return;
    }
    auto ait = econ_anchors_.find(p_addr);
    if (ait != econ_anchors_.end()) {
        e.prev_anchor = ait->second;
    }
    const std::string action_copy = e.action;
    auto& st = undo_stack_[p_addr];
    st.push_back(std::move(e));
    if (static_cast<int>(st.size()) > kUndoMaxDepth) {
        st.erase(st.begin());
    }
    // "Desfazer lote" so vale logo apos o lote: uma edicao avulsa depois dele invalida o snapshot
    if (!journal_batch_open_) {
        last_batch_undo_.clear();
        last_batch_label_.clear();
    }
    // Linha do tempo global: um lote vira uma unica entrada
    if (journal_batch_open_ && !journal_.empty()) {
        journal_.back().addrs.push_back(p_addr);
    } else {
        JournalEntry je;
        je.seq = ++journal_seq_;
        je.time = static_cast<long long>(std::time(nullptr));
        je.action = action_copy;
        je.addrs.push_back(p_addr);
        journal_.push_back(std::move(je));
        if (journal_.size() > 300) journal_.erase(journal_.begin());
    }
    ++journal_rev_;
}

void PESEngine::journal_begin_batch(const std::string& action) {
    JournalEntry je;
    je.seq = ++journal_seq_;
    je.time = static_cast<long long>(std::time(nullptr));
    je.action = action;
    journal_.push_back(std::move(je));
    journal_batch_open_ = true;
}

void PESEngine::journal_end_batch() {
    journal_batch_open_ = false;
    if (!journal_.empty() && journal_.back().addrs.empty()) journal_.pop_back();
    if (journal_.size() > 300) journal_.erase(journal_.begin());
    ++journal_rev_;
}

void PESEngine::journal_drop_addr(uint64_t addr) {
    for (size_t i = journal_.size(); i-- > 0;) {
        auto& a = journal_[i].addrs;
        auto it = std::find(a.begin(), a.end(), addr);
        if (it == a.end()) continue;
        a.erase(it);
        if (a.empty()) journal_.erase(journal_.begin() + static_cast<std::ptrdiff_t>(i));
        break;
    }
    ++journal_rev_;
}

std::vector<JournalRow> PESEngine::get_journal() {
    std::lock_guard<std::recursive_mutex> g(lock_);
    std::vector<JournalRow> out;
    out.reserve(journal_.size());
    const long long now = static_cast<long long>(std::time(nullptr));
    for (size_t i = journal_.size(); i-- > 0;) {
        const auto& je = journal_[i];
        JournalRow r;
        r.seq = je.seq;
        r.time = je.time;
        r.action = je.action;
        r.players = static_cast<int>(je.addrs.size());
        r.age_sec = static_cast<uint32_t>(std::max(0LL, now - je.time));
        if (je.addrs.size() == 1) {
            uint8_t nb[0x2E]{};
            if (mem_.read_into(je.addrs[0] + 0x38ULL, nb, sizeof(nb))) r.label = extract_cstr(nb, sizeof(nb));
            if (r.label.empty()) r.label = "?";
        }
        out.push_back(std::move(r));
    }
    return out;
}

ActionResult PESEngine::undo_journal_to(uint64_t seq) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    int restored = 0;
    int entries = 0;
    while (!journal_.empty() && journal_.back().seq >= seq) {
        JournalEntry je = journal_.back(); // copia: undo_last() mexe na linha do tempo
        for (uint64_t a : je.addrs) {
            auto r = undo_last(a);
            if (r.ok) ++restored;
        }
        if (!journal_.empty() && journal_.back().seq == je.seq) journal_.pop_back(); // pilha vazia (limite de profundidade)
        ++entries;
    }
    ++journal_rev_;
    return {restored > 0, restored > 0 ? "ok" : "nothing_to_undo", restored, entries};
}

std::string PESEngine::backup_squad_locked(std::string_view tag) {
    auto r = export_squad_pack_json(tag);
    if (!r.ok || r.text.empty()) return {};
    try {
        std::filesystem::path dir = fs_path_from_utf8(get_backups_directory());
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        std::time_t t = std::time(nullptr);
        std::tm lt{};
        localtime_s(&lt, &t);
        char stamp[32];
        std::strftime(stamp, sizeof(stamp), "%Y-%m-%d_%H-%M-%S", &lt);
        std::string tg(tag);
        for (char& ch : tg) if (!std::isalnum(static_cast<unsigned char>(ch))) ch = '_';
        static unsigned backup_seq = 0;
        std::string fname = "Backup_" + tg + "_" + stamp + "_" + std::to_string(++backup_seq) + ".json";
        std::filesystem::path file = dir / fs_path_from_utf8(fname);
        std::ofstream f(file, std::ios::binary | std::ios::trunc);
        if (!f) return {};
        f.write(r.text.data(), static_cast<std::streamsize>(r.text.size()));
        f.close();   // o ultimo flush acontece aqui: so depois dele o estado do stream vale
        const bool wrote_ok = !f.fail();
        if (!wrote_ok) {
            std::error_code rm_ec;
            std::filesystem::remove(file, rm_ec); // backup incompleto nao conta como backup
            return {};
        }
        // Retencao opt-in: poda os mais antigos so depois de o novo estar gravado (o novo e protegido)
        const int keep = backup_keep_.load(std::memory_order_relaxed);
        if (keep > 0) pes::backups::prune(dir, keep, file);
        auto u8 = file.u8string();
        return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
    } catch (...) {
        return {};
    }
}

ActionResult PESEngine::undo_last(uint64_t addr) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t c = 0, cur_p = 0, ms = 0;
    sync_pointers(c, cur_p, ms);
    uint64_t p = addr ? addr : cur_p;
    auto it = undo_stack_.find(p);
    if (it == undo_stack_.end() || it->second.empty()) return {false, "nothing_to_undo"};
    UndoEntry e = std::move(it->second.back());
    it->second.pop_back();
    {
        uint8_t probe[data::PLAYER_RECORD_SIZE]{};
        if (!mem_.read_into(p, probe, sizeof(probe)) || !is_sane_player_buf(probe)) {
            journal_drop_addr(p); // entrada obsoleta (tabela mudou): descarta sem gravar
            return {false, "read_failed"};
        }
    }
    mem_.write_from(p, e.bytes.data(), e.bytes.size());
    if (e.prev_anchor.has_value()) {
        econ_anchors_[p] = *e.prev_anchor;
    } else {
        econ_anchors_.erase(p);
    }
    wage_cache_time_ = 0.0;
    journal_drop_addr(p);
    return {true, "ok", static_cast<int>(it->second.size()), 0, false, e.action};
}

std::pair<int, std::string> PESEngine::decode_club_handle(uint32_t raw_u32, uint64_t career) {
    if (raw_u32 == 0 || raw_u32 == 0xFFFFFFFFu) return {0, {}};
    uint64_t key = (career ^ (static_cast<uint64_t>(raw_u32) * 0x9E3779B97F4A7C15ULL));
    auto it = club_name_cache_.find(key);
    if (it != club_name_cache_.end()) return it->second;

    uint32_t slot = raw_u32 % 16384u;
    int team_id = static_cast<int>(raw_u32 / 16384u);
    std::pair<int, std::string> res;

    if (career > 0x10000ULL && slot < 1200u) {
        uint64_t club_entry = career + 0xADF4BCULL + (static_cast<uint64_t>(slot) * 0x690ULL);
        uint8_t blk[0x54]{};
        if (mem_.read_into(club_entry, blk, sizeof(blk))) {
            uint32_t entry_h = 0;
            std::memcpy(&entry_h, blk, 4);
            if (entry_h == raw_u32 || (entry_h % 16384u) == slot) {
                std::string live_name = extract_cstr(blk + 0x04, 64);
                std::string abbr = extract_cstr(blk + 0x4A, 6);
                if (!live_name.empty()) {
                    res = {team_id, abbr.empty() ? live_name : (live_name + " [" + abbr + "]")};
                    club_name_cache_[key] = res;
                    return res;
                }
            }
        }
    }
    const char* mapped = data::lookup_club_team(team_id);
    res = {team_id, mapped ? std::string(mapped) : ("Club #" + std::to_string(team_id))};
    club_name_cache_[key] = res;
    return res;
}

uint32_t PESEngine::get_user_club_handle(uint64_t career) {
    if (!career) return 0;
    uint32_t uc = mem_.read_u32(career + 0x01642A2CULL);
    if (uc && uc != 0xFFFFFFFFu) return uc;
    uint64_t h0 = mem_.read_u64(career + 0x016EE854ULL);
    if ((h0 >> 32) != 0) {
        uint32_t lo = static_cast<uint32_t>(h0 & 0xFFFFFFFFULL);
        if (lo < 30000u) {
            uint32_t c0 = mem_.read_u32(career + static_cast<uint64_t>(lo) * data::PLAYER_RECORD_SIZE + 0x12CULL);
            if (c0 && c0 != 0xFFFFFFFFu) return c0;
        }
    }
    uint64_t bal_h = mem_.read_u64(career + 0x01819540ULL);
    if ((bal_h >> 32) != 0) {
        uint32_t lo = static_cast<uint32_t>(bal_h & 0xFFFFFFFFULL);
        if (lo < 30000u) {
            uint32_t c1 = mem_.read_u32(career + static_cast<uint64_t>(lo) * data::PLAYER_RECORD_SIZE + 0x12CULL);
            if (c1 && c1 != 0xFFFFFFFFu) return c1;
        }
    }
    return 0;
}

const std::vector<uint64_t>& PESEngine::get_user_squad_addresses(uint64_t career, bool force_rescan, std::vector<uint8_t>* out_raw_table) {
    static const std::vector<uint64_t> kEmpty;
    uint32_t uc = get_user_club_handle(career);
    if (!uc) return kEmpty;

    double now = now_sec();
    bool need = force_rescan ||
                squad_cache_.career != career ||
                squad_cache_.uc != uc ||
                squad_cache_.addrs.empty() ||
                (now - squad_cache_.timestamp) > 120.0 ||
                (out_raw_table != nullptr);

    if (!need) {
        // Quick spot-check: verify first and last cached players still belong to user's club
        if (mem_.read_u32(squad_cache_.addrs.front() + 0x12CULL) != uc ||
            mem_.read_u32(squad_cache_.addrs.back() + 0x12CULL) != uc) {
            need = true;
        }
    }

    if (need) {
        uint32_t count = mem_.read_u32(career + 0x00D0BCE8ULL);
        if (count == 0 || count > 30001u) count = 30000u;
        std::vector<uint8_t> local_buf;
        std::vector<uint8_t>& raw = out_raw_table ? *out_raw_table : local_buf;
        raw.resize(static_cast<size_t>(count) * data::PLAYER_RECORD_SIZE);
        std::vector<uint64_t> addrs;
        addrs.reserve(48);
        if (mem_.read_into(career, raw.data(), raw.size())) {
            for (uint32_t i = 0; i < count; ++i) {
                size_t off = static_cast<size_t>(i) * data::PLAYER_RECORD_SIZE;
                uint32_t club_h = 0;
                std::memcpy(&club_h, raw.data() + off + 0x12C, 4);
                if (club_h == uc) {
                    uint32_t pid = 0;
                    std::memcpy(&pid, raw.data() + off + 0x30, 4);
                    if (pid != 0 && pid != 0xFFFFFFFFu) {
                        addrs.push_back(career + off);
                    }
                }
            }
        }
        squad_cache_.career = career;
        squad_cache_.uc = uc;
        squad_cache_.timestamp = now;
        squad_cache_.addrs = std::move(addrs);
    }
    return squad_cache_.addrs;
}

void PESEngine::get_available_salary_budget(uint64_t career, bool force_refresh, int32_t& out_avail, int32_t& out_gross, int32_t& out_wages) {
    out_avail = out_gross = out_wages = 0;
    if (!career) return;
    int32_t gross = mem_.read_i32(career + 0x016ECC08ULL);
    double now = now_sec();
    if (!force_refresh && wage_cache_career_ == career && (now - wage_cache_time_) < 15.0) {
        out_gross = gross;
        out_wages = wage_cache_sum_;
        out_avail = gross - wage_cache_sum_;
        return;
    }

    std::unordered_map<uint64_t, int32_t> overrides;
    int32_t ov_sum = 0;
    uint8_t ov_raw[34 * 12]{};
    if (mem_.read_into(career + 0x016F0764ULL, ov_raw, sizeof(ov_raw))) {
        for (int i = 0; i < 34; ++i) {
            uint64_t ph = 0;
            int32_t ov = 0;
            std::memcpy(&ph, ov_raw + i * 12, 8);
            std::memcpy(&ov, ov_raw + i * 12 + 8, 4);
            if (ph != 0 && (ph & 0xFFFFULL) != 0xFFFFULL && (ph >> 32) != 0) {
                overrides[ph] = ov;
                ov_sum += ov;
            }
        }
    }

    const auto& addrs = get_user_squad_addresses(career, force_refresh);
    int32_t sum_sal = 0;
    std::vector<uint64_t> seen;
    seen.reserve(addrs.size());
    for (uint64_t p : addrs) {
        uint64_t ph = mem_.read_u64(p + 0x2CULL);
        if (std::find(seen.begin(), seen.end(), ph) == seen.end()) {
            seen.push_back(ph);
            if (overrides.find(ph) == overrides.end()) {
                sum_sal += mem_.read_i32(p + 0x15CULL);
            }
        }
    }
    sum_sal += ov_sum;
    wage_cache_career_ = career;
    wage_cache_time_ = now;
    wage_cache_sum_ = sum_sal;

    out_gross = gross;
    out_wages = sum_sal;
    out_avail = gross - sum_sal;
}

FinancesSnapshot PESEngine::get_finances() {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (!career) return {false, "no_career"};
    FinancesSnapshot f;
    f.ok = true;
    f.transfer_raw = mem_.read_i32(career + 0x016ECBF4ULL);
    get_available_salary_budget(career, true, f.salary_avail_raw, f.salary_gross_raw, f.squad_wages_raw);
    return f;
}

FinancesSnapshot PESEngine::set_finances(std::optional<std::string_view> trf_str, std::optional<std::string_view> sal_str) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (!career) return {false, "no_career"};

    std::optional<int32_t> t_raw, w_raw;
    if (trf_str.has_value() && !trf_str->empty()) {
        t_raw = parse_human_money_to_raw(*trf_str);
        if (!t_raw.has_value()) return {false, "invalid_money"};
    }
    if (sal_str.has_value() && !sal_str->empty()) {
        w_raw = parse_human_money_to_raw(*sal_str);
        if (!w_raw.has_value()) return {false, "invalid_money"};
    }
    if (t_raw.has_value()) mem_.write_i32(career + 0x016ECBF4ULL, *t_raw);
    if (w_raw.has_value()) {
        int32_t avail = 0, gross = 0, wages = 0;
        get_available_salary_budget(career, true, avail, gross, wages);
        int64_t new_gross = std::clamp<int64_t>(static_cast<int64_t>(*w_raw) + wages, 0, 2147483647LL);
        mem_.write_i32(career + 0x016ECC08ULL, static_cast<int32_t>(new_gross));
    }
    return get_finances();
}

FinancesSnapshot PESEngine::add_finance_delta(bool is_transfer, int32_t raw_delta) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (!career) return {false, "no_career"};
    uint64_t off = is_transfer ? 0x016ECBF4ULL : 0x016ECC08ULL;
    int32_t cur = mem_.read_i32(career + off);
    int64_t nv = std::clamp<int64_t>(static_cast<int64_t>(cur) + raw_delta, 0, 2147483647LL);
    mem_.write_i32(career + off, static_cast<int32_t>(nv));
    return get_finances();
}

StatusSnapshot PESEngine::get_status() {
    std::lock_guard<std::recursive_mutex> g(lock_);
    StatusSnapshot st;
    st.seq = ++status_seq_;
    st.conflict_other_wb = detect_legacy_workbench_running();
    {
        const std::wstring& te = mem_.target_exe;
        if (!te.empty()) {
            int need = WideCharToMultiByte(CP_UTF8, 0, te.data(), static_cast<int>(te.size()), nullptr, 0, nullptr, nullptr);
            st.target_exe.assign(static_cast<size_t>(std::max(need, 0)), 'x');
            if (need > 0) WideCharToMultiByte(CP_UTF8, 0, te.data(), static_cast<int>(te.size()), st.target_exe.data(), need, nullptr, nullptr);
        }
    }
    if (!ensure_attached()) {
        st.target_unsupported = !unsupported_exe_.empty();
        st.access_denied = mem_.access_denied;
        if (vlock_.active) drop_value_lock_locked();   // jogo fechado: a trava nao tem mais onde escrever
        st.val_lock_lost = vlock_lost_;
        return st;
    }
    tick_value_lock_locked();

    uint64_t career = 0, p_addr = 0, match_state = 0;
    HookStateBlock sb{};
    sync_pointers(career, p_addr, match_state, &sb);

    st.val_lock_active = vlock_.active;
    st.val_lock = vlock_.cfg;
    st.val_lock_addr = vlock_.addr;
    st.val_lock_name = vlock_.name;
    st.val_lock_lost = vlock_lost_;

    st.attached = true;
    st.exe = mem_.exe_name;
    st.pid = mem_.pid;
    st.career = career;
    st.in_match = match_state > 0x10000ULL;
    st.hooks = hooks_.active_hooks;
    st.training_mode = static_cast<int>(sb.cfgTrainingMultMode);
    st.stamina_mode = static_cast<int>(sb.cfgStaminaMode);
    st.freeze_clock = sb.cfgFreezeClock == 1;
    st.bal_lock = sb.cfgLockBALFocusPoints == 1;
    st.mode_code = "menu";

    if (match_state > 0x10000ULL) {
        int period = static_cast<int>(mem_.read_u32(match_state + 0x1168ULL));
        float prot_ticks = sb.valProtectedMatchTicks;
        float off_ticks = mem_.read_f32(match_state + 0x1174ULL);
        float half_ticks = std::max(0.0f, prot_ticks + off_ticks);
        int base_min = (period == 3) ? 45 : ((period == 5) ? 90 : ((period == 7) ? 105 : 0));
        double tot_min = static_cast<double>(base_min) + static_cast<double>(half_ticks) / 3600.0;
        int mm = static_cast<int>(std::floor(tot_min));
        int ss = static_cast<int>(std::floor((tot_min - mm) * 60.0 + 0.5));
        if (ss >= 60) { ++mm; ss = 0; }
        // Only fall back to ptrMatchHud when the clock tap hasn't recorded ticks/offsets yet.
        // While the game is paused, ptrMatchHud stays frozen at the pre-pause minute, whereas
        // (prot_ticks + off_ticks) immediately reflects user clock edits in real time.
        if (prot_ticks == 0.0f && off_ticks == 0.0f && sb.ptrMatchHud > 0x10000ULL) {
            int hud_min = mem_.read_i32(sb.ptrMatchHud + 0x12CULL);
            int hud_sec = mem_.read_i32(sb.ptrMatchHud + 0x130ULL);
            if (hud_min >= 0 && hud_min <= 150 && hud_sec >= 0 && hud_sec < 60) {
                mm = hud_min;
                ss = hud_sec;
            }
        }
        st.match_clock_valid = true;
        st.match_period = period;
        st.match_min = std::clamp(mm, 0, 150);
        st.match_sec = std::clamp(ss, 0, 59);
    }

    if (career > 0x10000ULL) {
        uint64_t bal_h = mem_.read_u64(career + 0x01819540ULL);
        st.mode_code = ((bal_h >> 32) != 0) ? "bal" : "ml";
        uint32_t uc = get_user_club_handle(career);
        if (uc) st.club = decode_club_handle(uc, career).second;
        st.has_finances = true;
        st.finances.ok = true;
        st.finances.transfer_raw = mem_.read_i32(career + 0x016ECBF4ULL);
        get_available_salary_budget(career, false, st.finances.salary_avail_raw, st.finances.salary_gross_raw, st.finances.squad_wages_raw);
        st.bal_focus = static_cast<int>(mem_.read_u8(career + 0x016ECBEAULL));
    }

    if (p_addr > 0x10000ULL) {
        uint8_t buf[data::PLAYER_RECORD_SIZE]{};
        if (mem_.read_into(p_addr, buf, sizeof(buf)) && is_sane_player_buf(buf)) {
            auto [ovr, reg_pos] = calc_ovr_from_buf(buf, sizeof(buf));
            st.has_player = true;
            st.player.addr = p_addr;
            st.player.name = extract_cstr(buf + 0x38, 0x2E);
            if (st.player.name.empty()) st.player.name = "?";
            st.player.ovr = ovr;
            st.player.pos = reg_pos;
            st.player.age = buf[0x1C] & 0x3F;
            st.player.stamina = static_cast<int>(get_bits_buf(buf, sizeof(buf), 0x146, 0, 7));
            st.player.arrow = std::min(4, static_cast<int>(get_bits_buf(buf, sizeof(buf), 0x147, 0, 3)));
            st.player.blink = get_bits_buf(buf, sizeof(buf), 0x146, 7, 1) != 0;
            st.player.injury = buf[0x148];
            uint32_t club_h = 0;
            std::memcpy(&club_h, buf + 0x12C, 4);
            st.player.club = decode_club_handle(club_h, career).second;
            auto uit = undo_stack_.find(p_addr);
            st.player.undo_count = (uit != undo_stack_.end()) ? static_cast<int>(uit->second.size()) : 0;
        }
    }
    return st;
}

PlayerDetails PESEngine::build_player_details_from_buf(uint64_t p, uint64_t career, const uint8_t* buf) {
    PlayerDetails d;
    d.ok = true;
    d.addr = p;
    auto [ovr, reg_pos] = calc_ovr_from_buf(buf, data::PLAYER_RECORD_SIZE);
    d.ovr = ovr;
    d.reg_pos = reg_pos;
    d.name = extract_cstr(buf + 0x38, 0x2E);
    d.shirt = extract_cstr(buf + 0x75, 0x12);
    std::memcpy(&d.pid, buf + 0x30, 4);
    uint32_t club_h = 0;
    std::memcpy(&club_h, buf + 0x12C, 4);
    d.club = decode_club_handle(club_h, career).second;
    uint16_t nat_raw = 0;
    std::memcpy(&nat_raw, buf + 0x144, 2);
    d.nationality = nat_raw & 0x1FF;
    d.age = buf[0x1C] & 0x3F;
    d.height = buf[0x00];
    d.weight = buf[0x01];
    d.foot = (buf[0x03] >> 7) & 1;
    d.playing_style = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x1D, 3, 5));
    d.wf_usage = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x23, 0, 2)) + 1;
    d.wf_acc = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x22, 4, 2)) + 1;
    d.form_cons = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x17, 4, 3)) + 1;
    d.inj_res = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x22, 6, 2)) + 1;
    d.stamina = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x146, 0, 7));
    d.arrow = std::min(4, static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x147, 0, 3)));
    d.blink = get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x146, 7, 1) != 0;
    d.injury = buf[0x148];
    uint16_t cyr = 0;
    std::memcpy(&cyr, buf + 0x138, 2);
    d.contract_yr = cyr;
    std::memcpy(&d.salary_raw, buf + 0x15C, 4);
    std::memcpy(&d.market_raw, buf + 0x174, 4);
    d.has_release_fee = get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x147, 6, 1) == 1;
    uint32_t d13c = 0;
    std::memcpy(&d13c, buf + 0x13C, 4);
    d.affection = static_cast<int>((d13c >> 18) & 0x7Fu);
    d.snap_age = buf[0x178];
    d.snap_ovr = buf[0x179];
    d.release_fee_raw = econ::calc_konami_release_fee_raw(d.market_raw, d.has_release_fee, d.age, d.affection, d.ovr);
    d.team_role = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x150, 0, 5));
    d.team_role_lvl = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x143, 6, 2));
    d.transfer_listed = get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x14A, 1, 1) == 1;
    d.loan_listed = get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x14A, 2, 1) == 1;

    d.stats.fill(40);
    for (const auto& s : data::STAT_DEFS) {
        d.stats[s.fid] = get_fid_from_buf(buf, s.fid);
    }
    for (int pi = 0; pi < 13; ++pi) {
        d.positions[pi] = {pi, calc_ovr_from_buf(buf, data::PLAYER_RECORD_SIZE, pi, false).first, get_apt_from_buf(buf, pi), pi == reg_pos};
    }
    for (size_t i = 0; i < data::PLAYER_SKILLS.size(); ++i) {
        const auto& sk = data::PLAYER_SKILLS[i];
        d.skills[i] = get_bits_buf(buf, data::PLAYER_RECORD_SIZE, sk.offset, sk.bit, 1) == 1;
    }
    for (size_t i = 0; i < data::COM_AI_STYLES.size(); ++i) {
        const auto& ai = data::COM_AI_STYLES[i];
        d.com_ai[i] = get_bits_buf(buf, data::PLAYER_RECORD_SIZE, ai.offset, ai.bit, 1) == 1;
    }
    auto uit = undo_stack_.find(p);
    d.undo_count = (uit != undo_stack_.end()) ? static_cast<int>(uit->second.size()) : 0;
    return d;
}

PlayerDetails PESEngine::get_player_details(uint64_t addr) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, cur_p = 0, ms = 0;
    sync_pointers(career, cur_p, ms);
    uint64_t p = addr ? addr : cur_p;
    if (p <= 0x10000ULL) return {false, "no_player"};
    uint8_t buf[data::PLAYER_RECORD_SIZE]{};
    if (!mem_.read_into(p, buf, sizeof(buf)) || !is_sane_player_buf(buf)) return {false, "read_failed"};
    return build_player_details_from_buf(p, career, buf);
}

// ---- Travas de valores -------------------------------------------------------------------------

void PESEngine::drop_value_lock_locked() {
    if (!vlock_.active) return;
    vlock_ = ValueLockState{};
    ++vlock_lost_;
}

void PESEngine::clear_value_lock() {
    std::lock_guard<std::recursive_mutex> g(lock_);
    vlock_ = ValueLockState{};   // desligar por vontade do usuario nao conta como perda
}

ActionResult PESEngine::set_value_lock(const ValueLockConfig& cfg_in, uint64_t addr) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!cfg_in.any()) {
        vlock_ = ValueLockState{};
        return {true, "ok"};
    }
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (addr > 0x10000ULL) p = addr;
    if (p <= 0x10000ULL) return {false, "no_player"};
    uint8_t buf[data::PLAYER_RECORD_SIZE]{};
    if (!mem_.read_into(p, buf, sizeof(buf)) || !is_sane_player_buf(buf)) return {false, "read_failed"};

    const ValueLockConfig cfg = clamp_lock_config(cfg_in);
    const uint32_t pid = record_player_id(buf);

    ValueLockState st;
    st.active = true;
    st.cfg = cfg;
    st.addr = p;
    st.player_id = pid;
    st.proc_pid = mem_.pid;
    {
        PlayerDetails d = build_player_details_from_buf(p, career, buf);
        st.name = d.name;
    }
    vlock_ = std::move(st);
    tick_value_lock_locked();   // efeito imediato, sem esperar o proximo ciclo
    return {true, "ok"};
}

void PESEngine::tick_value_lock_locked() {
    if (!vlock_.active) return;
    // Outro processo ou registro diferente do que foi travado: desliga em vez de escrever no lugar errado
    if (!mem_.attached() || mem_.pid != vlock_.proc_pid || vlock_.addr <= 0x10000ULL) {
        drop_value_lock_locked();
        return;
    }
    uint8_t buf[data::PLAYER_RECORD_SIZE]{};
    // O jogo recarrega os atletas (novo save, nova partida) em outro endereco e o bloco antigo continua legivel
    // com os valores velhos. Se o atleta capturado agora tem o mesmo id, a trava passa a seguir esse registro.
    if (manual_player_addr_ > 0x10000ULL && manual_player_addr_ != vlock_.addr) {
        uint8_t cand[data::PLAYER_RECORD_SIZE]{};
        if (mem_.read_into(manual_player_addr_, cand, sizeof(cand)) && is_sane_player_buf(cand) &&
            record_player_id(cand) == vlock_.player_id) {
            vlock_.addr = manual_player_addr_;
        }
    }
    if (!mem_.read_into(vlock_.addr, buf, sizeof(buf)) || !is_sane_player_buf(buf)) {
        // Telas de carregamento podem falhar uma leitura: so desliga depois de varias seguidas
        if (++vlock_.bad_reads >= 6) drop_value_lock_locked();
        return;
    }
    vlock_.bad_reads = 0;
    if (record_player_id(buf) != vlock_.player_id) {
        drop_value_lock_locked();
        return;
    }
    const uint64_t p = vlock_.addr;
    LockWrite writes[3];
    const int n = plan_lock_writes(buf, sizeof(buf), vlock_.cfg, writes);
    for (int i = 0; i < n; ++i) {
        switch (writes[i].field) {
            case LockField::Stamina: mem_.set_bits(p + kLockStaminaByte, 0, 7, writes[i].value); break;
            case LockField::Arrow:   mem_.set_bits(p + kLockArrowByte, 0, 3, writes[i].value); break;
            case LockField::Injury:  mem_.write_u8(p + kLockInjuryByte, 0); break;
        }
    }
}

PlayerDetails PESEngine::set_player_quick_condition(std::optional<int> stamina, std::optional<int> arrow, bool heal, uint64_t addr, std::optional<bool> blink) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, cur_p = 0, ms = 0;
    sync_pointers(career, cur_p, ms);
    uint64_t p = addr ? addr : cur_p;
    if (p <= 0x10000ULL) return {false, "no_player"};
    push_undo(p, "condition");
    if (stamina.has_value()) mem_.set_bits(p + 0x146ULL, 0, 7, static_cast<uint32_t>(std::clamp(*stamina, 0, 100)));
    if (arrow.has_value()) {
        int a = std::clamp(*arrow, 0, 4);
        mem_.set_bits(p + 0x147ULL, 0, 3, static_cast<uint32_t>(a));
    }
    if (blink.has_value()) mem_.set_bits(p + 0x146ULL, 7, 1, *blink ? 1u : 0u);
    if (heal) mem_.write_u8(p + 0x148ULL, 0);
    return get_player_details(p);
}

PlayerDetails PESEngine::update_player_bio(const BioUpdate& upd) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (p <= 0x10000ULL) return {false, "no_player"};

    // Atomic validation before undo or any RAM writes
    std::optional<int32_t> sr, mr;
    if (upd.salary_str.has_value() && !upd.salary_str->empty()) {
        sr = parse_human_money_to_raw(*upd.salary_str);
        if (!sr.has_value()) return {false, "invalid_money"};
    }
    if (upd.market_str.has_value() && !upd.market_str->empty()) {
        mr = parse_human_money_to_raw(*upd.market_str);
        if (!mr.has_value()) return {false, "invalid_money"};
    }

    uint8_t before_buf[data::PLAYER_RECORD_SIZE]{};
    if (!mem_.read_into(p, before_buf, sizeof(before_buf)) || !is_sane_player_buf(before_buf)) {
        return {false, "read_failed"};
    }
    int32_t before_sal = 0, before_mkt = 0;
    std::memcpy(&before_sal, before_buf + 0x15C, 4);
    std::memcpy(&before_mkt, before_buf + 0x174, 4);
    bool user_changed_sal = sr.has_value() && (*sr != before_sal);
    bool user_changed_mkt = mr.has_value() && (*mr != before_mkt);

    push_undo(p, "bio", before_buf);
    if (upd.name.has_value() && !upd.name->empty()) mem_.write_str(p + 0x38ULL, *upd.name, 46);
    if (upd.shirt.has_value()) {
        std::string sh = *upd.shirt;
        for (char& ch : sh) if (static_cast<unsigned char>(ch) < 128) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        mem_.write_str(p + 0x75ULL, sh, 18);
    }
    if (upd.nationality.has_value()) {
        uint16_t cur = mem_.read_u16(p + 0x144ULL);
        mem_.write_u16(p + 0x144ULL, static_cast<uint16_t>((cur & ~0x1FFu) | (static_cast<uint32_t>(*upd.nationality) & 0x1FFu)));
    }
    if (upd.age.has_value()) mem_.set_bits(p + 0x1CULL, 0, 6, static_cast<uint32_t>(std::clamp(*upd.age, 15, 50)));
    if (upd.height.has_value()) mem_.write_u8(p + 0x00ULL, static_cast<uint8_t>(std::clamp(*upd.height, 145, 215)));
    if (upd.weight.has_value()) mem_.write_u8(p + 0x01ULL, static_cast<uint8_t>(std::clamp(*upd.weight, 45, 125)));
    if (upd.foot.has_value()) mem_.set_bits(p + 0x03ULL, 7, 1, *upd.foot ? 1u : 0u);
    if (upd.contract_yr.has_value()) mem_.write_u16(p + 0x138ULL, static_cast<uint16_t>(std::clamp(*upd.contract_yr, 2019, 2150)));
    if (user_changed_sal) {
        mem_.write_i32(p + 0x15CULL, *sr);
        wage_cache_time_ = 0.0;
    }
    if (user_changed_mkt) {
        mem_.write_i32(p + 0x174ULL, *mr);
        uint8_t cur_buf[data::PLAYER_RECORD_SIZE]{};
        if (mem_.read_into(p, cur_buf, sizeof(cur_buf))) {
            auto [cur_ovr, _rp] = calc_ovr_from_buf(cur_buf, sizeof(cur_buf));
            uint8_t snap_pair[2] = {static_cast<uint8_t>(cur_buf[0x1C] & 0x3F), static_cast<uint8_t>(std::clamp(cur_ovr, 40, 99))};
            mem_.write_from(p + 0x178ULL, snap_pair, 2);
        }
    }
    if (upd.has_release_fee.has_value()) {
        mem_.set_bits(p + 0x147ULL, 6, 1, *upd.has_release_fee ? 1u : 0u);
    }
    if (upd.affection.has_value()) {
        mem_.set_bits(p + 0x13CULL, 18, 7, static_cast<uint32_t>(std::clamp(*upd.affection, 0, 100)));
    }
    if (upd.transfer_listed.has_value()) mem_.set_bits(p + 0x14AULL, 1, 1, *upd.transfer_listed ? 1u : 0u);
    if (upd.loan_listed.has_value()) mem_.set_bits(p + 0x14AULL, 2, 1, *upd.loan_listed ? 1u : 0u);
    if (upd.wf_usage.has_value()) mem_.set_bits(p + 0x23ULL, 0, 2, static_cast<uint32_t>(std::clamp(*upd.wf_usage - 1, 0, 3)));
    if (upd.wf_acc.has_value()) mem_.set_bits(p + 0x22ULL, 4, 2, static_cast<uint32_t>(std::clamp(*upd.wf_acc - 1, 0, 3)));
    if (upd.form_cons.has_value()) mem_.set_bits(p + 0x17ULL, 4, 3, static_cast<uint32_t>(std::clamp(*upd.form_cons - 1, 0, 7)));
    if (upd.inj_res.has_value()) mem_.set_bits(p + 0x22ULL, 6, 2, static_cast<uint32_t>(std::clamp(*upd.inj_res - 1, 0, 2)));
    if (upd.injury.has_value()) mem_.write_u8(p + 0x148ULL, static_cast<uint8_t>(std::clamp(*upd.injury, 0, 255)));

    uint8_t after_buf[data::PLAYER_RECORD_SIZE]{};
    if (!mem_.read_into(p, after_buf, sizeof(after_buf))) return get_player_details(p);

    EconSyncDelta ed;
    if (user_changed_sal || user_changed_mkt) {
        // User explicitly typed new market/salary: reset anchor so future edits scale from this manual value
        econ_anchors_.erase(p);
    }
    if ((upd.age.has_value() || upd.nationality.has_value()) && (!user_changed_mkt || !user_changed_sal)) {
        ed = apply_economy_sync_locked(p, after_buf, before_buf, !user_changed_mkt, !user_changed_sal);
    }
    PlayerDetails res = build_player_details_from_buf(p, career, after_buf);
    res.econ_delta = ed;
    return res;
}

EconSyncMode PESEngine::get_econ_sync_mode() {
    std::lock_guard<std::recursive_mutex> g(lock_);
    return econ_sync_mode_;
}

void PESEngine::set_econ_sync_mode(EconSyncMode mode) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    econ_sync_mode_ = mode;
}

static double calc_versatility_from_buf(const uint8_t* buf, size_t len) {
    if (!buf || len < 0x28) return 1.0;
    auto [pri_ovr, reg_pos] = calc_ovr_from_buf(buf, len);
    std::array<int, 13> pos_ovrs{};
    std::array<int, 13> pos_apts{};
    for (int pi = 0; pi < 13; ++pi) {
        pos_ovrs[pi] = calc_ovr_from_buf(buf, len, pi, false).first;
        pos_apts[pi] = get_apt_from_buf(buf, pi);
    }
    return econ::secondary_versatility_mult(reg_pos, pri_ovr, pos_ovrs, pos_apts);
}

EconSyncDelta PESEngine::apply_economy_sync_locked(
    uint64_t p, uint8_t* after_buf, const uint8_t* before_buf,
    bool allow_market, bool allow_salary)
{
    EconSyncDelta delta;
    delta.mode = econ_sync_mode_;
    if (econ_sync_mode_ == EconSyncMode::Off || !after_buf || !before_buf) return delta;

    auto [old_ovr, old_pos] = calc_ovr_from_buf(before_buf, data::PLAYER_RECORD_SIZE);
    auto [new_ovr, new_pos] = calc_ovr_from_buf(after_buf, data::PLAYER_RECORD_SIZE);
    double old_vers = calc_versatility_from_buf(before_buf, data::PLAYER_RECORD_SIZE);
    double new_vers = calc_versatility_from_buf(after_buf, data::PLAYER_RECORD_SIZE);
    int old_age = before_buf[0x1C] & 0x3F;
    int new_age = after_buf[0x1C] & 0x3F;
    uint16_t old_nat_raw = 0, new_nat_raw = 0;
    std::memcpy(&old_nat_raw, before_buf + 0x144, 2);
    std::memcpy(&new_nat_raw, after_buf + 0x144, 2);
    int old_nat = old_nat_raw & 0x1FF;
    int new_nat = new_nat_raw & 0x1FF;

    bool vers_changed = std::abs(new_vers - old_vers) > 1e-6;
    if (new_ovr == old_ovr && new_pos == old_pos && new_age == old_age && new_nat == old_nat && !vers_changed) {
        return delta;
    }

    uint32_t pid = 0;
    std::memcpy(&pid, before_buf + 0x30, 4);

    int32_t cur_mkt = 0, cur_sal = 0;
    std::memcpy(&cur_mkt, before_buf + 0x174, 4);
    std::memcpy(&cur_sal, before_buf + 0x15C, 4);
    bool has_rf = get_bits_buf(after_buf, data::PLAYER_RECORD_SIZE, 0x147, 6, 1) == 1;
    uint32_t d13c = 0;
    std::memcpy(&d13c, after_buf + 0x13C, 4);
    int aff = static_cast<int>((d13c >> 18) & 0x7Fu);

    auto& anc = econ_anchors_[p];
    if (!anc.valid || anc.pid != pid) {
        anc.pid = pid;
        anc.ovr = old_ovr;
        anc.age = old_age;
        anc.pos = old_pos;
        anc.nat = old_nat;
        anc.vers_mult = old_vers;
        anc.market_raw = cur_mkt;
        anc.salary_raw = cur_sal;
        anc.has_release_fee = has_rf;
        anc.release_fee_raw = econ::calc_konami_release_fee_raw(cur_mkt, has_rf, old_age, aff, old_ovr);
        anc.valid = true;
    }

    delta.old_ovr = old_ovr;
    delta.new_ovr = new_ovr;
    delta.old_market_raw = cur_mkt;
    delta.new_market_raw = cur_mkt;
    delta.old_salary_raw = cur_sal;
    delta.new_salary_raw = cur_sal;
    delta.has_release_fee = has_rf;
    delta.old_release_raw = econ::calc_konami_release_fee_raw(cur_mkt, has_rf, old_age, aff, old_ovr);
    delta.new_release_raw = delta.old_release_raw;

    bool do_market = allow_market && (econ_sync_mode_ == EconSyncMode::All || econ_sync_mode_ == EconSyncMode::MarketAndRelease);
    bool do_salary = allow_salary && (econ_sync_mode_ == EconSyncMode::All || econ_sync_mode_ == EconSyncMode::SalaryOnly);

    if (do_market) {
        int32_t nm = econ::scale_economy_value(
            anc.market_raw, anc.ovr, anc.age, anc.pos, anc.nat,
            new_ovr, new_age, new_pos, new_nat, false, anc.vers_mult, new_vers);
        std::memcpy(after_buf + 0x174, &nm, 4);
        after_buf[0x178] = static_cast<uint8_t>(std::clamp(new_age, 15, 63));
        after_buf[0x179] = static_cast<uint8_t>(std::clamp(new_ovr, 40, 99));
        mem_.write_i32(p + 0x174ULL, nm);
        uint8_t snap_pair[2] = {after_buf[0x178], after_buf[0x179]};
        mem_.write_from(p + 0x178ULL, snap_pair, 2);
        delta.new_market_raw = nm;
        delta.new_release_raw = econ::calc_konami_release_fee_raw(nm, has_rf, new_age, aff, new_ovr);
    }

    if (do_salary) {
        int32_t ns = econ::scale_economy_value(
            anc.salary_raw, anc.ovr, anc.age, anc.pos, anc.nat,
            new_ovr, new_age, new_pos, new_nat, true, anc.vers_mult, new_vers);
        std::memcpy(after_buf + 0x15C, &ns, 4);
        mem_.write_i32(p + 0x15CULL, ns);
        wage_cache_time_ = 0.0;
        delta.new_salary_raw = ns;
    }

    delta.applied = (delta.new_market_raw != delta.old_market_raw) || (delta.new_salary_raw != delta.old_salary_raw) || (delta.new_release_raw != delta.old_release_raw);
    return delta;
}

PlayerDetails PESEngine::recalibrate_player_economy(uint64_t addr) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, cur_p = 0, ms = 0;
    sync_pointers(career, cur_p, ms);
    uint64_t p = addr ? addr : cur_p;
    if (p <= 0x10000ULL) return {false, "no_player"};
    uint8_t buf[data::PLAYER_RECORD_SIZE]{};
    if (!mem_.read_into(p, buf, sizeof(buf)) || !is_sane_player_buf(buf)) return {false, "read_failed"};

    auto [cur_ovr, reg_pos] = calc_ovr_from_buf(buf, sizeof(buf));
    uint32_t pid = 0;
    std::memcpy(&pid, buf + 0x30, 4);
    int age = buf[0x1C] & 0x3F;
    uint16_t nat_raw = 0;
    std::memcpy(&nat_raw, buf + 0x144, 2);
    int nat = nat_raw & 0x1FF;
    int32_t old_mkt = 0, old_sal = 0;
    std::memcpy(&old_mkt, buf + 0x174, 4);
    std::memcpy(&old_sal, buf + 0x15C, 4);
    bool has_rf = get_bits_buf(buf, sizeof(buf), 0x147, 6, 1) == 1;
    uint32_t d13c = 0;
    std::memcpy(&d13c, buf + 0x13C, 4);
    int aff = static_cast<int>((d13c >> 18) & 0x7Fu);

    // Deterministic, idempotent recalibration directly from V3 calibrated curves
    // Primary/original position OVR dominates; playable secondary positions add a modest versatility factor.
    double vers = calc_versatility_from_buf(buf, sizeof(buf));
    int32_t new_mkt = econ::round_pes_money(econ::ref_market_raw(cur_ovr, age, reg_pos, nat) * vers, false);
    int32_t new_sal = econ::round_pes_money(econ::ref_salary_raw(cur_ovr, age, reg_pos, nat) * econ::salary_versatility_mult(vers), true);

    push_undo(p, "econ_recal", buf);
    EconSyncMode mode = (econ_sync_mode_ == EconSyncMode::Off) ? EconSyncMode::All : econ_sync_mode_;
    EconSyncDelta delta;
    delta.mode = mode;
    delta.old_ovr = cur_ovr;
    delta.new_ovr = cur_ovr;
    delta.old_market_raw = old_mkt;
    delta.new_market_raw = old_mkt;
    delta.old_salary_raw = old_sal;
    delta.new_salary_raw = old_sal;
    delta.has_release_fee = has_rf;
    delta.old_release_raw = econ::calc_konami_release_fee_raw(old_mkt, has_rf, age, aff, cur_ovr);
    delta.new_release_raw = delta.old_release_raw;

    if (mode == EconSyncMode::All || mode == EconSyncMode::MarketAndRelease) {
        std::memcpy(buf + 0x174, &new_mkt, 4);
        buf[0x178] = static_cast<uint8_t>(std::clamp(age, 15, 63));
        buf[0x179] = static_cast<uint8_t>(std::clamp(cur_ovr, 40, 99));
        mem_.write_i32(p + 0x174ULL, new_mkt);
        uint8_t snap_pair[2] = {buf[0x178], buf[0x179]};
        mem_.write_from(p + 0x178ULL, snap_pair, 2);
        delta.new_market_raw = new_mkt;
        delta.new_release_raw = econ::calc_konami_release_fee_raw(new_mkt, has_rf, age, aff, cur_ovr);
    }
    if (mode == EconSyncMode::All || mode == EconSyncMode::SalaryOnly) {
        std::memcpy(buf + 0x15C, &new_sal, 4);
        mem_.write_i32(p + 0x15CULL, new_sal);
        wage_cache_time_ = 0.0;
        delta.new_salary_raw = new_sal;
    }
    delta.applied = (delta.new_market_raw != delta.old_market_raw) ||
                    (delta.new_salary_raw != delta.old_salary_raw) ||
                    (delta.new_release_raw != delta.old_release_raw);

    // Re-anchor cleanly on the newly recalibrated baseline
    EconAnchor anc;
    anc.pid = pid;
    anc.ovr = cur_ovr;
    anc.age = age;
    anc.pos = reg_pos;
    anc.nat = nat;
    anc.vers_mult = vers;
    anc.market_raw = delta.new_market_raw;
    anc.salary_raw = delta.new_salary_raw;
    anc.has_release_fee = has_rf;
    anc.release_fee_raw = delta.new_release_raw;
    anc.valid = true;
    econ_anchors_[p] = anc;

    PlayerDetails res = build_player_details_from_buf(p, career, buf);
    res.econ_delta = delta;
    return res;
}

PlayerDetails PESEngine::set_player_stat(int fid, int val) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (p <= 0x10000ULL) return {false, "no_player"};
    uint8_t buf[data::PLAYER_RECORD_SIZE]{};
    if (!mem_.read_into(p, buf, sizeof(buf))) return {false, "read_failed"};
    uint8_t before_buf[data::PLAYER_RECORD_SIZE]{};
    std::memcpy(before_buf, buf, sizeof(buf));
    push_undo(p, "stat", buf);
    set_fid_in_buf(buf, fid, std::clamp(val, 40, 99));
    mem_.write_from(p, buf, 0x28);
    EconSyncDelta ed = apply_economy_sync_locked(p, buf, before_buf);
    PlayerDetails res = build_player_details_from_buf(p, career, buf);
    res.econ_delta = ed;
    return res;
}

PlayerDetails PESEngine::set_player_stats_batch(const std::vector<std::pair<int, int>>& items) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (p <= 0x10000ULL) return {false, "no_player"};
    uint8_t buf[data::PLAYER_RECORD_SIZE]{};
    if (!mem_.read_into(p, buf, sizeof(buf))) return {false, "read_failed"};
    uint8_t before_buf[data::PLAYER_RECORD_SIZE]{};
    std::memcpy(before_buf, buf, sizeof(buf));
    push_undo(p, "stats_batch", buf);
    for (const auto& [fid, v] : items) set_fid_in_buf(buf, fid, v);
    mem_.write_from(p, buf, 0x28);
    EconSyncDelta ed = apply_economy_sync_locked(p, buf, before_buf);
    PlayerDetails res = build_player_details_from_buf(p, career, buf);
    res.econ_delta = ed;
    return res;
}

PlayerDetails PESEngine::set_player_skill_idx(int idx, bool is_com_ai, bool enabled) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (p <= 0x10000ULL) return {false, "no_player"};
    const data::SkillDef* sk = nullptr;
    if (is_com_ai) {
        if (idx >= 0 && idx < static_cast<int>(data::COM_AI_STYLES.size())) sk = &data::COM_AI_STYLES[idx];
    } else {
        if (idx >= 0 && idx < static_cast<int>(data::PLAYER_SKILLS.size())) sk = &data::PLAYER_SKILLS[idx];
    }
    if (!sk) return {false, "invalid_arg"};
    push_undo(p, "skill");
    mem_.set_bits(p + sk->offset, sk->bit, 1, enabled ? 1u : 0u);
    return get_player_details(p);
}

PlayerDetails PESEngine::set_playing_style(int style_id) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (p <= 0x10000ULL) return {false, "no_player"};
    push_undo(p, "style");
    mem_.set_bits(p + 0x1DULL, 3, 5, static_cast<uint32_t>(std::clamp(style_id, 0, 21)));
    return get_player_details(p);
}

PlayerDetails PESEngine::set_team_role(int role_id, int level) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (p <= 0x10000ULL) return {false, "no_player"};
    push_undo(p, "team_role");
    mem_.set_bits(p + 0x150ULL, 0, 5, static_cast<uint32_t>(std::clamp(role_id, 0, 22)));
    mem_.set_bits(p + 0x143ULL, 6, 2, static_cast<uint32_t>(std::clamp(level, 0, 3)));
    return get_player_details(p);
}

PlayerDetails PESEngine::set_position_aptitude(int pos_idx, int grade) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (p <= 0x10000ULL || pos_idx < 0 || pos_idx >= 13 || grade < 0 || grade > 2) return {false, "invalid_arg"};
    uint8_t before_buf[data::PLAYER_RECORD_SIZE]{};
    if (!mem_.read_into(p, before_buf, sizeof(before_buf))) return {false, "read_failed"};
    push_undo(p, "pos_grade", before_buf);
    auto loc = data::POS_APT_LOC[pos_idx];
    mem_.set_bits(p + loc.off, loc.bit, 2, static_cast<uint32_t>(grade));
    uint8_t after_buf[data::PLAYER_RECORD_SIZE]{};
    if (!mem_.read_into(p, after_buf, sizeof(after_buf))) return get_player_details(p);
    EconSyncDelta ed = apply_economy_sync_locked(p, after_buf, before_buf);
    PlayerDetails res = build_player_details_from_buf(p, career, after_buf);
    res.econ_delta = ed;
    return res;
}

PlayerDetails PESEngine::set_registered_position(int pos_idx) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (p <= 0x10000ULL || pos_idx < 0 || pos_idx >= 13) return {false, "invalid_arg"};
    uint8_t before_buf[data::PLAYER_RECORD_SIZE]{};
    if (!mem_.read_into(p, before_buf, sizeof(before_buf))) return {false, "read_failed"};
    push_undo(p, "reg_pos", before_buf);
    mem_.set_bits(p + 0x07ULL, 4, 4, static_cast<uint32_t>(pos_idx));
    auto loc = data::POS_APT_LOC[pos_idx];
    mem_.set_bits(p + loc.off, loc.bit, 2, 2u);
    uint8_t after_buf[data::PLAYER_RECORD_SIZE]{};
    if (!mem_.read_into(p, after_buf, sizeof(after_buf))) return get_player_details(p);
    EconSyncDelta ed = apply_economy_sync_locked(p, after_buf, before_buf);
    PlayerDetails res = build_player_details_from_buf(p, career, after_buf);
    res.econ_delta = ed;
    return res;
}

static void attach_solver_info(PlayerDetails& d, const SolverResult& sr, const uint8_t* before_buf = nullptr, const uint8_t* after_buf = nullptr) {
    d.action_ok = sr.action_ok;
    d.code = sr.code;
    d.action_orig = sr.orig;
    d.action_final = sr.final_ovr;
    d.action_pri = sr.pri;
    d.action_pos = sr.pos;
    d.action_reg_pos = sr.reg_pos;
    d.action_needs_grade = sr.needs_grade;
    d.action_diffs.clear();
    if (before_buf && after_buf) {
        for (const auto& s : data::STAT_DEFS) {
            int ov = get_fid_from_buf(before_buf, s.fid);
            int nv = get_fid_from_buf(after_buf, s.fid);
            if (ov != nv) d.action_diffs.push_back({s.fid, ov, nv});
        }
        std::sort(d.action_diffs.begin(), d.action_diffs.end(), [](const StatDiff& a, const StatDiff& b) {
            int da = a.new_val - a.old_val;
            int db = b.new_val - b.old_val;
            if ((da > 0) != (db > 0)) return da > 0; // positive boosts first, then negative counterbalances
            if (std::abs(da) != std::abs(db)) return std::abs(da) > std::abs(db);
            return a.fid < b.fid;
        });
    }
}

PlayerDetails PESEngine::step_player_ovr(int delta) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (p <= 0x10000ULL) return {false, "no_player"};
    uint8_t buf[data::PLAYER_RECORD_SIZE]{};
    if (!mem_.read_into(p, buf, sizeof(buf))) return {false, "read_failed"};
    uint8_t before_buf[data::PLAYER_RECORD_SIZE]{};
    std::memcpy(before_buf, buf, sizeof(buf));
    auto [cur_ovr, reg_pos] = calc_ovr_from_buf(buf, sizeof(buf));
    SolverResult sr;
    EconSyncDelta ed;
    if (delta > 0 && cur_ovr >= 99) {
        sr.action_ok = true;
        sr.code = "already_at_ovr";
        sr.orig = cur_ovr;
        sr.final_ovr = cur_ovr;
        sr.pri = cur_ovr;
        sr.reg_pos = reg_pos;
    } else {
        sr = solve_primary_ovr_dna(buf, sizeof(buf), std::max(40, cur_ovr + delta));
        if (sr.has_write) {
            push_undo(p, "ovr_dna", buf);
            std::memcpy(buf, sr.out_header.data(), 0x28);
            mem_.write_from(p, buf, 0x28);
            ed = apply_economy_sync_locked(p, buf, before_buf);
        }
    }
    PlayerDetails res = build_player_details_from_buf(p, career, buf);
    attach_solver_info(res, sr, before_buf, buf);
    res.econ_delta = ed;
    return res;
}

PlayerDetails PESEngine::set_player_ovr(int target_ovr) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (p <= 0x10000ULL) return {false, "no_player"};
    uint8_t buf[data::PLAYER_RECORD_SIZE]{};
    if (!mem_.read_into(p, buf, sizeof(buf))) return {false, "read_failed"};
    uint8_t before_buf[data::PLAYER_RECORD_SIZE]{};
    std::memcpy(before_buf, buf, sizeof(buf));
    SolverResult sr = solve_primary_ovr_dna(buf, sizeof(buf), std::clamp(target_ovr, 40, 99));
    EconSyncDelta ed;
    if (sr.has_write) {
        push_undo(p, "ovr_dna", buf);
        std::memcpy(buf, sr.out_header.data(), 0x28);
        mem_.write_from(p, buf, 0x28);
        ed = apply_economy_sync_locked(p, buf, before_buf);
    }
    PlayerDetails res = build_player_details_from_buf(p, career, buf);
    attach_solver_info(res, sr, before_buf, buf);
    res.econ_delta = ed;
    return res;
}

PlayerDetails PESEngine::step_position_ovr(int pos_idx, int delta) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (p <= 0x10000ULL || pos_idx < 0 || pos_idx >= 13) return {false, "invalid_arg"};
    uint8_t buf[data::PLAYER_RECORD_SIZE]{};
    if (!mem_.read_into(p, buf, sizeof(buf))) return {false, "read_failed"};
    uint8_t before_buf[data::PLAYER_RECORD_SIZE]{};
    std::memcpy(before_buf, buf, sizeof(buf));
    int cur_ovr = calc_ovr_from_buf(buf, sizeof(buf), pos_idx, false).first;
    SolverResult sr = solve_secondary_isolated(buf, sizeof(buf), pos_idx, cur_ovr + delta);
    EconSyncDelta ed;
    if (sr.has_write) {
        push_undo(p, "pos_ovr", buf);
        std::memcpy(buf, sr.out_header.data(), 0x28);
        mem_.write_from(p, buf, 0x28);
        ed = apply_economy_sync_locked(p, buf, before_buf);
    }
    PlayerDetails res = build_player_details_from_buf(p, career, buf);
    attach_solver_info(res, sr, before_buf, buf);
    res.econ_delta = ed;
    return res;
}

PlayerDetails PESEngine::match_position_to_primary(int pos_idx) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (p <= 0x10000ULL || pos_idx < 0 || pos_idx >= 13) return {false, "invalid_arg"};
    uint8_t buf[data::PLAYER_RECORD_SIZE]{};
    if (!mem_.read_into(p, buf, sizeof(buf))) return {false, "read_failed"};
    uint8_t before_buf[data::PLAYER_RECORD_SIZE]{};
    std::memcpy(before_buf, buf, sizeof(buf));
    int pri_ovr = calc_ovr_from_buf(buf, sizeof(buf)).first;
    SolverResult sr = solve_secondary_isolated(buf, sizeof(buf), pos_idx, pri_ovr);
    EconSyncDelta ed;
    if (sr.has_write) {
        push_undo(p, "pos_ovr", buf);
        std::memcpy(buf, sr.out_header.data(), 0x28);
        mem_.write_from(p, buf, 0x28);
        ed = apply_economy_sync_locked(p, buf, before_buf);
    }
    PlayerDetails res = build_player_details_from_buf(p, career, buf);
    attach_solver_info(res, sr, before_buf, buf);
    res.econ_delta = ed;
    return res;
}

SquadRoster PESEngine::get_squad_roster() {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p_cur = 0, ms = 0;
    sync_pointers(career, p_cur, ms);
    if (!career) return {false, "no_career"};

    int cur_year = static_cast<int>(mem_.read_u16(career + 0x01642A24ULL));
    if (cur_year < 2019 || cur_year > 2150) cur_year = 2026;

    // Single bulk read: reuses the scanned table buffer with 0 extra syscalls per player!
    std::vector<uint8_t> raw_table;
    const auto& addrs = get_user_squad_addresses(career, true, &raw_table);

    SquadRoster r;
    r.ok = true;
    r.cur_year = cur_year;
    r.players.reserve(addrs.size());

    for (uint64_t p : addrs) {
        size_t off = static_cast<size_t>(p - career);
        if (off + data::PLAYER_RECORD_SIZE > raw_table.size()) continue;
        const uint8_t* buf = raw_table.data() + off;
        auto [ovr, reg_pos] = calc_ovr_from_buf(buf, data::PLAYER_RECORD_SIZE);
        SquadRow row;
        row.addr = p;
        row.name = extract_cstr(buf + 0x38, 0x2E);
        if (row.name.empty()) row.name = "?";
        row.name_norm = norm_text_utf8(row.name);
        row.ovr = ovr;
        row.pos = reg_pos;
        row.age = buf[0x1C] & 0x3F;
        uint16_t exp_y = 0;
        std::memcpy(&exp_y, buf + 0x138, 2);
        row.exp_y = (exp_y >= 2019 && exp_y <= 2150) ? static_cast<int>(exp_y) : 0;
        row.is_expiring = (exp_y >= 2019 && exp_y <= cur_year + 1);
        std::memcpy(&row.salary_raw, buf + 0x15C, 4);
        row.stamina = static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x146, 0, 7));
        row.arrow = std::min(4, static_cast<int>(get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x147, 0, 3)));
        row.blink = get_bits_buf(buf, data::PLAYER_RECORD_SIZE, 0x146, 7, 1) != 0;
        row.injury = buf[0x148];
        r.players.push_back(std::move(row));
    }

    std::sort(r.players.begin(), r.players.end(), [](const SquadRow& a, const SquadRow& b) {
        if (a.ovr != b.ovr) return a.ovr > b.ovr;
        return a.name < b.name;
    });
    uint32_t uc = get_user_club_handle(career);
    r.club = decode_club_handle(uc, career).second;
    return r;
}

ActionResult PESEngine::run_squad_commander(std::string_view mode, const std::vector<uint64_t>* only) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (mode != "stamina" && mode != "arrow" && mode != "heal" && mode != "contract" && mode != "ovr1" && mode != "econ_recal") {
        return {false, "invalid_arg"};
    }
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p_cur = 0, ms = 0;
    sync_pointers(career, p_cur, ms);
    if (!career) return {false, "no_career"};

    int cur_year = static_cast<int>(mem_.read_u16(career + 0x01642A24ULL));
    if (cur_year < 2019 || cur_year > 2150) cur_year = 2026;

    std::vector<uint8_t> raw_table;
    const auto all_addrs = get_user_squad_addresses(career, true, &raw_table);
    std::vector<uint64_t> addrs;
    if (only) {
        std::unordered_set<uint64_t> want(only->begin(), only->end());
        for (uint64_t a : all_addrs) if (want.count(a)) addrs.push_back(a);
        if (addrs.empty()) return {false, "no_player"};
    } else {
        addrs = all_addrs;
    }
    std::vector<BatchUndoItem> batch_backup;
    batch_backup.reserve(addrs.size());

    // Backup automatico do elenco inteiro antes de qualquer lote que mexe em valores
    std::string backup_path;
    if (mode == "econ_recal" || mode == "ovr1" || mode == "contract") {
        backup_path = backup_squad_locked(std::string("antes_") + std::string(mode));
        if (backup_path.empty()) return {false, "backup_failed"};
    }
    journal_begin_batch(std::string("batch_") + std::string(mode));
    struct JGuard { PESEngine* e; ~JGuard() { e->journal_end_batch(); } } jguard{this};

    auto save_batch_item = [&](uint64_t p, const uint8_t* snap) {
        if (!snap) return;
        BatchUndoItem bi;
        bi.addr = p;
        std::memcpy(bi.bytes.data(), snap, data::PLAYER_RECORD_SIZE);
        auto ait = econ_anchors_.find(p);
        if (ait != econ_anchors_.end()) bi.prev_anchor = ait->second;
        batch_backup.push_back(std::move(bi));
    };

    for (uint64_t p : addrs) {
        size_t off = static_cast<size_t>(p - career);
        const uint8_t* snap = (off + data::PLAYER_RECORD_SIZE <= raw_table.size()) ? (raw_table.data() + off) : nullptr;
        if (mode == "stamina") {
            save_batch_item(p, snap);
            push_undo(p, "squad_stamina", snap);
            mem_.set_bits(p + 0x146ULL, 0, 7, 100u);
        } else if (mode == "arrow") {
            save_batch_item(p, snap);
            push_undo(p, "squad_arrow", snap);
            mem_.set_bits(p + 0x147ULL, 0, 3, 4u);
            mem_.set_bits(p + 0x146ULL, 7, 1, 1u);
        } else if (mode == "heal") {
            save_batch_item(p, snap);
            push_undo(p, "squad_heal", snap);
            mem_.write_u8(p + 0x148ULL, 0);
        } else if (mode == "contract") {
            save_batch_item(p, snap);
            push_undo(p, "squad_contract", snap);
            uint16_t cur_exp = mem_.read_u16(p + 0x138ULL);
            int base_y = (cur_exp >= 2019 && cur_exp <= 2150) ? static_cast<int>(cur_exp) : cur_year;
            int new_exp = std::min(2150, std::max(base_y, cur_year + 5));
            mem_.write_u16(p + 0x138ULL, static_cast<uint16_t>(new_exp));
        } else if (mode == "ovr1" && snap) {
            auto [cur_ovr, _] = calc_ovr_from_buf(snap, data::PLAYER_RECORD_SIZE);
            if (cur_ovr < 99) {
                auto sr = solve_primary_ovr_dna(snap, data::PLAYER_RECORD_SIZE, std::min(99, cur_ovr + 1));
                if (sr.has_write) {
                    save_batch_item(p, snap);
                    push_undo(p, "ovr_dna", snap);
                    uint8_t after_buf[data::PLAYER_RECORD_SIZE]{};
                    std::memcpy(after_buf, snap, data::PLAYER_RECORD_SIZE);
                    std::memcpy(after_buf, sr.out_header.data(), 0x28);
                    mem_.write_from(p, after_buf, 0x28);
                    apply_economy_sync_locked(p, after_buf, snap);
                }
            }
        } else if (mode == "econ_recal" && snap) {
            save_batch_item(p, snap);
            push_undo(p, "squad_econ_recal", snap);
            auto [cur_ovr, reg_pos] = calc_ovr_from_buf(snap, data::PLAYER_RECORD_SIZE);
            uint32_t pid = 0;
            std::memcpy(&pid, snap + 0x30, 4);
            int age = snap[0x1C] & 0x3F;
            uint16_t nat_raw = 0;
            std::memcpy(&nat_raw, snap + 0x144, 2);
            int nat = nat_raw & 0x1FF;
            bool has_rf = get_bits_buf(snap, data::PLAYER_RECORD_SIZE, 0x147, 6, 1) == 1;
            uint32_t d13c = 0;
            std::memcpy(&d13c, snap + 0x13C, 4);
            int aff = static_cast<int>((d13c >> 18) & 0x7Fu);

            EconSyncMode eff_mode = (econ_sync_mode_ == EconSyncMode::Off) ? EconSyncMode::All : econ_sync_mode_;
            double vers = calc_versatility_from_buf(snap, data::PLAYER_RECORD_SIZE);
            int32_t new_mkt = econ::round_pes_money(econ::ref_market_raw(cur_ovr, age, reg_pos, nat) * vers, false);
            int32_t new_sal = econ::round_pes_money(econ::ref_salary_raw(cur_ovr, age, reg_pos, nat) * econ::salary_versatility_mult(vers), true);
            int32_t fin_mkt = 0, fin_sal = 0;
            std::memcpy(&fin_mkt, snap + 0x174, 4);
            std::memcpy(&fin_sal, snap + 0x15C, 4);
            if (eff_mode == EconSyncMode::All || eff_mode == EconSyncMode::MarketAndRelease) {
                mem_.write_i32(p + 0x174ULL, new_mkt);
                uint8_t sp[2] = {static_cast<uint8_t>(std::clamp(age, 15, 63)), static_cast<uint8_t>(std::clamp(cur_ovr, 40, 99))};
                mem_.write_from(p + 0x178ULL, sp, 2);
                fin_mkt = new_mkt;
            }
            if (eff_mode == EconSyncMode::All || eff_mode == EconSyncMode::SalaryOnly) {
                mem_.write_i32(p + 0x15CULL, new_sal);
                fin_sal = new_sal;
            }
            EconAnchor anc;
            anc.pid = pid; anc.ovr = cur_ovr; anc.age = age; anc.pos = reg_pos; anc.nat = nat;
            anc.vers_mult = vers;
            anc.market_raw = fin_mkt; anc.salary_raw = fin_sal;
            anc.has_release_fee = has_rf;
            anc.release_fee_raw = econ::calc_konami_release_fee_raw(fin_mkt, has_rf, age, aff, cur_ovr);
            anc.valid = true;
            econ_anchors_[p] = anc;
            wage_cache_time_ = 0.0;
        }
    }
    if (!batch_backup.empty()) {
        last_batch_undo_ = std::move(batch_backup);
        last_batch_label_ = std::string(mode);
    }
    return {true, "ok", static_cast<int>(addrs.size()), 0, !backup_path.empty(), backup_path};
}

ActionResult PESEngine::apply_skill_preset(int preset_id, const std::vector<uint64_t>* targets, bool replace) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (preset_id < 0 || preset_id >= static_cast<int>(presets::kSkillPresets.size())) return {false, "invalid_arg"};
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p_cur = 0, ms = 0;
    sync_pointers(career, p_cur, ms);

    std::vector<uint64_t> list;
    if (targets) list = *targets;
    else if (p_cur > 0x10000ULL) list.push_back(p_cur);
    {
        std::unordered_set<uint64_t> seen;
        std::vector<uint64_t> uniq;
        for (uint64_t a : list) if (seen.insert(a).second) uniq.push_back(a);
        list.swap(uniq);
    }
    if (list.empty()) return {false, "no_player"};

    // Resolve os nomes do preset para as definicoes de habilidade reais
    const auto& pr = presets::kSkillPresets[static_cast<size_t>(preset_id)];
    std::vector<const data::SkillDef*> wanted;
    for (auto nm : pr.skills) {
        if (nm.empty()) continue;
        for (const auto& sk : data::PLAYER_SKILLS) {
            if (nm == std::string_view(sk.key)) { wanted.push_back(&sk); break; }
        }
    }
    if (wanted.empty()) return {false, "invalid_arg"};

    std::string backup_path;
    if (list.size() > 1 || replace) {
        backup_path = backup_squad_locked("antes_preset_habilidades");
        if (backup_path.empty()) return {false, "backup_failed"};
    }

    std::vector<BatchUndoItem> batch_backup;
    journal_begin_batch("skill_preset");
    struct JGuard3 { PESEngine* e; ~JGuard3() { e->journal_end_batch(); } } jguard3{this};

    int changed = 0;
    for (uint64_t p : list) {
        uint8_t cur[data::PLAYER_RECORD_SIZE]{};
        if (!mem_.read_into(p, cur, sizeof(cur)) || !is_sane_player_buf(cur)) continue;
        // Limite nativo do jogo: 10 habilidades por atleta (nao ultrapassa em modo "adicionar")
        int have = 0;
        for (const auto& sk : data::PLAYER_SKILLS) {
            if (get_bits_buf(cur, sizeof(cur), sk.offset, sk.bit, 1) != 0 && !replace) ++have;
        }
        std::vector<const data::SkillDef*> to_add;
        for (const auto* sk : wanted) {
            bool on = get_bits_buf(cur, sizeof(cur), sk->offset, sk->bit, 1) != 0 && !replace;
            if (on) continue;
            if (have >= 10) break;
            to_add.push_back(sk);
            ++have;
        }
        if (!replace && to_add.empty()) continue; // ja tem as habilidades (ou esta no limite): nada a gravar

        BatchUndoItem bi;
        bi.addr = p;
        std::memcpy(bi.bytes.data(), cur, data::PLAYER_RECORD_SIZE);
        auto ait = econ_anchors_.find(p);
        if (ait != econ_anchors_.end()) bi.prev_anchor = ait->second;
        batch_backup.push_back(std::move(bi));
        push_undo(p, "skill_preset", cur);
        if (replace) {
            for (const auto& sk : data::PLAYER_SKILLS) mem_.set_bits(p + sk.offset, sk.bit, 1, 0u);
        }
        for (const auto* sk : to_add) mem_.set_bits(p + sk->offset, sk->bit, 1, 1u);
        ++changed;
    }
    if (changed > 0) {
        if (list.size() > 1) {
            last_batch_undo_ = std::move(batch_backup);
            last_batch_label_ = "skill_preset";
        }
        wage_cache_time_ = 0.0;
    }
    return {changed > 0, changed > 0 ? "ok" : "preset_noop", changed, 0, !backup_path.empty(), backup_path};
}

SearchResult PESEngine::search_players(std::string_view query, int pos_filter, int min_ovr, int max_age, bool free_only, int limit) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    double t0 = now_sec();
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p_cur = 0, ms = 0;
    sync_pointers(career, p_cur, ms);
    if (!career) return {false, "no_career"};

    uint32_t count = mem_.read_u32(career + 0x00D0BCE8ULL);
    if (count == 0 || count > 30001u) count = 30000u;

    std::vector<uint8_t> raw(static_cast<size_t>(count) * data::PLAYER_RECORD_SIZE);
    if (!mem_.read_into(career, raw.data(), raw.size())) return {false, "read_failed"};

    std::string q_norm = norm_text_utf8(query);
    bool has_q = !q_norm.empty();
    int movr = std::max(0, min_ovr);
    int mage = std::max(1, max_age);

    std::vector<SearchRow> results;
    results.reserve(256);

    for (uint32_t i = 0; i < count; ++i) {
        size_t off = static_cast<size_t>(i) * data::PLAYER_RECORD_SIZE;
        const uint8_t* rec = raw.data() + off;
        uint32_t pid = 0;
        std::memcpy(&pid, rec + 0x30, 4);
        if (pid == 0 || pid == 0xFFFFFFFFu) continue;
        int age = rec[0x1C] & 0x3F;
        if (age > mage) continue;
        int reg_pos = (rec[0x07] >> 4) & 0x0F;
        if (pos_filter >= 0 && reg_pos != pos_filter) continue;
        uint32_t club_h = 0;
        std::memcpy(&club_h, rec + 0x12C, 4);
        if (free_only && club_h != 0 && club_h != 0xFFFFFFFFu) continue;
        if (rec[0x38] == 0) continue;

        // Check OVR before UTF-8 string allocation when query is empty (ultra-fast filter!)
        int ovr = 0;
        if (!has_q) {
            ovr = calc_ovr_from_buf(rec, 0x28).first;
            if (ovr < movr) continue;
        }
        std::string name = extract_cstr(rec + 0x38, 0x2E);
        if (name.empty()) continue;
        if (has_q) {
            if (norm_text_utf8(name).find(q_norm) == std::string::npos) continue;
            ovr = calc_ovr_from_buf(rec, 0x28).first;
            if (ovr < movr) continue;
        }

        SearchRow row;
        row.addr = career + off;
        row.name = std::move(name);
        row.ovr = ovr;
        row.pos = reg_pos;
        row.age = age;
        row.club = decode_club_handle(club_h, career).second;
        std::memcpy(&row.salary_raw, rec + 0x15C, 4);
        uint16_t exp_y = 0;
        std::memcpy(&exp_y, rec + 0x138, 2);
        row.exp_y = (exp_y >= 2019 && exp_y <= 2150) ? static_cast<int>(exp_y) : 0;
        results.push_back(std::move(row));
    }

    int total = static_cast<int>(results.size());
    std::sort(results.begin(), results.end(), [](const SearchRow& a, const SearchRow& b) {
        if (a.ovr != b.ovr) return a.ovr > b.ovr;
        return a.name < b.name;
    });
    if (static_cast<int>(results.size()) > limit) results.resize(static_cast<size_t>(limit));

    SearchResult sr;
    sr.ok = true;
    sr.total = total;
    sr.elapsed_ms = (now_sec() - t0) * 1000.0;
    sr.players = std::move(results);
    return sr;
}

ScoutListsResult PESEngine::get_scout_lists() {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p_cur = 0, ms = 0;
    sync_pointers(career, p_cur, ms);
    if (!career) return {false, "no_career"};

    uint32_t count = mem_.read_u32(career + 0x00D0BCE8ULL);
    if (count == 0 || count > 30001u) count = 30000u;

    ScoutListsResult res;
    res.ok = true;
    {
        int cy = static_cast<int>(mem_.read_u16(career + 0x01642A24ULL));
        res.cur_year = (cy >= 2019 && cy <= 2150) ? cy : 2026;
    }
    for (const auto& sl : data::SCOUT_LISTS) {
        ScoutListState st;
        st.slot = sl.slot;
        st.cap = sl.cap;
        st.pt = sl.pt;
        st.en = sl.en;

        std::vector<uint64_t> handles(static_cast<size_t>(sl.cap), 0);
        if (mem_.read_into(career + sl.base, handles.data(), handles.size() * sizeof(uint64_t))) {
            for (int idx = 0; idx < sl.cap; ++idx) {
                uint64_t h = handles[idx];
                if (h != 0 && (h & 0xFFFFULL) != data::SCOUT_EMPTY_SENTINEL && (h >> 32) != 0) {
                    uint32_t lo = static_cast<uint32_t>(h & 0xFFFFFFFFULL);
                    if (lo < count) {
                        uint64_t p_addr = career + static_cast<uint64_t>(lo) * data::PLAYER_RECORD_SIZE;
                        uint8_t buf[data::PLAYER_RECORD_SIZE]{};
                        if (mem_.read_into(p_addr, buf, sizeof(buf))) {
                            uint64_t check_h = 0;
                            std::memcpy(&check_h, buf + 0x2C, 8);
                            if (check_h == h) {
                                auto [ovr, reg_pos] = calc_ovr_from_buf(buf, sizeof(buf));
                                uint32_t club_h = 0;
                                std::memcpy(&club_h, buf + 0x12C, 4);
                                ScoutEntry e;
                                e.slot_idx = idx;
                                e.handle = h;
                                e.addr = p_addr;
                                std::memcpy(&e.pid, buf + 0x30, 4);
                                e.name = extract_cstr(buf + 0x38, 0x2E);
                                e.ovr = ovr;
                                e.pos = reg_pos;
                                e.age = buf[0x1C] & 0x3F;
                                e.club = decode_club_handle(club_h, career).second;
                                uint16_t exp_y = 0;
                                std::memcpy(&exp_y, buf + 0x138, 2);
                                e.exp_y = (exp_y >= 2019 && exp_y <= 2150) ? static_cast<int>(exp_y) : 0;
                                std::memcpy(&e.market_raw, buf + 0x174, 4);
                                std::memcpy(&e.salary_raw, buf + 0x15C, 4);
                                e.arrow = std::min(4, static_cast<int>(get_bits_buf(buf, sizeof(buf), 0x147, 0, 3)));
                                e.blink = get_bits_buf(buf, sizeof(buf), 0x146, 7, 1) != 0;
                                e.injury = buf[0x148];
                                st.entries.push_back(std::move(e));
                            }
                        }
                    }
                }
            }
        }
        res.lists.push_back(std::move(st));
    }
    return res;
}

ActionResult PESEngine::add_to_scout_list(int slot_num, uint64_t player_addr) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, cur_p = 0, ms = 0;
    sync_pointers(career, cur_p, ms);
    uint64_t p = player_addr ? player_addr : cur_p;
    if (!career) return {false, "no_career"};
    if (p <= 0x10000ULL) return {false, "no_player"};

    const data::ScoutListDef* sl = nullptr;
    for (const auto& s : data::SCOUT_LISTS) if (s.slot == slot_num) { sl = &s; break; }
    if (!sl) return {false, "invalid_arg"};

    uint64_t p_handle = mem_.read_u64(p + 0x2CULL);
    if (!p_handle || (p_handle >> 32) == 0) return {false, "invalid_handle"};

    std::vector<uint64_t> handles(static_cast<size_t>(sl->cap), 0);
    if (!mem_.read_into(career + sl->base, handles.data(), handles.size() * sizeof(uint64_t))) return {false, "read_failed"};
    int free_idx = -1;
    for (int i = 0; i < sl->cap; ++i) {
        if (handles[i] == p_handle) return {true, "scout_dup", 0, slot_num};
        if (free_idx == -1 && (handles[i] == 0 || (handles[i] & 0xFFFFULL) == data::SCOUT_EMPTY_SENTINEL)) free_idx = i;
    }
    if (free_idx == -1) return {false, "scout_full", 0, slot_num};
    if (!scout_snapshot_push(career, "add")) return {false, "read_failed"};
    mem_.write_u64(career + sl->base + static_cast<uint64_t>(free_idx) * 8ULL, p_handle);
    return {true, "scout_added", 0, slot_num};
}

ActionResult PESEngine::remove_from_scout_list(int slot_num, uint64_t handle) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, cur_p = 0, ms = 0;
    sync_pointers(career, cur_p, ms);
    const data::ScoutListDef* sl = nullptr;
    for (const auto& s : data::SCOUT_LISTS) if (s.slot == slot_num) { sl = &s; break; }
    if (!career || !sl || !handle) return {false, "invalid_arg"};

    std::vector<uint64_t> handles(static_cast<size_t>(sl->cap), 0);
    if (!mem_.read_into(career + sl->base, handles.data(), handles.size() * sizeof(uint64_t))) return {false, "read_failed"};
    for (int i = 0; i < sl->cap; ++i) {
        if (handles[i] == handle) {
            if (!scout_snapshot_push(career, "remove")) return {false, "read_failed"};
            mem_.write_u64(career + sl->base + static_cast<uint64_t>(i) * 8ULL, data::SCOUT_EMPTY_SENTINEL);
            return {true, "ok"};
        }
    }
    return {false, "not_found"};
}

ActionResult PESEngine::clear_scout_list(int slot_num) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, cur_p = 0, ms = 0;
    sync_pointers(career, cur_p, ms);
    const data::ScoutListDef* sl = nullptr;
    for (const auto& s : data::SCOUT_LISTS) if (s.slot == slot_num) { sl = &s; break; }
    if (!career || !sl) return {false, "invalid_arg"};
    if (!scout_snapshot_push(career, "clear")) return {false, "read_failed"};
    std::vector<uint64_t> empty_buf(static_cast<size_t>(sl->cap), data::SCOUT_EMPTY_SENTINEL);
    mem_.write_from(career + sl->base, empty_buf.data(), empty_buf.size() * sizeof(uint64_t));
    return {true, "ok", 0, slot_num};
}

static_assert(data::SCOUT_LISTS.size() == 4, "ScoutSnap assume 4 listas de olheiros");

namespace {
inline bool scout_slot_free(uint64_t h) { return h == 0 || (h & 0xFFFFULL) == data::SCOUT_EMPTY_SENTINEL; }
const data::ScoutListDef* find_scout_def(int slot) {
    for (const auto& s : data::SCOUT_LISTS) if (s.slot == slot) return &s;
    return nullptr;
}
} // namespace

bool PESEngine::scout_snapshot_push(uint64_t career, const char* label) {
    ScoutSnap snap;
    snap.label = label ? label : "";
    for (size_t i = 0; i < data::SCOUT_LISTS.size(); ++i) {
        const auto& sl = data::SCOUT_LISTS[i];
        snap.lists[i].assign(static_cast<size_t>(sl.cap), data::SCOUT_EMPTY_SENTINEL);
        if (!mem_.read_into(career + sl.base, snap.lists[i].data(), snap.lists[i].size() * sizeof(uint64_t))) return false;
    }
    scout_undo_.push_back(std::move(snap));
    if (scout_undo_.size() > 12) scout_undo_.erase(scout_undo_.begin());
    return true;
}

ActionResult PESEngine::undo_scout_change() {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, cur_p = 0, ms = 0;
    sync_pointers(career, cur_p, ms);
    if (!career) return {false, "no_career"};
    if (scout_undo_.empty()) return {false, "nothing_to_undo"};
    ScoutSnap snap = std::move(scout_undo_.back());
    scout_undo_.pop_back();
    for (size_t i = 0; i < data::SCOUT_LISTS.size(); ++i) {
        const auto& sl = data::SCOUT_LISTS[i];
        if (snap.lists[i].size() != static_cast<size_t>(sl.cap)) continue;
        mem_.write_from(career + sl.base, snap.lists[i].data(), snap.lists[i].size() * sizeof(uint64_t));
    }
    return {true, "ok", static_cast<int>(scout_undo_.size()), 0, false, snap.label};
}

ActionResult PESEngine::add_many_to_scout_list(int slot_num, const std::vector<uint64_t>& player_addrs) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, cur_p = 0, ms = 0;
    sync_pointers(career, cur_p, ms);
    if (!career) return {false, "no_career"};
    const auto* sl = find_scout_def(slot_num);
    if (!sl || player_addrs.empty()) return {false, "invalid_arg"};

    std::vector<uint64_t> handles(static_cast<size_t>(sl->cap), 0);
    if (!mem_.read_into(career + sl->base, handles.data(), handles.size() * sizeof(uint64_t))) return {false, "read_failed"};
    bool snapped = false;
    int added = 0, skipped = 0;
    bool full = false;
    for (uint64_t p : player_addrs) {
        if (p <= 0x10000ULL) { ++skipped; continue; }
        {
            uint8_t probe[data::PLAYER_RECORD_SIZE]{};
            if (!mem_.read_into(p, probe, sizeof(probe)) || !is_sane_player_buf(probe)) { ++skipped; continue; }
        }
        uint64_t h = mem_.read_u64(p + 0x2CULL);
        if (!h || (h >> 32) == 0) { ++skipped; continue; }
        if (std::find(handles.begin(), handles.end(), h) != handles.end()) { ++skipped; continue; }
        int free_idx = -1;
        for (int i = 0; i < sl->cap; ++i) if (scout_slot_free(handles[i])) { free_idx = i; break; }
        if (free_idx < 0) { full = true; ++skipped; continue; }
        if (!snapped) {
            if (!scout_snapshot_push(career, "add_many")) return {false, "read_failed", added, skipped, full};
            snapped = true;
        }
        handles[free_idx] = h;
        mem_.write_u64(career + sl->base + static_cast<uint64_t>(free_idx) * 8ULL, h);
        ++added;
    }
    return {added > 0, added > 0 ? "ok" : (full ? "scout_full" : "scout_dup"), added, skipped, full};
}

ActionResult PESEngine::remove_many_from_scout_list(int slot_num, const std::vector<uint64_t>& handles_to_remove) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, cur_p = 0, ms = 0;
    sync_pointers(career, cur_p, ms);
    if (!career) return {false, "no_career"};
    const auto* sl = find_scout_def(slot_num);
    if (!sl || handles_to_remove.empty()) return {false, "invalid_arg"};

    std::vector<uint64_t> handles(static_cast<size_t>(sl->cap), 0);
    if (!mem_.read_into(career + sl->base, handles.data(), handles.size() * sizeof(uint64_t))) return {false, "read_failed"};
    bool snapped = false;
    int removed = 0;
    for (uint64_t h : handles_to_remove) {
        if (scout_slot_free(h)) continue; // 0 ou sentinela nunca e um atleta
        for (int i = 0; i < sl->cap; ++i) {
            if (handles[i] == h) {
                if (!snapped) {
                    if (!scout_snapshot_push(career, "remove_many")) return {false, "read_failed", removed, 0};
                    snapped = true;
                }
                handles[i] = data::SCOUT_EMPTY_SENTINEL;
                mem_.write_u64(career + sl->base + static_cast<uint64_t>(i) * 8ULL, data::SCOUT_EMPTY_SENTINEL);
                ++removed;
                break;
            }
        }
    }
    return {removed > 0, removed > 0 ? "ok" : "not_found", removed, static_cast<int>(handles_to_remove.size()) - removed};
}

ActionResult PESEngine::move_scout_entries(int from_slot, int to_slot, const std::vector<uint64_t>& handles_in, bool copy) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, cur_p = 0, ms = 0;
    sync_pointers(career, cur_p, ms);
    if (!career) return {false, "no_career"};
    const auto* src = find_scout_def(from_slot);
    const auto* dst = find_scout_def(to_slot);
    if (!src || !dst || from_slot == to_slot || handles_in.empty()) return {false, "invalid_arg"};

    std::vector<uint64_t> sh(static_cast<size_t>(src->cap), 0), dh(static_cast<size_t>(dst->cap), 0);
    if (!mem_.read_into(career + src->base, sh.data(), sh.size() * sizeof(uint64_t))) return {false, "read_failed"};
    if (!mem_.read_into(career + dst->base, dh.data(), dh.size() * sizeof(uint64_t))) return {false, "read_failed"};
    bool snapped = false;
    auto ensure_snap = [&]() {
        if (!snapped) snapped = scout_snapshot_push(career, copy ? "copy" : "move");
        return snapped;
    };

    int done = 0, skipped = 0;
    bool full = false;
    for (uint64_t h : handles_in) {
        if (scout_slot_free(h)) { ++skipped; continue; }
        int sidx = -1;
        for (int i = 0; i < src->cap; ++i) if (sh[i] == h) { sidx = i; break; }
        if (sidx < 0) { ++skipped; continue; }
        bool already = std::find(dh.begin(), dh.end(), h) != dh.end();
        if (!already) {
            int free_idx = -1;
            for (int i = 0; i < dst->cap; ++i) if (scout_slot_free(dh[i])) { free_idx = i; break; }
            if (free_idx < 0) { full = true; ++skipped; continue; }
            if (!ensure_snap()) return {false, "read_failed", done, skipped, full};
            dh[free_idx] = h;
            mem_.write_u64(career + dst->base + static_cast<uint64_t>(free_idx) * 8ULL, h);
        }
        // Mover remove da origem tambem quando o atleta ja estava no destino (evita duplicata entre listas)
        if (!copy) {
            if (!ensure_snap()) return {false, "read_failed", done, skipped, full};
            sh[sidx] = data::SCOUT_EMPTY_SENTINEL;
            mem_.write_u64(career + src->base + static_cast<uint64_t>(sidx) * 8ULL, data::SCOUT_EMPTY_SENTINEL);
        }
        if (!already || !copy) ++done; else ++skipped;
    }
    return {done > 0, done > 0 ? "ok" : (full ? "scout_full" : "scout_dup"), done, skipped, full};
}

ActionResult PESEngine::export_scout_lists_json() {
    std::lock_guard<std::recursive_mutex> g(lock_);
    auto lists = get_scout_lists();
    if (!lists.ok) return {false, lists.code};
    using json = nlohmann::json;
    json root;
    root["version"] = "13.0";
    root["kind"] = "scouts";
    root["created_at"] = cur_iso_datetime();
    root["lists"] = json::array();
    int total = 0;
    for (const auto& l : lists.lists) {
        json jl;
        jl["slot"] = l.slot;
        jl["name"] = l.en;
        jl["entries"] = json::array();
        for (const auto& e : l.entries) {
            jl["entries"].push_back({{"pid", e.pid}, {"name", e.name}, {"ovr", e.ovr}, {"pos", e.pos}, {"age", e.age}, {"club", e.club}});
            ++total;
        }
        root["lists"].push_back(std::move(jl));
    }
    return {true, "ok", total, 0, false, root.dump(2)};
}

ActionResult PESEngine::import_scout_lists_json(std::string_view json_str, bool replace) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, cur_p = 0, ms = 0;
    sync_pointers(career, cur_p, ms);
    if (!career) return {false, "no_career"};

    using json = nlohmann::json;
    try {
    json root = json::parse(json_str.begin(), json_str.end(), nullptr, false);
    if (root.is_discarded() || !root.is_object() || !root.contains("kind") || !root["kind"].is_string() ||
        root["kind"].get<std::string>() != "scouts" || !root.contains("lists") || !root["lists"].is_array()) {
        return {false, "invalid_json"};
    }

    uint32_t count = mem_.read_u32(career + 0x00D0BCE8ULL);
    if (count == 0 || count > 30001u) count = 30000u;
    std::vector<uint8_t> raw(static_cast<size_t>(count) * data::PLAYER_RECORD_SIZE);
    if (!mem_.read_into(career, raw.data(), raw.size())) return {false, "read_failed"};

    // Indices por ID do atleta e por nome normalizado (so vale se o nome for unico)
    std::unordered_map<uint32_t, uint32_t> by_pid;
    std::unordered_map<std::string, std::pair<uint32_t, int>> by_name;
    by_pid.reserve(count * 2);
    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t* rec = raw.data() + static_cast<size_t>(i) * data::PLAYER_RECORD_SIZE;
        if (!is_sane_player_buf(rec)) continue;
        uint32_t pid = 0;
        std::memcpy(&pid, rec + 0x30, 4);
        if (pid == 0) continue;
        by_pid.emplace(pid, i);
        std::string nm = norm_text_utf8(extract_cstr(rec + 0x38, 0x2E));
        if (nm.empty()) continue;
        auto it = by_name.find(nm);
        if (it == by_name.end()) by_name.emplace(std::move(nm), std::make_pair(i, 1));
        else ++it->second.second;
    }

    bool snapped = false;
    auto ensure_snap = [&]() {
        if (!snapped) snapped = scout_snapshot_push(career, "import");
        return snapped;
    };
    int added = 0, unmatched = 0, dup = 0;
    bool full = false;
    std::string unmatched_names;
    for (const auto& jl : root["lists"]) {
        if (!jl.is_object() || !jl.contains("slot") || !jl["slot"].is_number_integer()) continue;
        const auto* sl = find_scout_def(jl["slot"].get<int>());
        if (!sl || !jl.contains("entries") || !jl["entries"].is_array()) continue;
        std::vector<uint64_t> handles(static_cast<size_t>(sl->cap), 0);
        if (!mem_.read_into(career + sl->base, handles.data(), handles.size() * sizeof(uint64_t))) continue;
        bool cleared = !replace;
        for (const auto& je : jl["entries"]) {
            if (!je.is_object()) continue;
            uint32_t pid = (je.contains("pid") && je["pid"].is_number_unsigned()) ? je["pid"].get<uint32_t>() : 0u;
            std::string name = (je.contains("name") && je["name"].is_string()) ? je["name"].get<std::string>() : std::string();
            int64_t idx = -1;
            auto pit = by_pid.find(pid);
            if (pid != 0 && pit != by_pid.end()) {
                // Confere o nome para nao cruzar IDs reaproveitados entre saves diferentes
                const uint8_t* rec = raw.data() + static_cast<size_t>(pit->second) * data::PLAYER_RECORD_SIZE;
                if (name.empty() || norm_text_utf8(extract_cstr(rec + 0x38, 0x2E)) == norm_text_utf8(name)) idx = pit->second;
            }
            if (idx < 0 && !name.empty()) {
                auto nit = by_name.find(norm_text_utf8(name));
                if (nit != by_name.end() && nit->second.second == 1) idx = nit->second.first;
            }
            if (idx < 0) {
                ++unmatched;
                if (unmatched_names.size() < 600) { if (!unmatched_names.empty()) unmatched_names += ", "; unmatched_names += name.empty() ? "?" : name; }
                continue;
            }
            const uint8_t* rec = raw.data() + static_cast<size_t>(idx) * data::PLAYER_RECORD_SIZE;
            uint64_t h = 0;
            std::memcpy(&h, rec + 0x2C, 8);
            if (!h || (h >> 32) == 0) { ++unmatched; continue; }
            if (!cleared) {
                // Primeiro atleta casado desta lista: agora sim limpa a lista antes de preencher
                if (!ensure_snap()) return {false, "read_failed", added, unmatched};
                std::fill(handles.begin(), handles.end(), data::SCOUT_EMPTY_SENTINEL);
                mem_.write_from(career + sl->base, handles.data(), handles.size() * sizeof(uint64_t));
                cleared = true;
            }
            if (std::find(handles.begin(), handles.end(), h) != handles.end()) { ++dup; continue; }
            int free_idx = -1;
            for (int i = 0; i < sl->cap; ++i) if (scout_slot_free(handles[i])) { free_idx = i; break; }
            if (free_idx < 0) { full = true; continue; }
            if (!ensure_snap()) return {false, "read_failed", added, unmatched};
            handles[free_idx] = h;
            mem_.write_u64(career + sl->base + static_cast<uint64_t>(free_idx) * 8ULL, h);
            ++added;
        }
    }
    return {true, "ok", added, unmatched, full || dup > 0, unmatched_names};
    } catch (const std::exception&) {
        return {false, "invalid_json"};
    }
}

ActionResult PESEngine::set_training_mode(int mode) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    int m = std::clamp(mode, 0, 5);
    if (m > 0 && !hooks_.enable_hook_3_training()) return {false, "hook_failed"};
    mem_.write_u32(hooks_.sym_addr(HookSym::cfgTrainingMultMode), static_cast<uint32_t>(m));
    return {true, "ok", 0, m, hooks_.active_hooks[3]};
}

ActionResult PESEngine::set_stamina_mode(int mode) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    int m = std::clamp(mode, 0, 5);
    if (m > 0 && !hooks_.enable_hook_6_stamina()) return {false, "hook_failed"};
    mem_.write_u32(hooks_.sym_addr(HookSym::cfgStaminaMode), static_cast<uint32_t>(m));
    return {true, "ok", 0, m, hooks_.active_hooks[6]};
}

ActionResult PESEngine::set_bal_focus(int points, bool lock_enabled) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, cur_p = 0, ms = 0;
    sync_pointers(career, cur_p, ms);
    if (!career) return {false, "no_career"};
    int pts = std::clamp(points, 0, 99);
    if (pts > 21 || lock_enabled) hooks_.enable_hook_4_bal_cap();
    bal_lock_target_ = static_cast<uint8_t>(pts);
    mem_.write_u8(career + 0x016ECBEAULL, bal_lock_target_);
    mem_.write_u32(hooks_.sym_addr(HookSym::cfgLockBALFocusPoints), lock_enabled ? 1u : 0u);
    return {true, "ok", 0, pts, lock_enabled};
}

float PESEngine::read_protected_match_ticks_locked(bool just_armed) {
    float prot = mem_.read_f32(hooks_.sym_addr(HookSym::valProtectedMatchTicks));
    if (prot == 0.0f && just_armed) {
        for (int i = 0; i < 4; ++i) {
            Sleep(10);
            prot = mem_.read_f32(hooks_.sym_addr(HookSym::valProtectedMatchTicks));
            if (prot != 0.0f) break;
        }
    }
    return prot;
}

ActionResult PESEngine::set_freeze_clock(bool freeze) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    if (freeze) {
        uint64_t c = 0, p = 0, ms = 0;
        sync_pointers(c, p, ms);
        if (ms <= 0x10000ULL) return {false, "not_in_match"};
        bool was_armed = hooks_.active_hooks[5];
        if (!hooks_.enable_hook_5_match()) return {false, "hook_failed"};
        float prot = read_protected_match_ticks_locked(!was_armed);
        float off_t = mem_.read_f32(ms + 0x1174ULL);
        mem_.write_f32(hooks_.sym_addr(HookSym::valFrozenTotalTicks), prot + off_t);
    }
    if (hooks_.state_base) {
        mem_.write_u32(hooks_.sym_addr(HookSym::cfgFreezeClock), freeze ? 1u : 0u);
    }
    return {true, "ok", 0, 0, freeze};
}

ActionResult PESEngine::set_match_clock(int target_min, int target_sec) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t c = 0, p = 0, ms = 0;
    sync_pointers(c, p, ms);
    if (ms <= 0x10000ULL) return {false, "not_in_match"};
    bool was_armed = hooks_.active_hooks[5];
    if (!hooks_.enable_hook_5_match()) return {false, "hook_failed"};

    float prot = read_protected_match_ticks_locked(!was_armed);

    int tmin = std::clamp(target_min, 0, 130);
    int tsec = std::clamp(target_sec, 0, 59);
    double target_f = static_cast<double>(tmin) + static_cast<double>(tsec) / 60.0;

    int period = static_cast<int>(mem_.read_u32(ms + 0x1168ULL));
    int base_min = (period == 3) ? 45 : ((period == 5) ? 90 : ((period == 7) ? 105 : 0));
    if (period == 1 && target_f >= 50.0) {
        mem_.write_u32(ms + 0x1168ULL, 3u);
        base_min = 45;
    } else if (period == 3 && target_f < 45.0) {
        mem_.write_u32(ms + 0x1168ULL, 1u);
        base_min = 0;
    }

    double half_min = std::max(0.0, target_f - static_cast<double>(base_min));
    float desired_half_ticks = static_cast<float>(half_min * 3600.0);
    mem_.write_f32(ms + 0x1174ULL, desired_half_ticks - prot);
    if (mem_.read_u32(hooks_.sym_addr(HookSym::cfgFreezeClock)) == 1u) {
        mem_.write_f32(hooks_.sym_addr(HookSym::valFrozenTotalTicks), desired_half_ticks);
    }
    return {true, "ok", tmin, tsec, false};
}

ActionResult PESEngine::shift_match_clock(int delta_min) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t c = 0, p = 0, ms = 0;
    sync_pointers(c, p, ms);
    if (ms <= 0x10000ULL) return {false, "not_in_match"};
    bool was_armed = hooks_.active_hooks[5];
    if (!hooks_.enable_hook_5_match()) return {false, "hook_failed"};

    float prot = read_protected_match_ticks_locked(!was_armed);
    float off_t = mem_.read_f32(ms + 0x1174ULL);
    int period = static_cast<int>(mem_.read_u32(ms + 0x1168ULL));
    int base_min = (period == 3) ? 45 : ((period == 5) ? 90 : ((period == 7) ? 105 : 0));
    double cur_min = static_cast<double>(base_min) + static_cast<double>(std::max(0.0f, prot + off_t)) / 3600.0;
    double next_min = std::max(static_cast<double>(base_min), cur_min + static_cast<double>(delta_min));
    int mm = static_cast<int>(std::floor(next_min));
    int ss = static_cast<int>(std::floor((next_min - mm) * 60.0 + 0.5));
    if (ss >= 60) { ++mm; ss = 0; }
    return set_match_clock(mm, ss);
}

ActionResult PESEngine::reset_match_half() {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t c = 0, p = 0, ms = 0;
    sync_pointers(c, p, ms);
    if (ms <= 0x10000ULL) return {false, "not_in_match"};
    bool was_armed = hooks_.active_hooks[5];
    if (!hooks_.enable_hook_5_match()) return {false, "hook_failed"};
    float prot = read_protected_match_ticks_locked(!was_armed);
    int period = static_cast<int>(mem_.read_u32(ms + 0x1168ULL));
    int base_min = (period == 3) ? 45 : ((period == 5) ? 90 : ((period == 7) ? 105 : 0));
    mem_.write_f32(ms + 0x1174ULL, -prot);
    if (mem_.read_u32(hooks_.sym_addr(HookSym::cfgFreezeClock)) == 1u) {
        mem_.write_f32(hooks_.sym_addr(HookSym::valFrozenTotalTicks), 0.0f);
    }
    return {true, "ok", base_min, 0, false};
}

ActionResult PESEngine::end_match_half() {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t c = 0, p = 0, ms = 0;
    sync_pointers(c, p, ms);
    if (ms <= 0x10000ULL) return {false, "not_in_match"};
    bool was_armed = hooks_.active_hooks[5];
    if (!hooks_.enable_hook_5_match()) return {false, "hook_failed"};
    float prot = read_protected_match_ticks_locked(!was_armed);
    int period = static_cast<int>(mem_.read_u32(ms + 0x1168ULL));
    int base_min = (period == 3) ? 45 : ((period == 5) ? 90 : ((period == 7) ? 105 : 0));
    float max_half_min = (period >= 5) ? 15.5f : 46.5f;
    float desired_ticks = max_half_min * 3600.0f;
    mem_.write_f32(ms + 0x1174ULL, desired_ticks - prot);
    if (mem_.read_u32(hooks_.sym_addr(HookSym::cfgFreezeClock)) == 1u) {
        mem_.write_f32(hooks_.sym_addr(HookSym::valFrozenTotalTicks), desired_ticks);
    }
    int end_m = base_min + static_cast<int>(max_half_min);
    int end_s = static_cast<int>((max_half_min - std::floor(max_half_min)) * 60.0f);
    return {true, "ok", end_m, end_s, false};
}

ActionResult PESEngine::export_player_json() {
    return export_player_pack_json(0);
}

PlayerDetails PESEngine::import_player_json(std::string_view json_str) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p = 0, ms = 0;
    sync_pointers(career, p, ms);
    if (p <= 0x10000ULL) return {false, "no_player"};
    try {
        // Accept structured v13 pack JSON OR legacy v12 hex JSON seamlessly
        RosterPack pack;
        if (parse_roster_pack_json(json_str, pack) && !pack.players.empty()) {
            uint8_t cur[data::PLAYER_RECORD_SIZE]{};
            if (!mem_.read_into(p, cur, sizeof(cur)) || !is_sane_player_buf(cur)) return {false, "read_failed"};
            uint32_t cur_pid = 0;
            std::memcpy(&cur_pid, cur + 0x30, 4);
            std::string cur_norm = norm_text_utf8(extract_cstr(cur + 0x38, 0x2E));
            int pick = 0;
            if (pack.players.size() > 1) {
                for (size_t i = 0; i < pack.players.size(); ++i) {
                    if (pack.players[i].pid != 0 && pack.players[i].pid == cur_pid) { pick = static_cast<int>(i); break; }
                    if (!cur_norm.empty() && pack.players[i].name_norm == cur_norm) { pick = static_cast<int>(i); }
                }
            }
            push_undo(p, "import_json", cur);
            PackApplyOptions full_opts;
            full_opts.apply_stats_pos = true;
            full_opts.apply_skills_style = true;
            full_opts.apply_traits = true;
            full_opts.apply_team_role = true;
            full_opts.apply_bio = true;
            full_opts.apply_pack_economy = true;
            full_opts.auto_recal_economy = false;
            full_opts.apply_condition = true;
            uint8_t next[data::PLAYER_RECORD_SIZE]{};
            apply_pack_player_to_buf(next, cur, pack.players[pick], full_opts, econ_sync_mode_, nullptr);
            mem_.write_from(p, next, sizeof(next));
            econ_anchors_.erase(p);
            wage_cache_time_ = 0.0;
            return build_player_details_from_buf(p, career, next);
        }
        return {false, "invalid_json"};
    } catch (...) {
        return {false, "invalid_json"};
    }
}

std::string serialize_roster_pack_json(const RosterPack& pack) {
    nlohmann::json root;
    root["format"] = "pes21_wb_pack";
    root["version"] = pack.version.empty() ? "13.0" : pack.version;
    root["kind"] = pack.kind.empty() ? "squad" : pack.kind;
    root["label"] = pack.label;
    root["club"] = pack.club;
    root["created_at"] = pack.created_at.empty() ? cur_iso_datetime() : pack.created_at;
    root["cur_year"] = pack.cur_year;
    root["player_count"] = pack.players.size();

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& pp : pack.players) {
        nlohmann::json jp;
        jp["pid"] = pp.pid;
        jp["name"] = pp.name;
        jp["shirt"] = pp.shirt;
        jp["club"] = pp.club;
        jp["ovr"] = pp.ovr;
        jp["reg_pos"] = pp.reg_pos;
        jp["nationality"] = pp.nationality;
        jp["age"] = pp.age;
        jp["height"] = pp.height;
        jp["weight"] = pp.weight;
        jp["foot"] = pp.foot;
        jp["playing_style"] = pp.playing_style;
        jp["wf_usage"] = pp.wf_usage;
        jp["wf_acc"] = pp.wf_acc;
        jp["form_cons"] = pp.form_cons;
        jp["inj_res"] = pp.inj_res;
        jp["stamina"] = pp.stamina;
        jp["arrow"] = pp.arrow;
        jp["blink"] = pp.blink;
        jp["injury"] = pp.injury;
        jp["contract_yr"] = pp.contract_yr;
        jp["salary_raw"] = pp.salary_raw;
        jp["market_raw"] = pp.market_raw;
        jp["has_release_fee"] = pp.has_release_fee;
        jp["affection"] = pp.affection;
        jp["transfer_listed"] = pp.transfer_listed;
        jp["loan_listed"] = pp.loan_listed;
        jp["team_role"] = pp.team_role;
        jp["team_role_lvl"] = pp.team_role_lvl;

        nlohmann::json st_obj = nlohmann::json::object();
        for (const auto& s : data::STAT_DEFS) {
            st_obj[std::to_string(s.fid)] = pp.stats[s.fid];
        }
        jp["stats"] = std::move(st_obj);

        nlohmann::json apt_arr = nlohmann::json::array();
        for (int i = 0; i < 13; ++i) apt_arr.push_back(pp.pos_apt[i]);
        jp["pos_apt"] = std::move(apt_arr);

        nlohmann::json sk_arr = nlohmann::json::array();
        for (size_t i = 0; i < data::PLAYER_SKILLS.size(); ++i) {
            if (pp.skills[i]) sk_arr.push_back(static_cast<int>(i));
        }
        jp["skills"] = std::move(sk_arr);

        nlohmann::json ai_arr = nlohmann::json::array();
        for (size_t i = 0; i < data::COM_AI_STYLES.size(); ++i) {
            if (pp.com_ai[i]) ai_arr.push_back(static_cast<int>(i));
        }
        jp["com_ai"] = std::move(ai_arr);

        arr.push_back(std::move(jp));
    }
    root["players"] = std::move(arr);
    return root.dump(2);
}

bool parse_roster_pack_json(std::string_view json_str, RosterPack& out_pack) {
    out_pack = {};
    try {
        auto root = nlohmann::json::parse(json_str.begin(), json_str.end());
        if (!root.is_object()) return false;

        // Legacy v12 single-player hex format fallback
        if (root.contains("hex") && root["hex"].is_string() && !root.contains("players")) {
            std::vector<uint8_t> raw;
            if (!hex_to_bytes(root["hex"].get<std::string>(), raw) || raw.size() != data::PLAYER_RECORD_SIZE) {
                return false;
            }
            out_pack.version = root.value("version", std::string("12.0"));
            out_pack.kind = "player";
            out_pack.label = root.value("name", std::string("Legacy Player"));
            out_pack.players.push_back(extract_pack_player_from_buf(raw.data(), ""));
            return true;
        }

        if (!root.contains("players") || !root["players"].is_array()) return false;
        out_pack.version = root.value("version", std::string("13.0"));
        out_pack.kind = root.value("kind", std::string("squad"));
        out_pack.label = root.value("label", std::string(""));
        out_pack.club = root.value("club", std::string(""));
        out_pack.created_at = root.value("created_at", std::string(""));
        out_pack.cur_year = root.value("cur_year", 2026);

        for (const auto& jp : root["players"]) {
            if (!jp.is_object()) continue;
            PackPlayerData pp;
            pp.pid = jp.value("pid", 0u);
            pp.name = jp.value("name", std::string("?"));
            pp.name_norm = norm_text_utf8(pp.name);
            pp.shirt = jp.value("shirt", std::string(""));
            pp.club = jp.value("club", out_pack.club);
            pp.ovr = std::clamp(jp.value("ovr", 40), 40, 99);
            pp.reg_pos = std::clamp(jp.value("reg_pos", 0), 0, 12);
            pp.nationality = std::clamp(jp.value("nationality", 0), 0, 511);
            pp.age = std::clamp(jp.value("age", 20), 15, 50);
            pp.height = std::clamp(jp.value("height", 180), 145, 215);
            pp.weight = std::clamp(jp.value("weight", 75), 45, 125);
            pp.foot = jp.value("foot", 0) ? 1 : 0;
            pp.playing_style = std::clamp(jp.value("playing_style", 0), 0, 21);
            pp.wf_usage = std::clamp(jp.value("wf_usage", 2), 1, 4);
            pp.wf_acc = std::clamp(jp.value("wf_acc", 2), 1, 4);
            pp.form_cons = std::clamp(jp.value("form_cons", 5), 1, 8);
            pp.inj_res = std::clamp(jp.value("inj_res", 2), 1, 3);
            pp.stamina = std::clamp(jp.value("stamina", 100), 0, 100);
            pp.arrow = std::clamp(jp.value("arrow", 2), 0, 4);
            pp.blink = jp.value("blink", pp.arrow == 4);
            pp.injury = std::clamp(jp.value("injury", 0), 0, 255);
            pp.contract_yr = jp.value("contract_yr", 2026);
            pp.salary_raw = jp.value("salary_raw", 0);
            pp.market_raw = jp.value("market_raw", 0);
            pp.has_release_fee = jp.value("has_release_fee", false);
            pp.affection = std::clamp(jp.value("affection", 50), 0, 100);
            pp.transfer_listed = jp.value("transfer_listed", false);
            pp.loan_listed = jp.value("loan_listed", false);
            pp.team_role = std::clamp(jp.value("team_role", 0), 0, 22);
            pp.team_role_lvl = std::clamp(jp.value("team_role_lvl", 0), 0, 3);

            pp.stats.fill(40);
            if (jp.contains("stats") && jp["stats"].is_object()) {
                for (const auto& s : data::STAT_DEFS) {
                    std::string k = std::to_string(s.fid);
                    if (jp["stats"].contains(k) && jp["stats"][k].is_number_integer()) {
                        pp.stats[s.fid] = std::clamp(jp["stats"][k].get<int>(), 40, 99);
                    }
                }
            }
            pp.pos_apt.fill(0);
            if (jp.contains("pos_apt") && jp["pos_apt"].is_array()) {
                size_t n = std::min<size_t>(13, jp["pos_apt"].size());
                for (size_t i = 0; i < n; ++i) {
                    if (jp["pos_apt"][i].is_number_integer()) {
                        pp.pos_apt[i] = std::clamp(jp["pos_apt"][i].get<int>(), 0, 2);
                    }
                }
            }
            pp.skills.fill(false);
            if (jp.contains("skills") && jp["skills"].is_array()) {
                for (const auto& item : jp["skills"]) {
                    if (item.is_number_integer()) {
                        int idx = item.get<int>();
                        if (idx >= 0 && idx < static_cast<int>(pp.skills.size())) pp.skills[idx] = true;
                    }
                }
            }
            pp.com_ai.fill(false);
            if (jp.contains("com_ai") && jp["com_ai"].is_array()) {
                for (const auto& item : jp["com_ai"]) {
                    if (item.is_number_integer()) {
                        int idx = item.get<int>();
                        if (idx >= 0 && idx < static_cast<int>(pp.com_ai.size())) pp.com_ai[idx] = true;
                    }
                }
            }
            out_pack.players.push_back(std::move(pp));
        }
        return !out_pack.players.empty();
    } catch (...) {
        return false;
    }
}

std::string get_backups_directory() {
    std::filesystem::path dir = fs_path_from_utf8(get_packs_directory()) / L"Backups";
    auto u8 = dir.u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

std::string get_logs_directory() {
    // Ao lado de Packs: Documentos\Lettuce Live Editor\Logs
    std::filesystem::path dir = fs_path_from_utf8(get_packs_directory()).parent_path() / L"Logs";
    auto u8 = dir.u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

std::string get_packs_directory() {
    wchar_t doc_path[MAX_PATH]{};
    std::filesystem::path dir;
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_PERSONAL, nullptr, SHGFP_TYPE_CURRENT, doc_path)) && doc_path[0]) {
        // Documentos\Lettuce Live Editor\Packs (a pasta antiga "PES 2021 Workbench" e movida uma vez)
        dir = pes::dirs::migrate_dir(std::filesystem::path(doc_path), pes::dirs::kAppFolder, pes::dirs::kLegacyDocsFolder) / L"Packs";
    } else {
        dir = std::filesystem::current_path() / L"Packs";
    }
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    auto u8 = dir.u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

std::string make_default_pack_filename(std::string_view label, std::string_view club) {
    std::string base = !label.empty() ? std::string(label) : std::string(club);
    if (base.empty()) base = "Elenco";
    std::string clean;
    clean.reserve(base.size() + 24);
    for (unsigned char c : base) {
        if (std::isalnum(c)) clean.push_back(static_cast<char>(c));
        else if (c == ' ' || c == '-' || c == '_') {
            if (!clean.empty() && clean.back() != '_') clean.push_back('_');
        }
    }
    while (!clean.empty() && clean.back() == '_') clean.pop_back();
    if (clean.empty()) clean = "Pack";
    if (clean.size() > 36) clean.resize(36);
    std::time_t t = std::time(nullptr);
    std::tm tm_buf{};
    localtime_s(&tm_buf, &t);
    char ts[32];
    std::snprintf(ts, sizeof(ts), "_%04d%02d%02d_%02d%02d.json",
                  tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
                  tm_buf.tm_hour, tm_buf.tm_min);
    return clean + ts;
}

static std::filesystem::path fs_path_from_utf8(std::string_view s) {
    if (s.empty()) return {};
    int need = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (need <= 0) return {};
    std::wstring w(static_cast<size_t>(need), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), need);
    return std::filesystem::path(std::move(w));
}

bool save_text_file_utf8(const std::string& path, std::string_view content) {
    try {
        std::filesystem::path fp = fs_path_from_utf8(path);
        if (fp.has_parent_path()) {
            std::error_code ec;
            std::filesystem::create_directories(fp.parent_path(), ec);
        }
        std::ofstream ofs(fp, std::ios::binary | std::ios::trunc);
        if (!ofs) return false;
        ofs.write(content.data(), static_cast<std::streamsize>(content.size()));
        return ofs.good();
    } catch (...) {
        return false;
    }
}

bool load_text_file_utf8(const std::string& path, std::string& out_content) {
    out_content.clear();
    try {
        std::filesystem::path fp = fs_path_from_utf8(path);
        std::ifstream ifs(fp, std::ios::binary);
        if (!ifs) return false;
        std::ostringstream ss;
        ss << ifs.rdbuf();
        out_content = ss.str();
        return !out_content.empty();
    } catch (...) {
        return false;
    }
}

std::vector<PackLibraryItem> list_saved_packs() {
    std::vector<PackLibraryItem> items;
    try {
        std::filesystem::path dir = fs_path_from_utf8(get_packs_directory());
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec)) return items;
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != L".json") continue;
            auto p_u8 = entry.path().u8string();
            std::string p_str(reinterpret_cast<const char*>(p_u8.data()), p_u8.size());
            std::string txt;
            if (!load_text_file_utf8(p_str, txt)) continue;
            RosterPack pk;
            if (!parse_roster_pack_json(txt, pk)) continue;
            PackLibraryItem it;
            it.path = p_str;
            auto fn_u8 = entry.path().filename().u8string();
            it.filename = std::string(reinterpret_cast<const char*>(fn_u8.data()), fn_u8.size());
            it.label = pk.label.empty() ? it.filename : pk.label;
            it.club = pk.club;
            it.created_at = pk.created_at;
            it.kind = pk.kind;
            it.player_count = static_cast<int>(pk.players.size());
            double sum_o = 0.0;
            for (const auto& pl : pk.players) sum_o += pl.ovr;
            it.avg_ovr = pk.players.empty() ? 0.0 : (sum_o / static_cast<double>(pk.players.size()));
            items.push_back(std::move(it));
        }
        std::sort(items.begin(), items.end(), [](const PackLibraryItem& a, const PackLibraryItem& b) {
            if (a.created_at != b.created_at) return a.created_at > b.created_at;
            return a.filename > b.filename;
        });
    } catch (...) {
    }
    return items;
}

ActionResult PESEngine::export_squad_pack_json(std::string_view custom_label) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, p_cur = 0, ms = 0;
    sync_pointers(career, p_cur, ms);
    if (!career) return {false, "no_career"};

    int cur_year = static_cast<int>(mem_.read_u16(career + 0x01642A24ULL));
    if (cur_year < 2019 || cur_year > 2150) cur_year = 2026;

    std::vector<uint8_t> raw_table;
    const auto addrs = get_user_squad_addresses(career, true, &raw_table);
    if (addrs.empty()) return {false, "no_player"};

    uint32_t uc = get_user_club_handle(career);
    std::string club_name = decode_club_handle(uc, career).second;

    RosterPack pack;
    pack.version = "13.0";
    pack.kind = "squad";
    pack.club = club_name;
    pack.label = !custom_label.empty() ? std::string(custom_label)
                                       : (club_name.empty() ? std::string("Elenco") : club_name);
    pack.created_at = cur_iso_datetime();
    pack.cur_year = cur_year;
    pack.players.reserve(addrs.size());

    for (uint64_t p : addrs) {
        size_t off = static_cast<size_t>(p - career);
        if (off + data::PLAYER_RECORD_SIZE > raw_table.size()) continue;
        const uint8_t* buf = raw_table.data() + off;
        if (!is_sane_player_buf(buf)) continue;
        pack.players.push_back(extract_pack_player_from_buf(buf, club_name));
    }
    std::sort(pack.players.begin(), pack.players.end(), [](const PackPlayerData& a, const PackPlayerData& b) {
        if (a.ovr != b.ovr) return a.ovr > b.ovr;
        return a.name < b.name;
    });

    return {true, "ok", static_cast<int>(pack.players.size()), 0, false, serialize_roster_pack_json(pack)};
}

ActionResult PESEngine::export_player_pack_json(uint64_t addr) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, cur_p = 0, ms = 0;
    sync_pointers(career, cur_p, ms);
    uint64_t p = addr ? addr : cur_p;
    if (p <= 0x10000ULL) return {false, "no_player"};

    uint8_t buf[data::PLAYER_RECORD_SIZE]{};
    if (!mem_.read_into(p, buf, sizeof(buf)) || !is_sane_player_buf(buf)) return {false, "read_failed"};

    int cur_year = career ? static_cast<int>(mem_.read_u16(career + 0x01642A24ULL)) : 2026;
    if (cur_year < 2019 || cur_year > 2150) cur_year = 2026;
    uint32_t club_h = 0;
    std::memcpy(&club_h, buf + 0x12C, 4);
    std::string club_name = decode_club_handle(club_h, career).second;

    PackPlayerData pp = extract_pack_player_from_buf(buf, club_name);
    RosterPack pack;
    pack.version = "13.0";
    pack.kind = "player";
    pack.club = club_name;
    pack.label = pp.name;
    pack.created_at = cur_iso_datetime();
    pack.cur_year = cur_year;
    pack.players.push_back(std::move(pp));

    return {true, "ok", 1, 0, false, serialize_roster_pack_json(pack)};
}

PackPreviewResult PESEngine::preview_pack_import(
    std::string_view json_str,
    PackMatchScope scope,
    const PackApplyOptions& opts,
    int forced_pack_idx_for_current)
{
    std::lock_guard<std::recursive_mutex> g(lock_);
    PackPreviewResult res;
    res.scope = scope;
    if (!parse_roster_pack_json(json_str, res.pack)) {
        res.code = "invalid_json";
        return res;
    }
    if (!ensure_attached()) {
        res.code = "not_connected";
        return res;
    }
    uint64_t career = 0, cur_p = 0, ms = 0;
    sync_pointers(career, cur_p, ms);
    if (!career && scope != PackMatchScope::CurrentPlayer) {
        res.code = "no_career";
        return res;
    }

    struct TargetCand {
        uint64_t addr = 0;
        uint32_t pid = 0;
        std::string name;
        std::string name_norm;
        std::string club;
        int nat = 0;
        int pos = 0;
        std::array<uint8_t, data::PLAYER_RECORD_SIZE> buf{};
    };

    std::vector<TargetCand> cands;
    if (scope == PackMatchScope::CurrentPlayer) {
        if (cur_p <= 0x10000ULL) { res.code = "no_player"; return res; }
        TargetCand tc;
        tc.addr = cur_p;
        if (!mem_.read_into(cur_p, tc.buf.data(), tc.buf.size()) || !is_sane_player_buf(tc.buf.data())) {
            res.code = "read_failed";
            return res;
        }
        std::memcpy(&tc.pid, tc.buf.data() + 0x30, 4);
        tc.name = extract_cstr(tc.buf.data() + 0x38, 0x2E);
        tc.name_norm = norm_text_utf8(tc.name);
        uint32_t ch = 0; std::memcpy(&ch, tc.buf.data() + 0x12C, 4);
        tc.club = decode_club_handle(ch, career).second;
        uint16_t nr = 0; std::memcpy(&nr, tc.buf.data() + 0x144, 2);
        tc.nat = nr & 0x1FF;
        tc.pos = (tc.buf[0x07] >> 4) & 0x0F;
        cands.push_back(std::move(tc));
    } else if (scope == PackMatchScope::UserSquad) {
        std::vector<uint8_t> raw_table;
        const auto addrs = get_user_squad_addresses(career, true, &raw_table);
        uint32_t uc = get_user_club_handle(career);
        std::string uclub = decode_club_handle(uc, career).second;
        cands.reserve(addrs.size());
        for (uint64_t p : addrs) {
            size_t off = static_cast<size_t>(p - career);
            if (off + data::PLAYER_RECORD_SIZE > raw_table.size()) continue;
            const uint8_t* b = raw_table.data() + off;
            if (!is_sane_player_buf(b)) continue;
            TargetCand tc;
            tc.addr = p;
            std::memcpy(tc.buf.data(), b, data::PLAYER_RECORD_SIZE);
            std::memcpy(&tc.pid, b + 0x30, 4);
            tc.name = extract_cstr(b + 0x38, 0x2E);
            tc.name_norm = norm_text_utf8(tc.name);
            tc.club = uclub;
            uint16_t nr = 0; std::memcpy(&nr, b + 0x144, 2);
            tc.nat = nr & 0x1FF;
            tc.pos = (b[0x07] >> 4) & 0x0F;
            cands.push_back(std::move(tc));
        }
    } else {
        uint32_t count = mem_.read_u32(career + 0x00D0BCE8ULL);
        if (count == 0 || count > 30001u) count = 30000u;
        std::vector<uint8_t> raw(static_cast<size_t>(count) * data::PLAYER_RECORD_SIZE);
        if (!mem_.read_into(career, raw.data(), raw.size())) {
            res.code = "read_failed";
            return res;
        }
        // Build quick lookup sets of PIDs and normalized names present in the pack so we only copy matching records
        std::unordered_map<uint32_t, bool> pack_pids;
        std::unordered_map<std::string, bool> pack_names;
        for (const auto& pp : res.pack.players) {
            if (pp.pid != 0) pack_pids[pp.pid] = true;
            if (!pp.name_norm.empty()) pack_names[pp.name_norm] = true;
        }
        for (uint32_t i = 0; i < count; ++i) {
            size_t off = static_cast<size_t>(i) * data::PLAYER_RECORD_SIZE;
            const uint8_t* b = raw.data() + off;
            uint32_t pid = 0;
            std::memcpy(&pid, b + 0x30, 4);
            if (pid == 0 || pid == 0xFFFFFFFFu) continue;
            if (!is_sane_player_buf(b)) continue;
            bool hit = (pack_pids.find(pid) != pack_pids.end());
            std::string nm, nm_norm;
            if (!hit) {
                if (b[0x38] == 0) continue;
                nm = extract_cstr(b + 0x38, 0x2E);
                nm_norm = norm_text_utf8(nm);
                if (pack_names.find(nm_norm) == pack_names.end()) continue;
            } else {
                nm = extract_cstr(b + 0x38, 0x2E);
                nm_norm = norm_text_utf8(nm);
            }
            TargetCand tc;
            tc.addr = career + off;
            tc.pid = pid;
            tc.name = std::move(nm);
            tc.name_norm = std::move(nm_norm);
            std::memcpy(tc.buf.data(), b, data::PLAYER_RECORD_SIZE);
            uint32_t ch = 0; std::memcpy(&ch, b + 0x12C, 4);
            tc.club = decode_club_handle(ch, career).second;
            uint16_t nr = 0; std::memcpy(&nr, b + 0x144, 2);
            tc.nat = nr & 0x1FF;
            tc.pos = (b[0x07] >> 4) & 0x0F;
            cands.push_back(std::move(tc));
        }
    }

    // Index candidates by PID and normalized name
    std::unordered_map<uint32_t, int> by_pid;
    std::unordered_map<std::string, std::vector<int>> by_name;
    for (int i = 0; i < static_cast<int>(cands.size()); ++i) {
        if (cands[i].pid != 0 && by_pid.find(cands[i].pid) == by_pid.end()) by_pid[cands[i].pid] = i;
        if (!cands[i].name_norm.empty()) by_name[cands[i].name_norm].push_back(i);
    }

    auto count_skills_buf = [](const uint8_t* b) {
        int c = 0;
        for (const auto& sk : data::PLAYER_SKILLS) {
            if (get_bits_buf(b, data::PLAYER_RECORD_SIZE, sk.offset, sk.bit, 1) == 1) ++c;
        }
        return c;
    };

    std::vector<uint8_t> used_cand(cands.size(), 0);
    res.rows.reserve(res.pack.players.size());

    for (int pi = 0; pi < static_cast<int>(res.pack.players.size()); ++pi) {
        const auto& pp = res.pack.players[pi];
        PackPreviewRow row;
        row.pack_idx = pi;
        row.new_ovr = pp.ovr;
        row.new_pos = pp.reg_pos;

        int matched_ci = -1;
        PackMatchKind mk = PackMatchKind::None;

        if (scope == PackMatchScope::CurrentPlayer && forced_pack_idx_for_current >= 0) {
            if (pi == forced_pack_idx_for_current && !cands.empty()) {
                matched_ci = 0;
                mk = (cands[0].pid == pp.pid && pp.pid != 0) ? PackMatchKind::ExactPid : PackMatchKind::ManualCurrent;
            }
        } else {
            // 1. Exact Konami Player ID match
            if (pp.pid != 0) {
                auto it = by_pid.find(pp.pid);
                if (it != by_pid.end() && !used_cand[it->second]) {
                    matched_ci = it->second;
                    mk = PackMatchKind::ExactPid;
                }
            }
            // 2. Fallback: exact normalized name (disambiguated by nationality + position)
            if (matched_ci < 0 && !pp.name_norm.empty()) {
                auto it = by_name.find(pp.name_norm);
                if (it != by_name.end()) {
                    int best_ci = -1;
                    int best_score = -1;
                    for (int ci : it->second) {
                        if (used_cand[ci]) continue;
                        int sc = 0;
                        if (cands[ci].nat == pp.nationality) sc += 4;
                        if (cands[ci].pos == pp.reg_pos) sc += 2;
                        if (sc > best_score) { best_score = sc; best_ci = ci; }
                    }
                    if (best_ci >= 0) {
                        matched_ci = best_ci;
                        mk = PackMatchKind::ExactName;
                    }
                }
            }
            // In CurrentPlayer mode with single-player pack, allow applying directly to current player
            if (matched_ci < 0 && scope == PackMatchScope::CurrentPlayer && res.pack.players.size() == 1 && !cands.empty()) {
                matched_ci = 0;
                mk = PackMatchKind::ManualCurrent;
            }
        }

        if (matched_ci >= 0) {
            used_cand[matched_ci] = 1;
            const auto& tc = cands[matched_ci];
            row.selected = true;
            row.match_kind = mk;
            row.target_addr = tc.addr;
            row.target_pid = tc.pid;
            row.target_name = tc.name;
            row.target_club = tc.club;

            auto [co, cp] = calc_ovr_from_buf(tc.buf.data(), data::PLAYER_RECORD_SIZE);
            row.cur_ovr = co;
            row.cur_pos = cp;
            row.cur_skills_cnt = count_skills_buf(tc.buf.data());
            std::memcpy(&row.cur_market_raw, tc.buf.data() + 0x174, 4);
            std::memcpy(&row.cur_salary_raw, tc.buf.data() + 0x15C, 4);

            // Exact dry-run simulation on local copy (0 RAM writes!)
            uint8_t sim[data::PLAYER_RECORD_SIZE]{};
            apply_pack_player_to_buf(sim, tc.buf.data(), pp, opts, econ_sync_mode_, &row.stat_changes);
            auto [no, np] = calc_ovr_from_buf(sim, data::PLAYER_RECORD_SIZE);
            row.new_ovr = no;
            row.new_pos = np;
            row.new_skills_cnt = count_skills_buf(sim);
            std::memcpy(&row.new_market_raw, sim + 0x174, 4);
            std::memcpy(&row.new_salary_raw, sim + 0x15C, 4);

            if (mk == PackMatchKind::ExactPid) ++res.matched_pid;
            else ++res.matched_name;
        } else {
            row.selected = false;
            row.match_kind = PackMatchKind::None;
            row.target_name = pp.name;
            row.target_club = pp.club;
            row.cur_ovr = 0;
            row.new_ovr = pp.ovr;
            row.new_pos = pp.reg_pos;
            int sc = 0; for (bool b : pp.skills) if (b) ++sc;
            row.new_skills_cnt = sc;
            ++res.unmatched;
        }
        res.rows.push_back(std::move(row));
    }

    res.ok = true;
    res.code = "ok";
    return res;
}

ActionResult PESEngine::apply_pack_import(const PackPreviewResult& preview, const PackApplyOptions& opts) {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!preview.ok) return {false, "invalid_arg"};
    if (!ensure_attached()) return {false, "not_connected"};
    uint64_t career = 0, cur_p = 0, ms = 0;
    sync_pointers(career, cur_p, ms);

    std::vector<BatchUndoItem> batch_backup;
    int applied = 0;
    journal_begin_batch("pack_import");
    struct JGuard2 { PESEngine* e; ~JGuard2() { e->journal_end_batch(); } } jguard2{this};

    for (const auto& row : preview.rows) {
        if (!row.selected || row.match_kind == PackMatchKind::None || row.target_addr <= 0x10000ULL) continue;
        if (row.pack_idx < 0 || row.pack_idx >= static_cast<int>(preview.pack.players.size())) continue;

        uint8_t cur[data::PLAYER_RECORD_SIZE]{};
        if (!mem_.read_into(row.target_addr, cur, sizeof(cur)) || !is_sane_player_buf(cur)) continue;

        // Verify target PID did not shift since preview (save-change protection!)
        uint32_t live_pid = 0;
        std::memcpy(&live_pid, cur + 0x30, 4);
        if (row.target_pid != 0 && live_pid != row.target_pid) continue;

        BatchUndoItem bi;
        bi.addr = row.target_addr;
        std::memcpy(bi.bytes.data(), cur, data::PLAYER_RECORD_SIZE);
        auto ait = econ_anchors_.find(row.target_addr);
        if (ait != econ_anchors_.end()) bi.prev_anchor = ait->second;
        batch_backup.push_back(std::move(bi));

        push_undo(row.target_addr, "pack_import", cur);

        uint8_t next[data::PLAYER_RECORD_SIZE]{};
        apply_pack_player_to_buf(next, cur, preview.pack.players[row.pack_idx], opts, econ_sync_mode_, nullptr);
        mem_.write_from(row.target_addr, next, sizeof(next));
        econ_anchors_.erase(row.target_addr);
        ++applied;
    }

    if (applied > 0) {
        last_batch_undo_ = std::move(batch_backup);
        last_batch_label_ = preview.pack.label.empty() ? "pack_import" : preview.pack.label;
        wage_cache_time_ = 0.0;
    }
    return {applied > 0, applied > 0 ? "ok" : "nothing_applied", applied};
}

ActionResult PESEngine::undo_last_batch() {
    std::lock_guard<std::recursive_mutex> g(lock_);
    if (!ensure_attached()) return {false, "not_connected"};
    if (last_batch_undo_.empty()) return {false, "nothing_to_undo"};

    int restored = 0;
    for (const auto& bi : last_batch_undo_) {
        if (bi.addr <= 0x10000ULL) continue;
        uint8_t probe[data::PLAYER_RECORD_SIZE]{};
        if (!mem_.read_into(bi.addr, probe, sizeof(probe)) || !is_sane_player_buf(probe)) continue;
        mem_.write_from(bi.addr, bi.bytes.data(), bi.bytes.size());
        if (bi.prev_anchor.has_value()) econ_anchors_[bi.addr] = *bi.prev_anchor;
        else econ_anchors_.erase(bi.addr);
        journal_drop_addr(bi.addr);
        {
            auto us = undo_stack_.find(bi.addr);
            if (us != undo_stack_.end() && !us->second.empty()) us->second.pop_back();
        }
        ++restored;
    }
    std::string lbl = last_batch_label_;
    last_batch_undo_.clear();
    last_batch_label_.clear();
    wage_cache_time_ = 0.0;
    return {restored > 0, restored > 0 ? "ok" : "nothing_to_undo", restored, 0, false, lbl};
}

int PESEngine::last_batch_undo_count() const {
    std::lock_guard<std::recursive_mutex> g(lock_);
    return static_cast<int>(last_batch_undo_.size());
}

int PESEngine::scout_undo_depth() const {
    std::lock_guard<std::recursive_mutex> g(lock_);
    return static_cast<int>(scout_undo_.size());
}

std::string PESEngine::scout_undo_label() const {
    std::lock_guard<std::recursive_mutex> g(lock_);
    return scout_undo_.empty() ? std::string() : scout_undo_.back().label;
}

} // namespace pes
