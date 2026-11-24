"""
Generate a crisp multi-resolution Windows .ico (16, 24, 32, 48, 64, 128, 256px)
for PES 2021 Workbench Native (zero external dependencies beyond Python stdlib / Pillow fallback).
"""
import math
import struct
import zlib
from pathlib import Path

OUT_DIR = Path(r"C:\Users\joaov\Desktop\teste-ia\pes21-workbench\native\assets")
OUT_DIR.mkdir(parents=True, exist_ok=True)


def clamp(v: float, lo: float = 0.0, hi: float = 1.0) -> float:
    return lo if v < lo else (hi if v > hi else v)


def smoothstep(e0: float, e1: float, x: float) -> float:
    t = clamp((x - e0) / (e1 - e0))
    return t * t * (3.0 - 2.0 * t)


def sdf_rounded_box(px: float, py: float, bx: float, by: float, r: float) -> float:
    qx = abs(px) - bx + r
    qy = abs(py) - by + r
    return math.hypot(max(qx, 0.0), max(qy, 0.0)) + min(max(qx, qy), 0.0) - r


def sdf_segment(px: float, py: float, ax: float, ay: float, bx: float, by: float) -> float:
    pax, pay = px - ax, py - ay
    bax, bay = bx - ax, by - ay
    h = clamp((pax * bax + pay * bay) / (bax * bax + bay * bay + 1e-9))
    return math.hypot(pax - bax * h, pay - bay * h)


def sdf_glyph_21(x: float, y: float) -> float:
    """Smooth studio-grade '21' monogram in normalized [-1, 1] coordinates."""
    d = 999.0
    # Smooth 24-segment Bezier/ellipse arch for digit '2'
    pts2 = []
    cx, cy, rx, ry = -0.27, -0.21, 0.20, 0.19
    for i in range(25):
        # angle from 175 deg down through 90 deg (top) to -22 deg (right-lower)
        ang = math.radians(178.0 - (202.0 * i / 24.0))
        pts2.append((cx + rx * math.cos(ang), cy - ry * math.sin(ang)))
    # Smooth diagonal sweep down to bottom-left corner of '2'
    x0, y0 = pts2[-1]
    x1, y1 = -0.48, 0.39
    for i in range(1, 9):
        t = i / 8.0
        # Slight curve for athletic typography feel
        px = x0 + (x1 - x0) * t + math.sin(t * math.pi) * 0.035
        py = y0 + (y1 - y0) * t
        pts2.append((px, py))
    pts2.append((-0.05, 0.39))
    for i in range(len(pts2) - 1):
        d = min(d, sdf_segment(x, y, pts2[i][0], pts2[i][1], pts2[i + 1][0], pts2[i + 1][1]))

    # Digit '1' on the right
    d = min(d, sdf_segment(x, y, 0.34, -0.40, 0.34, 0.39))
    d = min(d, sdf_segment(x, y, 0.16, -0.25, 0.34, -0.40))
    d = min(d, sdf_segment(x, y, 0.17, 0.39, 0.50, 0.39))
    return d - 0.078


def render_rgba(size: int) -> bytes:
    """Render supersampled anti-aliased RGBA icon image."""
    ss = 3 if size <= 64 else 2
    buf = bytearray(size * size * 4)
    inv_s = 1.0 / size
    inv_ss = 1.0 / (ss * ss)

    for py in range(size):
        for px in range(size):
            ar = ag = ab = aa = 0.0
            for sy in range(ss):
                for sx in range(ss):
                    u = ((px + (sx + 0.5) / ss) * inv_s) * 2.0 - 1.0
                    v = ((py + (sy + 0.5) / ss) * inv_s) * 2.0 - 1.0

                    # Outer rounded squircle
                    d_box = sdf_rounded_box(u, v, 0.90, 0.90, 0.34)
                    if d_box > 0.03:
                        continue
                    alpha_mask = 1.0 - smoothstep(-0.015, 0.025, d_box)

                    # Rich dark-navy + cobalt radial/linear background
                    t_grad = clamp((u * 0.45 + v * 0.55 + 1.0) * 0.5)
                    r = 14.0 * (1.0 - t_grad) + 9.0 * t_grad
                    g = 24.0 * (1.0 - t_grad) + 15.0 * t_grad
                    b = 48.0 * (1.0 - t_grad) + 30.0 * t_grad

                    # Top-left electric blue glow
                    glow = math.exp(-((u + 0.45) ** 2 + (v + 0.55) ** 2) * 1.65)
                    r += glow * 34.0
                    g += glow * 78.0
                    b += glow * 165.0

                    # Bottom-right subtle emerald tactical glow
                    eglow = math.exp(-((u - 0.55) ** 2 + (v - 0.60) ** 2) * 2.4)
                    r += eglow * 6.0
                    g += eglow * 56.0
                    b += eglow * 44.0

                    # Subtle tactical pitch center-circle & halfway line watermark
                    d_ring = abs(math.hypot(u, v) - 0.62)
                    ring_a = (1.0 - smoothstep(0.010, 0.028, d_ring)) * 0.16
                    d_mid = abs(v)
                    mid_a = (1.0 - smoothstep(0.008, 0.022, d_mid)) * 0.10
                    wm = clamp(ring_a + mid_a)
                    r = r * (1.0 - wm) + 52.0 * wm
                    g = g * (1.0 - wm) + 211.0 * wm
                    b = b * (1.0 - wm) + 153.0 * wm

                    # Crisp metallic cobalt border
                    border_band = (1.0 - smoothstep(0.0, 0.042, abs(d_box + 0.024)))
                    br = 59.0 + (1.0 - t_grad) * 55.0
                    bg = 130.0 + (1.0 - t_grad) * 65.0
                    bb = 246.0
                    ba = border_band * 0.90
                    r = r * (1.0 - ba) + br * ba
                    g = g * (1.0 - ba) + bg * ba
                    b = b * (1.0 - ba) + bb * ba

                    # Crisp drop shadow behind '21'
                    d_sh = sdf_glyph_21(u - 0.025, v - 0.04)
                    sh_a = (1.0 - smoothstep(-0.02, 0.06, d_sh)) * 0.45
                    r *= (1.0 - sh_a)
                    g *= (1.0 - sh_a)
                    b *= (1.0 - sh_a)

                    # Crisp white/ice-blue '21' monogram
                    d_g = sdf_glyph_21(u, v)
                    ga = 1.0 - smoothstep(-0.015, 0.022, d_g)
                    if ga > 0.0:
                        gt = clamp((v + 0.45) / 0.90)
                        gr = 248.0 * (1.0 - gt) + 206.0 * gt
                        gg = 250.0 * (1.0 - gt) + 228.0 * gt
                        gb = 255.0
                        r = r * (1.0 - ga) + gr * ga
                        g = g * (1.0 - ga) + gg * ga
                        b = b * (1.0 - ga) + gb * ga

                    # Emerald live status dot (top-right corner)
                    d_dot = math.hypot(u - 0.58, v + 0.58)
                    dot_glow = math.exp(-(d_dot ** 2) * 32.0) * 0.45
                    r = r * (1.0 - dot_glow) + 52.0 * dot_glow
                    g = g * (1.0 - dot_glow) + 211.0 * dot_glow
                    b = b * (1.0 - dot_glow) + 153.0 * dot_glow
                    dot_a = 1.0 - smoothstep(0.085, 0.115, d_dot)
                    if dot_a > 0.0:
                        r = r * (1.0 - dot_a) + 52.0 * dot_a
                        g = g * (1.0 - dot_a) + 220.0 * dot_a
                        b = b * (1.0 - dot_a) + 160.0 * dot_a

                    ar += r * alpha_mask
                    ag += g * alpha_mask
                    ab += b * alpha_mask
                    aa += alpha_mask

            aa *= inv_ss
            idx = (py * size + px) * 4
            if aa > 1e-4:
                buf[idx + 0] = int(clamp(ar * inv_ss / aa, 0.0, 255.0))
                buf[idx + 1] = int(clamp(ag * inv_ss / aa, 0.0, 255.0))
                buf[idx + 2] = int(clamp(ab * inv_ss / aa, 0.0, 255.0))
                buf[idx + 3] = int(clamp(aa * 255.0, 0.0, 255.0))
    return bytes(buf)


def encode_png(w: int, h: int, rgba: bytes) -> bytes:
    def chunk(tag: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    raw_rows = bytearray()
    stride = w * 4
    for y in range(h):
        raw_rows.append(0)
        raw_rows.extend(rgba[y * stride : (y + 1) * stride])

    ihdr = struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", ihdr)
        + chunk(b"IDAT", zlib.compress(bytes(raw_rows), 9))
        + chunk(b"IEND", b"")
    )


def write_ico(path: Path, png_list: list) -> None:
    count = len(png_list)
    header = struct.pack("<HHH", 0, 1, count)
    entries = bytearray()
    offset = 6 + count * 16
    blobs = bytearray()
    for sz, png_bytes in png_list:
        w_b = 0 if sz >= 256 else sz
        h_b = 0 if sz >= 256 else sz
        entries.extend(struct.pack("<BBBBHHII", w_b, h_b, 0, 0, 1, 32, len(png_bytes), offset))
        blobs.extend(png_bytes)
        offset += len(png_bytes)
    path.write_bytes(header + bytes(entries) + bytes(blobs))


def main() -> None:
    sizes = [16, 24, 32, 48, 64, 128, 256]
    pngs = []
    for s in sizes:
        rgba = render_rgba(s)
        png = encode_png(s, s, rgba)
        pngs.append((s, png))
        if s == 256:
            (OUT_DIR / "app_icon_preview.png").write_bytes(png)
    ico_path = OUT_DIR / "app_icon.ico"
    write_ico(ico_path, pngs)
    print(f"Generated {ico_path} ({ico_path.stat().st_size:,} bytes, {len(sizes)} sizes)")


if __name__ == "__main__":
    main()
