"""
Gera native/src/ui/i18n_es_table.inc a partir de native/i18n_es.json.

O JSON mapeia o texto em INGLES (a chave usada em T(L, pt, en)) para o espanhol. Uso:
    python native/gen_es_table.py

A tabela sai ordenada por bytes UTF-8 da chave, porque o app procura por busca binaria (strcmp).
Cada entrada e conferida: os especificadores de printf (%d, %s, %.1f), o sufixo de ID do ImGui (##id),
as quebras de linha e os espacos nas pontas precisam bater com a frase em ingles.
"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SRC = ROOT / "i18n_es.json"
OUT = ROOT / "src" / "ui" / "i18n_es_table.inc"

FMT = re.compile(r"%[-+#0]*\d*(?:\.\d+)?[a-zA-Z%]")


def cpp_str(s: str) -> str:
    out = []
    for ch in s:
        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\r":
            out.append("\\r")
        elif ch == "\t":
            out.append("\\t")
        else:
            out.append(ch)
    return '"' + "".join(out) + '"'


def check(en: str, es: str) -> list:
    problems = []
    if sorted(FMT.findall(en)) != sorted(FMT.findall(es)):
        problems.append("especificadores de formato diferentes")
    if (en.split("##", 1)[1] if "##" in en else None) != (es.split("##", 1)[1] if "##" in es else None):
        problems.append("sufixo ## diferente")
    if en.count("\n") != es.count("\n"):
        problems.append("quebras de linha diferentes")
    if (en[:1] == " ") != (es[:1] == " ") or (en[-1:] == " ") != (es[-1:] == " "):
        problems.append("espaco na ponta diferente")
    if not es.strip():
        problems.append("traducao vazia")
    return problems


def main() -> int:
    data = json.loads(SRC.read_text(encoding="utf-8"))
    bad = 0
    for en, es in data.items():
        probs = check(en, es)
        if probs:
            bad += 1
            print(f"ERRO: {en!r} -> {es!r}: {', '.join(probs)}", file=sys.stderr)
    if bad:
        return 1
    items = sorted(data.items(), key=lambda kv: kv[0].encode("utf-8"))
    lines = ["// Gerado por native/gen_es_table.py a partir de native/i18n_es.json. Nao edite a mao."]
    for en, es in items:
        lines.append(f"{{{cpp_str(en)}, {cpp_str(es)}}},")
    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"{OUT.name}: {len(items)} entradas")
    return 0


if __name__ == "__main__":
    sys.exit(main())
