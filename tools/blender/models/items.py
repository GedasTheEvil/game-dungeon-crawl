"""Procedural items: the weapons (club, sword, spear, bow), the potion flask and the treasure chest.

    MCP:  p = ".../tools/blender/models/items.py"; g = {"__file__": p, "__name__": "items"}
          exec(open(p).read(), g); g["build"](bake=False)      # then g["export"](objs)
    CLI:  blender -b --python tools/blender/models/items.py -- [--export] [--bake] [--review out.png] [--only club,bow]

Blender space: Z up, real sizes in metres (the engine centres every item and scales its largest dimension to 1,
item::loadModel -> Centrify, then glScale item->scale). The weapons stand upright with the grip at the bottom and the
business end at the top (+Z): the engine draws the held weapon from its lowest point, tilted 45 deg towards the
facing direction (drawWeapon), and upright in the inventory. Flat faces (sword blade, bow) lie in the x-z plane,
so they face the camera (-Y); the bow's back bulges towards +x (the enemy when the player faces right).
The chest faces -Y with its lid open towards +Y; the engine draws it at rotA 0 with the tile's item standing inside.

Textures: one 512 PNG per item (textures/items/<name>.png), albedo x ambient occlusion like the monsters (no baked
light, the engine lights them). The potion is drawn tinted with the potion colour (inventory POTION_COLORS), so its
texture stays light and nearly grey: glass, liquid, cork and cord differ in brightness only.
"""

import importlib
import math
import os
import sys

import bpy
from mathutils import Vector as V

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import common  # noqa: E402
import decor  # noqa: E402

importlib.reload(common)
importlib.reload(decor)
from common import REPO, Builder, cone, ellipsoid, transform, tube  # noqa: E402
from decor import box, place, prism, revolve, rod, rot, stripes  # noqa: E402

COLL = "items_new"
ITEMS = ["club", "sword", "spear", "bow", "potion", "chest"]
FILES = {"chest": "treasure_chest"}  # model / texture stem when it differs from the item name
TEX_SIZE = 512
SPACING = 1.0  # items are spread along X in the scene (bake/review only; export is at the origin)

COL = {
    "wood": (0.34, 0.19, 0.08),
    "wood_dark": (0.19, 0.10, 0.04),
    "ash": (0.55, 0.40, 0.22),
    "ash_dark": (0.38, 0.26, 0.13),
    "leather": (0.30, 0.15, 0.06),
    "leather_dark": (0.15, 0.07, 0.03),
    "bronze": (0.62, 0.40, 0.14),
    "bronze_dark": (0.34, 0.20, 0.07),
    "verdigris": (0.16, 0.36, 0.28),
    "blade": (0.40, 0.21, 0.07),
    "blade_edge": (0.72, 0.48, 0.20),
    "gold": (0.85, 0.60, 0.16),
    "gold_dark": (0.48, 0.31, 0.08),
    "ebony": (0.05, 0.035, 0.03),
    "ivory": (0.82, 0.76, 0.60),
    "linen": (0.78, 0.72, 0.58),
    "sinew": (0.72, 0.62, 0.44),
    "red": (0.55, 0.07, 0.04),
    "blue": (0.05, 0.16, 0.50),
    "lining": (0.30, 0.05, 0.04),
    "cedar": (0.45, 0.22, 0.09),
    "cedar_dark": (0.26, 0.12, 0.05),
    "glass": (0.93, 0.93, 0.93),
    "liquid": (0.72, 0.72, 0.72),
    "cork": (0.58, 0.55, 0.50),
    "cork_dark": (0.44, 0.42, 0.38),
    "cord": (0.70, 0.68, 0.64),
    "wax": (0.52, 0.50, 0.48),
    "gem": (0.62, 0.05, 0.02),
    "glint": (1.0, 0.62, 0.5),
    "enamel": (0.02, 0.015, 0.01),
}


def materials():
    specs = {
        "wood": stripes("u", 1 / 9, [(3, "wood"), (1, "wood_dark")], "LINEAR"),
        "ash": stripes("u", 1 / 7, [(3, "ash"), (1, "ash_dark")], "LINEAR"),
        "leather": stripes("v", 0.012, [(2, "leather"), (1, "leather_dark")]),
        "bronze": stripes("v", 0.04, [(4, "bronze"), (1, "verdigris")], "LINEAR"),
        "bronze_plain": ("solid", "bronze"),
        "bronze_dark": ("solid", "bronze_dark"),
        "blade": stripes("u", 1 / 4, [(1, "blade_edge"), (1, "blade")], "LINEAR"),
        "gold": ("solid", "gold"),
        "gold_heap": stripes("u", 0.03, [(3, "gold"), (1, "gold_dark")], "LINEAR"),
        "gold_band": stripes("v", 0.006, [(2, "gold"), (1, "gold_dark")]),
        "ebony": ("solid", "ebony"),
        "ivory": ("solid", "ivory"),
        "sinew": ("solid", "sinew"),
        "linen_red": stripes("v", 0.01, [(2, "red"), (1, "linen")]),
        "cedar": stripes("u", 1 / 12, [(3, "cedar"), (1, "cedar_dark")], "LINEAR"),
        "cedar_plain": ("solid", "cedar"),
        "cedar_grain": stripes("v", 0.03, [(3, "cedar"), (1, "cedar_dark")], "LINEAR"),
        "lining": ("solid", "lining"),
        "blue": ("solid", "blue"),
        "red": ("solid", "red"),
        "frieze": stripes("u", 0.05, [(2, "blue"), (1, "gold"), (2, "red"), (1, "gold")]),
        "glass": ("solid", "glass"),
        "liquid": ("solid", "liquid"),
        "cork": stripes("u", 1 / 10, [(2, "cork"), (1, "cork_dark")], "LINEAR"),
        "cord": ("solid", "cord"),
        "wax": ("solid", "wax"),
        "gem": ("solid", "gem"),
        "glint": ("solid", "glint"),
        "enamel": ("solid", "enamel"),
    }
    return {name: common.make_material("item_" + name, spec, COL) for name, spec in specs.items()}


# ---------------------------------------------------------------- helpers


def diamond(th, a=0.0):
    """Lozenge cross-section (blades): radius multiplier for a unit ellipse."""
    return 1.0 / (abs(math.cos(th)) + abs(math.sin(th)))


def spindle(keys, n=12, ref=V((0, 1, 0)), shape=None, sub=2, bulge=(0.0, 0.0)):
    """Tube along Z through (z, rx[, ry]) keys."""
    return tube([(V((0, 0, k[0])), k[1], k[2] if len(k) > 2 else k[1]) for k in keys], sub=sub, n=n, ref=ref, shape=shape, bulge=bulge)


def band(b, mat, z, r, h, n=14):
    """Short ring around the Z axis (bindings, ferrules)."""
    b.add(spindle([(z - h / 2, r), (z + h / 2, r)], n=n, sub=1), mat, "root")


def wrap(b, mat, z0, z1, r, turns, n=6, steps=10):
    """Cord or leather strip wound round the Z axis from z0 to z1."""
    total = int(turns * steps)
    pts = [V((r * math.cos(2 * math.pi * k / steps), r * math.sin(2 * math.pi * k / steps), z0 + (z1 - z0) * k / total)) for k in range(total + 1)]
    b.add(tube([(p, 0.0035, 0.0035) for p in pts], sub=1, n=n, ref=V((0, 0, 1))), mat, "root")


def gem(b, M, centre, normal, rx, rz, depth, bezel=0.003):
    """Cabochon in an enamel bezel, domed along normal, with a glint (as in mechanism.gem)."""
    c, nrm = V(centre), V(normal).normalized()
    up = V((0, 0, 1)) if abs(nrm.z) < 0.9 else V((0, 1, 0))
    b.add(ellipsoid(c, (rx + bezel, rz + bezel, depth * 0.5), n=12, rings=4, axis=nrm, ref=up), M["enamel"], "root", "gem")
    b.add(ellipsoid(c, (rx, rz, depth), n=12, rings=5, axis=nrm, ref=up), M["gem"], "root", "gem")
    left = nrm.cross(up)
    g = c + nrm * (depth * 0.82) + up * (rz * 0.38) + left * (rx * 0.3)
    s = min(rx, rz) * 0.22
    b.add(ellipsoid(g, (s, s * 1.3, s * 0.5), n=6, rings=3, axis=nrm, ref=up), M["glint"], "root", "gem")


# ---------------------------------------------------------------- weapons


def build_club(b, M):
    """Knobbed acacia war club: leather-wrapped grip with a pommel knob, the shaft swelling into a knotty head
    set with bronze studs (the level 2 text: "Now with spikes"), a bronze band at the neck."""
    b.add(ellipsoid((0, 0, 0.012), (0.024, 0.024, 0.016), n=12, rings=5), M["wood"], "root")
    b.add(spindle([(0.02, 0.019), (0.2, 0.02)], n=12, sub=1), M["leather"], "root")
    wrap(b, M["leather"], 0.03, 0.19, 0.021, 9)

    def knots(th, a):
        return 1 + a * (0.08 * math.sin(5 * th + 1.0) + 0.05 * math.sin(3 * th + 2.2))

    head = [(0.19, 0.019, 0.019, 0.0), (0.3, 0.023, 0.023, 0.2), (0.42, 0.034, 0.034, 0.6), (0.52, 0.046, 0.044, 1.0), (0.585, 0.05, 0.048, 1.0),
            (0.63, 0.036, 0.035, 0.8)]
    b.add(tube([(V((0, 0, z)), rx, ry, a) for z, rx, ry, a in head], sub=3, n=16, ref=V((0, 1, 0)), shape=knots, caps=(False, True), bulge=(0, 0.02)),
          M["wood"], "root")
    band(b, M["bronze_plain"], 0.4, 0.035, 0.018, n=16)
    band(b, M["bronze_dark"], 0.39, 0.036, 0.004, n=16)
    band(b, M["bronze_dark"], 0.41, 0.036, 0.004, n=16)
    # Studs: three staggered rings of short pyramidal spikes.
    for ring_i, (z, r, k) in enumerate(((0.47, 0.041, 7), (0.53, 0.048, 8), (0.59, 0.049, 7))):
        for j in range(k):
            a = 2 * math.pi * (j + 0.5 * ring_i) / k
            d = V((math.cos(a), math.sin(a), 0.15))
            b.add(cone(V((0, 0, z)) + d.normalized() * (r - 0.004), d, 0.026, 0.008, n=4), M["bronze_plain"], "root")
    b.add(cone(V((0, 0, 0.64)), V((0, 0, 1)), 0.02, 0.009, n=4), M["bronze_plain"], "root")


def build_sword(b, M):
    """Bronze short sword of a tomb guard: a leaf blade with a gold-inlaid midrib, a gold-capped ebony grip with rivets
    and a lunate ivory pommel."""
    blade = [(0.13, 0.027, 0.0065), (0.16, 0.025, 0.0065), (0.3, 0.023, 0.006), (0.42, 0.024, 0.0055), (0.52, 0.019, 0.005), (0.58, 0.011, 0.004),
             (0.615, 0.0015, 0.0015)]
    b.add(spindle(blade, n=8, shape=diamond, sub=3), M["blade"], "root")
    for s in (-1, 1):  # midrib ridge on both faces
        b.add(tube([(V((0, s * 0.0035, 0.15)), 0.0035, 0.0018), (V((0, s * 0.0028, 0.5)), 0.0025, 0.0014), (V((0, s * 0.001, 0.6)), 0.001, 0.0008)],
                   sub=2, n=6, ref=V((0, 1, 0))), M["gold"], "root")
    # Guard: a gold crescent cupping the blade base.
    guard = [(V((x, 0, 0.13 + 0.25 * x * x / 0.04)), 0.008 - 0.1 * abs(x), 0.011) for x in (-0.04, -0.02, 0.0, 0.02, 0.04)]
    b.add(tube(guard, sub=3, n=8, ref=V((0, 1, 0)), bulge=(0.003, 0.003)), M["gold"], "root")
    # Grip: ebony, gold bands at both ends, three rivets through the tang.
    b.add(spindle([(0.035, 0.012, 0.009), (0.08, 0.014, 0.01), (0.125, 0.012, 0.009)], n=12), M["ebony"], "root")
    band(b, M["gold_band"], 0.04, 0.013, 0.012)
    band(b, M["gold_band"], 0.12, 0.013, 0.012)
    for z in (0.065, 0.08, 0.095):
        for s in (-1, 1):
            b.add(ellipsoid((0, s * 0.0095, z), (0.0035, 0.0035, 0.002), n=8, rings=3, axis=V((0, s, 0)), ref=V((0, 0, 1))), M["gold"], "root")
    # Lunate pommel: an ivory crescent with the horns pointing down.
    arc = []
    for k in range(9):
        a = math.radians(15 + 150 * k / 8)
        taper = math.sin(math.radians(15 + 150 * k / 8))
        arc.append((V((0.034 * math.cos(a), 0, 0.034 * math.sin(a) - 0.004)), 0.004 + 0.007 * taper, 0.008 + 0.002 * taper))
    b.add(tube(arc, sub=2, n=8, ref=V((0, 1, 0)), bulge=(0.002, 0.002)), M["ivory"], "root")


def build_spear(b, M):
    """Ash-shafted spear: bronze leaf blade with a midrib on a socket, a leather lashing with a red linen pennant
    below it, a bronze butt spike."""
    b.add(tube([(V((0, 0, 0.0)), 0.002, 0.002), (V((0, 0, 0.03)), 0.009, 0.009), (V((0, 0, 0.07)), 0.014, 0.014)], sub=2, n=10, caps=(False, True)),
          M["bronze_plain"], "root")
    band(b, M["bronze_dark"], 0.072, 0.0155, 0.01, n=10)
    b.add(spindle([(0.07, 0.0135), (0.8, 0.0145), (1.5, 0.013)], n=10), M["ash"], "root")
    wrap(b, M["leather"], 1.42, 1.5, 0.015, 7)
    # Pennant: two red linen streamers tied under the socket, fluttering towards +x.
    for s, (length, phase) in enumerate(((0.26, 0.0), (0.2, 1.3))):
        pts = []
        for k in range(9):
            t = k / 8
            pts.append(V((0.012 + length * t * 0.55, 0.004 * (s * 2 - 1), 1.44 - length * t * 0.8 + 0.02 * math.sin(6 * t + phase))))
        b.add(tube([(p, 0.012 * (1 - 0.6 * k / 8), 0.0015) for k, p in enumerate(pts)], sub=2, n=6, ref=V((0, 1, 0))), M["linen_red"], "root")
    # Socket and blade.
    b.add(spindle([(1.49, 0.017), (1.56, 0.015), (1.6, 0.011)], n=12), M["bronze"], "root")
    band(b, M["bronze_dark"], 1.495, 0.0185, 0.012, n=12)
    blade = [(1.59, 0.012, 0.007), (1.63, 0.036, 0.007), (1.68, 0.042, 0.0065), (1.74, 0.034, 0.006), (1.8, 0.018, 0.005), (1.855, 0.0015, 0.0015)]
    b.add(spindle(blade, n=8, shape=diamond, sub=3), M["blade"], "root")
    for s in (-1, 1):
        b.add(tube([(V((0, s * 0.004, 1.6)), 0.006, 0.003), (V((0, s * 0.003, 1.72)), 0.004, 0.002), (V((0, s * 0.001, 1.84)), 0.001, 0.001)],
                   sub=2, n=6, ref=V((0, 1, 0))), M["bronze_dark"], "root")


BOW_H, BOW_DEPTH = 0.6, 0.15  # half height, belly depth


def bow_point(t):
    """Limb centre line, t in -1..1 (bottom to top): an arc bulging to +x with tips recurving back."""
    x = BOW_DEPTH * (1 - t * t) - 0.02 * t ** 8
    return V((x, 0, BOW_H * t * (1 - 0.04 * t * t)))


def build_bow(b, M):
    """Self bow of acacia: limbs tapering from a leather-wrapped grip to gold-capped nocks, gold bands on the limbs,
    a sinew string between the tips."""
    keys = []
    for k in range(25):
        t = -1 + 2 * k / 24
        w = 0.016 - 0.009 * abs(t) ** 1.3  # across the bow (y)
        d = 0.012 - 0.006 * abs(t) ** 1.3  # front to back (x)
        keys.append((bow_point(t), d, w))
    b.add(tube(keys, sub=2, n=10, ref=V((0, 1, 0))), M["wood"], "root")
    # Grip wrap and limb bands (short tubes along the curve).
    def along(t0, t1, grow, mat, n=10):
        ks = []
        for k in range(5):
            t = t0 + (t1 - t0) * k / 4
            w = 0.016 - 0.009 * abs(t) ** 1.3 + grow
            d = 0.012 - 0.006 * abs(t) ** 1.3 + grow
            ks.append((bow_point(t), d, w))
        b.add(tube(ks, sub=1, n=n, ref=V((0, 1, 0))), mat, "root")

    along(-0.14, 0.14, 0.004, M["leather"])
    for t in (-0.62, -0.2, 0.2, 0.62):
        along(t - 0.03, t + 0.03, 0.002, M["gold_band"])
    tips = []
    for t in (-1, 1):
        p = bow_point(t)
        b.add(ellipsoid(p + V((0, 0, 0.006 * t)), (0.009, 0.009, 0.014), n=10, rings=5), M["gold"], "root")
        tips.append(p + V((-0.004, 0, -0.006 * t)))
    b.add(rod(tips[0], tips[1], 0.0022, n=6), M["sinew"], "root")


# ---------------------------------------------------------------- potion and chest


def build_potion(b, M):
    """Round-bottomed glass flask two-thirds full, a long neck with a rolled lip, a cork with a wax cap and a cord
    tied round the neck. Only brightness varies: the engine tints the whole model with the potion colour."""
    body_r, body_c = 0.05, 0.055
    liquid_top = body_c + 0.018
    glass = []
    for k in range(13):  # bottom pole to the shoulder
        a = math.radians(-90 + 150 * k / 12)
        glass.append((body_r * math.cos(a), body_c + body_r * math.sin(a)))
    profile = [(0.0, body_c - body_r)] + glass[1:] + [(0.016, 0.115), (0.014, 0.125), (0.014, 0.15), (0.019, 0.153), (0.019, 0.159), (0.013, 0.161)]
    # Glass above the liquid line is brighter; split the lathe at the liquid line.
    split = next(i for i, (_, z) in enumerate(profile) if z > liquid_top)
    mats = ["liquid" if i < split - 1 else "glass" for i in range(len(profile) - 1)]
    revolve(b, M, profile, mats, n=20)
    # Meniscus ring.
    zr = liquid_top
    rr = math.sqrt(max(body_r * body_r - (zr - body_c) ** 2, 0)) + 0.0005
    band(b, M["liquid"], zr, rr, 0.003, n=20)
    # Cork and wax cap.
    b.add(spindle([(0.145, 0.0115), (0.175, 0.0125)], n=12, sub=1, bulge=(0, 0.002)), M["cork"], "root")
    b.add(transform(ellipsoid((0, 0, 0.176), (0.016, 0.016, 0.008), n=14, rings=4), lambda v: V((v.x, v.y, max(v.z, 0.169)))), M["wax"], "root")
    # Cord round the neck with two hanging ends.
    band(b, M["cord"], 0.13, 0.0155, 0.004, n=14)
    band(b, M["cord"], 0.136, 0.0152, 0.004, n=14)
    for s in (-1, 1):
        pts = [V((0.004 * s, -0.015, 0.132)), V((0.008 * s, -0.019, 0.12)), V((0.012 * s, -0.021, 0.105))]
        b.add(tube([(p, 0.0022, 0.0022) for p in pts], sub=2, n=6, ref=V((0, 0, 1))), M["cord"], "root")


CHEST_W, CHEST_D, CHEST_H = 0.8, 0.46, 0.36  # box outside (without legs and lid)
CHEST_LEG = 0.05
CHEST_WALL = 0.025
LID_H = 0.09
LID_OPEN = 105.0  # degrees


def build_chest(b, M):
    """Painted cedar chest on short legs, gold corner caps and studs, a blue-red-gold frieze and ankh plaques
    on the front, red lining, the vaulted lid open towards the back, a heap of gold coins and a gem inside."""
    w, d, h, leg, t = CHEST_W, CHEST_D, CHEST_H, CHEST_LEG, CHEST_WALL
    z0, z1 = leg, leg + h
    # Walls as boxes (outside cedar, inside lining) and the floor.
    for sx, sy, x, y in ((w, t, 0, -d / 2 + t / 2), (w, t, 0, d / 2 - t / 2), (t, d - 2 * t, -w / 2 + t / 2, 0), (t, d - 2 * t, w / 2 - t / 2, 0)):
        b.add(place(box(sx, sy, h), loc=(x, y, (z0 + z1) / 2)), M["cedar_grain"], "root")
    for sx, sy, x, y in ((w - 2 * t, 0.003, 0, -d / 2 + t + 0.0015), (w - 2 * t, 0.003, 0, d / 2 - t - 0.0015),
                         (0.003, d - 2 * t, -w / 2 + t + 0.0015, 0), (0.003, d - 2 * t, w / 2 - t - 0.0015, 0)):
        b.add(place(box(sx, sy, h - 0.035), loc=(x, y, z0 + 0.03 + (h - 0.035) / 2)), M["lining"], "root")
    # Legs.
    for sx in (-1, 1):
        for sy in (-1, 1):
            b.add(place(box(0.05, 0.05, leg + 0.01), loc=(sx * (w / 2 - 0.03), sy * (d / 2 - 0.03), (leg + 0.01) / 2)), M["cedar_plain"], "root")
    # Rim moulding round the top and the base.
    for z, hh in ((z1 - 0.012, 0.024), (z0 + 0.012, 0.024)):
        for sx, sy, x, y in ((w + 0.012, 0.012, 0, -d / 2 - 0.004), (w + 0.012, 0.012, 0, d / 2 + 0.004),
                             (0.012, d + 0.012, -w / 2 - 0.004, 0), (0.012, d + 0.012, w / 2 + 0.004, 0)):
            b.add(place(box(sx, sy, hh), loc=(x, y, z)), M["gold" if z > z0 + 0.05 else "cedar_plain"], "root")
    # Gold corner caps (vertical angle strips).
    for sx in (-1, 1):
        for sy in (-1, 1):
            b.add(place(box(0.03, 0.03, h - 0.04), loc=(sx * (w / 2 - 0.01), sy * (d / 2 - 0.01), (z0 + z1) / 2)), M["gold"], "root")
    # Front: painted frieze band, two ankh plaques and a lock plate with a gem.
    fy = -d / 2 - 0.002
    b.add(place(box(w - 0.06, 0.004, 0.04), loc=(0, fy, z1 - 0.05)), M["frieze"], "root")
    b.add(place(box(w - 0.06, 0.004, 0.012), loc=(0, fy, z0 + 0.04)), M["blue"], "root")
    for x in (-0.24, 0.24):
        b.add(place(box(0.14, 0.004, 0.17), loc=(x, fy, z0 + 0.15)), M["blue"], "root")
        py = fy - 0.004
        b.add(place(box(0.012, 0.004, 0.08), loc=(x, py, z0 + 0.12)), M["gold"], "root")
        b.add(place(box(0.06, 0.004, 0.012), loc=(x, py, z0 + 0.165)), M["gold"], "root")
        loop = [(V((x + 0.018 * math.cos(2 * math.pi * k / 14), py, z0 + 0.195 + 0.024 * math.sin(2 * math.pi * k / 14))), 0.005, 0.003) for k in range(14)]
        b.add(tube(loop, sub=1, n=6, ref=V((0, -1, 0)), closed=True), M["gold"], "root")
    b.add(place(box(0.1, 0.006, 0.1), loc=(0, fy - 0.001, z1 - 0.1)), M["gold"], "root")
    gem(b, M, (0, fy - 0.004, z1 - 0.1), (0, -1, 0), 0.02, 0.028, 0.01)
    # Side handles (gold rings).
    for sx in (-1, 1):
        ring_pts = [(V((sx * (w / 2 + 0.012), 0.035 * math.cos(2 * math.pi * k / 14), z1 - 0.1 + 0.03 * math.sin(2 * math.pi * k / 14))), 0.006, 0.006)
                    for k in range(14)]
        b.add(tube(ring_pts, sub=1, n=6, ref=V((1, 0, 0)), closed=True), M["gold"], "root")
    # Treasure inside: a mound of gold heaped to the rim, coins lying on it, a gem (the tile's item stands in the middle).
    import random
    rnd = random.Random(5)
    mx, my, mh = w / 2 - t - 0.01, d / 2 - t - 0.01, 0.1

    def mound(x, y):
        q = (x / mx) ** 2 + (y / my) ** 2
        return z1 - 0.07 + mh * max(0.0, 1 - q) ** 0.7 - 0.03 * q

    grid = decor.surface(lambda u, v: (mx * (2 * u - 1), my * (2 * v - 1), mound(mx * (2 * u - 1), my * (2 * v - 1))), 16, 8)
    b.add(grid, M["gold_heap"], "root")
    for k in range(60):
        x, y = rnd.uniform(-mx * 0.9, mx * 0.9), rnd.uniform(-my * 0.9, my * 0.9)
        tilt = rot(x=rnd.uniform(-30, 30), y=rnd.uniform(-30, 30))
        b.add(place(spindle([(-0.002, 0.02), (0.002, 0.02)], n=10, sub=1), tilt, (x, y, mound(x, y) + 0.004)), M["gold"], "root")
    gem(b, M, (0.22, -0.06, mound(0.22, -0.06) + 0.012), (0.3, -0.5, 1), 0.025, 0.02, 0.018)
    gem(b, M, (-0.25, 0.05, mound(-0.25, 0.05) + 0.01), (-0.2, -0.4, 1), 0.02, 0.018, 0.015)
    # Lid: vaulted top over a flat frame, hinged on the back top edge and swung open.
    lid = Builder()
    lw, ld = w + 0.02, d + 0.02
    lid.add(place(box(lw, ld, 0.03), loc=(0, 0, 0.015)), M["cedar_grain"], "root")
    arch = []
    for k in range(9):
        a = math.pi * k / 8
        arch.append((ld / 2 * math.cos(a), 0.03 + (LID_H - 0.03) * math.sin(a)))
    lid.add(prism_x(arch, lw - 0.01), M["cedar"], "root")
    lid.add(place(box(lw - 0.12, ld - 0.12, 0.004), loc=(0, 0, -0.002)), M["lining"], "root")
    for x in (-lw / 2 + 0.02, 0, lw / 2 - 0.02):
        lid.add(transform(prism_x([(p[0] * 1.03, p[1] * 1.03) for p in arch], 0.03), lambda v, x=x: V((v.x + x, v.y, v.z))), M["gold"], "root")
    lid.add(place(box(lw - 0.06, 0.004, 0.022), loc=(0, -ld / 2 - 0.002, 0.015)), M["frieze"], "root")
    # Swing it open round the back top edge of the box: the lid's back edge on the hinge, the top turned to +Y.
    hinge = V((0, d / 2 + 0.01, z1))
    r = rot(x=-LID_OPEN)
    lid.verts = [hinge + r @ (V(v) - V((0, ld / 2, 0))) for v in lid.verts]
    merge(b, lid)


def prism_x(pts, length):
    """Polygon pts (y, z) extruded along X over length, centred on x = 0."""
    geo = prism(pts, length)
    return transform(geo, lambda v: V((v.z - length / 2, v.x, v.y)))


def merge(b, other):
    base = len(b.verts)
    b.verts.extend(other.verts)
    for f, uv, mi in zip(other.faces, other.fuv, other.fmat):
        mat = other.mats[mi]
        if mat not in b.mats:
            b.mats.append(mat)
        b.faces.append([base + i for i in f])
        b.fuv.append(uv)
        b.fmat.append(b.mats.index(mat))
    b.weights.extend(other.weights)
    b.tags.extend(other.tags)


BUILDERS = {
    "club": build_club,
    "sword": build_sword,
    "spear": build_spear,
    "bow": build_bow,
    "potion": build_potion,
    "chest": build_chest,
}


# ---------------------------------------------------------------- entry points


def extents(obj):
    vs = obj.data.vertices
    return tuple((min(v.co[a] for v in vs), max(v.co[a] for v in vs)) for a in range(3))


def build(bake=True, tex_dir=None, only=None):
    """Returns {item: obj}; each obj gets obj["texture"] = image name when baked."""
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    M = materials()
    names = [n for n in ITEMS if not only or n in only]
    xs = {name: (k - (len(ITEMS) - 1) / 2) * SPACING for k, name in enumerate(ITEMS)}
    objs = {}
    for name in names:
        b = Builder()
        BUILDERS[name](b, M)
        low = min(v.z for v in b.verts)
        b.verts = [V(v) - V((0, 0, low)) for v in b.verts]
        obj = common.finish_mesh(b, coll, "item_" + name)
        obj.data.set_sharp_from_angle(angle=math.radians(50))
        obj.location = (xs[name], 0, 0)
        common.uv_unwrap(obj, b.tags, boost={"gem": ((0, 0, 0), 1.6)})
        if bake:
            stem = FILES.get(name, name)
            tex = common.bake_texture(obj, os.path.join(tex_dir or bpy.app.tempdir or "/tmp", stem + ".png"), TEX_SIZE, "item_" + stem,
                                      ao_distance=0.05 if name != "chest" else 0.15)
            common.use_baked_material(obj, tex)
            obj["texture"] = tex.name
        (x0, x1), (y0, y1), (z0, z1) = extents(obj)
        print("item_{}: {} verts, {} tris, x {:.3f}..{:.3f}, y {:.3f}..{:.3f}, z {:.3f}..{:.3f}".format(
            name, len(obj.data.vertices), common.tri_count(obj), x0, x1, y0, y1, z0, z1))
        objs[name] = obj
    return objs


def export(objs, models_dir=None):
    models_dir = models_dir or os.path.join(REPO, "models", "items")
    p = os.path.join(REPO, "tools", "blender", "md3_export.py")
    g = {"__file__": p, "__name__": "md3_export"}
    exec(open(p).read(), g)
    for name, obj in objs.items():
        loc = obj.location.copy()
        obj.location = (0, 0, 0)
        bpy.context.view_layer.update()
        g["export_md3"](obj, os.path.join(models_dir, "%s.md3" % FILES.get(name, name)), 0, 0)
        obj.location = loc
    bpy.context.view_layer.update()


# ---------------------------------------------------------------- review


def review(path, objs):
    """Front and three-quarter line-ups, each item scaled to the same height (as the inventory shows them)."""
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    if scene.world is None:
        scene.world = bpy.data.worlds.new("World")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.25, 0.22, 0.18, 1)
    scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = 1.0
    scene.view_settings.view_transform = "Standard"
    for o in list(scene.objects):
        if o.name.startswith("review_"):
            bpy.data.objects.remove(o)
    sun = bpy.data.objects.new("review_sun", bpy.data.lights.new("review_sun", "SUN"))
    sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(50), 0, math.radians(-30))
    scene.collection.objects.link(sun)
    cam = bpy.data.objects.new("review_cam", bpy.data.cameras.new("review_cam"))
    scene.collection.objects.link(cam)
    scene.camera = cam
    cam.data.type = "ORTHO"
    # Scale each item so its largest dimension is 0.8 (like Centrify) and line them up.
    saved = {}
    for k, (name, obj) in enumerate(objs.items()):
        (x0, x1), (y0, y1), (z0, z1) = extents(obj)
        s = 0.8 / max(x1 - x0, y1 - y0, z1 - z0)
        saved[name] = (obj.location.copy(), obj.scale.copy())
        obj.scale = (s, s, s)
        obj.location = (k * 1.0 - (x0 + x1) / 2 * s, -(y0 + y1) / 2 * s, 0)
    n = len(objs)
    scene.render.resolution_x, scene.render.resolution_y = 260 * n, 300
    stem = os.path.splitext(path)[0]
    for suffix, rx, rz in (("", 90, 0), ("_34", 70, 35)):
        for o in objs.values():
            o.rotation_euler = (0, 0, 0)
        cam.rotation_euler = (math.radians(rx), 0, math.radians(rz))
        d = V((0, 0, -1))
        d.rotate(cam.rotation_euler)
        cam.location = V(((n - 1) / 2, 0, 0.42)) - d * 6
        cam.data.ortho_scale = n * 1.0 * (1.0 if not rz else 1.1)
        if rz:  # turn each item instead of the camera so the line-up stays side by side
            cam.rotation_euler = (math.radians(rx), 0, 0)
            d = V((0, 0, -1))
            d.rotate(cam.rotation_euler)
            cam.location = V(((n - 1) / 2, 0, 0.5)) - d * 6
            for o in objs.values():
                o.rotation_euler = (0, 0, math.radians(-rz))
        scene.render.filepath = stem + suffix + ".png"
        bpy.ops.render.render(write_still=True)
        print("review ->", scene.render.filepath)
    for name, obj in objs.items():
        obj.location, obj.scale = saved[name]
        obj.rotation_euler = (0, 0, 0)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    only = args[args.index("--only") + 1].split(",") if "--only" in args else None
    exporting = "--export" in args
    tex_dir = os.path.join(REPO, "textures", "items")
    objs = build(bake=exporting or "--bake" in args, tex_dir=tex_dir if exporting else None, only=only)
    if "--review" in args:
        review(args[args.index("--review") + 1], objs)
    if exporting:
        export(objs)
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "items.blend"))
