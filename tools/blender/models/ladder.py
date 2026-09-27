"""Procedural ladder segments for Ladder cells: a lashed wooden pole ladder and a pair of twisted lianas.

    MCP:  p = ".../tools/blender/models/ladder.py"; g = {"__file__": p, "__name__": "ladder"}
          exec(open(p).read(), g); g["build"](bake=False)      # then g["export"](objs)
    CLI:  blender -b --python tools/blender/models/ladder.py -- [--export] [--review out.png [--view z,scale]] [--only wood_a,vine_top]

Same space as decor.py: Z up, 1 unit = 1 tile, origin = floor level, horizontal centre of the tile, on the back
wall plane, the ladder stands out towards -Y (the camera). Each file is one cell of a shaft; the engine picks a
piece per cell (Dungeon::scatterLadders): the bottom piece on the floor, the top piece in the highest cell, mid
pieces in between (wooden ones mirrored at random; a mirrored liana would kink at the seams, its helix turning
the other way). Every piece meets its neighbours with the same rails at z = 0 and
z = 1 (x = +-RAIL_X, y = RAIL_Y, same radius, whole helix turns for the lianas), so any piece stacks on any
other. Nothing but the rails crosses z = 0 or z = 1.

Climbing (for the animations to come): holds every 1 / RUNGS tile, the lowest at 0.5 / RUNGS, hands and feet
at y ~ -0.06.
"""

import importlib
import math
import os
import random
import sys

import bpy
from mathutils import Vector as V

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import common  # noqa: E402
import decor  # noqa: E402

importlib.reload(common)
importlib.reload(decor)
from common import REPO, Builder, ellipsoid, smoothstep, transform, tube  # noqa: E402
from decor import bake_lit, blob, box, curve, place, prism, rock, rod, rot  # noqa: E402

COLL = "ladder_new"
PIECES = ["wood_a", "wood_b", "wood_c", "wood_top", "wood_bottom", "vine_a", "vine_b", "vine_c", "vine_top", "vine_bottom"]
SPACING = 1.2  # pieces are spread along X in the scene (bake/review only; export is at the origin)

RUNGS = 12  # holds per tile
RAIL_X = 0.1  # rail centre lines at x = +-RAIL_X
RAIL_Y = -0.04  # rail centre, out from the back wall
RAIL_R = 0.013
VINE_TURNS = 2  # helix turns per tile of the twisted lianas (whole number: seamless)

COL = {
    "acacia": (0.46, 0.30, 0.15),
    "acacia_dark": (0.27, 0.16, 0.07),
    "acacia_new": (0.66, 0.50, 0.28),
    "end_grain": (0.62, 0.48, 0.28),
    "rope": (0.68, 0.55, 0.30),
    "rope_dark": (0.42, 0.31, 0.15),
    "sandstone": (0.55, 0.42, 0.25),
    "sandstone_dark": (0.36, 0.26, 0.14),
    "sand": (0.72, 0.56, 0.32),
    "liana": (0.42, 0.33, 0.17),
    "liana_dark": (0.22, 0.17, 0.08),
    "stem": (0.32, 0.38, 0.13),
    "leaf": (0.22, 0.44, 0.09),
    "leaf_dark": (0.13, 0.27, 0.06),
    "leaf_dry": (0.46, 0.38, 0.13),
    "root": (0.50, 0.39, 0.22),
    "root_dark": (0.30, 0.22, 0.11),
}


def materials():
    stripes = decor.stripes
    specs = {
        "wood": stripes("u", 1 / 7, [(3, "acacia"), (1, "acacia_dark")], "LINEAR"),
        "wood_new": stripes("u", 1 / 7, [(3, "acacia_new"), (1, "acacia")], "LINEAR"),
        "end_grain": ("solid", "end_grain"),
        "rope": stripes("v", 0.006, [(1, "rope"), (1, "rope_dark")], "LINEAR"),
        "stone": ("solid", "sandstone"),
        "stone_dark": ("solid", "sandstone_dark"),
        "sand": ("solid", "sand"),
        "liana": stripes("u", 1 / 5, [(2, "liana"), (1, "liana_dark")], "LINEAR"),
        "stem": ("solid", "stem"),
        "leaf": stripes("u", 1 / 2, [(1, "leaf"), (1, "leaf_dark")], "LINEAR"),
        "leaf_dry": stripes("u", 1 / 2, [(1, "leaf_dry"), (1, "liana")], "LINEAR"),
        "root": stripes("u", 1 / 4, [(2, "root"), (1, "root_dark")], "LINEAR"),
    }
    return {name: common.make_material("ladder_" + name, spec, COL) for name, spec in specs.items()}


def clip_wall(geo):
    """Keep floor patches out of the back wall."""
    return transform(geo, lambda v: V((v[0], min(v[1], -0.002), v[2])))


def envelope(z):
    """0 at both seams, 1 in the middle: interior wobble that keeps the seams matching."""
    return math.sin(math.pi * min(max(z, 0.0), 1.0))


# ---------------------------------------------------------------- wooden ladder


def rail_keys(side, z0, z1, seed, r=RAIL_R, steps=12):
    """Keys of a hand-hewn pole: slight bow and knobbly thickness between the seams."""
    rnd = random.Random(seed)
    bow, ph = rnd.uniform(-0.008, 0.008), rnd.uniform(0, 6.28)
    keys = []
    for k in range(steps + 1):
        z = z0 + (z1 - z0) * k / steps
        e = envelope(z)
        x = side * RAIL_X + bow * e + 0.003 * e * math.sin(7 * z + ph)
        rr = r * (1 + 0.08 * e * math.sin(2 * math.pi * 3 * z + ph))
        keys.append(((x, RAIL_Y + 0.002 * e * math.sin(5 * z + ph), z), rr, rr))
    return keys


def rail_x(keys, z):
    """x of the rail at height z (keys are evenly spaced in z)."""
    zs = [k[0][2] for k in keys]
    i = min(max(int((z - zs[0]) / (zs[1] - zs[0])), 0), len(keys) - 2)
    t = (z - zs[i]) / (zs[i + 1] - zs[i])
    return keys[i][0][0] * (1 - t) + keys[i + 1][0][0] * t


def add_rail(b, M, keys, caps=(False, False)):
    b.add(tube(keys, sub=1, n=9, caps=caps), M["wood"], "root")


def lashing(b, M, x, z, side):
    """Crossed rope wraps binding a rung to the rail at (x, z)."""
    c = V((x, RAIL_Y - 0.008, z))
    r = RAIL_R + 0.004
    for tilt in (40, -40):
        ring = [(c + rot(y=tilt * side) @ V((r * math.cos(a), r * 1.25 * math.sin(a), 0)), 0.0028, 0.0028)
                for a in (2 * math.pi * k / 8 for k in range(8))]
        b.add(tube(ring, sub=1, n=4, closed=True), M["rope"], "root")


def rung(b, M, xl, xr, z, seed, mat="wood", r=0.0082):
    """Rung lying on the front of both rails, poking out past them, cut ends showing."""
    rnd = random.Random(seed)
    dz = rnd.uniform(-0.004, 0.004)
    y = RAIL_Y - RAIL_R - r * 0.7
    a, e = V((xl - 0.018, y, z - dz)), V((xr + 0.018, y, z + dz))
    mid = (a + e) / 2 + V((0, 0, -0.002))
    b.add(tube([(a, r, r), (mid, r * 1.06, r * 1.06), (e, r, r)], sub=2, n=7, caps=(False, False)), M[mat], "root")
    for p, d in ((a, a - e), (e, e - a)):
        b.add(ellipsoid(p, (r, r, 0.0015), n=7, rings=3, axis=d), M["end_grain"], "root")
    lashing(b, M, xl, z - dz * 0.8, -1)
    lashing(b, M, xr, z + dz * 0.8, 1)


def broken_rung(b, M, xl, xr, z):
    """Snapped rung: a stub hangs from each lashing, splintered ends."""
    y = RAIL_Y - RAIL_R - 0.006
    for x, s, drop, length in ((xl, 1, 0.022, 0.07), (xr, -1, 0.05, 0.05)):
        a = V((x - s * 0.016, y, z))
        e = a + V((s * length, -0.004, -drop))
        b.add(tube([(a, 0.0082, 0.0082), (e, 0.0075, 0.0075)], sub=1, n=7, caps=(True, False)), M["wood"], "root")
        d = (e - a).normalized()
        for k in range(3):
            tip = e + d * (0.008 + 0.004 * k) + V((0, 0.003 * (k - 1), 0.003 * (k - 1)))
            b.add(tube([(e - d * 0.002 + V((0, 0.003 * (k - 1), 0.002 * (k - 1))), 0.003, 0.003), (tip, 0.0006, 0.0006)], sub=1, n=4),
                  M["end_grain"], "root")
        lashing(b, M, x, z, -s)


def rope_splice(b, M, keys, z0, z1):
    """Rope wound tightly round a cracked rail."""
    turns = 9
    path = []
    for k in range(turns * 6 + 1):
        t = k / (turns * 6)
        z = z0 + (z1 - z0) * t
        x = rail_x(keys, z)
        a = 2 * math.pi * turns * t
        path.append(((x + (RAIL_R + 0.002) * math.cos(a), RAIL_Y + (RAIL_R + 0.002) * math.sin(a), z), 0.0026, 0.0026))
    b.add(tube(path, sub=1, n=4), M["rope"], "root")


def dangling_rope(b, M, x, z, length, seed):
    rnd = random.Random(seed)
    pts = [V((x, RAIL_Y - RAIL_R - 0.006, z))]
    for k in range(1, 5):
        pts.append(pts[-1] + V((rnd.uniform(-0.006, 0.006), -0.002, -length / 4)))
    b.add(curve(pts, 0.0026, n=4, ref=V((0, 1, 0))), M["rope"], "root")
    b.add(ellipsoid(pts[-1], (0.004, 0.004, 0.005), n=5, rings=3), M["rope"], "root")


def rung_z(k, seed):
    return (k + 0.5) / RUNGS + random.Random(seed * 31 + k).uniform(-0.003, 0.003)


def wood_ladder(b, M, seed, z0=0.0, z1=1.0, skip=(), broken=(), new=(), caps=(False, False)):
    keys = [rail_keys(s, z0, z1, seed * 7 + i) for i, s in enumerate((-1, 1))]
    for k in keys:
        add_rail(b, M, k, caps)
    for k in range(RUNGS):
        z = rung_z(k, seed)
        if z < z0 + 0.02 or z > z1 - 0.02 or k in skip:
            continue
        xl, xr = rail_x(keys[0], z), rail_x(keys[1], z)
        if k in broken:
            broken_rung(b, M, xl, xr, z)
        else:
            rung(b, M, xl, xr, z, seed * 100 + k, "wood_new" if k in new else "wood")
    return keys


def build_wood_a(b, M):
    """Plain stretch: rungs lashed with crossed palm-fibre rope, one rope end left hanging."""
    keys = wood_ladder(b, M, 1)
    dangling_rope(b, M, rail_x(keys[1], rung_z(7, 1)) + 0.004, rung_z(7, 1) - 0.004, 0.05, 3)


def build_wood_b(b, M):
    """A snapped rung and a cracked rail bound with rope."""
    keys = wood_ladder(b, M, 2, broken=(5,))
    rope_splice(b, M, keys[0], 0.12, 0.2)


def build_wood_c(b, M):
    """A rung replaced with fresh wood, a knot on the rail, a wall peg holding the ladder off the wall."""
    keys = wood_ladder(b, M, 3, new=(8,))
    z = 0.3
    x = rail_x(keys[1], z)
    b.add(ellipsoid((x - 0.004, RAIL_Y - 0.01, z), (0.01, 0.008, 0.014), n=8, rings=4), M["wood"], "root")
    b.add(ellipsoid((x - 0.004, RAIL_Y - 0.017, z), (0.004, 0.002, 0.006), n=6, rings=3), M["end_grain"], "root")
    # Bronze-age fix: a wooden peg driven into the wall behind the left rail, rope tying the rail to it.
    z = 0.62
    x = rail_x(keys[0], z)
    b.add(rod((x, 0.02, z), (x, RAIL_Y + RAIL_R * 0.3, z), 0.006, n=6), M["wood"], "root")
    b.add(tube([(V((x + 0.017 * math.cos(a), -0.021 + 0.022 * math.sin(a), z + 0.004 * math.sin(2 * a))), 0.0026, 0.0026)
                for a in (2 * math.pi * k / 10 for k in range(10))], sub=1, n=4, closed=True), M["rope"], "root")


WOOD_BEAM_Z = 0.9


def build_wood_top(b, M):
    """Highest cell: the rails end just over a beam wedged between the side walls and are roped to it."""
    rail_top = WOOD_BEAM_Z + 0.05
    keys = wood_ladder(b, M, 4, z1=rail_top, caps=(False, True))
    for k in keys:
        top = V(k[-1][0])
        b.add(ellipsoid(top, (RAIL_R * 0.95, RAIL_R * 0.95, 0.002), n=9, rings=3), M["end_grain"], "root")
    # Beam: squared-off log from wall to wall, resting in notches cut into the side walls.
    beam = [((x, RAIL_Y + 0.02, WOOD_BEAM_Z + 0.002 * math.sin(9 * x)), 0.022, 0.018) for x in (-0.53, -0.3, 0.0, 0.3, 0.53)]
    b.add(tube(beam, sub=2, n=8, ref=V((0, 0, 1)), shape=lambda th, a: 1 + 0.08 * math.cos(4 * th), caps=(False, False)), M["wood"], "root")
    for k in keys:
        x = rail_x(k, WOOD_BEAM_Z)
        for dx in (-0.01, 0.0, 0.01):
            ring = [(V((x + dx + 0.006 * math.sin(a), RAIL_Y + 0.004 + 0.036 * math.cos(a), WOOD_BEAM_Z + 0.03 * math.sin(a))), 0.0026, 0.0026)
                    for a in (2 * math.pi * j / 10 for j in range(10))]
            b.add(tube(ring, sub=1, n=4, closed=True), M["rope"], "root")
    dangling_rope(b, M, rail_x(keys[0], WOOD_BEAM_Z) - 0.012, WOOD_BEAM_Z - 0.03, 0.07, 9)


def build_wood_bottom(b, M):
    """On the floor: rail feet set on flat stones, a rung fallen at the foot."""
    keys = wood_ladder(b, M, 5, z0=0.01, skip=(0,), caps=(True, False))
    for i, k in enumerate(keys):
        x = k[0][0][0]
        b.add(place(prism(blob(0.035, 40 + i, n=7, jitter=0.25), 0.012), rot(z=20 * i), (x, RAIL_Y - 0.004, 0.0)), M["stone" if i else "stone_dark"], "root")
    for c, r, seed in (((-0.17, -0.07, 0.01), (0.022, 0.018, 0.014), 51), ((0.18, -0.1, 0.008), (0.016, 0.014, 0.011), 52),
                       ((0.03, -0.14, 0.005), (0.01, 0.009, 0.008), 53)):
        b.add(rock(c, r, seed), M["stone"], "root")
    b.add(clip_wall(place(prism(blob(0.2, 55, n=9, jitter=0.2), 0.003), rot(z=10), (0.0, -0.08, 0.0))), M["sand"], "root")
    fallen = rot(z=-18)
    a, e = fallen @ V((-0.11, 0, 0)), fallen @ V((0.11, 0, 0))
    base = V((-0.02, -0.15, 0.0082))
    b.add(rod(base + a, base + e, 0.0082, n=7, caps=(False, False)), M["wood"], "root")
    for p, d in ((base + a, a - e), (base + e, e - a)):
        b.add(ellipsoid(p, (0.0082, 0.0082, 0.0015), n=7, rings=3, axis=d), M["end_grain"], "root")


# ---------------------------------------------------------------- lianas


def strand_path(side, z0, z1, phase, seed, r_helix=0.008, steps=32):
    """One strand of a liana: a helix round the rail line (whole turns per tile) plus the rail's interior sway."""
    rnd = random.Random(seed)
    sway, ph = rnd.uniform(-0.02, 0.02), rnd.uniform(0, 6.28)
    pts = []
    for k in range(steps + 1):
        z = z0 + (z1 - z0) * k / steps
        e = envelope(z)
        a = 2 * math.pi * VINE_TURNS * z + phase
        cx = side * RAIL_X + sway * e + 0.006 * e * math.sin(4 * z + ph)
        pts.append(V((cx + r_helix * math.cos(a), RAIL_Y + r_helix * math.sin(a), z)))
    return pts


def liana_centre(side, z, seed):
    rnd = random.Random(seed)
    sway, ph = rnd.uniform(-0.02, 0.02), rnd.uniform(0, 6.28)
    e = envelope(z)
    return V((side * RAIL_X + sway * e + 0.006 * e * math.sin(4 * z + ph), RAIL_Y, z))


def add_liana(b, M, side, seed, z0=0.0, z1=1.0, ends=None):
    """Two strands twisted round each other. ends(z, p) -> p lets the top/bottom pieces bend the strands away."""
    for s, (phase, r) in enumerate(((0.0, 0.011), (math.pi, 0.0085))):
        pts = strand_path(side, z0, z1, phase, seed)
        if ends:
            pts = [ends(p.z, p, s) for p in pts]
        keys = [(p, r, r) for p in pts]
        b.add(tube(keys, sub=1, n=7, caps=(False, False)), M["liana"], "root")


def leaf(b, M, base, direction, length, seed, mat="leaf", ref=V((0, 1, 0))):
    """Pointed leaf on a short stalk, flat side along ref (the camera for wall leaves)."""
    d = V(direction).normalized()
    w = length * random.Random(seed).uniform(0.26, 0.34)
    t = 0.0016
    keys = [(base + d * length * f, max(w * k, 0.0012), t) for f, k in ((0.0, 0.05), (0.12, 0.2), (0.35, 1.0), (0.65, 0.8), (0.9, 0.3), (1.0, 0.05))]
    b.add(tube(keys, sub=1, n=6, ref=ref), M[mat], "root")
    b.add(rod(base - d * 0.006, base + d * 0.006, 0.0014, n=3), M["stem"], "root")


def coil(b, M, centre, z, side, r, seed):
    """A vine end wrapped 1.5 times round a liana."""
    rnd = random.Random(seed)
    ph = rnd.uniform(0, 6.28)
    ring = []
    for k in range(13):
        a = ph + 3 * math.pi * k / 12
        ring.append((centre + V((0.022 * math.cos(a), 0.02 * math.sin(a), z + 0.012 * (k / 12 - 0.5) * side)), r, r))
    b.add(tube(ring, sub=1, n=4), M["liana"], "root")


def tendril(b, M, a, e, seed, sag=0.02, r=0.0045, leaves=2):
    """Thinner vine strung between the lianas (the climbing holds): sags, wobbles and coils round both lianas."""
    rnd = random.Random(seed)
    a, e = V(a), V(e)
    pts = []
    for k in range(7):
        t = k / 6
        p = a.lerp(e, t) + V((0, -0.006 * math.sin(math.pi * t), -sag * math.sin(math.pi * t) ** 1.3 + 0.003 * math.sin(9 * t + seed)))
        pts.append((p, r * (1 - 0.25 * math.sin(math.pi * t)), r))
    b.add(tube(pts, sub=2, n=5, caps=(True, True)), M["liana"], "root")
    coil(b, M, a + V((-0.004, 0.008, 0)), 0, -1, r * 0.8, seed)
    coil(b, M, e + V((0.004, 0.008, 0)), 0, 1, r * 0.8, seed + 1)
    for k in range(leaves):
        t = rnd.uniform(0.2, 0.8)
        p = a.lerp(e, t) + V((0, -0.006, -sag * math.sin(math.pi * t) ** 1.3))
        leaf(b, M, p, (rnd.uniform(-0.7, 0.7), -0.3, rnd.uniform(-1.0, -0.4)), rnd.uniform(0.035, 0.05), seed * 13 + k,
             "leaf_dry" if rnd.random() < 0.3 else "leaf")


def aerial_root(b, M, top, length, seed):
    rnd = random.Random(seed)
    pts = [V(top)]
    for k in range(4):
        pts.append(pts[-1] + V((rnd.uniform(-0.006, 0.006), -0.001, -length / 4)))
    b.add(curve(pts, 0.0022, n=4, ref=V((0, 1, 0)), r_end=0.0008), M["root"], "root")


def vine_holds(b, M, seed, z0=0.0, z1=1.0, skip=()):
    """Holds between the lianas every 1 / RUNGS tile, some slanting, plus leaves on the lianas."""
    rnd = random.Random(seed)
    for k in range(RUNGS):
        z = rung_z(k, seed)
        if z < z0 + 0.03 or z > z1 - 0.03 or k in skip:
            continue
        slant = rnd.choice((0.0, 0.0, 0.02, -0.02))
        a = liana_centre(-1, z - slant, seed * 7) + V((0.004, -0.008, 0))
        e = liana_centre(1, z + slant, seed * 7 + 1) + V((-0.004, -0.008, 0))
        tendril(b, M, a, e, seed * 50 + k, sag=rnd.uniform(0.012, 0.028), r=rnd.uniform(0.0042, 0.0058), leaves=rnd.choice((0, 1, 1, 2)))
    for k in range(10):
        side = rnd.choice((-1, 1))
        z = rnd.uniform(z0 + 0.05, z1 - 0.07)
        p = liana_centre(side, z, seed * 7 + (side > 0)) + V((side * 0.01, -0.006, 0))
        leaf(b, M, p, (side * rnd.uniform(0.4, 1.0), -0.3, rnd.uniform(-0.8, 0.2)), rnd.uniform(0.04, 0.055), seed * 17 + k,
             "leaf_dry" if rnd.random() < 0.25 else "leaf")
    for k in range(2):
        side = rnd.choice((-1, 1))
        z = rnd.uniform(z0 + 0.25, z1 - 0.05)
        aerial_root(b, M, liana_centre(side, z, seed * 7 + (side > 0)) + V((side * 0.01, -0.004, 0)), rnd.uniform(0.08, 0.16), seed * 3 + k)


def build_vines(b, M, seed, **kw):
    for i, side in enumerate((-1, 1)):
        add_liana(b, M, side, seed * 7 + i)
    vine_holds(b, M, seed, **kw)


def build_vine_a(b, M):
    """Two lianas twisted like rope, thin vines strung between them for holds."""
    build_vines(b, M, 1)


def build_vine_b(b, M):
    """A gap where a hold has rotted away, a dead cluster of leaves."""
    build_vines(b, M, 2, skip=(6,))
    for k in range(4):
        p = liana_centre(1, 0.52 + 0.01 * k, 15) + V((0.008, -0.008, 0))
        leaf(b, M, p, (0.4 + 0.2 * k, -0.4, -1.0), 0.03, 90 + k, "leaf_dry")


def build_vine_c(b, M):
    """Lush stretch: a side shoot curling off the left liana, more leaves."""
    build_vines(b, M, 3)
    rnd = random.Random(33)
    base = liana_centre(-1, 0.4, 21)
    pts = [base, base + V((-0.04, -0.01, 0.03)), base + V((-0.07, -0.015, 0.09)), base + V((-0.06, -0.02, 0.13)), base + V((-0.045, -0.02, 0.12))]
    b.add(curve(pts, 0.004, n=5, r_end=0.0012), M["stem"], "root")
    for k, p in enumerate(pts[1:4]):
        leaf(b, M, p, (rnd.uniform(-1, 0), -0.4, rnd.uniform(-0.3, 0.6)), 0.032, 70 + k)


VINE_ANCHOR_Z = 0.9


def build_vine_top(b, M):
    """Highest cell: the lianas flatten onto the back wall and split into roots creeping along under the ceiling."""

    def ends(z, p, s):
        t = smoothstep(VINE_ANCHOR_Z - 0.12, 0.99, z)
        return p.lerp(V((p.x + (0.03 if p.x > 0 else -0.03) * (s + 1), -0.006 - 0.004 * s, p.z)), t)

    for i, side in enumerate((-1, 1)):
        add_liana(b, M, side, 4 * 7 + i, z1=0.99, ends=ends)
    vine_holds(b, M, 4, z1=VINE_ANCHOR_Z - 0.04)
    rnd = random.Random(44)
    for side in (-1, 1):
        for k in range(4):
            start = V((side * (RAIL_X + 0.01 + 0.02 * k), -0.008, VINE_ANCHOR_Z - 0.02 + 0.02 * k))
            reach = rnd.uniform(0.14, 0.3)
            rise = rnd.uniform(0.04, 0.09)
            ph = rnd.uniform(0, 6.28)
            pts = []
            for j in range(7):
                f = j / 6
                pts.append(V((start.x + side * reach * f, -0.008 + 0.004 * f,
                              min(start.z + rise * math.sin(f * 1.6) + 0.012 * math.sin(10 * f + ph), 0.99))))
            b.add(curve(pts, rnd.uniform(0.006, 0.009), n=5, ref=V((0, 1, 0)), r_end=0.0015), M["root"], "root")
        for k in range(3):
            leaf(b, M, V((side * (RAIL_X + 0.015), -0.02, VINE_ANCHOR_Z - 0.03 - 0.03 * k)), (side * rnd.uniform(0.3, 1), -0.3, rnd.uniform(-1, -0.2)), 0.05,
                 80 + k + (side > 0) * 5)
        aerial_root(b, M, V((side * (RAIL_X + 0.03), -0.012, VINE_ANCHOR_Z)), 0.14, 88 + side)


def build_vine_bottom(b, M):
    """On the floor: the lianas bend into roots spreading over the floor into drifted sand."""

    def ends(z, p, s):
        if z > 0.12:
            return p
        t = 1 - z / 0.12
        spread = (1 if p.x > 0 else -1) * (0.05 + 0.04 * s)
        return p + V((spread * t * t, -0.05 * t * t * (1 + s), 0))

    for i, side in enumerate((-1, 1)):
        add_liana(b, M, side, 5 * 7 + i, ends=ends)
    vine_holds(b, M, 5, z0=0.06)
    rnd = random.Random(55)
    for side in (-1, 1):
        for k in range(3):
            start = V((side * (RAIL_X + 0.05), -0.04 - 0.03 * k, 0.004))
            a = math.radians(side * rnd.uniform(20, 80))
            reach = rnd.uniform(0.08, 0.16)
            pts = [start + V((math.sin(a) * reach * j / 3, -math.cos(a) * reach * j / 3 * 0.8, 0.003 * (3 - j))) for j in range(4)]
            b.add(curve(pts, 0.005, n=5, r_end=0.001), M["root"], "root")
    b.add(clip_wall(place(prism(blob(0.24, 57, n=10, jitter=0.2), 0.004), rot(z=-8), (0.0, -0.1, 0.0))), M["sand"], "root")
    for k in range(6):
        p = V((rnd.uniform(-0.2, 0.2), rnd.uniform(-0.2, -0.05), 0.006))
        leaf(b, M, p, (rnd.uniform(-1, 1), rnd.uniform(-1, 0), 0.05), 0.03, 60 + k, "leaf_dry", ref=V((0, 0, 1)))
    b.add(rock((0.19, -0.12, 0.008), (0.018, 0.015, 0.012), 58), M["stone"], "root")


BUILDERS = {name: globals()["build_" + name] for name in PIECES}


# ---------------------------------------------------------------- entry points


def build_env(coll):
    """Back wall under the whole line-up (AO occluder for the bake, backdrop for reviews). No floor: the mid
    pieces must not get darker at the bottom than at the top, or the seams would show."""
    half = SPACING * len(PIECES) / 2 + 0.5
    verts = [(-half, 0, -0.5), (half, 0, -0.5), (half, 0, 1.5), (-half, 0, 1.5)]
    mesh = bpy.data.meshes.new("ladder_env")
    mesh.from_pydata(verts, [], [[0, 3, 2, 1]])
    obj = bpy.data.objects.new("ladder_env", mesh)
    coll.objects.link(obj)
    return obj


def build(bake=True, tex_dir=None, only=None):
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    env = bpy.data.meshes.get("ladder_env")
    if env:
        bpy.data.meshes.remove(env)
    M = materials()
    build_env(coll)
    objs = {}
    for k, name in enumerate(PIECES):
        if only and name not in only:
            continue
        b = Builder()
        BUILDERS[name](b, M)
        obj = common.finish_mesh(b, coll, "ladder_" + name)
        obj.data.set_sharp_from_angle(angle=math.radians(50))
        obj.location.x = (k - (len(PIECES) - 1) / 2) * SPACING
        common.uv_unwrap(obj, b.tags)
        if bake:
            tex = bake_lit(obj, os.path.join(tex_dir or bpy.app.tempdir or "/tmp", "ladder_%s.png" % name))
            common.use_baked_material(obj, tex)
        vs = obj.data.vertices
        print("ladder_{}: {} verts, {} tris, x {:.3f}..{:.3f}, y {:.3f}..{:.3f}, z {:.3f}..{:.3f}".format(
            name, len(vs), common.tri_count(obj), min(v.co.x for v in vs), max(v.co.x for v in vs), min(v.co.y for v in vs),
            max(v.co.y for v in vs), min(v.co.z for v in vs), max(v.co.z for v in vs)))
        objs[name] = obj
    return objs


def export(objs, models_dir=None):
    models_dir = models_dir or os.path.join(REPO, "Models", "ladders")
    p = os.path.join(REPO, "tools", "blender", "md3_export.py")
    g = {"__file__": p, "__name__": "md3_export"}
    exec(open(p).read(), g)
    for name, obj in objs.items():
        x = obj.location.x
        obj.location.x = 0.0
        bpy.context.view_layer.update()
        g["export_md3"](obj, os.path.join(models_dir, "ladder_%s.md3" % name), 0, 0)
        obj.location.x = x
    bpy.context.view_layer.update()


def review(path, objs, view=(3.0, 6.2)):
    """Front view of one shaft per style (bottom, a, b, c, a mirrored, top stacked), seams included.
    view = (centre z, ortho scale): zoom in on part of the stack."""
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    stack = ["bottom", "a", "b", "c", "a", "top"]
    scene.render.resolution_x, scene.render.resolution_y = (800, 1500) if view[1] > 3 else (1400, 1400)
    if scene.world is None:
        scene.world = bpy.data.worlds.new("World")
    scene.world.color = (0.25, 0.25, 0.25)
    for o in list(scene.collection.objects):
        if o.name.startswith("review_"):
            data = o.data
            bpy.data.objects.remove(o)
            if isinstance(data, bpy.types.Mesh):
                bpy.data.meshes.remove(data)
    sun = bpy.data.objects.new("review_sun", bpy.data.lights.new("review_sun", "SUN"))
    sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(60), 0, math.radians(-20))
    scene.collection.objects.link(sun)
    for o in objs.values():
        o.hide_render = True
    for col, style in enumerate(("wood", "vine")):
        for row, part in enumerate(stack):
            src = objs.get("%s_%s" % (style, part))
            if src is None:
                continue
            o = src.copy()
            o.name = "review_%s_%d" % (style, row)
            o.hide_render = False
            o.location = (col * 1.0 - 0.5, 0, row)
            if row == 4 and style == "wood":  # the engine mirrors wooden pieces only (see the module doc)
                o.scale.x = -1
            scene.collection.objects.link(o)
    mesh = bpy.data.meshes.new("review_wall")
    mesh.from_pydata([(-1, 0.001, 0), (1, 0.001, 0), (1, 0.001, 6), (-1, 0.001, 6)], [], [[0, 3, 2, 1]])
    scene.collection.objects.link(bpy.data.objects.new("review_wall", mesh))
    cam = bpy.data.objects.new("review_cam", bpy.data.cameras.new("review_cam"))
    scene.collection.objects.link(cam)
    scene.camera = cam
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = view[1]
    cam.rotation_euler = (math.radians(90), 0, 0)
    cam.location = (0, -6, view[0])
    env = bpy.data.objects.get("ladder_env")
    env.hide_render = True
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    env.hide_render = False
    for o in objs.values():
        o.hide_render = False


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    only = args[args.index("--only") + 1].split(",") if "--only" in args else None
    exporting = "--export" in args
    objs = build(bake=exporting or "--bake" in args, tex_dir=os.path.join(REPO, "Textures", "ladders") if exporting else None, only=only)
    if "--review" in args:
        view = tuple(float(v) for v in args[args.index("--view") + 1].split(",")) if "--view" in args else (3.0, 6.2)
        review(args[args.index("--review") + 1], objs, view)
    if exporting:
        export(objs)
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "ladder.blend"))
