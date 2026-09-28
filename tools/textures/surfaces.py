"""Generate the dungeon surface textures in textures/dungeon/ (RGB, 512 x 512 each).

    python3 tools/textures/surfaces.py [out_dir] [--preview preview.png]

Order and file names must match the *_STYLE_NAMES tables in src/world/decor.h.
Image top = top of a wall, back edge (against the back wall) of a floor or ceiling.

Seams: the variants of one kind share their base (block layout, noise fields, frieze, sand line), and
every variant's own detail fades out near the edges, so any two variants tile side by side and above
each other. All noise is periodic (FFT synthesis), so a texture also tiles with itself. The one
exception is wall_plaster_broken: plaster on the left, bare stone (as wall_stone) on the right, the
transition from a painted wall to a stone one (mirrored when the stone is on the left).

Relief is baked in as shading from the upper left; the walls are flat quads.
"""

import math
import os
import random
import sys
from functools import lru_cache

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

N = 512
EDGE = 56  # px from an edge where a variant's own detail has faded out

Y = np.arange(N, dtype=np.float32)[:, None]
X = np.arange(N, dtype=np.float32)[None, :]

LIME = np.array([0.76, 0.67, 0.51])
LIME_LIGHT = np.array([0.86, 0.78, 0.61])
MORTAR = np.array([0.48, 0.40, 0.29])
SAND = np.array([0.85, 0.74, 0.54])
SAND_DARK = np.array([0.72, 0.60, 0.42])
PLASTER = np.array([0.88, 0.80, 0.63])
SANDSTONE = np.array([0.62, 0.47, 0.32])
SANDSTONE_LIGHT = np.array([0.78, 0.62, 0.43])
SLAB = np.array([0.72, 0.65, 0.53])

INK = np.array([0.13, 0.10, 0.08])
RED = np.array([0.62, 0.26, 0.15])
BLUE = np.array([0.20, 0.36, 0.58])
GREEN = np.array([0.23, 0.47, 0.36])
YELLOW = np.array([0.82, 0.63, 0.25])
CREAM = np.array([0.90, 0.85, 0.72])
DADO = np.array([0.42, 0.18, 0.12])
NIGHT = np.array([0.15, 0.21, 0.40])


# ---------------------------------------------------------------- helpers


@lru_cache(maxsize=None)
def noise(seed, p=1.0, sx=1.0, sy=1.0, lo=1.0, hi=N / 2):
    """Periodic noise, mean 0, std 1. Amplitude ~ f^-p: 0.5 grainy, 1 cloudy, 1.6 smooth.
    sx / sy > 1 squash the x / y frequencies: features stretched along that axis."""
    rng = np.random.default_rng(seed)
    f = np.fft.fft2(rng.standard_normal((N, N)))
    fy = np.fft.fftfreq(N)[:, None] * N
    fx = np.fft.fftfreq(N)[None, :] * N
    r = np.sqrt((fx * sx) ** 2 + (fy * sy) ** 2)
    r[0, 0] = 1
    amp = np.where((r < lo) | (r > hi), 0.0, r ** -p)
    out = np.real(np.fft.ifft2(f * amp))
    out -= out.mean()
    return (out / out.std()).astype(np.float32)


def smoothstep(e0, e1, v):
    t = np.clip((v - e0) / (e1 - e0), 0, 1)
    return t * t * (3 - 2 * t)


def lerp(a, b, t):
    t = np.asarray(t)
    if t.ndim == 2:
        t = t[..., None]
    return a + (b - a) * t


def edge_fade(u=True, v=True):
    """1 inside, 0 at the edges a variant has to match its neighbours on."""
    f = np.ones((N, N), np.float32)
    if u:
        f = f * smoothstep(0, EDGE, X) * smoothstep(0, EDGE, N - 1 - X)
    if v:
        f = f * smoothstep(0, EDGE, Y) * smoothstep(0, EDGE, N - 1 - Y)
    return f


def soften(a, sigma):
    """Periodic Gaussian blur."""
    fy = np.fft.fftfreq(N)[:, None]
    fx = np.fft.fftfreq(N)[None, :]
    k = np.exp(-2 * (np.pi * sigma) ** 2 * (fx * fx + fy * fy))
    return np.real(np.fft.ifft2(np.fft.fft2(a) * k)).astype(np.float32)


def relief(h, strength):
    """Lambert shading of height field h (periodic) lit from the upper left, 1 on flat ground.
    The height is blurred first: grain belongs in the colour, shaded it turns into noise."""
    h = soften(h, 1.2)
    gx = (np.roll(h, -1, 1) - np.roll(h, 1, 1)) * 0.5 * strength
    gy = (np.roll(h, -1, 0) - np.roll(h, 1, 0)) * 0.5 * strength
    light = np.array([-0.42, -0.58, 0.70])
    light /= np.linalg.norm(light)
    d = (-gx * light[0] - gy * light[1] + light[2]) / np.sqrt(gx * gx + gy * gy + 1)
    return np.clip(d / light[2], 0.35, 1.35)


def blur(a, radius):
    img = Image.fromarray(np.clip(a * 255, 0, 255).astype(np.uint8))
    return np.asarray(img.filter(ImageFilter.GaussianBlur(radius)), np.float32) / 255


def speckle(seed, density):
    """Sparse dark / light grains, -1..1."""
    rng = np.random.default_rng(seed)
    s = rng.random((N, N))
    return np.where(s < density, -1.0, np.where(s > 1 - density, 1.0, 0.0)).astype(np.float32)


def walk(rng, start, ang, length, width, step=6, wander=0.4, branch=0.1, depth=0, out=None):
    """Random-walk polylines for cracks and veins. Returns [(points, w0, w1)]."""
    out = [] if out is None else out
    pts = [start]
    x, y = start
    steps = max(2, int(length / step))
    for i in range(steps):
        ang += rng.uniform(-wander, wander)
        x += math.cos(ang) * step
        y += math.sin(ang) * step
        pts.append((x, y))
        if depth < 2 and i > 2 and rng.random() < branch:
            walk(rng, (x, y), ang + rng.choice((-1, 1)) * rng.uniform(0.5, 1.0), length * (1 - i / steps) * 0.5,
                 width * 0.6, step, wander, branch * 0.7, depth + 1, out)
    out.append((pts, width, max(0.6, width * 0.3)))
    return out


def lines(paths):
    """Draw tapering polylines at 2x, return a 0..1 coverage mask."""
    img = Image.new("L", (N * 2, N * 2), 0)
    d = ImageDraw.Draw(img)
    for pts, w0, w1 in paths:
        n = len(pts) - 1
        for i in range(n):
            w = 2 * (w0 + (w1 - w0) * i / max(n, 1))
            a, b = (pts[i][0] * 2, pts[i][1] * 2), (pts[i + 1][0] * 2, pts[i + 1][1] * 2)
            d.line([a, b], fill=255, width=max(1, int(round(w))))
            d.ellipse([b[0] - w / 2, b[1] - w / 2, b[0] + w / 2, b[1] + w / 2], fill=255)
    return np.asarray(img.resize((N, N), Image.LANCZOS), np.float32) / 255


def cracks(seed, count, length, width):
    rng = random.Random(seed)
    paths = []
    for _ in range(count):
        start = (rng.uniform(EDGE * 2, N - EDGE * 2), rng.uniform(EDGE * 2, N - EDGE * 2))
        walk(rng, start, rng.uniform(0, 2 * math.pi), length, width, out=paths)
        walk(rng, start, rng.uniform(0, 2 * math.pi), length * 0.6, width * 0.8, out=paths)
    return lines(paths)


def finish(col, h, strength):
    return np.clip(col * relief(h, strength)[..., None], 0, 1)


# ---------------------------------------------------------------- block layouts


def layout(rows, seed):
    """rows: [(y0, y1, [joint x...])] covering 0..N. Joints wrap around x (period N).
    Returns (distance to the nearest joint, distance to the nearest horizontal joint, block tint)."""
    rng = np.random.default_rng(seed)
    dist = np.zeros((N, N), np.float32)
    dist_h = np.zeros((N, N), np.float32)
    tint = np.zeros((N, N, 3), np.float32)
    for y0, y1, joints in rows:
        js = np.array(sorted(joints), np.float32)
        yy = np.arange(y0, y1, dtype=np.float32)[:, None]
        dy = np.minimum(yy - y0, y1 - yy)
        d = np.abs(X[..., None] - js)
        dx = np.minimum(d, N - d).min(-1)
        k = np.searchsorted(js, X[0], side="right") % len(js)
        dist[y0:y1] = np.minimum(dy, dx)
        dist_h[y0:y1] = np.broadcast_to(dy, (y1 - y0, N))
        tints = 1 + rng.uniform(-0.07, 0.06, (len(js), 1)) + rng.uniform(-0.01, 0.01, (len(js), 3)) * [1.0, 0.6, -0.4]
        tint[y0:y1] = tints[k][None, :, :]
    return dist, dist_h, tint


# Courses of the wall blocks (image rows from the top) and the vertical joints in each.
WALL_ROWS = [(0, 96, [0, 250]), (96, 232, [120, 390]), (232, 372, [56, 300]), (372, 512, [190, 440])]
FLOOR_ROWS = [(0, 160, [70, 330]), (160, 340, [200, 455]), (340, 512, [24, 270])]
CEILING_ROWS = [(0, 512, [0, 170, 342])]


# ---------------------------------------------------------------- walls


def sand_line():
    """Height of the sand along the foot of a stone or rock wall, shared by every variant (px from the bottom)."""
    return 20 + 7 * noise(101, 1.4)[0][None, :]


def stone(extra_sand=None, chips=0, crack_count=0, joint_sand=0.0):
    dist, dist_h, tint = layout(WALL_ROWS, 102)
    mott = noise(103, 1.3)
    grain = noise(104, 0.4)
    fine = speckle(105, 0.02)
    d = dist + 1.5 * noise(106, 1.0)
    face = smoothstep(0.5, 7, d) ** 0.7
    col = lerp(LIME, LIME_LIGHT, np.clip(0.5 + 0.25 * mott, 0, 1)) * tint
    col *= (1 + 0.035 * grain + 0.05 * fine)[..., None]
    col = lerp(MORTAR, col, face)
    h = face + 0.07 * mott
    fade = edge_fade()

    if chips:
        rng = random.Random(107)
        for _ in range(chips):  # spalls at block corners, the fresh stone lighter
            y0, y1, js = rng.choice(WALL_ROWS[1:])
            cx = rng.choice(js) + rng.uniform(-12, 12)
            cy = rng.choice((y0, y1)) + rng.uniform(-10, 10)
            cx = min(max(cx, EDGE * 2), N - EDGE * 2)
            r = rng.uniform(16, 34)
            m = smoothstep(0, 2, r - np.hypot(X - cx, Y - cy) + 4 * noise(108, 1.1)) * fade
            h -= 0.45 * m
            col = lerp(col, col * 0.86, m)
    if crack_count:
        c = cracks(109, crack_count, 150, 4.5) * fade
        h -= 0.5 * c
        col *= (1 - 0.45 * c)[..., None]

    # Sand in the foot of the wall and, blown in, on the horizontal joints.
    line = sand_line() + (0 if extra_sand is None else extra_sand)
    sand = smoothstep(-2, 2, Y - (N - line) + 4 * noise(110, 0.8))
    if joint_sand:
        sand = np.maximum(sand, joint_sand * smoothstep(5, 2, dist_h + 2 * noise(111, 0.7)) * smoothstep(-0.2, 0.6, noise(112, 1.2)) * fade)
    sand_col = lerp(SAND_DARK, SAND, np.clip(0.55 + 0.25 * noise(113, 0.6), 0, 1))
    col = lerp(col, sand_col, sand)
    h = lerp(h[..., None], (0.9 + 0.1 * noise(114, 1.2))[..., None], sand)[..., 0]
    return col, h


def frieze():
    """Paint of the plastered walls: (colour, coverage)."""
    col = np.zeros((N, N, 3), np.float32)
    a = np.zeros((N, N), np.float32)

    def band(y0, y1, c):
        col[y0:y1] = c
        a[y0:y1] = 1

    band(14, 18, INK)
    band(18, 28, RED)
    band(28, 31, INK)
    # Block border: coloured panels between cream / black / cream stripes, 32 px each.
    y0, y1 = 31, 70
    colours = [BLUE, RED, GREEN, YELLOW]
    for k in range(N // 32):
        x0 = k * 32
        col[y0:y1, x0:x0 + 32] = colours[k % 4]
        col[y0:y1, x0:x0 + 3] = CREAM
        col[y0:y1, x0 + 3:x0 + 5] = INK
        col[y0:y1, x0 + 5:x0 + 7] = CREAM
    a[y0:y1] = 1
    band(70, 73, INK)
    band(73, 80, YELLOW)
    band(80, 82, INK)
    # Dado at the foot of the wall.
    band(444, 447, INK)
    band(447, 453, YELLOW)
    band(453, 455, INK)
    band(455, N, DADO)
    col[455:] *= (1 + 0.08 * noise(201, 1.2)[455:])[..., None]
    return col, a


def plaster(worn=False):
    mott = noise(202, 1.5)
    grain = noise(203, 0.5)
    col = PLASTER * (1 + 0.035 * mott + 0.02 * grain)[..., None]
    h = 1.35 + 0.035 * noise(204, 1.8)
    paint, cover = frieze()
    loss = noise(205, 1.1)
    fade = edge_fade()
    cover = cover * (1 - smoothstep(1.2, 1.5, loss))
    if worn:
        cover *= 1 - smoothstep(0.3, 0.9, noise(206, 1.2)) * fade * 0.8
    col = lerp(col, lerp(paint, PLASTER, 0.12), cover * 0.94)
    holes = np.zeros((N, N), np.float32)
    if worn:  # water stains running down from the top, flaked patches, cracks
        streak = smoothstep(0.2, 1.6, noise(207, 1.1, sy=6)) * smoothstep(420, 60, Y) * fade
        col *= (1 - 0.22 * streak)[..., None]
        col = lerp(col, col * np.array([0.92, 0.9, 0.96]), smoothstep(0.8, 1.8, noise(208, 1.4)) * fade)
        holes = smoothstep(1.55, 1.75, noise(209, 1.25)) * fade
        c = cracks(210, 2, 170, 2.6) * fade
        h -= 0.3 * c
        col *= (1 - 0.5 * c)[..., None]
    return col, h, holes


def plastered(mask):
    """Plaster over the stone where mask is 1, AO on the stone round its edge."""
    s_col, s_h = stone()
    p_col, p_h, holes = plaster(worn=mask is not None)
    m = np.ones((N, N), np.float32) if mask is None else mask
    m = m * (1 - holes)
    ao = np.clip(blur(m, 5) - m, 0, 1)
    s_col = s_col * (1 - 0.9 * ao)[..., None]
    # Shaded apart, each is periodic; the broken edge is not, so its lip gets gradients that do not wrap.
    gy, gx = np.gradient(blur(m, 1.2) * 6)
    lip = np.clip(1 + 0.42 * gx + 0.58 * gy, 0.4, 1.6)
    return np.clip(lerp(finish(s_col, s_h, 5.0), finish(p_col, p_h, 5.0), m) * lip[..., None], 0, 1)


def wall_plaster():
    return plastered(None)


def wall_plaster_worn():
    return plastered(np.ones((N, N), np.float32))


def wall_plaster_broken():
    """Plaster broken off towards the right edge, bare stone (as wall_stone) at it."""
    line = 250 + 45 * noise(211, 1.6)[:, :1] + 14 * noise(212, 0.9)[:, :1]
    line = np.clip(line, 150, 330)
    m = smoothstep(-1.2, 1.2, line - X + 7 * noise(213, 0.8))
    m *= 1 - smoothstep(1.3, 1.6, noise(214, 1.1)) * smoothstep(0, 40, line - X) * edge_fade()  # a few islands broken out
    return plastered(m)


def wall_stone():
    col, h = stone()
    return finish(col, h, 5.0)


def wall_stone_cracked():
    col, h = stone(chips=3, crack_count=1)
    return finish(col, h, 5.0)


def wall_stone_sand():
    mound = 95 * np.sin(np.pi * X / N) ** 2 * (0.75 + 0.25 * noise(301, 1.8)[0][None, :])
    col, h = stone(extra_sand=mound * edge_fade(v=False)[0][None, :], joint_sand=0.9)
    return finish(col, h, 5.0)


def rough(strata_amount, vein=False, dark=1.0):
    big = noise(401, 1.7)
    mid = noise(402, 1.1)
    fine = noise(403, 0.5)
    strata = noise(404, 1.3, sx=9)
    fade = edge_fade()
    amount = 0.35 + (strata_amount - 0.35) * fade
    # Chisel marks: patches of narrow parallel grooves in two directions.
    g1 = (0.5 + 0.5 * np.sin(2 * np.pi * (26 * X + 38 * Y) / N)) ** 6
    g2 = (0.5 + 0.5 * np.sin(2 * np.pi * (-34 * X + 30 * Y) / N)) ** 6
    p = noise(405, 1.8)
    chisel = g1 * smoothstep(0.3, 0.9, p) + g2 * smoothstep(-0.3, -0.9, p)
    h = 1.4 * big + 0.35 * mid - 0.12 * chisel + 0.3 * amount * strata
    col = lerp(SANDSTONE, SANDSTONE_LIGHT, np.clip(0.45 + 0.28 * amount * strata + 0.12 * mid + 0.1 * big, 0, 1))
    col *= (1 + 0.05 * fine + 0.05 * speckle(406, 0.015))[..., None] * dark
    if vein:  # a calcite vein, light and slightly proud of the rock
        v = lines(walk(random.Random(407), (EDGE * 2, N * 0.55), -0.25, 380, 6.0, wander=0.25, branch=0.06)) * fade
        col = lerp(col, np.array([0.88, 0.84, 0.74]) * dark, v * 0.85)
        h += 0.3 * v
    return col, h


def rough_wall(strata_amount, vein=False):
    col, h = rough(strata_amount, vein)
    sand = smoothstep(-2, 2, Y - (N - sand_line()) + 4 * noise(110, 0.8))
    col = lerp(col, lerp(SAND_DARK, SAND, np.clip(0.55 + 0.25 * noise(113, 0.6), 0, 1)), sand)
    h = lerp(h[..., None], np.full((N, N, 1), 0.2), sand)[..., 0]
    return finish(col, h, 4.0)


def wall_rough():
    return rough_wall(0.35)


def wall_rough_strata():
    return rough_wall(1.4, vein=True)


# ---------------------------------------------------------------- floors


def floor(extra_sand=0.0, crack_count=0, broken=False):
    dist, _, tint = layout(FLOOR_ROWS, 501)
    mott = noise(502, 1.3)
    grain = noise(503, 0.4)
    fade = edge_fade(v=False)
    face = smoothstep(0.5, 8, dist + 1.5 * noise(504, 1.0)) ** 0.6
    col = SLAB * tint * (1 + 0.06 * mott + 0.03 * grain + 0.04 * speckle(505, 0.02))[..., None]
    col = lerp(MORTAR * 0.9, col, face)
    h = face + 0.06 * mott
    if broken:  # a slab corner broken off and sunk
        m = smoothstep(0, 4, 42 - np.hypot(X - 330, Y - 340) + 10 * noise(506, 0.9)) * fade
        h -= 0.6 * m
        col *= (1 - 0.18 * m)[..., None]
    if crack_count:
        c = cracks(507, crack_count, 190, 4.5) * fade
        h -= 0.5 * c
        col *= (1 - 0.45 * c)[..., None]
    # Sand: in the joints, drifted against the back wall, in patches; more or less of it per variant.
    s = 0.9 * noise(508, 1.4) + 2.6 * (1 - Y / N) ** 3 + 1.6 * smoothstep(9, 1, dist) + extra_sand * fade - 1.0
    sand = smoothstep(-0.6, 0.6, s)
    sand_col = lerp(SAND_DARK, SAND, np.clip(0.6 + 0.2 * noise(509, 0.7) + 0.12 * noise(510, 1.5), 0, 1))
    sand_col *= (1 + 0.05 * speckle(511, 0.03))[..., None]
    col = lerp(col, sand_col, sand)
    h = lerp(h[..., None], (0.95 + 0.05 * noise(512, 1.5))[..., None], sand)[..., 0]
    return finish(col, h, 5.0)


def floor_slabs():
    return floor()


def floor_sand():
    return floor(extra_sand=1.6)


def floor_cracked():
    return floor(extra_sand=-0.4, crack_count=2, broken=True)


# ---------------------------------------------------------------- ceilings and rock


def star(d, cx, cy, r):
    pts = []
    for k in range(10):
        a = -math.pi / 2 + k * math.pi / 5
        rr = r if k % 2 == 0 else r * 0.42
        pts.append((cx + rr * math.cos(a), cy + rr * math.sin(a)))
    d.polygon(pts, fill=255)


def ceiling_stars():
    """Night sky of the tomb ceilings: yellow stars on blue plaster."""
    img = Image.new("L", (N * 2, N * 2), 0)
    d = ImageDraw.Draw(img)
    for r in range(8):
        for c in range(8):
            for dx in (-N, 0, N):  # draw the wrapped copies so the grid tiles
                star(d, 2 * (c * 64 + 32 + (32 if r % 2 else 0) + dx), 2 * (r * 64 + 32), 30)
    stars = np.asarray(img.resize((N, N), Image.LANCZOS), np.float32) / 255
    mott = noise(601, 1.4)
    col = NIGHT * (1 + 0.08 * mott + 0.03 * noise(602, 0.5))[..., None]
    col = lerp(col, YELLOW, stars * (1 - smoothstep(0.9, 1.3, noise(603, 1.0))))
    holes = smoothstep(1.6, 1.8, noise(604, 1.3))
    s_col, s_h = stone()
    h = lerp(np.full((N, N, 1), 1.3), s_h[..., None], holes)[..., 0]
    col = lerp(col, s_col * 0.85, holes)
    return finish(col, h, 5.0)


def ceiling_slabs():
    dist, _, tint = layout(CEILING_ROWS, 701)
    mott = noise(702, 1.3)
    face = smoothstep(0.5, 9, dist + 2 * noise(703, 1.0)) ** 0.7
    col = lerp(LIME, LIME_LIGHT, np.clip(0.4 + 0.25 * mott, 0, 1)) * tint * 0.85
    col = lerp(MORTAR * 0.7, col, face)
    col *= (1 + 0.035 * noise(704, 0.4) + 0.04 * speckle(705, 0.02))[..., None]
    h = face + 0.08 * mott
    return finish(col, h, 5.0)


def ceiling_rough():
    col, h = rough(0.5, dark=0.8)
    return finish(col, h, 4.0)


def rock():
    """Face of the solid rock between the rooms: dark, heavy relief. Tiles over 2 x 2 cells."""
    big = noise(801, 1.8)
    mid = noise(802, 1.1)
    strata = noise(803, 1.3, sx=8)
    fine = noise(804, 0.5)
    cr = smoothstep(0.93, 1.0, 1 - np.abs(noise(805, 1.4)))  # thin fissures along the zero line of a noise
    h = 1.6 * big + 0.4 * mid + 0.3 * strata - 0.25 * cr
    col = lerp(np.array([0.22, 0.17, 0.12]), np.array([0.38, 0.30, 0.21]),
               np.clip(0.5 + 0.2 * strata + 0.15 * mid + 0.12 * big, 0, 1))
    col *= (1 + 0.05 * fine + 0.06 * speckle(806, 0.02) - 0.5 * cr)[..., None]
    return finish(col, h, 4.0)


# Order = WALL_STYLE_NAMES / FLOOR_STYLE_NAMES / CEILING_STYLE_NAMES in src/world/decor.h.
TEXTURES = [
    ("wall_plaster", wall_plaster),
    ("wall_plaster_worn", wall_plaster_worn),
    ("wall_plaster_broken", wall_plaster_broken),
    ("wall_stone", wall_stone),
    ("wall_stone_cracked", wall_stone_cracked),
    ("wall_stone_sand", wall_stone_sand),
    ("wall_rough", wall_rough),
    ("wall_rough_strata", wall_rough_strata),
    ("floor_slabs", floor_slabs),
    ("floor_sand", floor_sand),
    ("floor_cracked", floor_cracked),
    ("ceiling_stars", ceiling_stars),
    ("ceiling_slabs", ceiling_slabs),
    ("ceiling_rough", ceiling_rough),
    ("rock", rock),
]


def to_image(a):
    return Image.fromarray((np.clip(a, 0, 1) * 255 + 0.5).astype(np.uint8))


def main(out_dir, preview):
    images = {}
    for name, fn in TEXTURES:
        img = to_image(fn())
        img.save(os.path.join(out_dir, name + ".png"), optimize=True)
        images[name] = img
        print("wrote", name)
    if preview:  # every texture, then two strips of walls side by side to check the seams
        q = N // 2
        sheet = Image.new("RGB", (q * 8, q * 4), (0, 0, 0))
        for k, (name, _) in enumerate(TEXTURES):
            sheet.paste(images[name].resize((q, q)), ((k % 8) * q, (k // 8) * q))
        strip = ["wall_plaster", "wall_plaster_worn", "wall_plaster", "wall_plaster_broken", "wall_stone", "wall_stone_sand",
                 "wall_stone_cracked", "wall_stone"]
        for k, name in enumerate(strip):
            sheet.paste(images[name].resize((q, q)), (k * q, 2 * q))
        strip = ["wall_rough", "wall_rough_strata", "wall_rough", "floor_slabs", "floor_sand", "floor_cracked", "floor_slabs", "rock"]
        for k, name in enumerate(strip):
            sheet.paste(images[name].resize((q, q)), (k * q, 3 * q))
        sheet.save(preview)
        print("wrote", preview)


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    args = sys.argv[1:]
    preview_path = None
    if "--preview" in args:
        i = args.index("--preview")
        preview_path = args[i + 1]
        del args[i:i + 2]
    main(args[0] if args else os.path.join(here, "..", "..", "textures", "dungeon"), preview_path)
