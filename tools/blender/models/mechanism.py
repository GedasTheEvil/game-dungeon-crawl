"""Procedural level mechanics: coloured keys, key gates, wall levers, falling rocks and the ceiling crack over a rock trap.

    MCP:  p = ".../tools/blender/models/mechanism.py"; g = {"__file__": p, "__name__": "mechanism"}
          exec(open(p).read(), g); g["build"](bake=False)      # then g["export"](objs)
    CLI:  blender -b --python tools/blender/models/mechanism.py -- [--export] [--bake] [--review out.png] [--only key,gate]

Same space as decor.py: Z up, 1 unit = 1 tile, no Centrify (the engine draws them at glScale 40). Tile frame
(gate, lever_base, ceiling_crack): origin = floor level, horizontal centre of the tile, on the back wall plane;
the tile runs from y = 0 (back wall) to y = -1 (front opening, the camera side), the ceiling is at z = 1.
Game space of the MD3 = (x, z, -y), so Dungeon::drawDecorTile's transform (translate TILE_HALF, 0, -TILE_SIZE;
scale TILE_SIZE) puts these where they belong.

Free models, origin on their own axis (x = y = 0, lowest point z = 0), to be drawn from the tile centre
(translate TILE_HALF, lift, -TILE_HALF like the old ankh item) instead of the back wall:
  key   - upright ankh key; spin axis = the shaft (Z through the origin). Engine lift ~0.35 tile.
  rock  - boulder; centre of its bulk at z ~ ROCK_CENTRE_Z.
lever_handle origin = its pivot; draw it in the lever_base frame translated by LEVER_PIVOT, then rotated around
the depth axis (game Z; +35 deg in glRotatef tips the grip to -x as seen by the camera). At 0 deg it points up (+Z).

Textures: one PNG per model and lock colour (key, gate, lever_base: textures/mechanisms/<model>_<colour>.png,
same UV layout, only the gems / painted accents differ) or one per model (lever_handle, rock, ceiling_crack).
Lighting is baked like decor.py (suns from the camera side and above + sky); the gate gets two suns from the
left and right front so both its long sides read, the key a front and a back sun because it spins.
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
from common import REPO, Builder, ellipsoid, transform, tube  # noqa: E402
from decor import blob, box, ground, place, prism, revolve, rod, rot  # noqa: E402

COLL = "mechanism_new"
MODELS = ["key", "gate", "lever_base", "lever_handle", "rock", "ceiling_crack"]
COLOURED = {"key", "gate", "lever_base"}  # one texture per lock colour
LOCK_COLOURS = ["red", "blue", "green", "gold"]  # lock colour 1..4
TEX_SIZE = {"key": 256, "gate": 512, "lever_base": 256, "lever_handle": 256, "rock": 256, "ceiling_crack": 256}
SPACING = 1.4  # models are spread along X in the scene (bake/review only; export is at the origin)
BAKE_LIFT = {"key": 0.35, "rock": 0.3}  # free models hang in the air while baking, as in game

LEVER_PIVOT = V((0.0, -0.05, 0.42))  # handle pivot in the lever_base frame (tile units, Blender axes)
LEVER_ANGLE = 35.0  # the engine swings the handle between -LEVER_ANGLE (off) and +LEVER_ANGLE (pulled)
ROCK_CENTRE_Z = 0.19

# Gem colour, glint and painted accent per lock colour.
GEMS = {
    "red": {"gem": (0.62, 0.05, 0.02), "glint": (1.0, 0.62, 0.5), "accent": (0.50, 0.06, 0.03)},  # carnelian
    "blue": {"gem": (0.05, 0.14, 0.70), "glint": (0.62, 0.74, 1.0), "accent": (0.05, 0.12, 0.50)},  # lapis lazuli
    "green": {"gem": (0.03, 0.62, 0.42), "glint": (0.62, 1.0, 0.84), "accent": (0.04, 0.42, 0.30)},  # turquoise
    "gold": {"gem": (1.0, 0.84, 0.08), "glint": (1.0, 0.97, 0.75), "accent": (0.92, 0.70, 0.05)},  # yellow amber
}

COL = {
    "gold": (0.78, 0.55, 0.16),
    "gold_dark": (0.46, 0.30, 0.08),
    "bronze": (0.44, 0.28, 0.10),
    "bronze_dark": (0.24, 0.14, 0.05),
    "verdigris": (0.14, 0.34, 0.26),
    "sandstone": (0.62, 0.48, 0.29),
    "sandstone_dark": (0.40, 0.29, 0.16),
    "sandstone_light": (0.74, 0.60, 0.38),
    "crack": (0.05, 0.035, 0.02),
    "sand": (0.76, 0.60, 0.35),
    "enamel": (0.02, 0.015, 0.01),
    "wood": (0.30, 0.18, 0.08),
    "gem": GEMS["red"]["gem"],
    "glint": GEMS["red"]["glint"],
    "accent": GEMS["red"]["accent"],
}


def materials():
    stripes = decor.stripes
    specs = {
        "gold": stripes("v", 0.02, [(4, "gold"), (1, "gold_dark")], "LINEAR"),
        "gold_plain": ("solid", "gold"),
        "bronze": stripes("v", 0.05, [(3, "bronze"), (1, "verdigris")], "LINEAR"),
        "bronze_plain": ("solid", "bronze"),
        "bronze_dark": ("solid", "bronze_dark"),
        "stone": ("solid", "sandstone"),
        "stone_dark": ("solid", "sandstone_dark"),
        "stone_light": ("solid", "sandstone_light"),
        "crack": ("solid", "crack"),
        "sand": ("solid", "sand"),
        "enamel": ("solid", "enamel"),
        "wood": stripes("u", 1 / 7, [(3, "wood"), (1, "bronze_dark")], "LINEAR"),
        "gem": ("solid", "gem"),
        "glint": ("solid", "glint"),
        "accent": ("solid", "accent"),
        "frieze": stripes("u", 0.05, [(3, "accent"), (1, "gold")]),
    }
    return {name: common.make_material("mech_" + name, spec, COL) for name, spec in specs.items()}


def set_lock_colour(M, colour):
    """Recolour the gem materials in place (same meshes and UVs, a new bake per colour)."""
    g = GEMS[colour]
    for key in ("gem", "glint", "accent"):
        M[key].node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (*g[key], 1.0)
    ramp = next(n for n in M["frieze"].node_tree.nodes if n.type == "VALTORGB")
    ramp.color_ramp.elements[0].color = (*g["accent"], 1.0)


# ---------------------------------------------------------------- helpers


def ring(centre, rx, rz, r_rad, r_depth, normal, n=10, steps=24):
    """Closed oval tube in the plane perpendicular to normal (normal = +-Y or +-X), radii rx (horizontal), rz."""
    c, nrm = V(centre), V(normal).normalized()
    side = V((0, 0, 1)).cross(nrm)
    keys = [(c + side * (rx * math.cos(2 * math.pi * k / steps)) + V((0, 0, rz * math.sin(2 * math.pi * k / steps))), r_rad, r_depth)
            for k in range(steps)]
    return tube(keys, sub=1, n=n, ref=nrm, closed=True)


def gem(b, M, centre, normal, rx, rz, depth, bezel=0.006, n=14, rings=6):
    """Cabochon gem set in a black enamel bezel, domed along normal, with a small glint (upper left as seen from the front)."""
    c, nrm = V(centre), V(normal).normalized()
    up = V((0, 0, 1))
    b.add(ellipsoid(c, (rx + bezel, rz + bezel, depth * 0.5), n=n, rings=4, axis=nrm, ref=up), M["enamel"], "root", "gem")
    b.add(ellipsoid(c, (rx, rz, depth), n=n, rings=rings, axis=nrm, ref=up), M["gem"], "root", "gem")
    left = nrm.cross(up)  # -x as seen by a viewer looking along -normal
    g = c + nrm * (depth * 0.82) + up * (rz * 0.38) + left * (rx * 0.3)
    s = min(rx, rz) * 0.22
    b.add(ellipsoid(g, (s, s * 1.3, s * 0.5), n=6, rings=3, axis=nrm, ref=up), M["glint"], "root", "gem")


def lumpy(c, r, seed, amp=0.22, n=10, rings=7, waves=5):
    """Lumpy ellipsoid like decor.rock, without the floor clamp."""
    rnd = random.Random(seed)
    ws = [(V((rnd.gauss(0, 1), rnd.gauss(0, 1), rnd.gauss(0, 1))).normalized(), rnd.uniform(0, 6.28), rnd.uniform(2.0, 4.5)) for _ in range(waves)]
    c = V(c)

    def fn(v):
        d = V(v) - c
        u = V((d.x / r[0], d.y / r[1], d.z / r[2]))
        un = u.normalized() if u.length > 1e-9 else V((0, 0, 1))
        return c + d * (1 + amp * sum(math.sin(fr * un.dot(a) + ph) for a, ph, fr in ws) / len(ws))

    return transform(ellipsoid(c, r, n, rings), fn)


def slab(pts, z0, z1):
    """Prism of polygon pts (x, y) between z0 and z1."""
    return place(prism(pts, z1 - z0), loc=(0, 0, z0))


def yz_box(y0, y1, z0, z1, half_x, x=0.0):
    """Box spanning y0..y1, z0..z1, x +- half_x."""
    return place(box(2 * half_x, y1 - y0, z1 - z0), loc=(x, (y0 + y1) / 2, (z0 + z1) / 2))


# ---------------------------------------------------------------- models


def build_key(b, M):
    """Upright ankh key: the loop of the ankh is the bow with an oval gem, the arms are the grip, the stem is the shaft
    with a two-tooth bit. Face of the ankh in the x-z plane (towards -Y)."""
    gold = M["gold"]
    shaft = [((0, 0, 0.0), 0.010, 0.010), ((0, 0, 0.012), 0.011, 0.011), ((0, 0, 0.075), 0.011, 0.011), ((0, 0, 0.08), 0.016, 0.016),
             ((0, 0, 0.092), 0.016, 0.016), ((0, 0, 0.097), 0.011, 0.011), ((0, 0, 0.15), 0.013, 0.013), ((0, 0, 0.185), 0.017, 0.012)]
    b.add(tube([(V(p), rx, ry) for p, rx, ry in shaft], sub=1, n=12, ref=V((0, 1, 0)), bulge=(0.003, 0.0)), gold, "root")
    # Bit: two teeth and a ward notch on the +x side.
    b.add(place(box(0.034, 0.012, 0.014), loc=(0.024, 0, 0.012)), gold, "root")
    b.add(place(box(0.026, 0.012, 0.012), loc=(0.02, 0, 0.042)), gold, "root")
    b.add(place(box(0.012, 0.013, 0.05), loc=(0.036, 0, 0.027)), gold, "root")
    # Ankh arms, flared at the ends.
    arms = [((-0.066, 0, 0.19), 0.022, 0.009), ((-0.05, 0, 0.19), 0.017, 0.009), ((-0.02, 0, 0.19), 0.011, 0.01), ((0.0, 0, 0.19), 0.012, 0.011),
            ((0.02, 0, 0.19), 0.011, 0.01), ((0.05, 0, 0.19), 0.017, 0.009), ((0.066, 0, 0.19), 0.022, 0.009)]
    b.add(tube([(V(p), rx, ry) for p, rx, ry in arms], sub=2, n=10, ref=V((0, 1, 0)), bulge=(0.002, 0.002)), gold, "root")
    # Loop (bow) with the gem filling it on both faces.
    lc, lrx, lrz, lw = V((0, 0, 0.252)), 0.037, 0.052, 0.011
    b.add(ring(lc, lrx, lrz, lw, 0.009, (0, -1, 0), n=10, steps=28), gold, "root")
    b.add(ellipsoid(lc, (lrx - lw * 0.6, lrz - lw * 0.6, 0.006), n=16, rings=4, axis=V((0, 1, 0)), ref=V((0, 0, 1))), M["enamel"], "root", "gem")
    for s in (-1, 1):
        gem(b, M, lc, (0, s, 0), lrx - lw - 0.001, lrz - lw - 0.001, 0.011, bezel=0.0015)
    # Collar where the loop meets the arms.
    b.add(ellipsoid((0, 0, 0.2), (0.016, 0.012, 0.01), n=10, rings=4), M["gold_plain"], "root")


GATE_HALF = 0.085  # half thickness in x of the frame
GATE_TOP = 0.86  # underside of the stone lintel


def build_gate(b, M):
    """Bronze portcullis across the corridor (plane y-z at x = 0): stone lintel under the ceiling and a stone stile
    against the back wall, a bronze-clad front stile with the lock cartouche facing the camera, a grid of bars with
    spiked feet, and a gem medallion on both long sides."""
    h = GATE_HALF
    # Stone lintel under the ceiling, painted frieze in the lock colour along both long sides and the front.
    b.add(yz_box(-0.99, 0.0, GATE_TOP, 1.0, h + 0.015), M["stone"], "root")
    for s in (-1, 1):
        b.add(yz_box(-0.97, -0.03, 0.905, 0.955, 0.004, x=s * (h + 0.017)), M["accent"], "root")
        b.add(yz_box(-0.97, -0.03, 0.885, 0.895, 0.003, x=s * (h + 0.017)), M["gold_plain"], "root")
        b.add(yz_box(-0.97, -0.03, 0.965, 0.975, 0.003, x=s * (h + 0.017)), M["gold_plain"], "root")
    b.add(place(box(2 * h + 0.02, 0.012, 0.05), loc=(0, -0.994, 0.93)), M["accent"], "root")
    # Stone stile against the back wall (with a groove line) and the front stile.
    b.add(yz_box(-0.13, 0.0, 0.0, GATE_TOP, h), M["stone"], "root")
    b.add(yz_box(-0.135, -0.125, 0.0, GATE_TOP, h + 0.004), M["stone_dark"], "root")
    b.add(yz_box(-1.0 + 0.03, -0.87, 0.0, GATE_TOP, h), M["bronze"], "root")
    for z0, z1 in ((0.0, 0.05), (GATE_TOP - 0.05, GATE_TOP)):
        b.add(yz_box(-0.995, -0.865, z0, z1, h + 0.008), M["bronze_dark"], "root")
    # Cartouche on the front face: gold oval with the gem, a tie bar underneath, studs above.
    cz = 0.52
    front = -1.0 + 0.03
    b.add(yz_box(front - 0.004, front, cz - 0.17, cz + 0.17, h - 0.012), M["gold_plain"], "root")
    b.add(ring((0, front - 0.012, cz), 0.052, 0.12, 0.011, 0.009, (0, -1, 0), n=8, steps=28), M["gold"], "root")
    b.add(place(box(0.11, 0.016, 0.018), loc=(0, front - 0.012, cz - 0.145)), M["gold"], "root")
    gem(b, M, (0, front - 0.004, cz), (0, -1, 0), 0.036, 0.098, 0.018, bezel=0.004)
    for z in (0.12, 0.3, 0.74):
        b.add(ellipsoid((0, front - 0.004, z), (0.014, 0.014, 0.008), n=8, rings=3, axis=V((0, -1, 0)), ref=V((0, 0, 1))), M["gold_plain"], "root")
    # Grid: vertical bars with spiked feet, flat cross straps with rivets.
    ys = [-0.8 + k * (0.6 / 5) for k in range(6)]
    for y in ys:
        b.add(rod((0, y, 0.07), (0, y, GATE_TOP + 0.01), 0.018, n=8, caps=(True, False)), M["bronze"], "root")
        b.add(tube([(V((0, y, 0.075)), 0.02, 0.02), (V((0, y, 0.04)), 0.013, 0.013), (V((0, y, 0.0)), 0.002, 0.002)], sub=1, n=8,
                   ref=V((1, 0, 0)), caps=(False, False)), M["bronze_dark"], "root")
    for z in (0.2, 0.45, 0.7):
        b.add(yz_box(-0.88, -0.12, z - 0.022, z + 0.022, 0.028), M["bronze_plain"], "root")
        for y in ys:
            for s in (-1, 1):
                b.add(ellipsoid((s * 0.03, y, z), (0.009, 0.009, 0.006), n=6, rings=3, axis=V((s, 0, 0)), ref=V((0, 0, 1))), M["gold_plain"], "root")
    # Medallion with the lock gem on both long sides (seen when the gate is left or right of the player).
    for s in (-1, 1):
        mc = V((s * 0.03, -0.5, 0.575))
        b.add(ring(mc, 0.075, 0.075, 0.013, 0.012, (s, 0, 0), n=8, steps=24), M["gold"], "root")
        b.add(ellipsoid(mc, (0.07, 0.07, 0.012), n=16, rings=3, axis=V((s, 0, 0)), ref=V((0, 0, 1))), M["bronze_dark"], "root")
        gem(b, M, mc + V((s * 0.008, 0, 0)), (s, 0, 0), 0.046, 0.046, 0.02, bezel=0.006)


def build_lever_base(b, M):
    """Stone plate on the back wall with a bronze border, the slot the handle swings in, the bearing boss and the lock gem."""
    zc, hw, hh = 0.4, 0.12, 0.16
    b.add(yz_box(-0.03, 0.0, zc - hh, zc + hh, hw), M["stone"], "root")
    corners = [V((-hw, -0.033, zc - hh)), V((hw, -0.033, zc - hh)), V((hw, -0.033, zc + hh)), V((-hw, -0.033, zc + hh))]
    for k in range(4):
        b.add(rod(corners[k], corners[(k + 1) % 4], 0.008, n=6), M["bronze"], "root")
    for c in corners:
        b.add(ellipsoid(c + V((0, -0.002, 0)), (0.012, 0.012, 0.012), n=8, rings=4), M["gold_plain"], "root")
    # Frieze along the top in the lock colour.
    b.add(yz_box(-0.034, -0.029, zc + hh - 0.045, zc + hh - 0.02, hw - 0.02), M["frieze"], "root")
    # Slot: dark arc above the pivot, stops at both ends.
    p = LEVER_PIVOT
    arc_r = 0.085
    pts = []
    for k in range(13):
        a = math.radians(-LEVER_ANGLE - 6 + (2 * LEVER_ANGLE + 12) * k / 12)
        pts.append(V((-math.sin(a) * arc_r, -0.031, p.z + math.cos(a) * arc_r)))
    for a, e in zip(pts, pts[1:]):
        b.add(rod(a, e, 0.011, n=6), M["enamel"], "root")
    for a in (-LEVER_ANGLE - 12, LEVER_ANGLE + 12):
        r = math.radians(a)
        b.add(ellipsoid((-math.sin(r) * arc_r, -0.034, p.z + math.cos(r) * arc_r), (0.009, 0.009, 0.009), n=8, rings=4), M["bronze_plain"], "root")
    # Bearing boss the handle turns on.
    boss = [(0.0, 0.0), (0.034, 0.0), (0.034, 0.008), (0.028, 0.016), (0.0, 0.016)]
    revolve(b, M, boss, ["bronze_plain"] * 4, n=16, r=rot(x=90), loc=(p.x, -0.03 - 0.004, p.z))
    # Gem below the pivot.
    gem(b, M, (0, -0.03, zc - 0.095), (0, -1, 0), 0.034, 0.028, 0.016, bezel=0.006)


def build_lever_handle(b, M):
    """Handle: collar on the pivot (the origin), bronze arm pointing up (+Z), wooden grip knob."""
    collar = [(0.0, 0.0), (0.024, 0.0), (0.026, 0.006), (0.026, 0.018), (0.02, 0.024), (0.0, 0.024)]
    revolve(b, M, collar, ["bronze_plain"] * 5, n=16, r=rot(x=90), loc=(0, 0.002, 0))
    b.add(ellipsoid((0, -0.024, 0), (0.008, 0.008, 0.005), n=8, rings=3, axis=V((0, -1, 0)), ref=V((0, 0, 1))), M["gold_plain"], "root")
    b.add(tube([(V((0, -0.012, 0.0)), 0.013, 0.01), (V((0, -0.012, 0.05)), 0.01, 0.008), (V((0, -0.012, 0.17)), 0.008, 0.008)], sub=2, n=8,
               ref=V((0, 1, 0)), caps=(False, True)), M["bronze"], "root")
    b.add(tube([(V((0, -0.012, 0.16)), 0.012, 0.012), (V((0, -0.012, 0.175)), 0.019, 0.019), (V((0, -0.012, 0.21)), 0.021, 0.021),
                (V((0, -0.012, 0.235)), 0.014, 0.014)], sub=2, n=12, ref=V((0, 1, 0)), bulge=(0.0, 0.008)), M["wood"], "root")
    b.add(tube([(V((0, -0.012, 0.155)), 0.013, 0.013), (V((0, -0.012, 0.168)), 0.013, 0.013)], sub=1, n=12, ref=V((0, 1, 0))), M["gold_plain"], "root")


def build_rock(b, M):
    """Chunk of ceiling sandstone broken off along a few fracture planes, one of them the flat dressed face of the
    old ceiling underside, with smaller lumps stuck to it."""
    c = V((0, 0, ROCK_CENTRE_Z))
    geo = lumpy(c, (0.2, 0.18, 0.17), 7, amp=0.16, n=14, rings=9)
    # (normal, distance from the centre): the first is the dressed face, the others fractures.
    planes = [(V((0.2, 0.3, 0.93)), 0.115), (V((0.9, -0.3, 0.2)), 0.15), (V((-0.7, -0.6, 0.3)), 0.14), (V((-0.4, 0.8, -0.3)), 0.13),
              (V((0.3, -0.5, -0.8)), 0.13), (V((-0.6, 0.1, -0.8)), 0.14), (V((0.5, 0.75, -0.1)), 0.14)]
    planes = [(n.normalized(), d) for n, d in planes]

    def cut(v):
        p = V(v)
        for nrm, dist in planes:
            d = (p - c).dot(nrm) - dist
            if d > 0:
                p = p - nrm * d
        return p

    b.add(transform(geo, cut), M["stone"], "root")
    for k, (off, r) in enumerate((((0.12, -0.08, -0.07), (0.06, 0.05, 0.05)), ((-0.12, -0.09, 0.03), (0.05, 0.045, 0.045)),
                                   ((-0.03, 0.1, -0.1), (0.06, 0.05, 0.045)))):
        b.add(lumpy(c + V(off), r, 20 + k, amp=0.25, n=7, rings=4), M["stone_dark" if k % 2 else "stone_light"], "root")


def build_ceiling_crack(b, M):
    """Loose, cracked ceiling stones sagging out of the ceiling (z = 1) over a rock trap, dark gaps between them,
    hairline cracks running out and a thin trickle of sand."""
    top = 1.0
    b.add(place(slab(blob(0.27, 5, n=9, jitter=0.15), top - 0.002, top + 0.02), loc=(0, -0.55, 0)), M["crack"], "root")
    rnd = random.Random(3)
    stones = [((-0.13, -0.42), 0.11, 0.07, 8), ((0.03, -0.36), 0.1, 0.09, 12), ((0.17, -0.47), 0.09, 0.06, -6), ((-0.06, -0.6), 0.1, 0.1, 5),
              ((0.12, -0.66), 0.085, 0.075, -10), ((-0.2, -0.64), 0.07, 0.05, 14), ((0.0, -0.8), 0.08, 0.05, -4)]
    for k, ((x, y), r, sag, tilt) in enumerate(stones):
        geo = prism(blob(r, 40 + k, n=6, jitter=0.2), sag + 0.02)
        geo = place(geo, rot(x=rnd.uniform(-1, 1) * abs(tilt), y=tilt), (x, y, top - sag))
        b.add(geo, M["stone_light" if k % 3 == 0 else ("stone" if k % 3 == 1 else "stone_dark")], "root")
    # Hairline cracks radiating from the patch.
    for k in range(7):
        a = 2 * math.pi * k / 7 + rnd.uniform(-0.3, 0.3)
        p = V((0.0, -0.55, top - 0.006))
        pts = [p]
        for s in range(4):
            a += rnd.uniform(-0.5, 0.5)
            p = p + V((math.cos(a) * 0.07, math.sin(a) * 0.06, 0))
            pts.append(p)
        for u, w in zip(pts[1:], pts[2:]):
            b.add(rod(u, w, 0.006, 0.004, n=4), M["crack"], "root")
    # Pebble about to drop and a sand trickle.
    b.add(lumpy((0.07, -0.52, top - 0.075), (0.022, 0.02, 0.018), 9, n=8, rings=5), M["stone_dark"], "root")
    b.add(tube([(V((-0.03, -0.5, top - 0.05)), 0.006, 0.006), (V((-0.03, -0.5, top - 0.12)), 0.003, 0.003), (V((-0.03, -0.5, top - 0.2)), 0.0012, 0.0012)],
               sub=2, n=6, ref=V((1, 0, 0)), caps=(True, True)), M["sand"], "root")


BUILDERS = {
    "key": build_key,
    "gate": build_gate,
    "lever_base": build_lever_base,
    "lever_handle": build_lever_handle,
    "rock": build_rock,
    "ceiling_crack": build_ceiling_crack,
}

# Suns per model (x tilt deg, z turn deg, energy) and sky strength; default = decor.py's light.
LIGHT = {
    "key": ([(50, 0, 0.55), (50, 180, 0.4)], 0.55),
    "gate": ([(50, 35, 0.5), (50, -35, 0.5)], 0.45),
    "rock": ([(50, 0, 0.7), (60, 150, 0.25)], 0.5),
    "ceiling_crack": ([(-30, 0, 0.5), (60, 0, 0.3)], 0.6),
}


# ---------------------------------------------------------------- texture


def bake_lit(obj, out_path, name, size, suns, sky):
    """Bake colour x light into one PNG (like decor.bake_lit, with configurable suns)."""
    import numpy as np

    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 96
    if scene.world is None:
        scene.world = bpy.data.worlds.new("World")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (1, 1, 1, 1)
    scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = sky
    for o in list(scene.collection.objects):
        if o.name.startswith("mech_sun"):
            bpy.data.objects.remove(o)
    for k, (rx, rz, energy) in enumerate(suns):
        sun = bpy.data.objects.new("mech_sun%d" % k, bpy.data.lights.new("mech_sun%d" % k, "SUN"))
        scene.collection.objects.link(sun)
        sun.data.energy = energy
        sun.data.angle = math.radians(10)
        sun.rotation_euler = (math.radians(rx), 0, math.radians(rz))

    img = bpy.data.images.new(name + "_bake", size, size, alpha=False)
    for m in obj.data.materials:
        node = m.node_tree.nodes.get("bake_target") or m.node_tree.nodes.new("ShaderNodeTexImage")
        node.name = "bake_target"
        node.image = img
        m.node_tree.nodes.active = node
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.bake(type="DIFFUSE", pass_filter={"COLOR", "DIRECT", "INDIRECT"}, margin=4, use_clear=True)
    for m in obj.data.materials:
        m.node_tree.nodes.remove(m.node_tree.nodes["bake_target"])
    px = np.clip(np.array(img.pixels[:]), 0.0, 1.0)
    tex = bpy.data.images.get(name) or bpy.data.images.new(name, size, size, alpha=False)
    tex.pixels = px.tolist()
    tex.filepath_raw = out_path
    tex.file_format = "PNG"
    tex.save()
    bpy.data.images.remove(img)
    return tex


# ---------------------------------------------------------------- entry points


def build_env(coll, xs):
    """Floor and back wall under the line-up (AO occluders); a ceiling slab over the ceiling crack only."""
    half = SPACING * len(MODELS) / 2 + 0.5
    verts = [(-half, -1.0, 0), (half, -1.0, 0), (half, 0, 0), (-half, 0, 0), (-half, 0, 1.2), (half, 0, 1.2)]
    faces = [[0, 1, 2, 3], [3, 2, 5, 4]]
    if "ceiling_crack" in xs:
        x = xs["ceiling_crack"]
        base = len(verts)
        verts += [(x - 0.6, -1.0, 1.0), (x + 0.6, -1.0, 1.0), (x + 0.6, 0, 1.0), (x - 0.6, 0, 1.0)]
        faces.append([base, base + 3, base + 2, base + 1])
    mesh = bpy.data.meshes.new("mech_env")
    mesh.from_pydata(verts, [], faces)
    obj = bpy.data.objects.new("mech_env", mesh)
    coll.objects.link(obj)
    return obj


def extents(obj):
    vs = obj.data.vertices
    return tuple((min(v.co[a] for v in vs), max(v.co[a] for v in vs)) for a in range(3))


def build(bake=True, tex_dir=None, only=None):
    """Returns {model: obj}; each obj gets obj["textures"] = {variant: image name} when baked."""
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    env = bpy.data.meshes.get("mech_env")
    if env:
        bpy.data.meshes.remove(env)
    M = materials()
    names = [n for n in MODELS if not only or n in only]
    xs = {name: (k - (len(MODELS) - 1) / 2) * SPACING for k, name in enumerate(MODELS)}
    build_env(coll, xs)
    objs = {}
    for name in names:
        b = Builder()
        BUILDERS[name](b, M)
        if name in BAKE_LIFT:  # free models: lowest point on z = 0
            low = min(v.z for v in b.verts)
            b.verts = [V(v) - V((0, 0, low)) for v in b.verts]
        obj = common.finish_mesh(b, coll, "mech_" + name)
        obj.data.set_sharp_from_angle(angle=math.radians(50 if name != "rock" else 35))
        obj.location = (xs[name], 0, BAKE_LIFT.get(name, 0.0))
        common.uv_unwrap(obj, b.tags, boost={"gem": ((0, 0, 0), 1.6)})
        if bake:
            suns, sky = LIGHT.get(name, ([(50, 0, 0.75)], 0.45))
            texs = {}
            for colour in (LOCK_COLOURS if name in COLOURED else [None]):
                if colour:
                    set_lock_colour(M, colour)
                stem = name + ("_" + colour if colour else "")
                tex = bake_lit(obj, os.path.join(tex_dir or bpy.app.tempdir or "/tmp", stem + ".png"), "mech_" + stem, TEX_SIZE[name], suns, sky)
                texs[colour or ""] = tex.name
            set_lock_colour(M, "red")
            common.use_baked_material(obj, bpy.data.images[texs[LOCK_COLOURS[0] if name in COLOURED else ""]])
            obj["textures"] = texs
        (x0, x1), (y0, y1), (z0, z1) = extents(obj)
        print("mech_{}: {} verts, {} tris, x {:.3f}..{:.3f}, y {:.3f}..{:.3f}, z {:.3f}..{:.3f}".format(
            name, len(obj.data.vertices), common.tri_count(obj), x0, x1, y0, y1, z0, z1))
        objs[name] = obj
    return objs


def export(objs, models_dir=None):
    models_dir = models_dir or os.path.join(REPO, "models", "mechanisms")
    os.makedirs(models_dir, exist_ok=True)
    p = os.path.join(REPO, "tools", "blender", "md3_export.py")
    g = {"__file__": p, "__name__": "md3_export"}
    exec(open(p).read(), g)
    for name, obj in objs.items():
        loc = obj.location.copy()
        obj.location = (0, 0, 0)
        bpy.context.view_layer.update()
        g["export_md3"](obj, os.path.join(models_dir, "%s.md3" % name), 0, 0)
        obj.location = loc
    bpy.context.view_layer.update()


# ---------------------------------------------------------------- review


def _unlit(image, name):
    """Textured-only material (the engine draws these props without lighting of their own)."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nt = mat.node_tree
    for node in list(nt.nodes):
        nt.nodes.remove(node)
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    em = nt.nodes.new("ShaderNodeEmission")
    tex = nt.nodes.new("ShaderNodeTexImage")
    tex.image = image
    nt.links.new(tex.outputs["Color"], em.inputs["Color"])
    nt.links.new(em.outputs["Emission"], out.inputs["Surface"])
    return mat


def _instance(scene, src, variant, loc, rz=0.0, ry=0.0):
    """Linked copy of src at loc with its baked texture for variant (unlit)."""
    o = src.copy()
    o.data = src.data.copy()
    o.name = "review_%s_%s" % (src.name, variant or "x")
    texs = src.get("textures")
    if texs:
        o.data.materials.clear()
        o.data.materials.append(_unlit(bpy.data.images[texs[variant or ""]], "review_mat_" + texs[variant or ""]))
    o.location = V(loc)
    o.rotation_euler = (0, math.radians(ry), math.radians(rz))
    o.hide_render = False
    scene.collection.objects.link(o)
    return o


def _corridor(scene, x0, x1):
    """Corridor from x0 to x1 in tile units: floor, back wall, ceiling, with the game's wall texture."""
    path = os.path.join(REPO, "textures", "dungeon", "wallback.png")
    img = bpy.data.images.load(path, check_existing=True)
    mesh = bpy.data.meshes.new("review_corridor")
    verts = [(x0, -1, 0), (x1, -1, 0), (x1, 0, 0), (x0, 0, 0), (x0, 0, 1), (x1, 0, 1), (x0, -1, 1), (x1, -1, 1)]
    mesh.from_pydata(verts, [], [[0, 1, 2, 3], [3, 2, 5, 4], [4, 5, 7, 6]])
    uv = mesh.uv_layers.new(name="UVMap")
    tiles = x1 - x0
    uvs = [(0, 0), (tiles, 0), (tiles, 1), (0, 1), (0, 0), (tiles, 0), (tiles, 1), (0, 1), (0, 0), (tiles, 0), (tiles, 1), (0, 1)]
    uv.data.foreach_set("uv", [c for p in uvs for c in p])
    mat = bpy.data.materials.new("review_wall")
    nt = mat.node_tree
    for node in list(nt.nodes):
        nt.nodes.remove(node)
    out, em, tex = (nt.nodes.new(t) for t in ("ShaderNodeOutputMaterial", "ShaderNodeEmission", "ShaderNodeTexImage"))
    tex.image = img
    em.inputs["Strength"].default_value = 0.55
    nt.links.new(tex.outputs["Color"], em.inputs["Color"])
    nt.links.new(em.outputs["Emission"], out.inputs["Surface"])
    mesh.materials.append(mat)
    obj = bpy.data.objects.new("review_corridor", mesh)
    scene.collection.objects.link(obj)
    # Dark rock above and below the corridor, like the solid cells in game.
    rock = bpy.data.meshes.new("review_rock")
    rock.from_pydata([(x0, -1, 1), (x1, -1, 1), (x1, -1, 2.5), (x0, -1, 2.5), (x0, -1, -1.5), (x1, -1, -1.5), (x1, -1, 0), (x0, -1, 0)], [],
                     [[0, 1, 2, 3], [4, 5, 6, 7]])
    m2 = bpy.data.materials.new("review_black")
    m2.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.02, 0.015, 0.01, 1)
    rock.materials.append(m2)
    scene.collection.objects.link(bpy.data.objects.new("review_rock", rock))
    # Player stand-in for scale (~0.37 tile tall, walks at y = -0.5).
    bpy.ops.mesh.primitive_cylinder_add(radius=0.06, depth=0.37, location=(0, -0.5, 0.185))
    p = bpy.context.object
    p.name = "review_player"
    m3 = bpy.data.materials.new("review_player")
    m3.node_tree.nodes["Principled BSDF"].inputs["Base Color"].default_value = (0.3, 0.18, 0.1, 1)
    p.data.materials.append(m3)


def _clear_review(scene):
    for o in list(bpy.data.objects):
        if o.name.startswith("review_"):
            data = o.data
            bpy.data.objects.remove(o)
            if isinstance(data, bpy.types.Mesh) and data.users == 0:
                bpy.data.meshes.remove(data)


def _render(scene, path, res):
    scene.render.resolution_x, scene.render.resolution_y = res
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    print("review ->", path)


def _setup(scene):
    scene.render.engine = "BLENDER_EEVEE"
    if scene.world is None:
        scene.world = bpy.data.worlds.new("World")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.2, 0.2, 0.2, 1)
    scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = 1.0
    scene.view_settings.view_transform = "Standard"
    for o in list(scene.collection.objects):
        if o.name.startswith("mech_sun"):
            bpy.data.objects.remove(o)
    cam = bpy.data.objects.new("review_cam", bpy.data.cameras.new("review_cam"))
    scene.collection.objects.link(cam)
    scene.camera = cam
    return cam


def review(path, objs):
    """Close-up line-ups (every colour variant, unlit baked textures) and two corridor views from the game camera.
    Writes <path> (gates + levers), <stem>_small.png (keys, rock, crack) and <stem>_corridor{1,2}.png."""
    scene = bpy.context.scene
    stem = os.path.splitext(path)[0]
    env = bpy.data.objects.get("mech_env")
    if env:
        env.hide_render = True
    for o in objs.values():
        o.hide_render = True
    _clear_review(scene)
    cam = _setup(scene)
    variants = lambda name: LOCK_COLOURS if name in COLOURED and objs[name].get("textures") else [None]  # noqa: E731

    # 1. Gates and levers per colour, seen a little from the right and above.
    for i, colour in enumerate(variants("gate") if "gate" in objs else []):
        _instance(scene, objs["gate"], colour, (i * 1.3 - 2.0, 0, 0))
    for i, colour in enumerate(variants("lever_base") if "lever_base" in objs else []):
        base = V((i * 1.3 - 2.0 + 0.45, 0, -0.9))
        _instance(scene, objs["lever_base"], colour, base)
        if "lever_handle" in objs:
            _instance(scene, objs["lever_handle"], None, base + LEVER_PIVOT, ry=-LEVER_ANGLE if i % 2 else LEVER_ANGLE)
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = 5.6
    cam.rotation_euler = (math.radians(78), 0, math.radians(18))
    d = V((0, 0, -1))
    d.rotate(cam.rotation_euler)
    cam.location = V((0.0, -0.5, 0.1)) - d * 8
    _render(scene, path, (1600, 1000))
    _clear_review(scene)

    # 2. Small models: keys, rock, crack.
    x = -1.1
    for colour in variants("key") if "key" in objs else []:
        _instance(scene, objs["key"], colour, (x, -0.5, 0.0), rz=20)
        x += 0.25
    if "rock" in objs:
        _instance(scene, objs["rock"], None, (x + 0.2, -0.5, 0.0))
    if "ceiling_crack" in objs:
        _instance(scene, objs["ceiling_crack"], None, (x + 0.85, 0, -0.62))
    cam = _setup(scene)
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = 2.6
    cam.rotation_euler = (math.radians(85), 0, 0)
    d = V((0, 0, -1))
    d.rotate(cam.rotation_euler)
    cam.location = V((0.0, -0.5, 0.2)) - d * 6
    _render(scene, stem + "_small.png", (1300, 600))
    _clear_review(scene)

    # 3. Corridor from the game camera: eye 0.5 tile above the floor, 2 tiles in front of the tile front, 45 deg fov.
    for shot, cam_x, place_fn in (
        (1, 0.0, lambda: [
            "gate" in objs and _instance(scene, objs["gate"], variants("gate")[1 if len(variants("gate")) > 1 else 0], (1.1, 0, 0)),
            "key" in objs and _instance(scene, objs["key"], variants("key")[1 if len(variants("key")) > 1 else 0], (0.45, -0.5, 0.35), rz=30),
            "lever_base" in objs and _instance(scene, objs["lever_base"], variants("lever_base")[1 if len(variants("lever_base")) > 1 else 0], (-1.1, 0, 0)),
            "lever_handle" in objs and _instance(scene, objs["lever_handle"], None, V((-1.1, 0, 0)) + LEVER_PIVOT, ry=-LEVER_ANGLE),
        ]),
        (2, 0.0, lambda: [
            "gate" in objs and _instance(scene, objs["gate"], variants("gate")[0], (-1.3, 0, 0)),
            "ceiling_crack" in objs and _instance(scene, objs["ceiling_crack"], None, (0.9, 0, 0)),
            "rock" in objs and _instance(scene, objs["rock"], None, (0.9, -0.5, 0.45), rz=25),
            "lever_base" in objs and _instance(scene, objs["lever_base"], variants("lever_base")[-1], (1.9, 0, 0)),
            "lever_handle" in objs and _instance(scene, objs["lever_handle"], None, V((1.9, 0, 0)) + LEVER_PIVOT, ry=LEVER_ANGLE),
        ]),
    ):
        cam = _setup(scene)
        _corridor(scene, -4, 4)
        place_fn()
        cam.data.type = "PERSP"
        cam.data.sensor_fit = "VERTICAL"
        cam.data.angle = math.radians(45)
        cam.rotation_euler = (math.radians(90), 0, 0)
        cam.location = (cam_x, -3.0, 0.5)
        _render(scene, "%s_corridor%d.png" % (stem, shot), (800, 500))
        _clear_review(scene)
    for o in objs.values():
        o.hide_render = False
    if env:
        env.hide_render = False


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    only = args[args.index("--only") + 1].split(",") if "--only" in args else None
    exporting = "--export" in args
    tex_dir = os.path.join(REPO, "textures", "mechanisms")
    if exporting:
        os.makedirs(tex_dir, exist_ok=True)
    objs = build(bake=exporting or "--bake" in args, tex_dir=tex_dir if exporting else None, only=only)
    if "--review" in args:
        review(args[args.index("--review") + 1], objs)
    if exporting:
        export(objs)
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "mechanism.blend"))
