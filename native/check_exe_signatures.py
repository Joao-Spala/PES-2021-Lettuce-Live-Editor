"""
Confere, SEM executar o jogo, se um executavel baseado na engine do PES 2021 e compativel com o app.

Le as assinaturas de native/src/engine/signatures.hpp (a mesma fonte do app), abre o .exe como arquivo,
procura cada uma nas secoes EXECUTAVEIS (IMAGE_SCN_MEM_EXECUTE, como o app) e conta as ocorrencias. Tambem conta as referencias aos campos da
estrutura da carreira que o app usa (contador de atletas, orcamento, teto salarial, salarios, ano e clube).

Uso:
    python native/check_exe_signatures.py caminho\\do\\jogo.exe [outro.exe ...]

Resultado esperado para um executavel compativel: toda assinatura com "matches 1" (unica) e as referencias de
campo com o mesmo numero de ocorrencias do executavel de referencia. O arquivo precisa estar descompactado
(sem protecao que cifre o codigo em disco): num executavel cifrado nada casa mesmo que o jogo em memoria case.
"""
import mmap
import re
import struct
import sys
from pathlib import Path

HEADER = Path(__file__).resolve().parent / "src" / "engine" / "signatures.hpp"

FIELDS = [
    ("player count    +0xD0BCE8 ", rb"\xE8\xBC\xD0\x00"),
    ("transfer budget +0x16ECBF4", rb"\xF4\xCB\x6E\x01"),
    ("salary cap      +0x16ECC08", rb"\x08\xCC\x6E\x01"),
    ("wage overrides  +0x16F0764", rb"\x64\x07\x6F\x01"),
    ("year            +0x1642A24", rb"\x24\x2A\x64\x01"),
    ("user club       +0x1642A2C", rb"\x2C\x2A\x64\x01"),
    ("record size 0x17C (imul)  ", rb"\x69[\x00-\xFF]\x7C\x01\x00\x00"),
]
REFERENCE_COUNTS = [187, 62, 10, 1, 44, 1, 226]   # PES 2021 Steam 1.7.2.0


def load_signatures():
    text = HEADER.read_text(encoding="utf-8")
    sigs = re.findall(r'SigDef\s+\w+\{"([^"]+)",\s*"([^"]+)",\s*(0x[0-9A-Fa-f]+)\}', text)
    if not sigs:
        sys.exit("nenhuma assinatura encontrada em " + str(HEADER))
    return [(i, p, int(h, 16)) for i, p, h in sigs]


def to_regex(pattern):
    parts = []
    for tok in pattern.split():
        parts.append(b"." if tok in ("?", "??") else re.escape(bytes([int(tok, 16)])))
    return re.compile(b"".join(parts), re.S)


def read_sections(mm):
    e = struct.unpack_from("<I", mm, 0x3C)[0]
    if mm[e:e + 4] != b"PE\0\0":
        sys.exit("nao e um PE valido")
    nsec = struct.unpack_from("<H", mm, e + 6)[0]
    ts = struct.unpack_from("<I", mm, e + 8)[0]
    optsz = struct.unpack_from("<H", mm, e + 20)[0]
    soi = struct.unpack_from("<I", mm, e + 24 + 56)[0]
    secs = []
    for i in range(nsec):
        name, vsize, va, rawsz, rawptr, _, _, _, _, ch = struct.unpack_from("<8sIIIIIIHHI", mm, e + 24 + optsz + i * 40)
        secs.append((name.rstrip(b"\0").decode("latin1"), va, vsize, rawptr, rawsz, ch))
    return ts, soi, secs


def count_in(mm, rx, secs, limit=64):
    hits = []
    for name, va, vsize, rawptr, rawsz, ch in secs:
        if rawsz == 0 or not (ch & 0x20000000):
            continue
        pos, end = rawptr, rawptr + min(rawsz, vsize or rawsz)
        while len(hits) < limit:
            m = rx.search(mm, pos, end)
            if not m:
                break
            hits.append((name, va + (m.start() - rawptr)))
            pos = m.start() + 1
    return hits


def main_code(secs):
    """Secao principal de codigo: a maior secao executavel com menos de 256 MB (ignora o bloco gigante de dados)."""
    cands = [x for x in secs if (x[5] & 0x20000000) and 0 < x[4] and x[2] < 0x10000000]
    return [max(cands, key=lambda x: x[4])] if cands else []


def check(path):
    sigs = load_signatures()
    with open(path, "rb") as f:
        mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
        ts, soi, secs = read_sections(mm)
        print(f"{path}\n  PE timestamp 0x{ts:08X}, size of image 0x{soi:08X}")
        ok = True
        for sid, pat, hint in sigs:
            hits = count_in(mm, to_regex(pat), secs)
            where = ", ".join(f"{n}+0x{r:X}" for n, r in hits[:3])
            state = "unique" if len(hits) == 1 else ("NOT FOUND" if not hits else "AMBIGUOUS")
            ok = ok and len(hits) == 1
            moved = "" if (len(hits) == 1 and hits[0][1] == hint) else f" (reference 0x{hint:X})"
            print(f"  {sid:18s} {state:9s} {where}{moved}")
        print("  campos da carreira (referencias na secao principal de codigo, referencia do PES 2021 entre parenteses):")
        layout_ok = True
        for (label, pat), ref in zip(FIELDS, REFERENCE_COUNTS):
            n = len(count_in(mm, re.compile(pat, re.S), main_code(secs), limit=100000))
            same = n == ref
            layout_ok = layout_ok and same
            print(f"    {label} {n:5d} ({ref}) {'ok' if same else 'DIFERENTE'}")
        print("  veredito:", "compativel pelas assinaturas e pela estrutura" if ok and layout_ok else
              ("assinaturas ok, mas a estrutura da carreira difere: NAO edite sem revisar os offsets" if ok else "assinaturas faltando: recursos serao desligados"))
        return ok and layout_ok


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    results = [check(p) for p in sys.argv[1:]]
    sys.exit(0 if all(results) else 1)
