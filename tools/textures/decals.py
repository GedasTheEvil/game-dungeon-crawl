"""Generate the wall decal atlas media/textures/decorations/decals.png (RGBA, 8 x 8 cells of 256 px).

    python3 tools/textures/decals.py [out.png]

Every decal is drawn at 2x and downsampled. Cell order must match DECAL_DEFS in src/level/decor.h
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
GRID = 8  # room for 64 decals

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


# ---------------------------------------------------------------- Duat scenes (plan duat-wall-paintings)
# Painted wall scenes in the register style of the tomb walls: profile figures in flat paint with an ink outline on a
# band of plaster, weathered. Figures face right; flipped() turns one around.

SKIN = (150, 70, 40, 240)  # a man's red-brown
KILT = PAINT["white"]
WIG = (34, 40, 70, 240)
NIGHT = (24, 38, 84, 230)  # the night sky, Nut's body


def flipped(img, x):
    """img mirrored about the vertical line x."""
    return img.transform(img.size, Image.AFFINE, (-1, 0, 2 * x, 0, 1, 0), resample=Image.BICUBIC)


def scene_panel(d, top, bottom):
    """The plaster band of a register, with its ground line."""
    d.rectangle([6, top, S - 6, bottom], fill=(232, 216, 170, 80))
    d.rectangle([6, top, S - 6, bottom], outline=(120, 78, 40, 200), width=5)
    d.line([(6, bottom - 16), (S - 6, bottom - 16)], fill=INK, width=4)
    for k in range(12):  # the band of coloured blocks under the ground line
        d.rectangle([6 + k * 42, bottom - 14, 6 + (k + 1) * 42, bottom - 2], fill=PAINT[COLOURS[k % 3]])


def limb(d, pts, w, c):
    stroke(d, pts, w + 5, w + 5, INK)
    stroke(d, pts, w, w, c)


def head(d, x, y, u, kind, skin):
    """A head looking right, its centre (x, y); u is a tenth of the figure's height."""
    r = 0.8 * u
    if kind == "jackal":
        d.polygon([(x - r * 0.9, y - r * 0.4), (x - r * 0.6, y - r * 2.6), (x - r * 0.1, y - r * 0.9), (x + r * 0.3, y - r * 2.6), (x + r * 0.5, y - r * 0.7),
                   (x + r * 2.3, y + r * 0.1), (x + r * 2.2, y + r * 0.5), (x + r * 0.5, y + r * 0.8), (x - r * 0.9, y + r * 0.9)], fill=PAINT["black"], outline=INK, width=3)
        d.ellipse([x + r * 0.35, y - r * 0.3, x + r * 0.75, y + r * 0.05], fill=PAINT["gold"])
    elif kind == "ibis":
        d.ellipse([x - r, y - r, x + r, y + r], fill=PAINT["black"], outline=INK, width=3)
        beak = [(x + r * (0.8 + 2.6 * t), y + r * (0.1 + 1.6 * t * t)) for t in (i / 8 for i in range(9))]
        limb(d, beak, max(2, int(r * 0.35)), PAINT["black"])
        d.ellipse([x + r * 0.15, y - r * 0.35, x + r * 0.5, y], fill=PAINT["white"])
    elif kind == "ram":
        d.polygon([(x - r * 1.0, y - r * 1.0), (x + r * 0.6, y - r * 1.1), (x + r * 2.2, y + r * 0.3), (x + r * 2.0, y + r * 1.0), (x + r * 0.8, y + r * 1.1),
                   (x - r * 1.0, y + r * 1.0)], fill=skin, outline=INK, width=3)
        d.ellipse([x + r * 0.7, y - r * 0.6, x + r * 1.1, y - r * 0.2], fill=INK)
        spiral = [(x - r * 0.1 + r * 1.1 * math.cos(t) * (1 - t / 10), y + r * 0.2 + r * 1.1 * math.sin(t) * (1 - t / 10)) for t in (-1.2 + i * 0.4 for i in range(20))]
        limb(d, spiral, max(4, int(r * 0.5)), PAINT["gold"])
        return
    else:
        d.ellipse([x - r, y - r, x + r, y + r], fill=skin, outline=INK, width=3)
        d.ellipse([x + r * 0.3, y - r * 0.3, x + r * 0.6, y], fill=INK)
    # the striped wig of a god falls behind the head to the shoulder
    d.polygon([(x - r * 1.1, y - r * 0.8), (x - r * 0.1, y - r * 0.9), (x - r * 0.2, y + r * 1.8), (x - r * 1.2, y + r * 1.6)], fill=WIG, outline=INK, width=3)


def figure(d, x, g, h, kind="human", skin=SKIN, arms=None, stride=1.0):
    """A standing figure looking right, feet on the ground line g at x, h tall. arms: two polylines in units u from
    the shoulder (back arm, front arm); None hangs them."""
    u = h / 10
    sh = g - 8 * u
    for fx, hx in ((-1.2 * stride, -0.35), (1.3 * stride, 0.35)):
        limb(d, [(x + hx * u, g - 3.4 * u), (x + fx * u, g - 0.3 * u)], max(3, int(0.55 * u)), skin)
        d.polygon([(x + fx * u - 0.2 * u, g - 0.4 * u), (x + fx * u + 0.9 * u, g - 0.1 * u), (x + fx * u - 0.2 * u, g)], fill=skin, outline=INK, width=2)
    d.polygon([(x - 1.4 * u, sh), (x + 1.4 * u, sh), (x + 0.6 * u, g - 5.2 * u), (x - 0.5 * u, g - 5.2 * u)], fill=skin, outline=INK, width=3)
    d.polygon([(x - 0.8 * u, g - 5.5 * u), (x + 0.9 * u, g - 5.5 * u), (x + 1.5 * u, g - 3.2 * u), (x - 1.1 * u, g - 3.2 * u)], fill=KILT, outline=INK, width=3)
    d.line([(x - 0.8 * u, g - 5.3 * u), (x + 0.9 * u, g - 5.3 * u)], fill=PAINT["red"], width=max(2, int(0.25 * u)))
    if arms is None:
        arms = ([(0, 0), (-0.4, 2.0), (-0.3, 3.4)], [(0, 0), (0.4, 2.0), (0.6, 3.4)])
    for side, arm in zip((-1.2, 1.2), arms):
        limb(d, [(x + (side + ax) * u, sh + ay * u) for ax, ay in arm], max(3, int(0.45 * u)), skin)
    d.rectangle([x - 0.35 * u, sh - 0.6 * u, x + 0.35 * u, sh + 0.1 * u], fill=skin)
    head(d, x + 0.15 * u, sh - 1.2 * u, u, kind, skin)
    return u


def sun_disc(d, x, y, r, colour="red"):
    d.ellipse([x - r, y - r, x + r, y + r], fill=PAINT[colour], outline=PAINT["gold"], width=max(2, int(r * 0.2)))
    d.ellipse([x - r, y - r, x + r, y + r], outline=INK, width=2)


def serpent(d, pts, w, colour, spots=True):
    limb(d, pts, w, colour)
    if spots:
        for k in range(2, len(pts) - 2, 3):
            x, y = pts[k]
            d.ellipse([x - w * 0.22, y - w * 0.22, x + w * 0.22, y + w * 0.22], fill=INK)


def night_barque():
    """Ra's night barque: the ram-headed sun in his shrine, Mehen coiled over it, towed by three gods."""
    img = layer()
    d = ImageDraw.Draw(img)
    scene_panel(d, 110, 410)
    water = 394
    for k in range(9):  # the water of the Duat under the boat
        x0 = 20 + k * 54
        d.line([(x0 + i * 6, water - 6 + 5 * math.sin(i)) for i in range(9)], fill=PAINT["blue"], width=4)
    # the papyrus hull, both ends raised high
    keel = [(180 + 300 * t, 372 + 10 * math.sin(math.pi * t)) for t in (i / 16 for i in range(17))]
    deck = [(470 - 280 * t, 344) for t in (i / 4 for i in range(5))]
    d.polygon([(150, 286), (166, 290)] + keel + [(500, 282), (512 - 20, 280), (468, 344)] + deck[1:] + [(170, 344)], fill=PAINT["gold"], outline=INK, width=4)
    d.line([(178, 354), (466, 354)], fill=PAINT["red"], width=6)
    # the shrine on deck, and in it the ram-headed sun
    d.rectangle([262, 166, 392, 344], fill=(232, 216, 170, 220), outline=INK, width=4)
    d.rectangle([254, 154, 400, 170], fill=PAINT["green"], outline=INK, width=3)
    ra = layer()  # he looks ahead, the way the boat sails
    figure(ImageDraw.Draw(ra), 327, 342, 136, "ram", SKIN, ([(0, 0), (-0.2, 2.0), (0.3, 3.4)], [(0, 0), (1.2, 1.8), (2.0, 0.8)]), stride=0.6)
    sun_disc(ImageDraw.Draw(ra), 325, 190, 14)
    img = Image.alpha_composite(img, flipped(ra, 327))
    d = ImageDraw.Draw(img)
    # Mehen, the coiled serpent, arches over the shrine
    coil = [(327 + 92 * math.cos(math.pi * (1 - t)), 164 - 40 * math.sin(math.pi * t)) for t in (i / 24 for i in range(25))]
    serpent(d, coil, 12, PAINT["green"])
    d.polygon([(414, 166), (436, 156), (432, 174)], fill=PAINT["green"], outline=INK, width=3)
    # the towing rope from the prow, and three gods hauling it, walking away from the boat
    prow = (156, 300)
    d.line([prow, (16, 300)], fill=INK, width=6)
    d.line([prow, (16, 300)], fill=PAINT["gold"], width=2)
    for k, x in enumerate((30, 78, 126)):
        hauler = layer()
        figure(ImageDraw.Draw(hauler), x, water - 2, 118, "jackal" if k == 1 else "human", PAINT["black"] if k == 1 else SKIN,
               ([(0, 0), (-0.8, 1.6), (-1.6, 2.6)], [(0, 0), (-0.2, 1.8), (-0.8, 2.6)]), stride=1.3)
        img = Image.alpha_composite(img, flipped(hauler, x))
    img = flipped(img, S / 2)  # the barque sails left, into the next hour
    return weather(img, 31, 0.45)


def knife(d, x, y, length, ang):
    tip = (x + math.cos(ang) * length, y + math.sin(ang) * length)
    side = (math.cos(ang + math.pi / 2) * 8, math.sin(ang + math.pi / 2) * 8)
    d.polygon([(x - side[0], y - side[1]), tip, (x + side[0] * 1.6, y + side[1] * 1.6)], fill=(210, 214, 220, 240), outline=INK, width=3)
    hx, hy = x - math.cos(ang) * 28, y - math.sin(ang) * 28
    limb(d, [(x, y), (hx, hy)], 9, PAINT["red"])


def cat(d, x, g, h):
    """The cat of Ra sitting up, a knife in its fore paw."""
    u = h / 10
    d.ellipse([x - 2.6 * u, g - 5.5 * u, x + 1.6 * u, g], fill=(196, 150, 70, 240), outline=INK, width=3)
    d.polygon([(x - 0.4 * u, g - 4.8 * u), (x + 1.5 * u, g - 5.6 * u), (x + 1.8 * u, g - 0.2 * u), (x + 0.8 * u, g)], fill=(196, 150, 70, 240), outline=INK, width=3)
    d.ellipse([x + 0.2 * u, g - 8.4 * u, x + 2.4 * u, g - 5.6 * u], fill=(196, 150, 70, 240), outline=INK, width=3)
    for ex in (0.5, 1.6):
        d.polygon([(x + ex * u, g - 7.8 * u), (x + (ex + 0.3) * u, g - 9.6 * u), (x + (ex + 0.7) * u, g - 7.9 * u)], fill=(196, 150, 70, 240), outline=INK, width=3)
    d.ellipse([x + 1.6 * u, g - 7.4 * u, x + 2.0 * u, g - 7.0 * u], fill=PAINT["green"], outline=INK, width=1)
    for k in range(3):
        d.line([(x - 1.5 * u + k * 0.5 * u, g - 4.6 * u), (x - 1.2 * u + k * 0.5 * u, g - 3.2 * u)], fill=INK, width=3)
    stroke(d, [(x - 2.4 * u, g - 0.4 * u), (x - 3.6 * u, g - 0.6 * u), (x - 4.2 * u, g - 2.0 * u)], 0.7 * u, 0.5 * u, INK)
    limb(d, [(x + 1.2 * u, g - 4.6 * u), (x + 2.6 * u, g - 4.2 * u)], int(0.6 * u), (196, 150, 70, 240))
    knife(d, x + 2.8 * u, g - 4.3 * u, 4.2 * u, 0.55)


def apep_bound():
    """Apep, the serpent of chaos, pinned to the ground by knives; the cat of Ra cuts off his head."""
    img = layer()
    d = ImageDraw.Draw(img)
    scene_panel(d, 140, 400)
    g = 384
    body = [(28 + 360 * t, g - 44 - 30 * math.sin(t * 2.5 * math.pi) * (1 - 0.3 * t)) for t in (i / 40 for i in range(41))]
    serpent(d, body, 26, (150, 96, 50, 240))
    hx, hy = body[-1]
    d.polygon([(hx - 8, hy - 20), (hx + 40, hy - 8), (hx + 44, hy + 10), (hx - 8, hy + 18)], fill=(150, 96, 50, 240), outline=INK, width=4)
    d.ellipse([hx + 14, hy - 10, hx + 22, hy - 2], fill=PAINT["white"], outline=INK, width=2)
    for k in (6, 15, 24, 33):  # the knives through the coils
        x, y = body[k]
        knife(d, x + 4, y - 64, 84, math.pi / 2 + 0.1)
    cat(d, 430, g, 160)
    return weather(img, 32, 0.45)


def heart(d, x, y, s, colour):
    d.polygon([(x - s * 0.5, y - s * 0.7), (x + s * 0.5, y - s * 0.7), (x + s * 0.6, y), (x + s * 0.3, y + s * 0.7), (x - s * 0.3, y + s * 0.7), (x - s * 0.6, y)],
              fill=colour, outline=INK, width=3)
    d.rectangle([x - s * 0.3, y - s, x + s * 0.3, y - s * 0.7], fill=colour, outline=INK, width=2)


def ammit(d, x, g, h):
    """The devourer, crouched: crocodile head, lion's fore part, hippo's hind part."""
    u = h / 10
    d.ellipse([x - 4.2 * u, g - 5.4 * u, x - 0.8 * u, g - 0.4 * u], fill=(170, 116, 90, 240), outline=INK, width=3)  # hippo
    d.ellipse([x - 1.6 * u, g - 6.0 * u, x + 1.8 * u, g - 1.4 * u], fill=(206, 160, 70, 240), outline=INK, width=3)  # lion
    for k in range(5):
        d.ellipse([x - 1.2 * u + k * 0.6 * u, g - 4.8 * u + (k % 2) * 0.8 * u, x - 0.9 * u + k * 0.6 * u, g - 4.5 * u + (k % 2) * 0.8 * u], fill=INK)
    limb(d, [(x + 0.8 * u, g - 2.0 * u), (x + 1.2 * u, g - 0.2 * u), (x + 2.0 * u, g - 0.2 * u)], int(0.7 * u), (206, 160, 70, 240))
    limb(d, [(x - 3.2 * u, g - 1.0 * u), (x - 3.0 * u, g - 0.2 * u), (x - 2.2 * u, g - 0.2 * u)], int(0.8 * u), (170, 116, 90, 240))
    d.polygon([(x + 0.6 * u, g - 7.6 * u), (x + 1.8 * u, g - 7.8 * u), (x + 5.0 * u, g - 6.4 * u), (x + 5.0 * u, g - 5.6 * u), (x + 1.6 * u, g - 5.0 * u),
               (x + 0.4 * u, g - 5.0 * u)], fill=PAINT["green"], outline=INK, width=3)  # crocodile
    d.line([(x + 2.0 * u, g - 6.0 * u), (x + 5.0 * u, g - 6.0 * u)], fill=INK, width=2)
    for k in range(5):
        tx = x + 2.2 * u + k * 0.55 * u
        d.polygon([(tx, g - 6.0 * u), (tx + 0.25 * u, g - 5.6 * u), (tx + 0.5 * u, g - 6.0 * u)], fill=PAINT["white"])
    d.ellipse([x + 1.4 * u, g - 7.4 * u, x + 1.9 * u, g - 6.9 * u], fill=PAINT["gold"], outline=INK, width=1)
    d.polygon([(x - 0.4 * u, g - 7.8 * u), (x + 1.0 * u, g - 7.8 * u), (x + 0.8 * u, g - 4.6 * u), (x - 0.6 * u, g - 4.8 * u)], fill=WIG, outline=INK, width=2)


def weighing():
    """The weighing of the heart: Anubis steadies the balance, heart against feather, Thoth writes, Ammit waits."""
    img = layer()
    d = ImageDraw.Draw(img)
    scene_panel(d, 110, 410)
    g = 392
    bx, top = 210, 160
    d.rectangle([bx - 8, top, bx + 8, g], fill=PAINT["gold"], outline=INK, width=3)
    d.polygon([(bx - 30, g), (bx + 30, g), (bx + 16, g - 20), (bx - 16, g - 20)], fill=PAINT["gold"], outline=INK, width=3)
    d.rectangle([bx - 110, top + 8, bx + 110, top + 18], fill=PAINT["gold"], outline=INK, width=3)
    d.ellipse([bx - 14, top - 18, bx + 14, top + 10], fill=PAINT["green"], outline=INK, width=3)  # Maat's head on the post
    for s in (-1, 1):
        px = bx + s * 100
        for dx in (-26, 26):
            d.line([(px, top + 18), (px + dx, top + 90)], fill=INK, width=2)
        d.chord([px - 34, top + 74, px + 34, top + 106], 0, 180, fill=PAINT["gold"], outline=INK, width=3)
    heart(d, bx - 100, top + 66, 18, PAINT["red"])
    g_feather(d, bx + 74, top + 6, 52, 84, (244, 240, 226, 245))
    d.line([(bx, top + 18), (bx, g - 60)], fill=INK, width=2)  # the plumb line
    figure(d, 70, g, 170, "jackal", PAINT["black"], ([(0, 0), (0.6, 2.0), (1.4, 3.2)], [(0, 0), (1.6, 1.0), (3.2, 0.6)]), stride=0.7)
    ibis = layer()
    figure(ImageDraw.Draw(ibis), 330, g, 170, "ibis", SKIN, ([(0, 0), (0.8, 1.8), (1.8, 2.2)], [(0, 0), (1.4, 1.2), (2.6, 0.4)]), stride=0.7)
    dd = ImageDraw.Draw(ibis)
    dd.rectangle([352, 256, 362, 300], fill=PAINT["white"], outline=INK, width=2)  # his palette
    ibis = flipped(ibis, 330)  # Thoth faces the scale
    img = Image.alpha_composite(img, ibis)
    ammit(ImageDraw.Draw(img), 418, g, 90)
    return weather(img, 33, 0.45)


def nut():
    """Nut, the sky, arched over the world with stars on her body; the sun goes into her mouth and is born at her hips."""
    img = layer()
    d = ImageDraw.Draw(img)
    g = 440

    def arch(rx, ry, n=40):
        """A boxy arch over the ground line g: Nut's back (outer) or belly (inner), from right to left."""
        pts = []
        for i in range(n + 1):
            a = math.pi * i / n
            c, s_ = math.cos(a), math.sin(a)
            pts.append((256 + rx * math.copysign(abs(c) ** 0.35, c), g - ry * s_ ** 0.35))
        return pts

    outer, inner = arch(226, 330), arch(184, 280)
    d.polygon(outer + list(reversed(inner)), fill=NIGHT, outline=INK, width=4)
    rng = random.Random(34)
    for _ in range(40):  # stars, five-pointed, along her body
        k = rng.randrange(len(outer))
        t = rng.uniform(0.2, 0.8)
        x = outer[k][0] * (1 - t) + inner[k][0] * t
        y = outer[k][1] * (1 - t) + inner[k][1] * t
        if y > g - 24:
            continue
        r = 6
        pts = [(x + r * (1 if j % 2 == 0 else 0.4) * math.cos(-math.pi / 2 + j * math.pi / 5),
                y + r * (1 if j % 2 == 0 else 0.4) * math.sin(-math.pi / 2 + j * math.pi / 5)) for j in range(10)]
        d.polygon(pts, fill=PAINT["gold"])
    # her hands and feet on the earth
    for x in (52, 460):
        d.ellipse([x - 22, g - 10, x + 22, g + 6], fill=NIGHT, outline=INK, width=3)
    # her head at the west (left) end, by her arms, looking in toward the world
    hx, hy = 98, 360
    d.polygon([(hx - 30, hy - 30), (hx + 6, hy - 34), (hx + 2, hy + 34), (hx - 30, hy + 30)], fill=WIG, outline=INK, width=3)
    d.ellipse([hx - 14, hy - 24, hx + 26, hy + 18], fill=(196, 140, 90, 240), outline=INK, width=3)
    d.ellipse([hx + 12, hy - 10, hx + 18, hy - 4], fill=INK)
    sun_disc(d, hx + 46, hy + 2, 15)  # the evening sun, swallowed
    sun_disc(d, 412, 150, 18)  # the morning sun, born again at her hips
    d.line([(20, g + 8), (S - 20, g + 8)], fill=PAINT["green"], width=10)  # the earth
    d.line([(20, g + 8), (S - 20, g + 8)], fill=INK, width=2)
    return weather(img, 34, 0.4)


def baboon(d, x, g, h):
    """A seated baboon facing right, the forearms raised in praise."""
    u = h / 10
    grey = (128, 136, 110, 240)
    d.ellipse([x - 2.6 * u, g - 5.2 * u, x + 1.2 * u, g], fill=grey, outline=INK, width=3)  # rump and back
    d.ellipse([x - 1.6 * u, g - 8.4 * u, x + 1.4 * u, g - 4.0 * u], fill=(170, 176, 150, 240), outline=INK, width=3)  # the mane
    d.polygon([(x + 0.6 * u, g - 7.6 * u), (x + 2.6 * u, g - 7.0 * u), (x + 2.6 * u, g - 6.0 * u), (x + 0.8 * u, g - 5.6 * u)], fill=(200, 120, 110, 240), outline=INK, width=3)
    d.ellipse([x + 0.9 * u, g - 7.6 * u, x + 1.3 * u, g - 7.2 * u], fill=INK)
    limb(d, [(x + 0.4 * u, g - 4.6 * u), (x + 1.8 * u, g - 4.0 * u), (x + 2.6 * u, g - 6.4 * u)], int(0.55 * u), grey)
    limb(d, [(x + 1.0 * u, g - 0.6 * u), (x + 1.8 * u, g - 0.3 * u)], int(0.6 * u), grey)
    stroke(d, [(x - 2.4 * u, g - 0.8 * u), (x - 3.2 * u, g - 0.2 * u)], 0.5 * u, 0.4 * u, INK)


def baboons():
    """Thoth's baboons greeting the rising sun."""
    img = layer()
    d = ImageDraw.Draw(img)
    scene_panel(d, 150, 400)
    g = 384
    for k, x in enumerate((70, 190, 310)):
        baboon(d, x, g - (10 if k == 1 else 0), 130)
    sun_disc(d, 440, 250, 44)
    for k in range(9):  # rays toward the baboons
        a = math.pi * (0.6 + 0.08 * k)
        d.line([(440 + 54 * math.cos(a), 250 + 54 * math.sin(a)), (440 + 80 * math.cos(a), 250 + 80 * math.sin(a))], fill=PAINT["gold"], width=4)
    return weather(img, 35, 0.45)


def lion(d, x, g, h):
    """A couchant lion facing right."""
    u = h / 10
    tawny = (206, 160, 70, 240)
    d.rounded_rectangle([x - 6 * u, g - 3.6 * u, x + 1.5 * u, g], radius=int(1.6 * u), fill=tawny, outline=INK, width=3)
    d.ellipse([x - 0.6 * u, g - 7.6 * u, x + 3.0 * u, g - 2.6 * u], fill=(170, 100, 40, 240), outline=INK, width=3)  # mane
    d.polygon([(x + 1.6 * u, g - 7.0 * u), (x + 3.8 * u, g - 6.2 * u), (x + 3.8 * u, g - 4.8 * u), (x + 1.6 * u, g - 4.2 * u)], fill=tawny, outline=INK, width=3)
    d.ellipse([x + 2.6 * u, g - 6.4 * u, x + 3.0 * u, g - 6.0 * u], fill=INK)
    limb(d, [(x + 1.0 * u, g - 0.6 * u), (x + 4.0 * u, g - 0.5 * u)], int(0.7 * u), tawny)
    stroke(d, [(x - 5.8 * u, g - 2.0 * u), (x - 7.0 * u, g - 3.6 * u), (x - 6.6 * u, g - 4.6 * u)], 0.5 * u, 0.4 * u, INK)


def aker():
    """Aker, the two lions back to back guarding the horizon, the sun between them."""
    img = layer()
    d = ImageDraw.Draw(img)
    scene_panel(d, 170, 400)
    g = 384
    right = layer()
    lion(ImageDraw.Draw(right), 360, g, 100)
    img = Image.alpha_composite(img, right)
    img = Image.alpha_composite(img, flipped(right, S / 2))
    d = ImageDraw.Draw(img)
    d.chord([200, g - 60, 256, g + 60], 180, 360, fill=PAINT["gold"], outline=INK, width=3)  # the horizon's two hills
    d.chord([256, g - 60, 312, g + 60], 180, 360, fill=PAINT["gold"], outline=INK, width=3)
    sun_disc(d, 256, g - 58, 30)
    return weather(img, 36, 0.45)


# Order = atlas cell = DECAL_DEFS in src/level/decor.h.
DECALS = [crack_a, crack_b, crack_floor, lambda: vines(4, 4, 34, False), lambda: vines(5, 3, 26, True), roots, stain, glyph_column, glyph_row, cartouche,
          wedjat, winged_sun, papyrus, dry_grass, creeper, moss, night_barque, apep_bound, weighing, nut, baboons, aker]


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
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "..", "media", "textures", "decorations", "decals.png"))
