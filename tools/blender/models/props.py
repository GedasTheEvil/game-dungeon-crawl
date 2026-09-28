"""Procedural level props: the gateway at entrances, exits and riddle gates (sphinx.md3), the ankh shrine that wins
the game, the question mark over riddle gates and the spike trap.

    MCP:  p = ".../tools/blender/models/props.py"; g = {"__file__": p, "__name__": "props"}
          exec(open(p).read(), g); g["build"](bake=False)      # then g["export"](objs)
    CLI:  blender -b --python tools/blender/models/props.py -- [--export] [--bake] [--review out.png] [--only sphinx,ankh]

Blender space: Z up, 1 unit = 1 tile, the camera looks along +Y. The engine centres each model on its bounding box
(x, y), puts its lowest point on the floor and scales its largest dimension to 1 (Centrify), then draws it at
glScale 40 (one tile) for the gateway and the ankh, 10 for the question mark, 16 / 40 for spikes / death trap.
So the gateway and the ankh are built with their largest dimension exactly 1 and stay in tile units:
  sphinx      - doorway in the y-z plane at the tile's left edge (x = -0.5..-0.42, the plasma portal quad of
                Dungeon::Draw sits at x = -0.49, y +-0.25, z 0..0.875, inside the opening), two recumbent
                jackals on shrine plinths flanking the aisle, facing +x. Exits are drawn turned 180 deg.
  ankh        - gold ankh on a stepped alabaster dais between four obelisks, 1 x 1 tile, 0.93 tall.
  questionmark - gold and lapis striped question mark with a carnelian dot, spins around its vertical axis.
  spikes      - bronze spikes in a sandstone frame, 1 wide, 0.56 deep.

Textures: albedo x ambient occlusion (no baked light: the engine lights the props like the monsters):
textures/props/{sphinx,ankh,questionmark}.png, textures/traps/spikes.png.
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
import items  # noqa: E402

importlib.reload(common)
importlib.reload(decor)
importlib.reload(items)
from common import REPO, Builder, cone, ellipsoid, transform, tube  # noqa: E402
from decor import box, place, rod, rot, stripes  # noqa: E402
from items import band, gem, prism_x, spindle  # noqa: E402

COLL = "props_new"
PROPS = ["sphinx", "ankh", "questionmark", "spikes"]
CATEGORY = {"sphinx": "props", "ankh": "props", "questionmark": "props", "spikes": "traps"}
TEX_SIZE = {"sphinx": 1024, "ankh": 1024, "questionmark": 256, "spikes": 512}
SPACING = 1.6

COL = {
    "sandstone": (0.62, 0.48, 0.29),
    "sandstone_dark": (0.40, 0.29, 0.16),
    "sandstone_light": (0.76, 0.63, 0.42),
    "alabaster": (0.86, 0.82, 0.70),
    "alabaster_dark": (0.66, 0.60, 0.48),
    "granite": (0.30, 0.20, 0.17),
    "granite_dark": (0.16, 0.10, 0.09),
    "black": (0.025, 0.022, 0.025),
    "black_hi": (0.07, 0.065, 0.075),
    "gold": (0.85, 0.60, 0.16),
    "gold_dark": (0.48, 0.31, 0.08),
    "blue": (0.05, 0.16, 0.50),
    "blue_dark": (0.02, 0.07, 0.25),
    "red": (0.55, 0.07, 0.04),
    "turquoise": (0.05, 0.50, 0.42),
    "bronze": (0.55, 0.34, 0.12),
    "bronze_dark": (0.28, 0.16, 0.06),
    "bronze_hi": (0.75, 0.52, 0.22),
    "verdigris": (0.16, 0.36, 0.28),
    "blood": (0.25, 0.02, 0.01),
    "gem": (0.62, 0.05, 0.02),
    "glint": (1.0, 0.62, 0.5),
    "enamel": (0.02, 0.015, 0.01),
    "white": (0.85, 0.83, 0.76),
}


def materials():
    specs = {
        "stone": ("solid", "sandstone"),
        "stone_dark": ("solid", "sandstone_dark"),
        "stone_light": ("solid", "sandstone_light"),
        "stone_courses": stripes("v", 0.12, [(12, "sandstone"), (1, "sandstone_dark")]),
        "alabaster": ("solid", "alabaster"),
        "alabaster_dark": ("solid", "alabaster_dark"),
        "granite": stripes("v", 0.05, [(3, "granite"), (1, "granite_dark")], "LINEAR"),
        "black": ("solid", "black"),
        "black_hi": ("solid", "black_hi"),
        "gold": ("solid", "gold"),
        "gold_band": stripes("v", 0.008, [(2, "gold"), (1, "gold_dark")]),
        "blue": ("solid", "blue"),
        "red": ("solid", "red"),
        "turquoise": ("solid", "turquoise"),
        "white": ("solid", "white"),
        "frieze": stripes("u", 0.04, [(2, "blue"), (1, "gold"), (2, "red"), (1, "gold"), (2, "turquoise"), (1, "gold")]),
        "nemes": stripes("v", 0.11, [(3, "gold"), (1, "blue")]),
        "bronze": stripes("v", 0.04, [(4, "bronze"), (1, "verdigris")], "LINEAR"),
        "bronze_dark": ("solid", "bronze_dark"),
        "spike": stripes("u", 1 / 6, [(1, "bronze_hi"), (1, "bronze")], "LINEAR"),
        "blood": ("solid", "blood"),
        "gem": ("solid", "gem"),
        "glint": ("solid", "glint"),
        "enamel": ("solid", "enamel"),
    }
    return {name: common.make_material("prop_" + name, spec, COL) for name, spec in specs.items()}


def xbox(x0, x1, y0, y1, z0, z1):
    return place(box(x1 - x0, y1 - y0, z1 - z0), loc=((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2))


def cornice(b, M, x0, x1, y0, y1, z, h, flare, mat="stone"):
    """Egyptian cavetto cornice over the box x0..x1, y0..y1 at height z: a torus roll, the flaring hollow, a flat top."""
    b.add(xbox(x0 - 0.006, x1 + 0.006, y0 - 0.006, y1 + 0.006, z, z + h * 0.18), M["gold_band"], "root")
    steps = 4
    for k in range(steps):
        f = flare * ((k + 1) / steps) ** 2
        zz = z + h * (0.18 + 0.62 * k / steps)
        b.add(xbox(x0 - f, x1 + f, y0 - f, y1 + f, zz, zz + h * 0.62 / steps), M[mat], "root")
    b.add(xbox(x0 - flare, x1 + flare, y0 - flare, y1 + flare, z + h * 0.8, z + h), M["stone_light" if mat == "stone" else mat], "root")


# ---------------------------------------------------------------- gateway


DOOR_X = -0.488  # back face of the door frame (the cornice overhangs it to the tile edge, x = -0.5)
DOOR_T = 0.07  # frame thickness along x
OPEN_Y = 0.28  # half width of the opening (the portal quad is +-0.25)
JAMB_W = 0.11
OPEN_TOP = 0.885  # underside of the lintel (portal quad top 0.875)
PLINTH_Y = 0.25  # plinth centre lines at y = +-PLINTH_Y
PLINTH_W = 0.2
PLINTH_H = 0.2
PLINTH_X = (-0.37, 0.5)


JACKAL_LEN = 0.71  # rump to fore paw tips


def jackal(b, M, x, y):
    """Recumbent Anubis jackal on the plinth top at (x, y, PLINTH_H), facing +x, after the one from Tutankhamun's tomb:
    the neck upright, the head held high with tall ears (gold linings), a deep chest, the rump haunches rounded, the
    forelegs stretched out flat; black with a gold collar, a gold sash down the chest
    and gilded eyes. Points are traced in pixels off a side photo (rump x 60, plinth top y 385, 500 px long), y px across."""
    o = V((x, y, PLINTH_H))
    k = JACKAL_LEN / 500
    blk, gold = M["black"], M["gold"]

    def J(px, py, pw=0.0):
        return o + V(((px - 60) * k - JACKAL_LEN / 2, pw * k, (385 - py) * k))

    def keys(pts):  # (px, py, pw, width px, height px) -> tube keys
        return [(J(px, py, pw), rw * k, rh * k) for px, py, pw, rw, rh in pts]

    up, fwd = V((0, 0, 1)), V((1, 0, 0))
    # Torso from the rump to the shoulders, the deep chest, the neck.
    b.add(tube(keys([(72, 332, 0, 30, 42), (120, 318, 0, 40, 56), (200, 320, 0, 38, 54), (285, 305, 0, 40, 66), (335, 288, 0, 34, 58)]),
               sub=4, n=24, ref=up, bulge=(18 * k, 10 * k)), blk, "root")
    b.add(ellipsoid(J(335, 312), (34 * k, 58 * k, 38 * k), n=20, rings=10, axis=fwd, ref=up), blk, "root")
    b.add(tube(keys([(322, 275, 0, 34, 44), (322, 215, 0, 28, 32), (336, 162, 0, 25, 29), (345, 140, 0, 24, 27)]), sub=3, n=20, ref=fwd), blk, "root")
    for s in (-1, 1):
        b.add(ellipsoid(J(112, 326, s * 24), (22 * k, 52 * k, 55 * k), n=20, rings=10, axis=fwd, ref=up), blk, "root")  # haunch
        b.add(tube(keys([(130, 352, s * 36, 12, 14), (165, 372, s * 36, 10, 10), (205, 376, s * 34, 10, 9)]), sub=2, n=10, ref=up), blk, "root")  # hock
        b.add(ellipsoid(J(212, 376, s * 34), (11 * k, 9 * k, 20 * k), n=10, rings=5, axis=fwd, ref=up), blk, "root")  # hind paw
        leg = [(318, 300, s * 24, 20, 24), (336, 358, s * 24, 14, 16), (362, 376, s * 24, 11, 10), (500, 380, s * 24, 10, 9)]
        b.add(tube(keys(leg), sub=2, n=10, ref=up), blk, "root")
        b.add(ellipsoid(J(527, 378, s * 24), (13 * k, 8 * k, 32 * k), n=10, rings=5, axis=fwd, ref=up), blk, "root")  # fore paw
    # Head: skull with a brow and cheeks, the long snout pointing slightly down with a jaw line, nose.
    b.add(ellipsoid(J(346, 138), (30 * k, 31 * k, 40 * k), n=24, rings=12, axis=fwd, ref=up), blk, "root")
    for s in (-1, 1):
        b.add(ellipsoid(J(360, 146, s * 12), (16 * k, 16 * k, 24 * k), n=16, rings=8, axis=fwd, ref=up), blk, "root")  # cheek
    b.add(tube(keys([(360, 134, 0, 25, 28), (385, 139, 0, 20, 22), (412, 147, 0, 15, 16), (435, 153, 0, 9, 11), (447, 157, 0, 5, 7)]), sub=3, n=20,
               ref=up, bulge=(0, 4 * k)), blk, "root")
    b.add(tube(keys([(360, 152, 0, 18, 10), (400, 160, 0, 11, 6), (436, 162, 0, 5, 3)]), sub=3, n=14, ref=up), blk, "root")  # lower jaw
    b.add(ellipsoid(J(447, 155), (6 * k, 5 * k, 6 * k), n=12, rings=6, axis=fwd, ref=up), M["black_hi"], "root")
    # Ears: tall leaves rooted in the head, cupped towards the front (bean-shaped section hollowed at the front),
    # a gold lining filling the hollow.
    def cup(th, depth):
        return 1.0 - depth * max(0.0, math.sin(th)) ** 2

    for s in (-1, 1):
        face = V((1, s * 0.45, 0)).normalized()  # the hollow looks forward and a little outwards
        # (px, py, pw, half width, half thickness, cup depth); the first key is buried in the skull.
        ear = [(334, 136, s * 12, 14, 12, 0.0), (330, 118, s * 16, 22, 10, 0.35), (326, 100, s * 19, 25, 9, 0.7), (321, 78, s * 22, 24, 8, 0.8),
               (316, 56, s * 25, 20, 7, 0.8), (311, 36, s * 27, 13, 5, 0.7), (306, 20, s * 29, 6, 3, 0.5), (303, 11, s * 30, 1.5, 1.5, 0.0)]
        b.add(tube([(J(px, py, pw), w * k, t * k, d) for px, py, pw, w, t, d in ear], sub=3, n=24, ref=face, shape=cup), blk, "root")
        lining = [(J(px, py, pw) + face * (t * 0.45) * k, w * 0.84 * k, t * 0.3 * k, d * 0.6) for px, py, pw, w, t, d in ear[1:-1]]
        lining.append((J(305, 17, s * 29) + face * 1.2 * k, 1.0 * k, 0.8 * k, 0.0))
        b.add(tube(lining, sub=3, n=20, ref=face, shape=cup, bulge=(1.5 * k, 0)), gold, "root")
        # Gilded eye: gold outline and brow line, white, black pupil.
        eye = J(368, 132, s * 25)
        side = V((0, s, 0))
        b.add(ellipsoid(eye, (12 * k, 5 * k, 3 * k), n=10, rings=3, axis=side, ref=up), gold, "root")
        b.add(ellipsoid(eye + side * 1.5 * k, (8 * k, 3.2 * k, 2.2 * k), n=10, rings=3, axis=side, ref=up), M["white"], "root")
        b.add(ellipsoid(eye + side * 2.5 * k + V((2 * k, 0, 0)), (2.5 * k, 2.5 * k, 1.5 * k), n=8, rings=3, axis=side, ref=up), blk, "root")
        b.add(tube([(J(352, 122, s * 26), 2 * k, 1.5 * k), (J(372, 123, s * 26), 2 * k, 1.5 * k), (J(390, 131, s * 22), 1 * k, 1 * k)], sub=2, n=6,
                   ref=side), gold, "root")
        # Sash: a gold band falling from the collar down the chest.
        sash = [(346, 224, s * 20, 4, 1.2), (364, 268, s * 24, 4, 1.2), (371, 310, s * 23, 3.5, 1.2), (364, 350, s * 18, 3, 1.2)]
        b.add(tube(keys(sash), sub=2, n=6, ref=V((0, s, 0))), gold, "root")
    # Collar round the neck.
    zc = J(324, 212)
    collar = [(zc + V((31 * k * math.cos(2 * math.pi * j / 18), 29 * k * math.sin(2 * math.pi * j / 18), 0)), 3.5 * k, 8 * k) for j in range(18)]
    b.add(tube(collar, sub=1, n=6, ref=up, closed=True), M["gold_band"], "root")


def plinth(b, M, y):
    """Shrine plinth: plinth base, black body with gold djed pillars and tyet knots on both long sides, cornice."""
    x0, x1 = PLINTH_X
    y0, y1 = y - PLINTH_W / 2, y + PLINTH_W / 2
    b.add(xbox(x0, x1, y0, y1, 0.0, 0.02), M["stone_dark"], "root")
    b.add(xbox(x0 + 0.01, x1 - 0.01, y0 + 0.01, y1 - 0.01, 0.02, PLINTH_H - 0.035), M["black"], "root")
    b.add(xbox(x0 + 0.005, x1 - 0.005, y0 + 0.005, y1 - 0.005, 0.02, 0.035), M["gold_band"], "root")
    for side in (-1, 1):
        fy = y + side * (PLINTH_W / 2 - 0.009)
        n = 6
        for k in range(n):
            x = x0 + 0.06 + (x1 - x0 - 0.12) * k / (n - 1)
            if k % 2 == 0:  # djed pillar: shaft and four bars
                b.add(xbox(x - 0.008, x + 0.008, fy - 0.003, fy + 0.003, 0.05, 0.14), M["gold"], "root")
                for z in (0.115, 0.125, 0.135, 0.145):
                    b.add(xbox(x - 0.02, x + 0.02, fy - 0.004, fy + 0.004, z - 0.003, z + 0.003), M["gold"], "root")
            else:  # tyet knot: loop, arms, skirt
                loop = [(V((x + 0.012 * math.cos(2 * math.pi * j / 12), fy, 0.13 + 0.016 * math.sin(2 * math.pi * j / 12))), 0.004, 0.003) for j in range(12)]
                b.add(tube(loop, sub=1, n=6, ref=V((0, side, 0)), closed=True), M["red"], "root")
                b.add(xbox(x - 0.02, x + 0.02, fy - 0.003, fy + 0.003, 0.106, 0.113), M["red"], "root")
                b.add(xbox(x - 0.007, x + 0.007, fy - 0.003, fy + 0.003, 0.05, 0.11), M["red"], "root")
                b.add(xbox(x - 0.016, x + 0.016, fy - 0.003, fy + 0.003, 0.05, 0.065), M["red"], "root")
    cornice(b, M, x0 + 0.01, x1 - 0.01, y0 + 0.01, y1 - 0.01, PLINTH_H - 0.035, 0.035, 0.008, mat="gold")


def build_sphinx(b, M):
    """Gateway: sandstone doorway with a cavetto cornice, painted jambs, a winged sun disc over the opening, and two
    Anubis jackals on shrine plinths guarding the aisle (in the old game files this model was called the sphinx)."""
    x0, x1 = DOOR_X, DOOR_X + DOOR_T
    for s in (-1, 1):
        ya, yb = sorted((s * OPEN_Y, s * (OPEN_Y + JAMB_W)))
        b.add(xbox(x0, x1, ya, yb, 0.0, OPEN_TOP), M["stone_courses"], "root")
        # Base block and a painted band of blue / gold on the inner face (+x) and the outer side.
        b.add(xbox(x0, x1 + 0.005, ya - 0.005, yb + 0.005, 0.0, 0.05), M["stone_dark"], "root")
        b.add(xbox(x1, x1 + 0.003, ya + 0.02, yb - 0.02, 0.08, OPEN_TOP - 0.04), M["blue"], "root")
        for k in range(7):  # hieroglyph column: gold signs on blue
            z = 0.13 + k * 0.1
            yc = (ya + yb) / 2
            kind = k % 3
            if kind == 0:
                b.add(xbox(x1 + 0.003, x1 + 0.007, yc - 0.028, yc + 0.028, z, z + 0.012), M["gold"], "root")
                b.add(xbox(x1 + 0.003, x1 + 0.007, yc - 0.006, yc + 0.006, z + 0.012, z + 0.05), M["gold"], "root")
            elif kind == 1:
                b.add(ellipsoid((x1 + 0.004, yc, z + 0.03), (0.022, 0.004, 0.022), n=12, rings=4, axis=V((1, 0, 0)), ref=V((0, 0, 1))), M["gold"], "root")
                b.add(ellipsoid((x1 + 0.007, yc, z + 0.03), (0.008, 0.003, 0.008), n=8, rings=3, axis=V((1, 0, 0)), ref=V((0, 0, 1))), M["red"], "root")
            else:
                for j in range(3):
                    b.add(xbox(x1 + 0.003, x1 + 0.007, yc - 0.03 + j * 0.024, yc - 0.018 + j * 0.024, z, z + 0.055), M["gold"], "root")
        # Front / back face of the jamb (seen straight on by the camera): red and gold bands.
        fy = s * (OPEN_Y + JAMB_W) + s * 0.002
        for z0, z1, mat in ((0.08, 0.1, "gold"), (0.1, OPEN_TOP - 0.06, "red"), (OPEN_TOP - 0.06, OPEN_TOP - 0.04, "gold")):
            b.add(xbox(x0 + 0.012, x1 - 0.012, min(fy, fy - s * 0.003), max(fy, fy - s * 0.003), z0, z1), M[mat], "root")
    # Lintel and cornice.
    top = OPEN_TOP + 0.05
    b.add(xbox(x0, x1, -OPEN_Y - JAMB_W - 0.01, OPEN_Y + JAMB_W + 0.01, OPEN_TOP, top), M["stone"], "root")
    cornice(b, M, x0, x1, -OPEN_Y - JAMB_W - 0.01, OPEN_Y + JAMB_W + 0.01, top, 0.04, 0.012)
    # Winged sun disc on the inner face of the lintel, a uraeus either side.
    fx = x1 + 0.004
    zc = OPEN_TOP + 0.026
    b.add(ellipsoid((fx, 0, zc), (0.022, 0.006, 0.022), n=14, rings=4, axis=V((1, 0, 0)), ref=V((0, 0, 1))), M["red"], "root")
    b.add(ring_x((fx, 0, zc), 0.024, 0.004), M["gold"], "root")
    for s in (-1, 1):
        for k, (length, h, dz) in enumerate(((0.2, 0.012, 0.008), (0.17, 0.01, -0.006), (0.13, 0.008, -0.018))):
            y0 = s * 0.028
            y1 = s * (0.028 + length)
            mat = ("blue", "gold", "turquoise")[k]
            b.add(xbox(fx - 0.003, fx + 0.002, min(y0, y1), max(y0, y1), zc + dz - h / 2, zc + dz + h / 2), M[mat], "root")
        b.add(xbox(fx - 0.002, fx + 0.004, s * 0.02 - 0.004, s * 0.02 + 0.004, zc - 0.03, zc + 0.0), M["gold"], "root")
    # Jackals on their plinths.
    for s in (-1, 1):
        plinth(b, M, s * PLINTH_Y)
        jackal(b, M, 0.09, s * PLINTH_Y)


def ring_x(c, r, w, steps=18):
    """Thin ring in the y-z plane (normal +x)."""
    c = V(c)
    keys = [(c + V((0, r * math.cos(2 * math.pi * k / steps), r * math.sin(2 * math.pi * k / steps))), w, w * 0.8) for k in range(steps)]
    return tube(keys, sub=1, n=6, ref=V((1, 0, 0)), closed=True)


# ---------------------------------------------------------------- ankh shrine


DAIS = 1.0  # side of the dais (the largest dimension)


def obelisk(b, M, x, y, h):
    """Granite obelisk with a gold pyramidion and a column of gold signs on the faces towards the centre."""
    base, topw = 0.075, 0.05
    b.add(xbox(x - base / 2 - 0.012, x + base / 2 + 0.012, y - base / 2 - 0.012, y + base / 2 + 0.012, 0.0, 0.03), M["alabaster_dark"], "root")
    shaft = h - 0.06
    verts = []
    for z, w in ((0.03, base), (0.03 + shaft, topw)):
        verts += [V((x - w / 2, y - w / 2, z)), V((x + w / 2, y - w / 2, z)), V((x + w / 2, y + w / 2, z)), V((x - w / 2, y + w / 2, z))]
    apex = V((x, y, h))
    verts.append(apex)
    faces = [[0, 1, 5, 4], [1, 2, 6, 5], [2, 3, 7, 6], [3, 0, 4, 7], [3, 2, 1, 0]]
    fuv = [[(verts[i].x + verts[i].y, verts[i].z) for i in f] for f in faces]
    b.add((verts[:8], faces, fuv), M["granite"], "root")
    pyr = [verts[4], verts[5], verts[6], verts[7], apex]
    b.add((pyr, [[0, 1, 4], [1, 2, 4], [2, 3, 4], [3, 0, 4]], [[(0, 0), (1, 0), (0.5, 1)]] * 4), M["gold"], "root")
    # Signs on the two inner faces.
    for axis, s in ((0, -1 if x > 0 else 1), (1, -1 if y > 0 else 1)):
        for k in range(5):
            z = 0.1 + k * (shaft - 0.12) / 5
            w = base - (base - topw) * (z - 0.03) / shaft
            off = s * (w / 2 + 0.002)
            size = 0.012 if k % 2 else 0.018
            c = V((x + (off if axis == 0 else 0), y + (off if axis == 1 else 0), z))
            half = V((0.002 if axis == 0 else size, 0.002 if axis == 1 else size, 0.012))
            b.add(place(box(*(half * 2)), loc=c), M["gold"], "root")


def build_ankh(b, M):
    """The goal of the last level: a large gold ankh with a carnelian in its loop, standing on a stepped alabaster
    dais between four granite obelisks with gold tips, a painted frieze round the dais."""
    s0, s1 = DAIS / 2, DAIS / 2 - 0.08
    b.add(xbox(-s0, s0, -s0 + 0.002, s0, 0.0, 0.045), M["alabaster_dark"], "root")
    b.add(xbox(-s0 + 0.01, s0 - 0.01, -s0, -s0 + 0.01, 0.012, 0.034), M["frieze"], "root")  # front frieze
    b.add(xbox(-s1, s1, -s1, s1, 0.045, 0.085), M["alabaster"], "root")
    b.add(xbox(-0.2, 0.2, -0.2, 0.2, 0.085, 0.12), M["alabaster"], "root")
    b.add(xbox(-0.19, 0.19, -0.201, -0.19, 0.092, 0.113), M["gold_band"], "root")
    for x in (-1, 1):
        for y in (-1, 1):
            obelisk(b, M, x * (s1 - 0.06), y * (s1 - 0.06), 0.93)
    # Ankh: shaft, arms and loop in the x-z plane (face to the camera), a gem in the loop on both faces.
    z0 = 0.12
    gold = M["gold"]
    b.add(xbox(-0.07, 0.07, -0.05, 0.05, z0, z0 + 0.03), M["gold_band"], "root")
    shaft = [(V((0, 0, z0 + 0.03)), 0.045, 0.028), (V((0, 0, z0 + 0.12)), 0.034, 0.024), (V((0, 0, z0 + 0.3)), 0.026, 0.022), (V((0, 0, z0 + 0.36)), 0.03, 0.022)]
    b.add(tube(shaft, sub=2, n=12, ref=V((0, 1, 0))), gold, "root")
    arms = [(V((x, 0, z0 + 0.38)), 0.045 if abs(x) > 0.12 else 0.028 + 0.1 * abs(x) * abs(x), 0.02) for x in (-0.16, -0.13, -0.06, 0.0, 0.06, 0.13, 0.16)]
    arms = [(p, rx, ry) for p, rx, ry in arms]
    b.add(tube([(p, ry, rx * 0.8) for p, rx, ry in arms], sub=2, n=10, ref=V((0, 1, 0)), bulge=(0.004, 0.004)), gold, "root")
    lc, lrx, lrz = V((0, 0, z0 + 0.5)), 0.075, 0.1
    loop = [(lc + V((lrx * math.cos(2 * math.pi * k / 28), 0, lrz * math.sin(2 * math.pi * k / 28))), 0.024, 0.02) for k in range(28)]
    b.add(tube(loop, sub=1, n=10, ref=V((0, -1, 0)), closed=True), gold, "root")
    b.add(ellipsoid(lc, (lrx - 0.015, lrz - 0.015, 0.008), n=16, rings=4, axis=V((0, 1, 0)), ref=V((0, 0, 1))), M["enamel"], "root", "gem")
    for s in (-1, 1):
        gem(b, M, lc, (0, s, 0), lrx - 0.025, lrz - 0.025, 0.02, bezel=0.004)
    b.add(ellipsoid((0, 0, z0 + 0.39), (0.04, 0.03, 0.03), n=10, rings=4), gold, "root")


# ---------------------------------------------------------------- question mark and spikes


def build_questionmark(b, M):
    """Question mark of gold striped with lapis (like a nemes headcloth), a carnelian ball as the dot."""
    pts = []
    # Hook: three quarters of a circle from the lower left round the top to the right and back to the middle.
    cx, cz, r = 0.0, 0.62, 0.2
    for k in range(15):
        a = math.radians(200 - 290 * k / 14)
        pts.append(V((cx + r * math.cos(a), 0, cz + r * math.sin(a))))
    pts += [V((0.02, 0, 0.36)), V((0.0, 0, 0.3)), V((0.0, 0, 0.22))]
    keys = []
    for k, p in enumerate(pts):
        t = k / (len(pts) - 1)
        w = 0.042 + 0.014 * math.sin(math.pi * t)
        keys.append((p, w * 0.7, w))
    b.add(tube(keys, sub=3, n=12, ref=V((0, 1, 0)), bulge=(0.02, 0.015)), M["nemes"], "root")
    b.add(ellipsoid((0, 0, 0.07), (0.07, 0.07, 0.07), n=16, rings=8), M["red"], "root")
    b.add(ellipsoid((-0.024, -0.05, 0.095), (0.015, 0.02, 0.008), n=8, rings=3, axis=V((-0.3, -0.8, 0.4)), ref=V((0, 0, 1))), M["glint"], "root")


def build_spikes(b, M):
    """Spike trap: a sandstone frame round a dark pit floor, rows of bronze spikes of uneven height (a few bent, the
    tips of some stained with old blood)."""
    w, d = 0.5, 0.28
    b.add(xbox(-w, w, -d, d, 0.0, 0.015), M["stone_dark"], "root")
    for x0, x1, y0, y1 in ((-w, w, -d, -d + 0.04), (-w, w, d - 0.04, d), (-w, -w + 0.04, -d, d), (w - 0.04, w, -d, d)):
        b.add(xbox(x0, x1, y0, y1, 0.0, 0.04), M["stone"], "root")
    b.add(xbox(-w + 0.04, w - 0.04, -d + 0.04, d - 0.04, 0.015, 0.02), M["black"], "root")
    rnd = random.Random(11)
    rows, cols = 4, 9
    for i in range(rows):
        for j in range(cols):
            x = -w + 0.08 + (2 * w - 0.16) * (j + (0.5 if i % 2 else 0.0)) / (cols - 0.5)
            y = -d + 0.08 + (2 * d - 0.16) * i / (rows - 1)
            if abs(x) > w - 0.06:
                continue
            h = rnd.uniform(0.15, 0.21)
            lean = V((rnd.uniform(-0.12, 0.12), rnd.uniform(-0.12, 0.12), 1.0))
            if rnd.random() < 0.08:
                lean = V((rnd.uniform(-0.6, 0.6), rnd.uniform(-0.4, 0.4), 1.0))
            base = V((x, y, 0.02))
            b.add(cone(base, lean, h, 0.026, n=6), M["spike"], "root")
            b.add(band_at(base + V((0, 0, 0.004)), 0.031, 0.012), M["bronze_dark"], "root")
            if rnd.random() < 0.25:
                tip = base + lean.normalized() * h * 0.75
                b.add(cone(tip - lean.normalized() * 0.001, lean, h * 0.25 + 0.003, 0.0085, n=6), M["blood"], "root")


def band_at(c, r, h, n=8):
    c = V(c)
    return tube([(c - V((0, 0, h / 2)), r, r), (c + V((0, 0, h / 2)), r, r)], sub=1, n=n, ref=V((0, 1, 0)))


BUILDERS = {"sphinx": build_sphinx, "ankh": build_ankh, "questionmark": build_questionmark, "spikes": build_spikes}


# ---------------------------------------------------------------- entry points


def build(bake=True, tex_dir=None, only=None):
    """Returns {prop: obj}; each obj gets obj["texture"] = image name when baked."""
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    M = materials()
    names = [n for n in PROPS if not only or n in only]
    xs = {name: (k - (len(PROPS) - 1) / 2) * SPACING for k, name in enumerate(PROPS)}
    objs = {}
    for name in names:
        b = Builder()
        BUILDERS[name](b, M)
        low = min(v.z for v in b.verts)
        b.verts = [V(v) - V((0, 0, low)) for v in b.verts]
        obj = common.finish_mesh(b, coll, "prop_" + name)
        obj.data.set_sharp_from_angle(angle=math.radians(50))
        obj.location = (xs[name], 0, 0)
        common.uv_unwrap(obj, b.tags, boost={"gem": ((0, 0, 0), 1.6)})
        if bake:
            out = os.path.join(tex_dir or bpy.app.tempdir or "/tmp", CATEGORY[name], name + ".png") if tex_dir else os.path.join("/tmp", name + ".png")
            tex = common.bake_texture(obj, out, TEX_SIZE[name], "prop_" + name, ao_distance=0.08)
            common.use_baked_material(obj, tex)
            obj["texture"] = tex.name
        (x0, x1), (y0, y1), (z0, z1) = items.extents(obj)
        print("prop_{}: {} verts, {} tris, x {:.3f}..{:.3f}, y {:.3f}..{:.3f}, z {:.3f}..{:.3f}".format(
            name, len(obj.data.vertices), common.tri_count(obj), x0, x1, y0, y1, z0, z1))
        objs[name] = obj
    return objs


def export(objs):
    p = os.path.join(REPO, "tools", "blender", "md3_export.py")
    g = {"__file__": p, "__name__": "md3_export"}
    exec(open(p).read(), g)
    for name, obj in objs.items():
        loc = obj.location.copy()
        obj.location = (0, 0, 0)
        bpy.context.view_layer.update()
        g["export_md3"](obj, os.path.join(REPO, "models", CATEGORY[name], "%s.md3" % name), 0, 0)
        obj.location = loc
    bpy.context.view_layer.update()


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    only = args[args.index("--only") + 1].split(",") if "--only" in args else None
    exporting = "--export" in args
    objs = build(bake=exporting or "--bake" in args, tex_dir=os.path.join(REPO, "textures") if exporting else None, only=only)
    if "--review" in args:
        items.review(args[args.index("--review") + 1], objs)
    if exporting:
        export(objs)
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "props.blend"))
