"""Linter estático para validação estrutural, simetria Auto Assembler e bitfields da Cheat Table."""

import pathlib
import re
import xml.etree.ElementTree as ET

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
CT_PATH = REPO_ROOT / "dist" / "PES 2021 - v10.0 Pro Overhaul.CT"


def run_lint() -> None:
    assert CT_PATH.exists(), f"Cheat Table não encontrada em {CT_PATH}"
    raw_text = CT_PATH.read_text(encoding="utf-8")
    assert re.search(r"c:\\users\\", raw_text, re.IGNORECASE) is None, "Caminho pessoal detectado no .CT compilado!"

    tree = ET.parse(CT_PATH)
    root = tree.getroot()

    seen_ids = set()
    issues = []
    bit_map: dict[str, dict[int, str]] = {}

    for ce in root.iter("CheatEntry"):
        cid = ce.findtext("ID")
        desc = ce.findtext("Description") or ""
        if cid in seen_ids:
            issues.append(f"ID duplicado: {cid} ({desc})")
        seen_ids.add(cid)

        last_state = ce.find("LastState")
        if last_state is not None and "RealAddress" in last_state.attrib:
            issues.append(f"LastState RealAddress sujo em [{cid}] {desc}")

        vt = ce.findtext("VariableType")
        if vt == "Auto Assembler Script":
            code = ce.findtext("AssemblerScript") or ""
            if "[ENABLE]" not in code or "[DISABLE]" not in code:
                issues.append(f"Script sem [ENABLE]/[DISABLE] em [{cid}] {desc}")
            en, dis = code.split("[DISABLE]", 1)
            if "{$lua}" in code:
                if "syntaxcheck" not in en:
                    issues.append(f"Script Lua sem guarda syntaxcheck em [{cid}] {desc}")
            else:
                allocs = re.findall(r"\balloc\(\s*([A-Za-z0-9_]+)\s*,\s*[^,)]+(?:,\s*([^)]+))?\)", en)
                deallocs = set(re.findall(r"\bdealloc\(\s*([A-Za-z0-9_]+)\s*\)", dis))
                for sym, near in allocs:
                    if not near:
                        issues.append(f"alloc({sym}) sem hint 64-bit ±2GB em [{cid}] {desc}")
                    if sym not in deallocs:
                        issues.append(f"alloc({sym}) sem dealloc simétrico em [{cid}] {desc}")
                reg = set(re.findall(r"\bregistersymbol\(\s*([A-Za-z0-9_]+)\s*\)", en))
                unreg = set(re.findall(r"\bunregistersymbol\(\s*([A-Za-z0-9_]+)\s*\)", dis))
                if reg != unreg:
                    issues.append(f"registersymbol/unregistersymbol assimétrico em [{cid}] {desc}: {reg ^ unreg}")

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
                        issues.append(f"Colisão de bit em {base} bit {b}: '{bmap[b]}' vs '{desc}'")
                    else:
                        bmap[b] = desc

    assert not issues, "Falhas no linter:\n" + "\n".join(issues)
    print(f"LINT OK: {len(seen_ids)} registros auditados, 0 colisões de bits, 0 caminhos pessoais!")


if __name__ == "__main__":
    run_lint()
