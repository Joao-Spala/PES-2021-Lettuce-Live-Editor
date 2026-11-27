"""
Gera os badges do README como SVG proprios em docs/assets/badges (nada vem de servicos externos).

Uso: python docs/assets/gen_badges.py
A versao do badge de versao vem de native/src/app_version.hpp (kAppVersion), entao rode de novo ao subir a versao.
Largura do texto medida com a fonte Verdana (Pillow). Sem Verdana instalada, usa uma estimativa por caractere.
"""
import re
import sys
from pathlib import Path
from xml.sax.saxutils import escape

ROOT = Path(__file__).resolve().parents[2]
OUT = Path(__file__).resolve().parent / "badges"

try:
    from PIL import ImageFont
    _FONT = ImageFont.truetype(r"C:\Windows\Fonts\verdanab.ttf", 10)
except Exception:  # sem Pillow ou sem Verdana
    _FONT = None

H = 28
PAD = 12
SPACING = 1.0   # espacamento extra entre letras (estilo "for-the-badge")
FONT_FAMILY = "Verdana,Geneva,DejaVu Sans,sans-serif"


def text_width(s: str) -> float:
    if _FONT is not None:
        w = _FONT.getlength(s)
    else:
        w = sum(7.2 if c.isupper() or c.isdigit() else 6.2 for c in s)
    return w + SPACING * len(s)


KOFI_CUP = (
    "M2 7h14v6a5 5 0 0 1-5 5H7a5 5 0 0 1-5-5V7z M16 8h2a2.5 2.5 0 0 1 0 5h-2 "
    "M5 21h10"
)


def badge(name: str, label: str, message: str, color: str, label_color: str = "#555555", icon: str = "") -> None:
    label, message = label.upper(), message.upper()
    icon_w = 22 if icon else 0
    lw = text_width(label) + 2 * PAD + icon_w
    mw = text_width(message) + 2 * PAD
    w = lw + mw
    lx = (lw + icon_w) / 2 if icon else lw / 2
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{w:.0f}" height="{H}" viewBox="0 0 {w:.0f} {H}" role="img" aria-label="{escape(label)}: {escape(message)}">',
        f"<title>{escape(label)}: {escape(message)}</title>",
        f'<g shape-rendering="crispEdges"><rect width="{lw:.0f}" height="{H}" fill="{label_color}"/><rect x="{lw:.0f}" width="{mw:.0f}" height="{H}" fill="{color}"/></g>',
    ]
    if icon == "kofi":
        parts.append(f'<g transform="translate({PAD - 2},{(H - 18) / 2:.1f}) scale(0.82)" fill="none" stroke="#ffffff" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="{KOFI_CUP}"/></g>')
    parts.append(
        f'<g fill="#ffffff" text-anchor="middle" font-family="{FONT_FAMILY}" font-weight="bold" font-size="10" text-rendering="geometricPrecision">'
        f'<text x="{lx:.1f}" y="17.5" textLength="{text_width(label) - SPACING:.0f}" lengthAdjust="spacing">{escape(label)}</text>'
        f'<text x="{lw + mw / 2:.1f}" y="17.5" textLength="{text_width(message) - SPACING:.0f}" lengthAdjust="spacing">{escape(message)}</text>'
        "</g></svg>"
    )
    (OUT / f"{name}.svg").write_text("".join(parts) + "\n", encoding="utf-8")


def app_version() -> str:
    src = (ROOT / "native" / "src" / "app_version.hpp").read_text(encoding="utf-8")
    m = re.search(r'kAppVersion\s*=\s*"([0-9A-Za-z.+-]+)"', src)
    if not m:
        sys.exit("kAppVersion nao encontrado em native/src/app_version.hpp")
    return m.group(1)


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    badge("version", "Version", "v" + app_version(), "#84CC16")
    badge("stack", "Native", "C++20 + DX11 + ImGui", "#84CC16")
    badge("game", "eFootball PES 2021", "v1.7.2.0 (DP 7.0)", "#0095D9")
    badge("compat", "Sider 7.3 / Football Life", "Compatible", "#F59E0B")
    badge("license", "License", "MIT", "#8B5CF6")
    badge("languages", "Languages", "PT-BR | EN | ES", "#2EA043")
    badge("kofi-en", "Ko-fi", "Buy me a coffee", "#FF5E5B", icon="kofi")
    badge("kofi-pt", "Ko-fi", "Pague um café", "#FF5E5B", icon="kofi")
    badge("pix", "Pix", "QR Code", "#32BCAD")
    print("badges em", OUT)


if __name__ == "__main__":
    main()
