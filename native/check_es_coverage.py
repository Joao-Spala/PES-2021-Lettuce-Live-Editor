"""
Confere a cobertura do espanhol: toda frase em ingles usada em T(L, "pt", "en") no codigo precisa ter
entrada em native/i18n_es.json (senao a interface mostra ingles). Sai com codigo 1 se faltar alguma.

Uso: python native/check_es_coverage.py
Frases que so existem em tabelas de dados (habilidades, estilos, funcoes) ficam fora de proposito.
"""
import glob
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(ROOT, "src")

ESCAPES = {"n": "\n", "t": "\t", '"': '"', "\\": "\\", "'": "'", "r": "\r"}


def parse_literals(s, i):
    """Le um ou mais literais de string C++ adjacentes a partir de s[i]. Retorna (valor, novo_i) ou None."""
    out = []
    n = len(s)
    got = False
    while True:
        j = i
        while j < n and s[j] in " \t\r\n":
            j += 1
        if j < n and s.startswith("//", j):
            while j < n and s[j] != "\n":
                j += 1
            i = j
            continue
        if j < n and s[j] == '"':
            j += 1
            buf = []
            while j < n and s[j] != '"':
                if s[j] == "\\":
                    buf.append(ESCAPES.get(s[j + 1], "\\" + s[j + 1]))
                    j += 2
                else:
                    buf.append(s[j])
                    j += 1
            out.append("".join(buf))
            got = True
            i = j + 1
            continue
        break
    return ("".join(out), i) if got else None


def collect_keys():
    keys = set()
    call = re.compile(r"\bT\(\s*[A-Za-z_][A-Za-z0-9_\.\->]*\s*,\s*")
    files = [f for f in glob.glob(os.path.join(SRC, "**", "*.*pp"), recursive=True) if "game_data" not in f]
    for f in files:
        s = open(f, encoding="utf-8", errors="replace").read()
        for m in call.finditer(s):
            r1 = parse_literals(s, m.end())
            if not r1:
                continue
            j = r1[1]
            while j < len(s) and s[j] in " \t\r\n":
                j += 1
            if j >= len(s) or s[j] != ",":
                continue
            r2 = parse_literals(s, j + 1)
            if r2:
                keys.add(r2[0])
    return keys


def main():
    keys = collect_keys()
    es = json.load(open(os.path.join(ROOT, "i18n_es.json"), encoding="utf-8"))
    missing = sorted(k for k in keys if k not in es)
    for k in missing:
        print("sem traducao:", repr(k))
    print(f"{len(keys)} frases no codigo, {len(missing)} sem traducao em espanhol")
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
