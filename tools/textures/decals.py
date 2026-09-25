"""Generate the wall decal atlas Textures/decorations/decals.png (RGBA, 4 x 4 cells of 256 px).

    python3 tools/textures/decals.py [out.png]

Every decal is drawn at 2x and downsampled. Cell order must match DECAL_DEFS in src/world/decor.h
(anchor: ceiling decals hang from the top edge of their cell, floor decals grow from the bottom edge).
Transparent pixels get the colour of the nearest paint so linear filtering and mipmaps do not pull
dark fringes in.
"""

import math
import os
import random
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

CELL = 256
S = CELL * 2  # drawing resolution per cell
GRID = 4

INK = (40, 28, 18, 255)
PAINT = {
    "blue": (46, 86, 150, 235),
    "red": (160, 62, 34, 235),
    "green": (54, 120, 90, 235),
    "black": (30, 24, 20, 240),
    "gold": (205, 152, 48, 235),
    "white": (225, 214, 180, 225),
}


def layer():
    return Image.new("RGBA", (S, S), (0, 0, 0, 0))


def over(*layers):
    out = layer()
    for lay in layers:
        out = Image.alpha_composite(out, lay)
    return out


def rotate_pts(pts, ang, origin):
    c, s = math.cos(ang), math.sin(ang)
    ox, oy = origin
    return [(ox + x * c - y * s, oy + x * s + y * c) for x, y in pts]


def weather(img, seed, amount, grain=24):
    """Knock holes into the paint: alpha *= smooth noise."""
    rng = np.random.default_rng(seed)
    noise = Image.fromarray((rng.random((grain, grain)) * 255).astype(np.uint8)).resize((S, S), Image.BICUBIC)
    n = np.asarray(noise, dtype=np.float32) / 255.0
    fine = np.asarray(Image.fromarray((rng.random((S // 4, S // 4)) * 255).astype(np.uint8)).resize((S, S), Image.BILINEAR), dtype=np.float32) / 255
    keep = np.clip((n * 0.7 + fine * 0.3 - 0.22) / 0.35, 0, 1)
    a = np.asarray(img, dtype=np.float32)
    a[..., 3] *= (1 - amount) + amount * keep
    return Image.fromarray(a.astype(np.uint8))


def stroke(draw, pts, w0, w1, color):
    """Polyline with width tapering from w0 to w1, round joints."""
    n = len(pts) - 1
    for i in range(n):
        w = w0 + (w1 - w0) * i / max(n, 1)
        draw.line([pts[i], pts[i + 1]], fill=color, width=max(1, int(round(w))))
        r = w / 2
        x, y = pts[i + 1]
        draw.ellipse([x - r, y - r, x + r, y + r], fill=color)


def leaf_pts(length, width, n=10):
    left = [(length * i / n, width * math.sin(math.pi * i / n) ** 0.8 * (1 - 0.3 * i / n)) for i in range(n + 1)]
    return left + [(x, -y) for x, y in reversed(left[1:-1])]


def leaf(draw, base, ang, length, width, color, outline=None):
    pts = rotate_pts(leaf_pts(length, width), ang, base)
    draw.polygon(pts, fill=color, outline=outline or tuple(int(c * 0.55) for c in color[:3]) + (255,), width=2)


# ---------------------------------------------------------------- cracks


def walk(rng, start, ang, length, width, step=10, wander=0.35, pull=None, branch=0.1, depth=0, out=None):
    """Random-walk polylines (cracks, roots). Returns [(points, w0, w1)]."""
    out = [] if out is None else out
    pts = [start]
    x, y = start
    steps = max(2, int(length / step))
    for i in range(steps):
        ang += rng.uniform(-wander, wander)
        if pull is not None:
            ang += (pull - ang) * 0.15
        x += math.cos(ang) * step
        y += math.sin(ang) * step
        pts.append((x, y))
        if depth < 3 and i > 2 and rng.random() < branch:
            walk(rng, (x, y), ang + rng.choice((-1, 1)) * rng.uniform(0.5, 1.0), length * (1 - i / steps) * 0.6, width * (1 - i / steps) * 0.6,
                 step, wander, pull, branch * 0.8, depth + 1, out)
    out.append((pts, width, max(1.0, width * 0.25)))
    return out


def draw_cracks(paths):
    shadow, light, core = layer(), layer(), layer()
    ds, dl, dc = ImageDraw.Draw(shadow), ImageDraw.Draw(light), ImageDraw.Draw(core)
    for pts, w0, w1 in paths:
        stroke(ds, pts, w0 + 8, w1 + 4, (50, 38, 22, 70))
        stroke(dl, [(x + 2.5, y + 2.5) for x, y in pts], w0 * 0.7, max(1, w1 * 0.7), (240, 226, 190, 150))
        stroke(dc, pts, w0, w1, (38, 27, 16, 235))
    return over(shadow.filter(ImageFilter.GaussianBlur(4)), light, core)


def crack_a():
    rng = random.Random(1)
    paths = walk(rng, (60, 230), -0.15, 420, 9, branch=0.12)
    return draw_cracks(paths)


def crack_b():
    """Impact star: cracks radiating from a chipped hole."""
    rng = random.Random(2)
    paths = []
    for k in range(6):
        a = 2 * math.pi * k / 6 + rng.uniform(-0.3, 0.3)
        walk(rng, (256, 256), a, rng.uniform(120, 220), 7, branch=0.08, out=paths)
    img = draw_cracks(paths)
    d = ImageDraw.Draw(img)
    chip = [(256 + r * math.cos(a), 256 + r * math.sin(a)) for a, r in ((2 * math.pi * k / 9, rng.uniform(16, 30)) for k in range(9))]
    d.polygon(chip, fill=(52, 38, 24, 240))
    d.polygon([(x - 4, y - 4) for x, y in chip[:5]], outline=(235, 220, 180, 160), width=3)
    return img


def crack_floor():
    rng = random.Random(3)
    paths = walk(rng, (250, S), -math.pi / 2, 460, 12, pull=-math.pi / 2, branch=0.14)
    return draw_cracks(paths)


# ---------------------------------------------------------------- ceiling growth


GREENS = [(58, 100, 38, 255), (78, 124, 42, 255), (96, 138, 52, 255), (62, 84, 32, 255), (150, 140, 62, 255)]


def vine_stems(rng, count, down=True, length=(250, 470), start_y=None):
    stems = []
    for k in range(count):
        x0 = S * (0.15 + 0.7 * (k + rng.random() * 0.6) / count)
        y0 = 0 if down else S
        length_k = rng.uniform(*length)
        amp, ph, freq, drift = rng.uniform(10, 30), rng.uniform(0, 6), rng.uniform(0.012, 0.022), rng.uniform(-0.1, 0.1)
        pts = [(x0 + amp * math.sin(ph + freq * t) + t * drift, y0 + (t if down else -t)) for t in range(0, int(length_k), 8)]
        stems.append(pts)
    return stems


def vines(seed, count, leaf_len, flowers):
    rng = random.Random(seed)
    img = layer()
    d = ImageDraw.Draw(img)
    for pts in vine_stems(rng, count):
        stroke(d, pts, 8, 2.5, (46, 70, 26, 255))
        for i in range(2, len(pts) - 1, 3):
            (x0, y0), (x1, y1) = pts[i], pts[i + 1]
            a = math.atan2(y1 - y0, x1 - x0)
            side = 1 if (i // 3) % 2 else -1
            size = leaf_len * (1 - 0.5 * i / len(pts)) * rng.uniform(0.8, 1.15)
            leaf(d, (x0, y0), a + side * rng.uniform(0.8, 1.3), size, size * 0.42, rng.choice(GREENS))
        if flowers:
            for i in range(4, len(pts), 11):
                x, y = pts[i]
                x += rng.uniform(-18, 18)
                for p in range(5):
                    pa = 2 * math.pi * p / 5
                    leaf(d, (x, y), pa, 13, 6, (70, 110, 200, 255), (30, 50, 110, 255))
                d.ellipse([x - 4, y - 4, x + 4, y + 4], fill=(230, 190, 60, 255))
    return img


def roots():
    rng = random.Random(7)
    paths = []
    for k in range(5):
        walk(rng, (S * (0.15 + 0.17 * k + rng.uniform(-0.04, 0.04)), -4), math.pi / 2 + rng.uniform(-0.4, 0.4), rng.uniform(220, 440), 10,
             step=8, wander=0.3, pull=math.pi / 2, branch=0.16, out=paths)
    dark, body = layer(), layer()
    dd, db = ImageDraw.Draw(dark), ImageDraw.Draw(body)
    for pts, w0, w1 in paths:
        stroke(dd, pts, w0 + 3, w1 + 2, (46, 30, 16, 255))
        stroke(db, [(x - 1, y - 1) for x, y in pts], w0 * 0.75, max(1, w1 * 0.6), (122, 88, 50, 255))
    img = over(dark, body)
    d = ImageDraw.Draw(img)
    for pts, w0, _ in paths:  # hairy rootlets
        for i in range(3, len(pts), 4):
            x, y = pts[i]
            a = rng.uniform(0, 2 * math.pi)
            d.line([(x, y), (x + math.cos(a) * 12, y + math.sin(a) * 12 + 5)], fill=(90, 62, 36, 200), width=1)
    return img


def stain():
    """Damp seepage streaking down from the ceiling with moss along the top."""
    rng = random.Random(8)
    wet = layer()
    d = ImageDraw.Draw(wet)
    for k in range(9):
        x = rng.uniform(80, 430)
        w = rng.uniform(14, 44)
        length = rng.uniform(150, 440)
        d.line([(x, 0), (x + rng.uniform(-10, 10), length)], fill=(58, 52, 30, 95), width=int(w))
        d.ellipse([x - w / 2, length - w / 2, x + w / 2, length + w / 2], fill=(58, 52, 30, 95))
    d.rectangle([40, 0, 470, 34], fill=(50, 46, 26, 120))
    wet = wet.filter(ImageFilter.GaussianBlur(12))
    moss = layer()
    dm = ImageDraw.Draw(moss)
    for _ in range(160):
        x, y = rng.uniform(50, 460), abs(rng.gauss(0, 22))
        r = rng.uniform(3, 9)
        g = rng.choice(GREENS[:4])
        dm.ellipse([x - r, y - r, x + r, y + r], fill=g[:3] + (200,))
    return over(wet, moss.filter(ImageFilter.GaussianBlur(1.2)))


# ---------------------------------------------------------------- hieroglyphs

# Each glyph draws into the box (x, y, w, h) in a painted colour with an ink outline.


def g_ankh(d, x, y, w, h, c):
    d.ellipse([x + w * 0.3, y, x + w * 0.7, y + h * 0.38], outline=INK, width=int(w * 0.16))
    d.ellipse([x + w * 0.3, y, x + w * 0.7, y + h * 0.38], outline=c, width=int(w * 0.1))
    d.rectangle([x + w * 0.12, y + h * 0.38, x + w * 0.88, y + h * 0.48], fill=c, outline=INK, width=3)
    d.rectangle([x + w * 0.42, y + h * 0.48, x + w * 0.58, y + h], fill=c, outline=INK, width=3)


def g_reed(d, x, y, w, h, c):
    d.polygon([(x + w * 0.45, y + h), (x + w * 0.35, y + h * 0.4), (x + w * 0.5, y), (x + w * 0.62, y + h * 0.45), (x + w * 0.55, y + h)], fill=c, outline=INK, width=3)
    d.line([(x + w * 0.5, y + h * 0.1), (x + w * 0.5, y + h)], fill=INK, width=2)


def g_water(d, x, y, w, h, c):
    pts = [(x + w * i / 8, y + h * (0.4 if i % 2 else 0.6)) for i in range(9)]
    d.line(pts, fill=INK, width=int(h * 0.16))
    d.line(pts, fill=c, width=int(h * 0.09))


def g_bread(d, x, y, w, h, c):
    d.chord([x + w * 0.1, y + h * 0.35, x + w * 0.9, y + h * 1.15], 180, 360, fill=c, outline=INK, width=3)


def g_sun(d, x, y, w, h, c):
    d.ellipse([x + w * 0.15, y + h * 0.15, x + w * 0.85, y + h * 0.85], fill=c, outline=INK, width=4)
    d.ellipse([x + w * 0.44, y + h * 0.44, x + w * 0.56, y + h * 0.56], fill=INK)


def g_feather(d, x, y, w, h, c):
    pts = [(x + w * 0.5, y + h), (x + w * 0.38, y + h * 0.5), (x + w * 0.42, y + h * 0.15), (x + w * 0.62, y), (x + w * 0.66, y + h * 0.35),
           (x + w * 0.56, y + h * 0.7)]
    d.polygon(pts, fill=c, outline=INK, width=3)
    d.line([(x + w * 0.5, y + h), (x + w * 0.56, y + h * 0.1)], fill=INK, width=2)


def g_snake(d, x, y, w, h, c):
    pts = [(x + w * (0.05 + 0.9 * t), y + h * (0.7 - 0.15 * math.sin(t * 9))) for t in (i / 16 for i in range(17))]
    d.line(pts, fill=INK, width=int(h * 0.2))
    d.line(pts, fill=c, width=int(h * 0.12))
    hx, hy = pts[-1]
    d.polygon([(hx - 6, hy - 14), (hx + 10, hy - 8), (hx + 4, hy + 6)], fill=c, outline=INK, width=2)


def g_djed(d, x, y, w, h, c):
    d.rectangle([x + w * 0.35, y + h * 0.2, x + w * 0.65, y + h], fill=c, outline=INK, width=3)
    for k in range(4):
        yy = y + h * (0.04 + 0.1 * k)
        d.rectangle([x + w * 0.18, yy, x + w * 0.82, yy + h * 0.06], fill=c, outline=INK, width=2)


def g_basket(d, x, y, w, h, c):
    d.chord([x + w * 0.05, y - h * 0.1, x + w * 0.95, y + h * 0.9], 0, 180, fill=c, outline=INK, width=3)
    d.arc([x + w * 0.7, y + h * 0.15, x + w * 1.0, y + h * 0.5], 270, 90, fill=INK, width=4)


def g_legs(d, x, y, w, h, c):
    d.polygon([(x + w * 0.3, y), (x + w * 0.45, y), (x + w * 0.45, y + h * 0.82), (x + w * 0.9, y + h * 0.82), (x + w * 0.9, y + h), (x + w * 0.3, y + h)],
              fill=c, outline=INK, width=3)


def g_bird(d, x, y, w, h, c):
    d.ellipse([x + w * 0.2, y + h * 0.3, x + w * 0.75, y + h * 0.72], fill=c, outline=INK, width=3)
    d.polygon([(x + w * 0.22, y + h * 0.55), (x + w * 0.02, y + h * 0.78), (x + w * 0.3, y + h * 0.68)], fill=c, outline=INK, width=3)
    d.ellipse([x + w * 0.6, y + h * 0.1, x + w * 0.86, y + h * 0.36], fill=c, outline=INK, width=3)
    d.polygon([(x + w * 0.85, y + h * 0.2), (x + w * 0.98, y + h * 0.26), (x + w * 0.84, y + h * 0.3)], fill=PAINT["gold"], outline=INK, width=2)
    d.ellipse([x + w * 0.7, y + h * 0.18, x + w * 0.76, y + h * 0.24], fill=INK)
    for lx in (0.42, 0.55):
        d.line([(x + w * lx, y + h * 0.7), (x + w * lx, y + h)], fill=INK, width=3)


def g_eye(d, x, y, w, h, c):
    """Wedjat, the eye of Horus."""
    d.arc([x + w * 0.05, y + h * 0.05, x + w * 0.95, y + h * 0.4], 190, 350, fill=INK, width=int(h * 0.07))
    d.chord([x + w * 0.1, y + h * 0.25, x + w * 0.9, y + h * 0.65], 180, 360, fill=PAINT["white"], outline=INK, width=4)
    d.chord([x + w * 0.1, y + h * 0.3, x + w * 0.9, y + h * 0.55], 0, 180, fill=PAINT["white"], outline=INK, width=4)
    d.ellipse([x + w * 0.38, y + h * 0.28, x + w * 0.56, y + h * 0.52], fill=c, outline=INK, width=3)
    d.line([(x + w * 0.9, y + h * 0.42), (x + w * 1.0, y + h * 0.42)], fill=INK, width=int(h * 0.05))
    d.line([(x + w * 0.4, y + h * 0.55), (x + w * 0.36, y + h * 0.95)], fill=INK, width=int(h * 0.06))
    spiral = [(x + w * (0.55 + 0.12 * math.cos(t) * (1 - t / 9)), y + h * (0.78 + 0.12 * math.sin(t) * (1 - t / 9))) for t in (i * 0.3 for i in range(28))]
    d.line([(x + w * 0.5, y + h * 0.55)] + spiral, fill=INK, width=int(h * 0.05))


def g_scarab(d, x, y, w, h, c):
    for s in (-1, 1):
        for k in range(3):
            y0 = y + h * (0.45 + 0.15 * k)
            d.line([(x + w * 0.5, y0), (x + w * (0.5 + s * 0.42), y0 + h * (0.12 * (k - 1)))], fill=INK, width=4)
    d.ellipse([x + w * 0.28, y + h * 0.35, x + w * 0.72, y + h * 0.95], fill=c, outline=INK, width=3)
    d.line([(x + w * 0.5, y + h * 0.5), (x + w * 0.5, y + h * 0.95)], fill=INK, width=2)
    d.chord([x + w * 0.34, y + h * 0.18, x + w * 0.66, y + h * 0.48], 180, 360, fill=c, outline=INK, width=3)
    d.ellipse([x + w * 0.35, y, x + w * 0.65, y + h * 0.28], fill=PAINT["red"], outline=INK, width=3)


GLYPHS = [g_ankh, g_reed, g_water, g_bread, g_sun, g_feather, g_snake, g_djed, g_basket, g_legs, g_bird, g_eye, g_scarab]
COLOURS = ["blue", "red", "green", "black", "gold"]


def panel(d, box):
    x0, y0, x1, y1 = box
    d.rectangle(box, fill=(232, 216, 170, 70))
    d.rectangle(box, outline=(120, 78, 40, 210), width=5)
    d.rectangle([x0 + 10, y0 + 10, x1 - 10, y1 - 10], outline=(120, 78, 40, 170), width=2)


def glyph_column():
    rng = random.Random(11)
    img = layer()
    d = ImageDraw.Draw(img)
    panel(d, (150, 12, 362, 500))
    for k, g in enumerate(rng.sample(GLYPHS, 5)):
        g(d, 196, 34 + k * 93, 120, 78, PAINT[COLOURS[k % len(COLOURS)]])
    return weather(img, 11, 0.55)


def glyph_row():
    rng = random.Random(12)
    img = layer()
    d = ImageDraw.Draw(img)
    panel(d, (10, 180, 502, 332))
    for k, g in enumerate(rng.sample(GLYPHS, 5)):
        g(d, 30 + k * 95, 206, 80, 100, PAINT[COLOURS[(k + 2) % len(COLOURS)]])
    return weather(img, 12, 0.55)


def cartouche():
    img = layer()
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([176, 36, 336, 446], radius=78, fill=(232, 216, 170, 90), outline=INK, width=12)
    d.rounded_rectangle([176, 36, 336, 446], radius=78, outline=PAINT["gold"], width=6)
    d.rectangle([150, 446, 362, 474], fill=PAINT["gold"], outline=INK, width=4)
    for k, (g, col) in enumerate(((g_sun, "red"), (g_scarab, "blue"), (g_bread, "green"))):
        g(d, 206, 80 + k * 118, 100, 96, PAINT[col])
    return weather(img, 13, 0.45)


def wedjat():
    img = layer()
    d = ImageDraw.Draw(img)
    g_eye(d, 40, 110, 432, 300, PAINT["blue"])
    return weather(img, 14, 0.5)


def winged_sun():
    img = layer()
    d = ImageDraw.Draw(img)
    cx, cy = 256, 220
    rows = [(PAINT["blue"], 0.0), (PAINT["red"], 0.33), (PAINT["green"], 0.66)]
    for s in (-1, 1):
        for colour, t in rows:
            for k in range(9):
                u0, u1 = k / 9, (k + 1) / 9
                span = 230 * (1 - 0.3 * t)
                xa, xb = cx + s * (40 + span * u0), cx + s * (40 + span * u1)
                top = cy - 30 + 40 * t + 26 * u0 ** 2
                feather_len = 42 * (1 - 0.35 * u0)
                d.polygon([(xa, top), (xb, top + 6), (xb, top + feather_len), (xa, top + feather_len + 4)], fill=colour, outline=INK, width=2)
    for s in (-1, 1):  # uraei
        pts = [(cx + s * (40 + 8 * math.sin(t)), cy + 20 + t * 12) for t in (i * 0.5 for i in range(9))]
        d.line(pts, fill=INK, width=10)
        d.line(pts, fill=PAINT["gold"], width=6)
    d.ellipse([cx - 46, cy - 70, cx + 46, cy + 22], fill=PAINT["red"], outline=PAINT["gold"], width=6)
    d.ellipse([cx - 46, cy - 70, cx + 46, cy + 22], outline=INK, width=2)
    return weather(img, 15, 0.5)


# ---------------------------------------------------------------- floor growth


def papyrus():
    rng = random.Random(21)
    img = layer()
    d = ImageDraw.Draw(img)
    for k in range(6):
        x0 = 256 + rng.uniform(-60, 60)
        lean = rng.uniform(-0.35, 0.35)
        length = rng.uniform(220, 420)
        top = (x0 + lean * length, S - length)
        pts = [(x0 + lean * t + 10 * math.sin(t / 60) * lean, S - t) for t in range(0, int(length), 10)] + [top]
        stroke(d, pts, 7, 4, (60, 104, 38, 255))
        for r in range(22):  # umbel: fan of thin rays
            a = -math.pi / 2 + lean + (r / 21 - 0.5) * 2.4
            ln = rng.uniform(40, 70)
            end = (top[0] + math.cos(a) * ln, top[1] + math.sin(a) * ln + ln * 0.25)
            d.line([top, end], fill=rng.choice(GREENS[1:3] + [(170, 160, 70, 255)]), width=2)
        d.ellipse([top[0] - 5, top[1] - 5, top[0] + 5, top[1] + 5], fill=(120, 110, 50, 255))
    for k in range(5):  # brown sheath leaves at the base
        x0 = 256 + rng.uniform(-70, 70)
        leaf(d, (x0, S + 4), -math.pi / 2 + rng.uniform(-0.6, 0.6), rng.uniform(50, 80), 12, (120, 92, 44, 255))
    return img


def dry_grass():
    rng = random.Random(22)
    img = layer()
    d = ImageDraw.Draw(img)
    straw = [(186, 156, 84, 255), (160, 128, 64, 255), (130, 104, 52, 255), (110, 124, 52, 255), (200, 176, 104, 255)]
    for k in range(42):
        x0 = 256 + rng.gauss(0, 60)
        length = rng.uniform(70, 230)
        bend = rng.uniform(-0.9, 0.9)
        w = rng.uniform(4, 8)
        n = 8
        left = [(x0 + bend * (t / n) ** 2 * length * 0.5 - w * (1 - t / n), S - length * t / n) for t in range(n + 1)]
        right = [(x0 + bend * (t / n) ** 2 * length * 0.5 + w * (1 - t / n), S - length * t / n) for t in range(n, -1, -1)]
        col = rng.choice(straw)
        d.polygon(left + right, fill=col, outline=tuple(int(c * 0.6) for c in col[:3]) + (255,), width=1)
    return img


def creeper():
    """Ivy climbing up from the floor."""
    rng = random.Random(23)
    img = layer()
    d = ImageDraw.Draw(img)
    for pts in vine_stems(rng, 3, down=False, length=(200, 440)):
        stroke(d, pts, 8, 2.5, (46, 70, 26, 255))
        for i in range(2, len(pts) - 1, 2):
            (x0, y0), (x1, y1) = pts[i], pts[i + 1]
            a = math.atan2(y1 - y0, x1 - x0)
            side = 1 if (i // 2) % 2 else -1
            size = 34 * (1 - 0.45 * i / len(pts)) * rng.uniform(0.85, 1.15)
            leaf(d, (x0, y0), a + side * rng.uniform(1.0, 1.5), size, size * 0.6, rng.choice(GREENS[:4]))
    return img


def moss():
    rng = random.Random(24)
    img = layer()
    d = ImageDraw.Draw(img)
    for _ in range(12):
        cx, cy = 256 + rng.gauss(0, 70), 256 + rng.gauss(0, 50)
        for _ in range(40):
            x, y = cx + rng.gauss(0, 26), cy + rng.gauss(0, 18)
            r = rng.uniform(4, 12)
            g = rng.choice(GREENS[:4])
            d.ellipse([x - r, y - r, x + r, y + r], fill=g[:3] + (170,))
    img = img.filter(ImageFilter.GaussianBlur(1.5))
    return weather(img, 24, 0.4, grain=12)


# Order = atlas cell = DECAL_DEFS in src/world/decor.h.
DECALS = [crack_a, crack_b, crack_floor, lambda: vines(4, 4, 34, False), lambda: vines(5, 3, 26, True), roots, stain, glyph_column, glyph_row, cartouche,
          wedjat, winged_sun, papyrus, dry_grass, creeper, moss]


def bleed(img):
    """Fill transparent pixels with the surrounding paint colour (alpha untouched)."""
    a = np.asarray(img, dtype=np.float32) / 255
    alpha = a[..., 3:4]
    rgb = a[..., :3]
    acc_c, acc_a = rgb * alpha, alpha.copy()
    out = rgb.copy()
    filled = alpha[..., 0] > 0.5
    for radius in (2, 6, 16, 40):
        c = np.stack([np.asarray(Image.fromarray((acc_c[..., i] * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(radius)), dtype=np.float32) for i in range(3)], -1)
        w = np.asarray(Image.fromarray((acc_a[..., 0] * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(radius)), dtype=np.float32)[..., None]
        ok = (w[..., 0] > 0.5) & ~filled
        out[ok] = c[ok] / w[ok]
        filled |= ok
    out[~filled] = rgb[alpha[..., 0] > 0.5].mean(axis=0) if (alpha > 0.5).any() else 0.5
    res = np.concatenate([out, alpha], -1)
    return Image.fromarray((np.clip(res, 0, 1) * 255).astype(np.uint8))


def main(path):
    atlas = Image.new("RGBA", (CELL * GRID, CELL * GRID), (0, 0, 0, 0))
    for k, fn in enumerate(DECALS):
        cell = fn().resize((CELL, CELL), Image.LANCZOS)
        atlas.paste(cell, ((k % GRID) * CELL, (k // GRID) * CELL))
    bleed(atlas).save(path)
    print("wrote", path)


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "..", "Textures", "decorations", "decals.png"))
