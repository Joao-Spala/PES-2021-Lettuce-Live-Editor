"""
Generates native/src/data/game_data.hpp directly from the verified PES 2021
Python tables (app/engine/pes_data.py + src/data/*.json) so C++20 native build
has 100% verbatim official PT-BR / EN game strings, OVR weights, team names,
and bitfield offsets embedded directly in the binary with zero external files.
"""

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "app"))

import engine.pes_data as pd


def c_str(s: str) -> str:
    # Escape as UTF-8 C++ string literal (using u8-compatible raw UTF-8 bytes)
    esc = (
        str(s)
        .replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\n", "\\n")
        .replace("\r", "")
    )
    return f'"{esc}"'


def main() -> None:
    out_path = ROOT / "native" / "src" / "data" / "game_data.hpp"
    out_path.parent.mkdir(parents=True, exist_ok=True)

    lines = []
    w = lines.append

    w("#pragma once")
    w("// Auto-generated from app/engine/pes_data.py & src/data/*.json")
    w("// Contains 100% verified PES 2021 official PT-BR (dt20_bra_x64.cpk) & EN tables.")
    w("#include <cstdint>")
    w("#include <cstddef>")
    w("#include <array>")
    w("")
    w("namespace pes::data {")
    w("")
    w("inline constexpr uint32_t PLAYER_RECORD_SIZE = 0x17C;")
    w("inline constexpr uint64_t SCOUT_EMPTY_SENTINEL = 0x0000FFFFULL;")
    w("")

    # 1. Positions (13)
    w("struct PositionInfo {")
    w("    int id;")
    w("    const char* pt;")
    w("    const char* en;")
    w("    const char* name_pt;")
    w("    const char* name_en;")
    w("};")
    w(f"inline constexpr std::array<PositionInfo, {len(pd.POSITIONS)}> POSITIONS = {{{{")
    for p in pd.POSITIONS:
        w(f"    {{{p['id']}, {c_str(p['pt'])}, {c_str(p['en'])}, {c_str(p['name_pt'])}, {c_str(p['name_en'])}}},")
    w("}};")
    w("")

    # Tactical display order
    w("inline constexpr std::array<int, 13> TACTICAL_ORDER = {12, 11, 9, 10, 8, 6, 5, 7, 4, 2, 1, 3, 0};")
    w("")

    # 2. Playing Styles (22)
    w("struct PlayingStyleInfo {")
    w("    int id;")
    w("    const char* pt;")
    w("    const char* en;")
    w("    const char* pos_pt;")
    w("    const char* pos_en;")
    w("    const char* desc_pt;")
    w("    const char* desc_en;")
    w("};")
    w(f"inline constexpr std::array<PlayingStyleInfo, {len(pd.PLAYING_STYLES)}> PLAYING_STYLES = {{{{")
    for s in pd.PLAYING_STYLES:
        w(
            f"    {{{s['id']}, {c_str(s['pt'])}, {c_str(s['en'])}, "
            f"{c_str(s['pos_pt'])}, {c_str(s['pos_en'])}, "
            f"{c_str(s['desc_pt'])}, {c_str(s['desc_en'])}}},"
        )
    w("}};")
    w("")

    # 3. Team Roles (23) with Captain flag & bitmask tags
    w("inline constexpr uint8_t ROLE_TAG_FIN    = 1 << 0;")
    w("inline constexpr uint8_t ROLE_TAG_COND   = 1 << 1;")
    w("inline constexpr uint8_t ROLE_TAG_XP     = 1 << 2;")
    w("inline constexpr uint8_t ROLE_TAG_SPIRIT = 1 << 3;")
    w("inline constexpr uint8_t ROLE_TAG_CAP    = 1 << 4;")
    w("")
    w("struct TeamRoleInfo {")
    w("    int id;")
    w("    const char* pt;")
    w("    const char* en;")
    w("    bool captain;")
    w("    uint8_t tag_mask;")
    w("    const char* desc_pt;")
    w("    const char* desc_en;")
    w("    const char* captain_pt;")
    w("    const char* captain_en;")
    w("};")
    tag_map = {"fin": 1, "cond": 2, "xp": 4, "spirit": 8, "cap": 16}
    w(f"inline constexpr std::array<TeamRoleInfo, {len(pd.TEAM_ROLES)}> TEAM_ROLES = {{{{")
    for r in pd.TEAM_ROLES:
        mask = 0
        for tg in r.get("tags", []):
            mask |= tag_map.get(tg, 0)
        cap_b = "true" if r.get("captain") else "false"
        w(
            f"    {{{r['id']}, {c_str(r['pt'])}, {c_str(r['en'])}, {cap_b}, {mask}u, "
            f"{c_str(r['desc_pt'])}, {c_str(r['desc_en'])}, "
            f"{c_str(r.get('captain_pt', ''))}, {c_str(r.get('captain_en', ''))}}},"
        )
    w("}};")
    w("")

    # 4. Stat Defs (25) + FID_LOC
    w("enum class StatGroup : uint8_t { Attack = 0, Tech = 1, Phys = 2, Def = 3, GK = 4 };")
    w("struct StatDef {")
    w("    int fid;")
    w("    StatGroup group;")
    w("    uint16_t offset;")
    w("    uint8_t bit_start;")
    w("    const char* pt;")
    w("    const char* en;")
    w("    const char* desc_pt;")
    w("    const char* desc_en;")
    w("};")
    grp_map = {
        "attack": "StatGroup::Attack",
        "tech": "StatGroup::Tech",
        "phys": "StatGroup::Phys",
        "def": "StatGroup::Def",
        "gk": "StatGroup::GK",
    }
    w(f"inline constexpr std::array<StatDef, {len(pd.STAT_DEFS)}> STAT_DEFS = {{{{")
    for s in pd.STAT_DEFS:
        off, bs = pd.FID_LOC[s["fid"]]
        w(
            f"    {{{s['fid']}, {grp_map[s['group']]}, 0x{off:02X}u, {bs}u, "
            f"{c_str(s['pt'])}, {c_str(s['en'])}, {c_str(s['desc_pt'])}, {c_str(s['desc_en'])}}},"
        )
    w("}};")
    w("")

    w("struct BitLoc { uint16_t off; uint8_t bit; };")
    w("inline constexpr std::array<BitLoc, 13> POS_APT_LOC = {{")
    for pi in range(13):
        off, bs = pd.POS_APT_LOC[pi]
        w(f"    {{0x{off:02X}u, {bs}u}},")
    w("}};")
    w("")
    w(f"inline constexpr std::array<int, {len(pd.OUTFIELD_FIDS)}> OUTFIELD_FIDS = {{{', '.join(map(str, pd.OUTFIELD_FIDS))}}};")
    w(f"inline constexpr std::array<int, {len(pd.GK_FIDS)}> GK_FIDS = {{{', '.join(map(str, pd.GK_FIDS))}}};")
    pos_remap_list = [pd.POS_REMAP[i] for i in range(13)]
    w(f"inline constexpr std::array<int, 13> POS_REMAP = {{{', '.join(map(str, pos_remap_list))}}};")
    w("")

    # 5. Skills (41) + COM AI (7)
    w("enum class SkillCat : uint8_t { Dribble = 0, Shoot = 1, Pass = 2, Def = 3, GkMental = 4, ComAI = 5 };")
    w("struct SkillDef {")
    w("    const char* key;")
    w("    SkillCat cat;")
    w("    uint16_t offset;")
    w("    uint8_t bit;")
    w("    const char* pt;")
    w("    const char* en;")
    w("    const char* desc_pt;")
    w("    const char* desc_en;")
    w("};")
    scat_map = {
        "dribble": "SkillCat::Dribble",
        "shoot": "SkillCat::Shoot",
        "pass": "SkillCat::Pass",
        "def": "SkillCat::Def",
        "gk_mental": "SkillCat::GkMental",
        "com_ai": "SkillCat::ComAI",
    }
    w(f"inline constexpr std::array<SkillDef, {len(pd.PLAYER_SKILLS)}> PLAYER_SKILLS = {{{{")
    for sk in pd.PLAYER_SKILLS:
        w(
            f"    {{{c_str(sk['key'])}, {scat_map[sk['cat']]}, 0x{sk['offset']:02X}u, {sk['bit']}u, "
            f"{c_str(sk['pt'])}, {c_str(sk['en'])}, {c_str(sk['desc_pt'])}, {c_str(sk['desc_en'])}}},"
        )
    w("}};")
    w("")
    w(f"inline constexpr std::array<SkillDef, {len(pd.COM_AI_STYLES)}> COM_AI_STYLES = {{{{")
    for sk in pd.COM_AI_STYLES:
        w(
            f"    {{{c_str(sk['key'])}, SkillCat::ComAI, 0x{sk['offset']:02X}u, {sk['bit']}u, "
            f"{c_str(sk['pt'])}, {c_str(sk['en'])}, {c_str(sk['desc_pt'])}, {c_str(sk['desc_en'])}}},"
        )
    w("}};")
    w("")

    # 6. Scout Lists (4)
    w("struct ScoutListDef {")
    w("    int slot;")
    w("    uint32_t base;")
    w("    int cap;")
    w("    const char* pt;")
    w("    const char* en;")
    w("};")
    w(f"inline constexpr std::array<ScoutListDef, {len(pd.SCOUT_LISTS)}> SCOUT_LISTS = {{{{")
    for sl in pd.SCOUT_LISTS:
        w(f"    {{{sl['slot']}, 0x{sl['base']:08X}u, {sl['cap']}, {c_str(sl['pt'])}, {c_str(sl['en'])}}},")
    w("}};")
    w("")

    # 7. Exact Konami OVR_DATA tables
    od = pd.OVR_DATA
    w("// Exact Konami OVR tables (fcn.1414e7a50 + fcn.141e640a0)")
    w(f"inline constexpr std::array<int, 13> OVR_POS_ORDER = {{{', '.join(map(str, od['pos_order']))}}};")
    w(f"inline constexpr std::array<int, 23> OVR_STAT_INDICES_23 = {{{', '.join(map(str, od['stat_indices_23']))}}};")
    w(f"inline constexpr std::array<int, 22> OVR_AVG_INDICES_22 = {{{', '.join(map(str, od['avg_indices_22']))}}};")
    w(f"inline constexpr std::array<int, 13> OVR_BASE_REG = {{{', '.join(map(str, od['base_reg']))}}};")
    w(f"inline constexpr std::array<int, 13> OVR_BASE_APT_A = {{{', '.join(map(str, od['base_apt_a']))}}};")
    w(f"inline constexpr std::array<int, 13> OVR_BASE_APT_B = {{{', '.join(map(str, od['base_apt_b']))}}};")
    w(f"inline constexpr std::array<int, {len(od['curve_100'])}> OVR_CURVE_100 = {{{', '.join(map(str, od['curve_100']))}}};")
    w("struct FieldSlotPair { int fid; int slot; };")
    w(f"inline constexpr std::array<FieldSlotPair, {len(od['field_to_slot'])}> OVR_FIELD_TO_SLOT = {{{{")
    for pair in od["field_to_slot"]:
        w(f"    {{{int(pair[0])}, {int(pair[1])}}},")
    w("}};")
    w("inline constexpr int OVR_WEIGHTS_23X13[23][13] = {")
    for row in od["weights_23x13"]:
        w(f"    {{{', '.join(map(str, row))}}},")
    w("};")
    w("")

    # 8. Nationalities (sorted by id for binary search / display)
    w("struct IdNamePair { int id; const char* name; };")
    w(f"inline constexpr std::array<IdNamePair, {len(pd.NATIONALITIES)}> NATIONALITIES = {{{{")
    for n in pd.NATIONALITIES:
        w(f"    {{{int(n['id'])}, {c_str(n['name'])}}},")
    w("}};")
    w("")

    # 9. BMPES Teams (sorted by id for O(log N) lookup)
    teams_sorted = sorted((int(k), str(v)) for k, v in pd.BMPES_TEAMS.items())
    w(f"inline constexpr std::array<IdNamePair, {len(teams_sorted)}> BMPES_TEAMS = {{{{")
    for tid, name in teams_sorted:
        w(f"    {{{tid}, {c_str(name)}}},")
    w("}};")
    w("")
    w("inline const char* lookup_bmpes_team(int team_id) {")
    w("    size_t lo = 0, hi = BMPES_TEAMS.size();")
    w("    while (lo < hi) {")
    w("        size_t mid = lo + (hi - lo) / 2;")
    w("        if (BMPES_TEAMS[mid].id < team_id) lo = mid + 1;")
    w("        else hi = mid;")
    w("    }")
    w("    if (lo < BMPES_TEAMS.size() && BMPES_TEAMS[lo].id == team_id) return BMPES_TEAMS[lo].name;")
    w("    return nullptr;")
    w("}")
    w("")
    w("} // namespace pes::data")
    w("")

    out_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"Generated {out_path} ({len(lines)} lines, {out_path.stat().st_size / 1024:.1f} KB)")


if __name__ == "__main__":
    main()
