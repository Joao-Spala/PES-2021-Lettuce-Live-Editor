"""Static linter for structural validation, Auto Assembler symmetry, bitfields, and zero personal paths across both PT-BR and EN Cheat Table editions."""

import pathlib
import re
import xml.etree.ElementTree as ET

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
CT_EDITIONS = [
    REPO_ROOT / "dist" / "PES 2021 - v11.1 Ultimate Studio (PT-BR).CT",
    REPO_ROOT / "dist" / "PES 2021 - v11.1 Ultimate Studio (EN).CT",
]


def lint_single_ct(ct_path: pathlib.Path) -> int:
    assert ct_path.exists(), f"Cheat Table not found: {ct_path}"
    raw_text = ct_path.read_text(encoding="utf-8")
    assert re.search(r"c:\\users\\", raw_text, re.IGNORECASE) is None, f"Personal path leaked in {ct_path.name}!"
    assert "__KONAMI_OVR_LUA__" not in raw_text, f"Unexpanded __KONAMI_OVR_LUA__ token in {ct_path.name}!"
    assert "__DEFAULT_LANG__" not in raw_text, f"Unexpanded __DEFAULT_LANG__ token in {ct_path.name}!"

    tree = ET.parse(ct_path)
    root = tree.getroot()

    seen_ids = set()
    issues = []
    bit_map: dict[str, dict[int, str]] = {}

    for ce in root.iter("CheatEntry"):
        cid = ce.findtext("ID")
        desc = ce.findtext("Description") or ""
        if cid in seen_ids:
            issues.append(f"Duplicate ID: {cid} ({desc})")
        seen_ids.add(cid)

        last_state = ce.find("LastState")
        if last_state is not None and "RealAddress" in last_state.attrib:
            issues.append(f"Dirty LastState RealAddress in [{cid}] {desc}")

        vt = ce.findtext("VariableType")
        if vt == "Auto Assembler Script":
            code = ce.findtext("AssemblerScript") or ""
            if "[ENABLE]" not in code or "[DISABLE]" not in code:
                issues.append(f"Script missing [ENABLE]/[DISABLE] in [{cid}] {desc}")
            en, dis = code.split("[DISABLE]", 1)
            if "{$lua}" in code:
                if "syntaxcheck" not in en:
                    issues.append(f"Lua script missing syntaxcheck guard in [{cid}] {desc}")
            else:
                allocs = re.findall(r"\balloc\(\s*([A-Za-z0-9_]+)\s*,\s*[^,)]+(?:,\s*([^)]+))?\)", en)
                deallocs = set(re.findall(r"\bdealloc\(\s*([A-Za-z0-9_]+)\s*\)", dis))
                for sym, near in allocs:
                    if not near:
                        issues.append(f"alloc({sym}) missing 64-bit ±2GB near-address hint in [{cid}] {desc}")
                    if sym not in deallocs:
                        issues.append(f"alloc({sym}) missing symmetric dealloc in [{cid}] {desc}")
                reg = set(re.findall(r"\bregistersymbol\(\s*([A-Za-z0-9_]+)\s*\)", en))
                unreg = set(re.findall(r"\bunregistersymbol\(\s*([A-Za-z0-9_]+)\s*\)", dis))
                if reg != unreg:
                    issues.append(f"Asymmetric registersymbol/unregistersymbol in [{cid}] {desc}: {reg ^ unreg}")

        elif vt == "Binary":
            base = ce.findtext("Address") or ""
            offs = [o.text or "0" for o in ce.findall("./Offsets/Offset")]
            if len(offs) == 1:
                byte_off = int(offs[0], 16)
                bs = int(ce.findtext("BitStart") or "0")
                bl = int(ce.findtext("BitLength") or "1")
                bmap = bit_map.setdefault(base, {})
                for b in range(byte_off * 8 + bs, byte_off * 8 + bs + bl):
                    if b in bmap:
                        issues.append(f"Bit overlap on {base} bit {b}: '{bmap[b]}' vs '{desc}'")
                    else:
                        bmap[b] = desc

    assert not issues, f"Linter failures in {ct_path.name}:\n" + "\n".join(issues)
    print(f"LINT OK [{ct_path.name}]: {len(seen_ids)} records audited, 0 bit overlaps, 0 personal paths!")
    return len(seen_ids)


def run_lint() -> None:
    counts = [lint_single_ct(p) for p in CT_EDITIONS]
    assert counts[0] == counts[1], f"PT-BR ({counts[0]}) and EN ({counts[1]}) record count mismatch!"


if __name__ == "__main__":
    run_lint()
