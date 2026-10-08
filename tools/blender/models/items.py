"""Procedural items: the weapons (club, dagger, short sword, khopesh, epsilon and duckbill axes, mace, spear, the
bows, sling, throwing stick, javelin), the missiles in
flight (arrow, sling stone), the potion vessels (POTION_MODELS), the treasure chest and the amulets (one per AmuletType, its tiers
share it: amulet_strength, ..., amulet_regeneration, amulet_venom).

    MCP:  p = ".../tools/blender/models/items.py"; g = {"__file__": p, "__name__": "items"}
          exec(open(p).read(), g); g["build"](bake=False)      # then g["export"](objs)
    CLI:  blender -b --python tools/blender/models/items.py -- [--export] [--bake] [--review out.png] [--only club,bow]

Blender space: Z up, real sizes in metres (the engine centres every item and scales its largest dimension to 1,
item::loadModel -> Centrify, then glScale item->scale). The weapons stand upright with the grip at the bottom and the
business end at the top (+Z): the engine draws the held weapon from its lowest point, tilted 45 deg towards the
facing direction (drawWeapon), and upright in the inventory. Flat faces (sword blade, bow) lie in the x-z plane,
so they face the camera (-Y); the bow's back bulges towards +x (the enemy when the player faces right).
The chest faces -Y with its lid open towards +Y; the engine draws it at rotA 0 with the tile's item standing inside.

The bows (bow, composite_bow) have BOW_FRAMES frames, the draw: frame 0 at rest (as the inventory and the pickups show it), then the string
pulled further back each frame with an arrow on it (collapsed to the nock point in frame 0); the engine picks the frame
from the draw time (Item::Draw pose). Texture and UVs come from the fully drawn bow. The arrow alone (arrow.md3,
tip at +Z) is the one in flight; the engine draws it in metres, not centred (Dungeon::drawMissiles). The sling stone,
the throwing stick and the javelin fly as models in metres too: the stone is its own model, the stick and the javelin
fly as their held models (the engine loads those twice, once centred for the hand).

Textures: one 512 PNG per item (textures/items/<name>.png), albedo x ambient occlusion like the monsters (no baked
light, the engine lights them). A potion vessel has no texture of its own: each potion bakes one on it (POTIONS,
textures/items/potion_<kind>.png) with its liquid colour; the engine draws it untinted.
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
ITEMS = ["club", "dagger", "sword", "khopesh", "epsilon_axe", "duckbill_axe", "mace", "spear", "bow", "composite_bow", "sling", "throwing_stick", "javelin", "arrow", "sling_stone",
         "potion_flask", "potion_lotus", "potion_pilgrim", "potion_canopic", "potion_cobra", "potion_ankh", "chest"] + ["amulet_" + t for t in ("strength", "armor", "health", "poison", "traps", "blunt", "slash",
                                                          "pierce", "regeneration", "venom")]
BOWS = ("bow", "composite_bow")  # exported with their draw frames
FILES = {"chest": "treasure_chest"}  # model / texture stem when it differs from the item name
TEX_SIZE = 512
# The potions, in ItemKind order: texture stem (textures/items/<stem>.png, PotionDef::texture in src/world/items.cpp),
# vessel model (POTION_MODELS there) and the palette keys its texture overrides.
POTIONS = [
    ("potion_small_health", "potion_flask", {"liquid": (0.70, 0.04, 0.03), "liquid_dark": (0.38, 0.02, 0.02)}),
    ("potion_large_health", "potion_lotus", {"liquid": (0.55, 0.02, 0.20), "liquid_dark": (0.30, 0.01, 0.10)}),
    ("potion_aphethamine", "potion_pilgrim", {"liquid": (0.32, 0.06, 0.50), "liquid_dark": (0.16, 0.03, 0.28)}),
    ("potion_stone_skin", "potion_canopic", {"liquid": (0.85, 0.36, 0.04), "liquid_dark": (0.48, 0.18, 0.02)}),
    ("potion_life", "potion_ankh", {"liquid": (0.85, 0.30, 0.06), "liquid_dark": (0.48, 0.15, 0.03)}),
    ("potion_small_stamina", "potion_flask", {"liquid": (0.36, 0.68, 0.18), "liquid_dark": (0.18, 0.36, 0.08)}),
    ("potion_large_stamina", "potion_lotus", {"liquid": (0.10, 0.60, 0.55), "liquid_dark": (0.05, 0.32, 0.30)}),
    ("potion_antidote", "potion_cobra", {"liquid": (0.04, 0.36, 0.15), "liquid_dark": (0.02, 0.20, 0.08)}),
]
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
    "glass": (0.78, 0.84, 0.82),
    "liquid": (0.72, 0.72, 0.72),  # each potion its own (POTIONS)
    "liquid_dark": (0.40, 0.40, 0.40),
    "cork": (0.50, 0.36, 0.22),
    "cork_dark": (0.34, 0.23, 0.13),
    "cord": (0.70, 0.68, 0.64),
    "wax": (0.42, 0.08, 0.05),
    "gem": (0.62, 0.05, 0.02),
    "glint": (1.0, 0.62, 0.5),
    "enamel": (0.02, 0.015, 0.01),
    "reed": (0.62, 0.50, 0.28),
    "reed_dark": (0.36, 0.26, 0.12),
    "feather": (0.84, 0.80, 0.70),
    "horn": (0.16, 0.10, 0.06),
    "bark": (0.86, 0.80, 0.64),
    "bark_dark": (0.55, 0.48, 0.36),
    "redwood": (0.40, 0.14, 0.07),
    "redwood_dark": (0.24, 0.07, 0.03),
    "stone": (0.46, 0.44, 0.40),
    "stone_dark": (0.28, 0.27, 0.25),
    "flax": (0.66, 0.52, 0.36),
    "flax_dark": (0.44, 0.32, 0.20),
    "carnelian": (0.60, 0.11, 0.05),
    "jasper": (0.42, 0.06, 0.05),
    "turquoise": (0.10, 0.50, 0.42),
    "faience": (0.16, 0.48, 0.26),
    "faience_dark": (0.07, 0.26, 0.13),
    "ink": (0.04, 0.03, 0.025),
    "lapis": (0.06, 0.12, 0.45),
    "alabaster": (0.84, 0.78, 0.64),
    "alabaster_dark": (0.68, 0.61, 0.47),
    "falcon": (0.42, 0.28, 0.14),
    "falcon_dark": (0.22, 0.14, 0.07),
}


def materials(palette=None):
    """palette: colours that replace COL's (a potion's liquid)."""
    pal = {**COL, **(palette or {})}
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
        "reed": stripes("v", 0.14, [(14, "reed"), (1, "reed_dark")], "LINEAR"),
        "fletch": stripes("v", 0.025, [(2, "feather"), (1, "red")]),
        "redwood": stripes("u", 1 / 8, [(3, "redwood"), (1, "redwood_dark")], "LINEAR"),
        "horn": ("solid", "horn"),
        "bark": stripes("u", 1 / 5, [(3, "bark"), (1, "bark_dark")], "LINEAR"),
        "stone": stripes("v", 0.02, [(3, "stone"), (1, "stone_dark")], "LINEAR"),
        "flax": stripes("v", 0.008, [(2, "flax"), (1, "flax_dark")]),
        "carnelian": ("solid", "carnelian"),
        "jasper": ("solid", "jasper"),
        "turquoise": ("solid", "turquoise"),
        "faience": stripes("v", 0.01, [(3, "faience"), (1, "faience_dark")], "LINEAR"),
        # The potion vessels.
        "liquid_dark": ("solid", "liquid_dark"),
        "glyphs": stripes("u", 1 / 40, [(3, "gold"), (1, "ink"), (2, "gold"), (1, "lapis"), (1, "gold"), (2, "ink")]),
        "alabaster": stripes("v", 0.011, [(4, "alabaster"), (1, "alabaster_dark")], "LINEAR"),
        "wig": stripes("u", 1 / 18, [(1, "lapis"), (1, "gold")]),
        "feathers": stripes("v", 0.006, [(3, "falcon"), (1, "falcon_dark")], "LINEAR"),
        "beak": ("solid", "gold_dark"),
        "scales": stripes("v", 0.004, [(2, "gold"), (1, "gold_dark")]),
        "hood": stripes("v", 0.005, [(2, "gold"), (1, "lapis"), (2, "gold"), (1, "carnelian")]),
    }
    return {name: common.make_material("item_" + name, spec, pal) for name, spec in specs.items()}


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


def gem(b, M, centre, normal, rx, rz, depth, bezel=0.003, bone="root"):
    """Cabochon in an enamel bezel, domed along normal, with a glint (as in mechanism.gem)."""
    c, nrm = V(centre), V(normal).normalized()
    up = V((0, 0, 1)) if abs(nrm.z) < 0.9 else V((0, 1, 0))
    b.add(ellipsoid(c, (rx + bezel, rz + bezel, depth * 0.5), n=12, rings=4, axis=nrm, ref=up), M["enamel"], bone, "gem")
    b.add(ellipsoid(c, (rx, rz, depth), n=12, rings=5, axis=nrm, ref=up), M["gem"], bone, "gem")
    left = nrm.cross(up)
    g = c + nrm * (depth * 0.82) + up * (rz * 0.38) + left * (rx * 0.3)
    s = min(rx, rz) * 0.22
    b.add(ellipsoid(g, (s, s * 1.3, s * 0.5), n=6, rings=3, axis=nrm, ref=up), M["glint"], bone, "gem")


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


def build_dagger(b, M):
    """Bronze dagger: a narrow leaf blade with a midrib, a cord-bound grip and a round bronze pommel."""
    blade = [(0.12, 0.018, 0.0055), (0.15, 0.019, 0.0055), (0.24, 0.015, 0.005), (0.3, 0.008, 0.0035), (0.34, 0.0012, 0.0012)]
    b.add(spindle(blade, n=8, shape=diamond, sub=3), M["blade"], "root")
    for s in (-1, 1):
        b.add(tube([(V((0, s * 0.003, 0.125)), 0.003, 0.0015), (V((0, s * 0.002, 0.3)), 0.0015, 0.001)], sub=2, n=6, ref=V((0, 1, 0))),
              M["gold"], "root")
    b.add(spindle([(0.1, 0.026, 0.006), (0.125, 0.028, 0.006)], n=8, sub=1), M["bronze_plain"], "root")  # the guard
    b.add(spindle([(0.03, 0.01), (0.065, 0.012), (0.1, 0.01)], n=10), M["ebony"], "root")
    wrap(b, M["cord"], 0.035, 0.095, 0.0115, 6)
    b.add(ellipsoid((0, 0, 0.018), (0.02, 0.02, 0.018), n=12, rings=5), M["bronze_plain"], "root")


def build_khopesh(b, M):
    """The khopesh: an ebony grip with gold bands, a straight dark bronze shank and the hooked sickle blade curving
    forward (+x) and down, the edge on the outer curve."""
    b.add(spindle([(0.0, 0.012, 0.009), (0.06, 0.014, 0.011), (0.13, 0.012, 0.01)], n=12), M["ebony"], "root")
    for z in (0.01, 0.05, 0.09, 0.125):
        band(b, M["gold_band"], z, 0.0135, 0.008)
    path = [(0.0, 0.13), (0.005, 0.22), (0.015, 0.3), (0.04, 0.38), (0.09, 0.45), (0.15, 0.49), (0.21, 0.5), (0.26, 0.47), (0.285, 0.42)]
    widths = [0.012, 0.014, 0.02, 0.028, 0.034, 0.036, 0.034, 0.024, 0.003]
    keys = [(V((x, 0, z)), w, 0.0055 if k < 2 else 0.004) for k, ((x, z), w) in enumerate(zip(path, widths))]
    b.add(tube(keys, sub=3, n=8, ref=V((0, 1, 0)), shape=diamond), M["bronze_dark"], "root")
    # The sharpened outer curve: a lighter strip along the blade's back edge.
    edge = []
    for k in range(3, len(path)):
        x, z = path[k]
        px, pz = path[k - 1]
        tx, tz = x - px, z - pz
        n = math.hypot(tx, tz)
        nx, nz = -tz / n, tx / n  # left normal: out of the hook
        edge.append((V((x + nx * widths[k] * 0.8, 0, z + nz * widths[k] * 0.8)), 0.004, 0.0045))
    b.add(tube(edge, sub=3, n=6, ref=V((0, 1, 0))), M["blade"], "root")


def axe_haft(b, M, length, lashings):
    """Wooden haft along Z with a leather-wrapped grip and cord lashings at the given heights."""
    b.add(spindle([(0.0, 0.016), (length * 0.5, 0.017), (length, 0.015)], n=10), M["wood"], "root")
    wrap(b, M["leather"], 0.02, 0.16, 0.0175, 8)
    for z in lashings:
        wrap(b, M["cord"], z - 0.018, z + 0.018, 0.0175, 4)


def build_epsilon_axe(b, M):
    """Epsilon axe: a long haft, the bronze crescent blade on three tangs lashed to it near the top (the openings
    between the tangs give it the shape of the letter)."""
    axe_haft(b, M, 0.7, (0.5, 0.58, 0.66))
    cx, cz, r = 0.035, 0.58, 0.12
    arc = []
    for k in range(13):
        a = math.radians(-62 + 124 * k / 12)
        w = 0.016 * (1 - 0.6 * abs(k - 6) / 6) + 0.004
        arc.append((V((cx + r * math.cos(a), 0, cz + r * math.sin(a))), w, 0.005))
    b.add(tube(arc, sub=2, n=8, ref=V((0, 1, 0)), shape=diamond), M["bronze_plain"], "root")
    for z in (0.5, 0.58, 0.66):  # the tangs, from the haft to the blade
        a = math.asin(max(-1, min(1, (z - cz) / r)))
        x = cx + r * math.cos(a) - 0.012
        b.add(place(box(x - 0.01, 0.007, 0.022), loc=((x + 0.01) / 2, 0, z)), M["bronze_plain"], "root")


def build_duckbill_axe(b, M):
    """Duckbill axe: a haft with a bronze blade at its top, reaching forward (+x) and narrowing like a bill, two long
    openings through it."""
    axe_haft(b, M, 0.62, (0.5, 0.6))
    z0, z1 = 0.47, 0.63  # the blade's height at the haft
    x0, x1 = 0.012, 0.14
    t = 0.008

    def zz(x, top):  # the blade narrows towards its edge
        k = (x - x0) / (x1 - x0)
        mid = (z0 + z1) / 2
        half = (z1 - z0) / 2 * (1 - 0.35 * k)
        return mid + half if top else mid - half

    def bar(xa, xb, za, zb):
        b.add(place(box(xb - xa, t, zb - za), loc=((xa + xb) / 2, 0, (za + zb) / 2)), M["bronze_plain"], "root")

    # A frame round two openings: the back, the middle and the edge upright, the top and bottom rails.
    bar(x0, x0 + 0.022, zz(x0, False), zz(x0, True))
    xm = (x0 + x1) / 2
    bar(xm - 0.008, xm + 0.008, zz(xm, False), zz(xm, True))
    bar(x1 - 0.03, x1, zz(x1, False), zz(x1, True))
    for top in (False, True):
        pts = [(x, zz(x, top)) for x in (x0, xm, x1)]
        for (xa, za), (xb, zb) in zip(pts, pts[1:]):
            b.add(rod(V((xa, 0, za + (-0.012 if top else 0.012))), V((xb, 0, zb + (-0.012 if top else 0.012))), 0.012, n=4), M["bronze_plain"], "root")
    b.add(place(box(0.008, t * 1.4, zz(x1, True) - zz(x1, False)), loc=(x1, 0, (z0 + z1) / 2)), M["blade"], "root")  # the edge


def build_mace(b, M):
    """Mace: a pear-shaped stone head bound with crossed leather straps on a cord-wrapped wooden haft."""
    b.add(spindle([(0.0, 0.016), (0.25, 0.017), (0.46, 0.015)], n=10), M["wood"], "root")
    wrap(b, M["cord"], 0.02, 0.2, 0.0175, 12)
    wrap(b, M["leather"], 0.38, 0.45, 0.017, 5)
    head = transform(decor.rock(V((0, 0, 0.53)), (0.07, 0.065, 0.075), seed=11, amp=0.06, n=14, rings=8),
                     lambda v: V((v.x * (0.75 + 0.25 * (v.z - 0.455) / 0.15), v.y * (0.75 + 0.25 * (v.z - 0.455) / 0.15), v.z)))
    b.add(head, M["stone"], "root")
    for a in (0.0, math.pi / 2):  # two straps crossing over the top
        pts = []
        for k in range(11):
            th = math.pi * k / 10
            pts.append(V((0.074 * math.cos(th) * math.cos(a), 0.074 * math.cos(th) * math.sin(a), 0.53 + 0.077 * math.sin(th))))
        b.add(tube([(p, 0.009, 0.003) for p in pts], sub=2, n=6, ref=V((0, 0, 1))), M["leather"], "root")


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
BOW_FRAMES = 8  # the draw, frame 0 at rest
BOW_FLEX = 0.1  # the tips bend back this far at full draw
BOW_NOCK_DRAWN = -0.4  # x of the string's centre at full draw (the grip is at BOW_DEPTH)
ARROW_REST = V((0, -0.03, 0.025))  # the nocked arrow passes the grip on the camera side, just above the fist
ARROW_LEN = 0.75


COMPOSITE_H = 0.55  # half height: shorter than the self-bow


def bow_point(t, draw=0.0, composite=False):
    """Limb centre line, t in -1..1 (bottom to top): an arc bulging to +x with tips recurving back, bent further back
    as the bow is drawn (draw 0..1). The composite bow's grip sits back towards the string between two humps, its
    tips recurve harder."""
    if composite:
        hump = 0.55 + 0.45 * (1 - math.exp(-(t / 0.32) ** 2))
        x = 0.17 * (1 - t * t) * hump - 0.05 * t ** 10 - BOW_FLEX * draw * t * t
        return V((x, 0, COMPOSITE_H * t * (1 - 0.04 * t * t - 0.05 * draw * t * t)))
    x = BOW_DEPTH * (1 - t * t) - 0.02 * t ** 8 - BOW_FLEX * draw * t * t
    return V((x, 0, BOW_H * t * (1 - 0.04 * t * t - 0.05 * draw * t * t)))


def build_arrow(b, M):
    """Reed arrow along +Z, nock at 0: a bronze leaf head on a sinew-bound foreshaft, three red-barred fletches.
    Thicker than a real one so it reads at the game's zoom."""
    L = ARROW_LEN
    b.add(spindle([(0.0, 0.011), (0.3, 0.012), (L - 0.07, 0.011)], n=8), M["reed"], "root")
    b.add(spindle([(-0.006, 0.007), (0.004, 0.013), (0.016, 0.013)], n=8, sub=1), M["ebony"], "root")  # the nock
    for z in (0.03, 0.2, L - 0.085):
        band(b, M["sinew"], z, 0.0135, 0.014, n=8)
    b.add(spindle([(L - 0.08, 0.012, 0.007), (L - 0.055, 0.034, 0.007), (L - 0.03, 0.028, 0.005), (L, 0.001, 0.001)],
                  n=8, shape=diamond, sub=2), M["blade"], "root")
    for k in range(3):
        a = math.radians(90 + 120 * k)
        out = V((math.cos(a), math.sin(a), 0))
        vane = []
        for z, h in ((0.035, 0.005), (0.06, 0.02), (0.15, 0.024), (0.19, 0.005)):
            vane.append((out * (0.011 + h / 2) + V((0, 0, z)), 0.002, h / 2))
        b.add(tube(vane, sub=2, n=6, ref=out), M["fletch"], "root")


def build_bow(b, M, draw=1.0, composite=False):
    """Self bow of acacia: limbs tapering from a leather-wrapped grip to gold-capped nocks, gold bands on the limbs,
    a sinew string between the tips, pulled back to BOW_NOCK_DRAWN at draw 1 with an arrow on it (no arrow at 0:
    collapsed onto the nock point). The composite bow: red-brown wood and horn under birch bark bands, horn tips,
    the double curve of bow_point."""
    keys = []
    for k in range(25):
        t = -1 + 2 * k / 24
        w = 0.016 - 0.009 * abs(t) ** 1.3  # across the bow (y)
        d = 0.012 - 0.006 * abs(t) ** 1.3  # front to back (x)
        keys.append((bow_point(t, draw, composite), d, w))
    b.add(tube(keys, sub=2, n=10, ref=V((0, 1, 0))), M["redwood" if composite else "wood"], "root")
    # Grip wrap and limb bands (short tubes along the curve).
    def along(t0, t1, grow, mat, n=10):
        ks = []
        for k in range(5):
            t = t0 + (t1 - t0) * k / 4
            w = 0.016 - 0.009 * abs(t) ** 1.3 + grow
            d = 0.012 - 0.006 * abs(t) ** 1.3 + grow
            ks.append((bow_point(t, draw, composite), d, w))
        b.add(tube(ks, sub=1, n=n, ref=V((0, 1, 0))), mat, "root")

    along(-0.14, 0.14, 0.004, M["leather"])
    if composite:
        for t0, t1 in ((-0.5, -0.36), (0.36, 0.5), (-0.97, -0.84), (0.84, 0.97)):
            along(t0, t1, 0.002, M["bark"])
    else:
        for t in (-0.62, -0.2, 0.2, 0.62):
            along(t - 0.03, t + 0.03, 0.002, M["gold_band"])
    tips = []
    for t in (-1, 1):
        p = bow_point(t, draw, composite)
        b.add(ellipsoid(p + V((0, 0, 0.006 * t)), (0.009, 0.009, 0.014), n=10, rings=5), M["horn" if composite else "gold"], "root")
        tips.append(p + V((-0.004, 0, -0.006 * t)))
    rest_x = (tips[0].x + tips[1].x) / 2
    nock = V((rest_x + (BOW_NOCK_DRAWN - rest_x) * draw, 0, ARROW_REST.z))
    for tip in tips:  # two halves, so the string can bend at the nock
        b.add(rod(tip, nock, 0.0022, n=6), M["sinew"], "root")
    arrow = Builder()
    build_arrow(arrow, M)
    # Lying along +x from the string; frame 0 has it collapsed on the nock (same vertex count in every frame).
    arrow.verts = [nock + V((0, ARROW_REST.y, 0)) + (V((v.z, v.y, -v.x)) if draw > 0 else V((0, 0, 0))) for v in arrow.verts]
    merge(b, arrow)


def build_composite_bow(b, M, draw=1.0):
    build_bow(b, M, draw, composite=True)


STONE_R = 0.05  # bigger than a real sling stone, so it reads in flight (the engine draws it bigger still)


def build_sling_stone(b, M):
    """A rounded river pebble."""
    b.add(decor.rock(V((0, 0, STONE_R)), (STONE_R, STONE_R * 0.9, STONE_R * 0.8), seed=4, amp=0.08), M["stone"], "root")


def build_sling(b, M):
    """Shepherd's sling: a finger loop at the bottom (the grip), two plaited flax cords up to a leather pouch cradling
    a stone. It hangs from the hand in the game (WeaponMotion tilts)."""
    loop = [(V((0.016 * math.cos(2 * math.pi * k / 12), 0, 0.02 + 0.016 * math.sin(2 * math.pi * k / 12))), 0.0035, 0.0035)
            for k in range(12)]
    b.add(tube(loop, sub=1, n=6, ref=V((0, 1, 0)), closed=True), M["flax"], "root")
    pouch_z = 0.44
    for s in (-1, 1):
        pts = [V((0.0, 0, 0.036)), V((s * 0.01, 0, 0.15)), V((s * 0.022, 0, 0.32)), V((s * 0.036, 0, pouch_z - 0.03))]
        b.add(tube([(p, 0.0032, 0.0032) for p in pts], sub=3, n=6, ref=V((0, 1, 0))), M["flax"], "root")
    # Pouch: a leather cradle, open towards the camera's back, a stone in it.
    pouch = [(V((x, 0.0, pouch_z + 0.03 * (1 - (x / 0.042) ** 2) - 0.012)), 0.006, 0.026 * (1 - 0.5 * (x / 0.042) ** 2))
             for x in (-0.042, -0.025, 0.0, 0.025, 0.042)]
    b.add(tube(pouch, sub=2, n=8, ref=V((0, 1, 0))), M["leather"], "root")
    b.add(decor.rock(V((0, 0, pouch_z + 0.03)), (0.026, 0.024, 0.022), seed=4, amp=0.08), M["stone"], "root")


def build_throwing_stick(b, M):
    """Hunting throwing stick of acacia, flat and gently bent, painted with red and blue bands and a gold tip band;
    the grip at the bottom, the bend towards +x."""
    keys = []
    for k in range(13):
        t = k / 12
        a = t * 1.15  # radians of bend
        r = 0.42
        p = V((r * (1 - math.cos(a)) * 0.8, 0, r * math.sin(a) * 1.25))
        w = 0.021 if 0.15 < t < 0.95 else 0.017
        keys.append((p, 0.022 if t > 0.1 else 0.018, w * 0.45))
    b.add(tube(keys, sub=2, n=10, ref=V((0, 1, 0))), M["wood"], "root")
    for t0, mat in ((0.0, "leather"), (0.48, "red"), (0.55, "blue"), (0.62, "red"), (0.93, "gold")):
        ks = []
        for k in range(3):
            t = t0 + 0.035 * k
            a = t * 1.15
            ks.append((V((0.42 * (1 - math.cos(a)) * 0.8, 0, 0.42 * math.sin(a) * 1.25)), 0.0245, 0.0115))
        if mat == "leather":
            ks = [(V((0, 0, z)), 0.021, 0.012) for z in (0.0, 0.05, 0.1)]
        b.add(tube(ks, sub=1, n=10, ref=V((0, 1, 0))), M[mat], "root")


JAVELIN_LEN = 1.3


def build_javelin(b, M):
    """Throwing javelin: a slim ash shaft, a long bronze head with a midrib on a socket, a cord grip at the balance
    point, a small bronze butt cap."""
    L = JAVELIN_LEN
    b.add(spindle([(0.0, 0.007), (0.02, 0.0105)], n=10, sub=1), M["bronze_plain"], "root")
    b.add(spindle([(0.015, 0.0105), (0.6, 0.0115), (L - 0.27, 0.0105)], n=10), M["ash"], "root")
    wrap(b, M["cord"], 0.52, 0.68, 0.012, 9)
    b.add(spindle([(L - 0.28, 0.0125), (L - 0.22, 0.011)], n=12), M["bronze"], "root")
    band(b, M["bronze_dark"], L - 0.275, 0.0135, 0.01, n=12)
    blade = [(L - 0.23, 0.01, 0.007), (L - 0.19, 0.03, 0.0075), (L - 0.12, 0.028, 0.0065), (L - 0.05, 0.015, 0.005), (L, 0.0015, 0.0015)]
    b.add(spindle(blade, n=8, shape=diamond, sub=3), M["blade"], "root")
    for s in (-1, 1):
        b.add(tube([(V((0, s * 0.004, L - 0.22)), 0.005, 0.0025), (V((0, s * 0.003, L - 0.1)), 0.0035, 0.0018), (V((0, s * 0.001, L - 0.01)), 0.001, 0.001)],
                   sub=2, n=6, ref=V((0, 1, 0))), M["bronze_dark"], "root")


# ---------------------------------------------------------------- potion and chest


# The potion vessels (POTION_MODELS), about the flask's size: up to 0.19 high, standing on z = 0. Several potions share
# a vessel; each potion bakes its own texture on it (POTIONS): the liquid colour ("liquid", "liquid_dark") and any
# palette key it overrides. Clear glass shows the liquid below the liquid line, "glass" above it.


def build_potion_flask(b, M):
    """Round-bottomed glass flask filled to the shoulder, a long neck with a rolled lip, a cork with a wax cap and a cord
    tied round the neck. Plain: the lesser potions."""
    body_r, body_c = 0.05, 0.055
    liquid_top = body_c + 0.034  # high: a chest hides the bottom
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
    band(b, M["flax"], 0.13, 0.0155, 0.004, n=14)
    band(b, M["flax"], 0.136, 0.0152, 0.004, n=14)
    for s in (-1, 1):
        pts = [V((0.004 * s, -0.015, 0.132)), V((0.008 * s, -0.019, 0.12)), V((0.012 * s, -0.021, 0.105))]
        b.add(tube([(p, 0.0022, 0.0022) for p in pts], sub=2, n=6, ref=V((0, 0, 1))), M["flax"], "root")


def build_potion_lotus(b, M):
    """Tall glass jar on a gold foot: an ovoid belly full to the shoulder, a slim neck that opens into a lotus flower of faience
    petals round a liquid mouth, two small gold handles, a band of glyphs on the shoulder. The larger potions."""
    profile = [(0.0, 0.0), (0.03, 0.0), (0.031, 0.004), (0.026, 0.009), (0.02, 0.014),  # foot
               (0.034, 0.026), (0.044, 0.045), (0.047, 0.062), (0.044, 0.08), (0.038, 0.092), (0.031, 0.1),  # belly
               (0.022, 0.108), (0.013, 0.118), (0.011, 0.13), (0.011, 0.145),  # shoulder, neck
               (0.016, 0.156), (0.025, 0.167), (0.032, 0.176), (0.028, 0.177), (0.0, 0.171)]  # flare, the mouth
    mats = ["gold"] * 4 + ["liquid"] * 6 + ["glyphs"] + ["glass"] * 3 + ["faience"] * 3 + ["gold", "liquid"]
    revolve(b, M, profile, mats, n=24)
    band(b, M["gold"], 0.0145, 0.0205, 0.004, n=24)
    band(b, M["gold"], 0.1, 0.0315, 0.003, n=24)  # the liquid line
    band(b, M["gold"], 0.146, 0.0122, 0.004, n=16)
    # Lotus petals: pointed leaves round the flare, leaning out, a second row between them.
    for row, (n, z0, lean, length) in enumerate(((8, 0.146, 0.55, 0.036), (8, 0.15, 0.85, 0.03))):
        for k in range(n):
            a = 2 * math.pi * (k + 0.5 * row) / n
            out = V((math.cos(a), math.sin(a), 0))
            d = (out * math.sin(lean) + V((0, 0, math.cos(lean)))).normalized()
            base = V((0, 0, z0)) + out * 0.012
            keys = [(base, 0.004, 0.0012), (base + d * length * 0.45, 0.0075, 0.0016), (base + d * length, 0.001, 0.0008)]
            nrm = out - out.dot(d) * d
            b.add(tube(keys, sub=3, n=8, ref=nrm.normalized()), M["faience"], "root")
    # Two small loop handles from the shoulder to the neck.
    for s in (-1, 1):
        pts = [V((s * 0.033, 0, 0.098)), V((s * 0.043, 0, 0.115)), V((s * 0.032, 0, 0.133)), V((s * 0.0115, 0, 0.136))]
        b.add(tube([(p, 0.0032, 0.0032) for p in pts], sub=3, n=8, ref=V((0, 1, 0))), M["gold"], "root")


def build_potion_pilgrim(b, M):
    """Flat round faience "New Year flask", glazed in the potion's colour: gold rings and a gold boss on both faces,
    a short neck with a lotus-bud stopper, two loop handles, a small oval foot."""
    t, r = 0.026, 0.062
    face = [(0.0, -t), (0.007, -t), (0.014, -0.0248), (0.02, -0.0236), (0.03, -0.0215), (0.036, -0.0196), (0.046, -0.0156),
            (0.05, -0.0132), (0.057, -0.0082), (r, 0.0)]
    face_mats = ["gold", "gold", "liquid", "liquid", "gold", "liquid", "liquid", "gold", "liquid"]
    profile = face + [(pr, -pz) for pr, pz in reversed(face[:-1])]  # the back face, mirrored
    mats = face_mats + list(reversed(face_mats))
    cz = 0.074
    revolve(b, M, profile, mats, n=32, r=rot(x=-90), loc=(0, 0, cz))
    b.add(spindle([(0.0, 0.019, 0.013), (0.016, 0.021, 0.014)], n=16, sub=1), M["liquid_dark"], "root")  # the foot
    b.add(spindle([(cz + r - 0.006, 0.0125), (cz + r + 0.02, 0.01), (cz + r + 0.024, 0.0135), (cz + r + 0.027, 0.0135)], n=14, sub=2), M["liquid"], "root")
    top = cz + r + 0.027
    b.add(ellipsoid((0, 0, top + 0.009), (0.0105, 0.0105, 0.013), n=12, rings=6), M["gold"], "root")
    band(b, M["gold"], top + 0.001, 0.0128, 0.003, n=14)
    for s in (-1, 1):
        pts = [V((s * 0.012, 0, cz + r + 0.012)), V((s * 0.03, 0, cz + r + 0.014)), V((s * 0.038, 0, cz + r - 0.006)), V((s * 0.034, 0, cz + r - 0.02))]
        b.add(tube([(p, 0.0042, 0.0042) for p in pts], sub=3, n=8, ref=V((0, 1, 0))), M["liquid_dark"], "root")


def build_potion_canopic(b, M):
    """Squat alabaster canopic jar, a band of glyphs between two glazed bands in the potion's colour round the belly,
    the lid the falcon head of Qebehsenuef with a striped wig."""
    profile = [(0.0, 0.0), (0.036, 0.0), (0.039, 0.004), (0.046, 0.02), (0.052, 0.04), (0.053, 0.05), (0.052, 0.06),
               (0.05, 0.068), (0.047, 0.077), (0.042, 0.086), (0.038, 0.09), (0.04, 0.093), (0.0, 0.094)]
    mats = ["alabaster"] * 4 + ["liquid", "glyphs", "liquid"] + ["alabaster"] * 3 + ["gold", "alabaster"]
    revolve(b, M, profile, mats, n=28)
    revolve(b, M, [(0.0, 0.093), (0.041, 0.093), (0.04, 0.099), (0.033, 0.106), (0.0, 0.108)], ["alabaster"] * 4, n=28)  # lid
    head = V((0, 0, 0.134))
    b.add(ellipsoid(head + V((0, 0.004, 0)), (0.028, 0.029, 0.03), n=16, rings=8), M["wig"], "root")
    for s in (-1, 1):  # lappets down to the shoulders
        keys = [(head + V((s * 0.02, -0.006, -0.002)), 0.008, 0.006), (head + V((s * 0.022, -0.012, -0.022)), 0.009, 0.005),
                (head + V((s * 0.022, -0.014, -0.032)), 0.009, 0.004)]
        b.add(tube(keys, sub=2, n=8, ref=V((0, 1, 0))), M["wig"], "root")
    b.add(ellipsoid(head + V((0, -0.012, 0.002)), (0.019, 0.02, 0.022), n=14, rings=7), M["feathers"], "root")  # the face
    beak = [(head + V((0, -0.03, 0.004)), 0.007, 0.006), (head + V((0, -0.041, -0.002)), 0.0045, 0.004),
            (head + V((0, -0.043, -0.01)), 0.0012, 0.0012)]
    b.add(tube(beak, sub=3, n=8, ref=V((0, 0, 1))), M["beak"], "root")
    for s in (-1, 1):
        eye = head + V((s * 0.012, -0.028, 0.008))
        b.add(ellipsoid(eye, (0.0045, 0.0035, 0.0028), n=8, rings=4, axis=V((s * 0.35, -1, 0)), ref=V((0, 0, 1))), M["enamel"], "root")


def build_potion_cobra(b, M):
    """Slim glass vial on a gold foot, a gold rearing cobra coiled round it: its hood, spread over the mouth, is the
    stopper. The snake potions (antidote, greater resistance)."""
    profile = [(0.0, 0.0), (0.024, 0.0), (0.026, 0.004), (0.022, 0.009), (0.019, 0.012), (0.02, 0.03), (0.02, 0.1),
               (0.017, 0.112), (0.011, 0.12), (0.011, 0.134), (0.0145, 0.137), (0.012, 0.139), (0.0, 0.138)]
    mats = ["gold"] * 4 + ["liquid"] * 3 + ["glass", "glass", "gold", "gold", "liquid"]
    revolve(b, M, profile, mats, n=20)
    band(b, M["gold"], 0.112, 0.0172, 0.0025, n=20)  # the liquid line
    # The coil: two turns up the vial, the tail thin at the foot, then up the back of the neck.
    keys = []
    turns, z0, z1, steps = 2.0, 0.016, 0.104, 28
    for k in range(steps + 1):
        u = k / steps
        a = math.radians(90) + 2 * math.pi * turns * u
        rr = 0.0245 - 0.0035 * u * u
        th = 0.0012 + 0.0038 * min(1, u * 1.6)
        keys.append((V((rr * math.cos(a), rr * math.sin(a), z0 + (z1 - z0) * u)), th, th * 0.85))
    keys += [(V((0, 0.019, 0.115)), 0.0052, 0.0044), (V((0, 0.017, 0.13)), 0.0055, 0.0045), (V((0, 0.012, 0.143)), 0.0055, 0.0045)]
    b.add(tube(keys, sub=2, n=8, ref=V((0.5, 0.5, 0.7))), M["scales"], "root")  # ref never along the coil
    # The hood over the mouth (the stopper), the head in front of its top, eyes and a ruby on the hood.
    b.add(ellipsoid((0, 0.004, 0.152), (0.021, 0.0055, 0.027), n=16, rings=8), M["hood"], "root")
    b.add(ellipsoid((0, -0.004, 0.17), (0.008, 0.011, 0.0065), n=12, rings=6, axis=V((0, -1, 0.35)), ref=V((0, 0, 1))), M["gold"], "root")
    for s in (-1, 1):
        b.add(ellipsoid((s * 0.0055, -0.009, 0.173), (0.0018, 0.0018, 0.0018), n=6, rings=3), M["enamel"], "root")
    gem(b, M, (0, -0.0035, 0.1555), (0, -1, 0), 0.0045, 0.006, 0.0025, bezel=0.0015)


def build_potion_ankh(b, M):
    """Ankh-shaped glass vessel full of liquid: the stem flares to a flat foot, the arms flare at the ends, the loop on
    top is the neck with a gold mouth and stopper; gold caps, a band of glyphs, a gold knot where the arms cross."""
    stem = [(V((0, 0, 0.0)), 0.026, 0.014), (V((0, 0, 0.012)), 0.021, 0.012), (V((0, 0, 0.05)), 0.0145, 0.0105),
            (V((0, 0, 0.098)), 0.011, 0.0095)]
    b.add(tube(stem, sub=4, n=16, ref=V((0, 1, 0))), M["liquid"], "root")
    b.add(tube([(V((0, 0, -0.0001)), 0.0275, 0.0152), (V((0, 0, 0.005)), 0.0272, 0.015)], sub=1, n=16, ref=V((0, 1, 0))), M["gold"], "root")
    b.add(tube([(V((0, 0, 0.03)), 0.0178, 0.012), (V((0, 0, 0.07)), 0.0133, 0.0103)], sub=1, n=16, ref=V((0, 1, 0))), M["glyphs"], "root")
    za = 0.105
    for s in (-1, 1):
        arm = [(V((0, 0, za)), 0.0095, 0.011), (V((s * 0.03, 0, za)), 0.0095, 0.0115), (V((s * 0.05, 0, za)), 0.011, 0.018)]  # depth, height
        b.add(tube(arm, sub=3, n=14, ref=V((0, 0, 1))), M["liquid"], "root")
        b.add(tube([(V((s * 0.0495, 0, za)), 0.0115, 0.0185), (V((s * 0.0535, 0, za)), 0.0115, 0.0185)], sub=1, n=14, ref=V((0, 0, 1))), M["gold"], "root")
    b.add(ellipsoid((0, 0, za), (0.016, 0.0125, 0.017), n=14, rings=7), M["gold"], "root")  # the knot
    # The loop: an upright oval of glass, the liquid in its lower half.
    lc, lw, lh = V((0, 0, 0.142)), 0.024, 0.03

    def arc(a0, a1, steps=12):
        return [(lc + V((lw * math.cos(a), 0, lh * math.sin(a))), 0.0085, 0.0085)
                for a in (a0 + (a1 - a0) * k / steps for k in range(steps + 1))]

    b.add(tube(arc(0, math.pi), sub=2, n=10, ref=V((0, 1, 0)), caps=(False, False)), M["glass"], "root")
    b.add(tube(arc(math.pi, 2 * math.pi), sub=2, n=10, ref=V((0, 1, 0)), caps=(False, False)), M["liquid"], "root")
    top = lc.z + lh  # the mouth on the loop's top and a gold stopper
    b.add(spindle([(top - 0.004, 0.0095), (top + 0.007, 0.0085), (top + 0.009, 0.011), (top + 0.012, 0.011)], n=14, sub=1), M["gold"], "root")
    b.add(ellipsoid((0, 0, top + 0.016), (0.008, 0.008, 0.008), n=12, rings=6), M["gold"], "root")


# ---------------------------------------------------------------- amulets
# A short cord loop (stylised: it reads in the inventory slot) with a gold bail and a pendant of its own, all in the
# x-z plane facing the camera (-Y), thickness along y. The pendant hangs from z = 0 down; the loop rises above it.
AMULET_LOOP_W, AMULET_LOOP_H = 0.06, 0.06


def slab(pts, thick, loc=(0, 0, 0)):
    """Convex polygon pts (x, z) as a plate thick along y, centred on y = 0, moved by loc."""
    return place(place(prism(pts, thick), rot(x=90), (0, thick / 2, 0)), None, loc)


def amulet_cord(b, M):
    pts = []
    for k in range(24):
        a = 2 * math.pi * k / 24
        # A drop: wide at the top, pinched where it meets the bail.
        x = AMULET_LOOP_W / 2 * math.sin(a) * (0.55 + 0.45 * (1 - math.cos(a)) / 2)
        z = 0.012 + AMULET_LOOP_H * (1 - math.cos(a)) / 2
        pts.append(V((x, 0.0, z)))
    b.add(tube([(p, 0.0028, 0.0028) for p in pts], sub=2, n=6, ref=V((0, 1, 0)), closed=True), M["leather"], "root")
    b.add(ellipsoid((0, 0, 0.006), (0.0075, 0.0055, 0.0085), n=10, rings=5), M["gold"], "root")  # the bail


def build_amulet_strength(b, M):
    """A jackal's fang, point down and curved, in a gold cap."""
    amulet_cord(b, M)
    keys = []
    for k in range(9):
        t = k / 8
        keys.append((V((0.012 * math.sin(t * 1.9) - 0.004, 0, -0.008 - 0.055 * t)), 0.0085 * (1 - t) ** 0.8 + 0.0008,
                     0.0065 * (1 - t) ** 0.8 + 0.0008))
    b.add(tube(keys, sub=2, n=10, ref=V((0, 1, 0))), M["ivory"], "root")
    band(b, M["gold"], -0.009, 0.0095, 0.007, n=12)


def build_amulet_armor(b, M):
    """A bronze scarab seen from above: wing cases split down the middle, thorax, head, six legs."""
    amulet_cord(b, M)
    b.add(ellipsoid((0, 0, -0.045), (0.022, 0.009, 0.026), n=14, rings=6), M["bronze_plain"], "root")
    b.add(ellipsoid((0, 0, -0.018), (0.018, 0.008, 0.009), n=12, rings=5), M["bronze_plain"], "root")
    b.add(ellipsoid((0, 0, -0.006), (0.009, 0.006, 0.006), n=10, rings=4), M["bronze_dark"], "root")
    b.add(slab([(-0.0009, -0.068), (0.0009, -0.068), (0.0009, -0.025), (-0.0009, -0.025)], 0.004, (0, -0.0075, 0)),
          M["bronze_dark"], "root")  # the split between the wing cases
    for side in (-1, 1):
        for z0, z1 in ((-0.02, -0.012), (-0.04, -0.036), (-0.058, -0.068)):
            b.add(rod((side * 0.016, 0, z0), (side * 0.03, 0, z1), 0.0018, n=6), M["bronze_dark"], "root")


def build_amulet_health(b, M):
    """The ib heart amulet in carnelian: a round jar with two lugs, a neck and a gold rim."""
    amulet_cord(b, M)
    b.add(ellipsoid((0, 0, -0.04), (0.02, 0.011, 0.023), n=14, rings=6), M["carnelian"], "root")
    for side in (-1, 1):
        b.add(ellipsoid((side * 0.02, 0, -0.026), (0.006, 0.005, 0.006), n=8, rings=4), M["carnelian"], "root")
    b.add(rod((0, 0, -0.021), (0, 0, -0.011), 0.0085, n=12), M["carnelian"], "root")
    band(b, M["gold"], -0.011, 0.0105, 0.004, n=12)


def build_amulet_poison(b, M):
    """Serket's scorpion in gold, head up: claws, legs, the tail curled to the side."""
    amulet_cord(b, M)
    b.add(ellipsoid((0, 0, -0.035), (0.011, 0.006, 0.022), n=12, rings=6), M["gold"], "root")
    for side in (-1, 1):
        b.add(rod((side * 0.007, 0, -0.02), (side * 0.02, 0, -0.008), 0.0022, n=6), M["gold"], "root")
        b.add(ellipsoid((side * 0.022, 0, -0.004), (0.0045, 0.003, 0.006), n=8, rings=4), M["gold"], "root")
        for k in range(3):
            z = -0.028 - 0.008 * k
            b.add(rod((side * 0.008, 0, z), (side * 0.021, 0, z - 0.006), 0.0013, n=5), M["gold_band"], "root")
    tail = [(0.002, -0.058), (0.01, -0.067), (0.021, -0.068), (0.028, -0.06), (0.029, -0.05)]
    for k, (x, z) in enumerate(tail):
        r = 0.0055 - 0.0006 * k
        b.add(ellipsoid((x, 0, z), (r, r * 0.8, r), n=8, rings=4), M["gold"], "root")
    b.add(cone(V((0.029, 0, -0.046)), V((-0.5, 0, 1)), 0.009, 0.003), M["carnelian"], "root")  # the sting


def build_amulet_traps(b, M):
    """The eye of Horus: the eye in white with a lapis pupil, the brow, the cheek mark and the spiral."""
    amulet_cord(b, M)
    b.add(ellipsoid((0, 0, -0.025), (0.026, 0.011, 0.004), n=16, rings=5, axis=V((0, -1, 0)), ref=V((0, 0, 1))),
          M["ivory"], "root")
    b.add(ellipsoid((0, -0.002, -0.025), (0.009, 0.009, 0.004), n=12, rings=5, axis=V((0, -1, 0)), ref=V((0, 0, 1))),
          M["blue"], "root")
    b.add(rod((-0.03, 0, -0.008), (0.028, 0, -0.008), 0.0028, n=6), M["blue"], "root")
    b.add(rod((-0.005, 0, -0.035), (-0.009, 0, -0.065), 0.0026, n=6), M["blue"], "root")
    spiral = [V((0.012 + (0.002 + 0.0012 * t) * math.cos(t / 2.4 + 1.2), 0, -0.048 + (0.002 + 0.0012 * t) * math.sin(t / 2.4 + 1.2)))
              for t in range(12, -1, -1)]
    b.add(tube([(V((0.008, 0, -0.035)), 0.0024, 0.0024)] + [(p, 0.0024, 0.0024) for p in spiral], sub=2, n=6,
               ref=V((0, 1, 0))), M["blue"], "root")


def build_amulet_blunt(b, M):
    """The djed pillar of Osiris in turquoise: a column with four bands at the top, on a foot."""
    amulet_cord(b, M)
    b.add(slab([(-0.008, -0.068), (0.008, -0.068), (0.008, -0.026), (-0.008, -0.026)], 0.008), M["turquoise"], "root")
    b.add(slab([(-0.014, -0.073), (0.014, -0.073), (0.014, -0.066), (-0.014, -0.066)], 0.01), M["turquoise"], "root")
    for k in range(4):
        z = -0.009 - 0.0055 * k
        b.add(slab([(-0.019, z - 0.0035), (0.019, z - 0.0035), (0.019, z), (-0.019, z)], 0.01), M["turquoise"], "root")


def build_amulet_slash(b, M):
    """The knot of Isis (tyet) in red jasper: a loop on top, arms hanging down, a long sash."""
    amulet_cord(b, M)
    ring = [V((0.0085 * math.sin(2 * math.pi * k / 16), 0, -0.016 + 0.0085 * math.cos(2 * math.pi * k / 16))) for k in range(16)]
    b.add(tube([(p, 0.0032, 0.0032) for p in ring], sub=2, n=6, ref=V((0, 1, 0)), closed=True), M["jasper"], "root")
    for side in (-1, 1):
        arm = [(0, -0.026), (side * 0.024, -0.026), (side * 0.022, -0.036), (side * 0.002, -0.034)]
        b.add(slab(arm if side > 0 else arm[::-1], 0.007), M["jasper"], "root")
    b.add(slab([(-0.006, -0.026), (0.006, -0.026), (0.013, -0.072), (-0.013, -0.072)], 0.008), M["jasper"], "root")


def build_amulet_pierce(b, M):
    """The shen ring in gold: a rope ring tied to a bar, a carnelian disc in it."""
    amulet_cord(b, M)
    ring = [V((0.019 * math.sin(2 * math.pi * k / 24), 0, -0.03 + 0.019 * math.cos(2 * math.pi * k / 24))) for k in range(24)]
    b.add(tube([(p, 0.0045, 0.0045) for p in ring], sub=2, n=8, ref=V((0, 1, 0)), closed=True), M["gold"], "root")
    b.add(rod((0, -0.003, -0.03), (0, 0.003, -0.03), 0.0145, n=16), M["carnelian"], "root")
    b.add(slab([(-0.024, -0.058), (0.024, -0.058), (0.024, -0.05), (-0.024, -0.05)], 0.008), M["gold"], "root")


def build_amulet_regeneration(b, M):
    """A green faience lotus: a fan of petals on a cup and a short stalk."""
    amulet_cord(b, M)
    for k in range(5):
        a = math.radians(-56 + 28 * k)
        d = V((math.sin(a), 0, math.cos(a)))
        c = V((0, 0, -0.045)) + d * 0.018
        b.add(ellipsoid(c, (0.0065, 0.003, 0.016), n=10, rings=5, axis=d, ref=V((0, 1, 0))), M["faience"], "root")
    b.add(ellipsoid((0, 0, -0.047), (0.012, 0.007, 0.007), n=12, rings=5), M["faience"], "root")
    b.add(rod((0, 0, -0.052), (0, 0, -0.072), 0.0028, n=8), M["faience"], "root")
    b.add(rod((0, 0, -0.012), (0, 0, -0.03), 0.0022, n=6), M["gold"], "root")  # from the bail to the bloom


def build_amulet_venom(b, M):
    """Wadjet's uraeus in gold, from the front: the flared hood with a carnelian inlay and lapis bands, the head on
    top, the tail coiled below the hood."""
    amulet_cord(b, M)
    b.add(ellipsoid((0, -0.002, -0.011), (0.0062, 0.0055, 0.0068), n=12, rings=5), M["gold"], "root")  # the head
    for side in (-1, 1):
        b.add(ellipsoid((side * 0.0028, -0.007, -0.0095), (0.0012, 0.001, 0.0012), n=6, rings=3), M["enamel"], "root")
    hood = [(0.0, -0.011), (0.011, -0.014), (0.018, -0.022), (0.0175, -0.031), (0.011, -0.043), (0.0025, -0.053),
            (-0.0025, -0.053), (-0.011, -0.043), (-0.0175, -0.031), (-0.018, -0.022), (-0.011, -0.014)]
    b.add(slab(hood[::-1], 0.005), M["gold"], "root")
    inlay = [(x * 0.62, -0.03 + (z + 0.03) * 0.7) for x, z in hood]
    b.add(slab(inlay[::-1], 0.0016, (0, -0.0028, 0)), M["carnelian"], "root")
    for k in range(3):
        z = -0.024 - 0.0055 * k
        w = 0.0055 - 0.001 * k
        b.add(slab([(-w, z - 0.0012), (w, z - 0.0012), (w, z), (-w, z)][::-1], 0.0012, (0, -0.0037, 0)), M["blue"],
              "root")
    tail = [(-0.002, -0.05), (0.011, -0.056), (0.008, -0.066), (-0.006, -0.068), (-0.013, -0.061)]
    b.add(tube([(V((x, 0, z)), 0.0042 - 0.0006 * k, 0.0036 - 0.0005 * k) for k, (x, z) in enumerate(tail)], sub=2, n=8,
               ref=V((0, 1, 0))), M["gold"], "root")


CHEST_W, CHEST_D, CHEST_H = 0.8, 0.46, 0.36  # box outside (without legs and lid)
CHEST_LEG = 0.05
CHEST_WALL = 0.025
LID_H = 0.09
LID_OPEN = 105.0  # degrees


def build_chest(b, M, lid_bone="root", heap_bone="root"):
    """Painted cedar chest on short legs, gold corner caps and studs, a blue-red-gold frieze and ankh plaques
    on the front, red lining, the vaulted lid open towards the back, a heap of gold coins and a gem inside.
    lid_bone / heap_bone: vertex weights of the lid and of the treasure (the mimic, mimic.py, rigs them)."""
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
    b.add(grid, M["gold_heap"], heap_bone)
    for k in range(60):
        x, y = rnd.uniform(-mx * 0.9, mx * 0.9), rnd.uniform(-my * 0.9, my * 0.9)
        tilt = rot(x=rnd.uniform(-30, 30), y=rnd.uniform(-30, 30))
        b.add(place(spindle([(-0.002, 0.02), (0.002, 0.02)], n=10, sub=1), tilt, (x, y, mound(x, y) + 0.004)), M["gold"], heap_bone)
    gem(b, M, (0.22, -0.06, mound(0.22, -0.06) + 0.012), (0.3, -0.5, 1), 0.025, 0.02, 0.018, bone=heap_bone)
    gem(b, M, (-0.25, 0.05, mound(-0.25, 0.05) + 0.01), (-0.2, -0.4, 1), 0.02, 0.018, 0.015, bone=heap_bone)
    # Lid: vaulted top over a flat frame, hinged on the back top edge and swung open.
    lid = Builder()
    lw, ld = w + 0.02, d + 0.02
    lid.add(place(box(lw, ld, 0.03), loc=(0, 0, 0.015)), M["cedar_grain"], lid_bone)
    arch = []
    for k in range(9):
        a = math.pi * k / 8
        arch.append((ld / 2 * math.cos(a), 0.03 + (LID_H - 0.03) * math.sin(a)))
    lid.add(prism_x(arch, lw - 0.01), M["cedar"], lid_bone)
    lid.add(place(box(lw - 0.12, ld - 0.12, 0.004), loc=(0, 0, -0.002)), M["lining"], lid_bone)
    for x in (-lw / 2 + 0.02, 0, lw / 2 - 0.02):
        lid.add(transform(prism_x([(p[0] * 1.03, p[1] * 1.03) for p in arch], 0.03), lambda v, x=x: V((v.x + x, v.y, v.z))), M["gold"], lid_bone)
    lid.add(place(box(lw - 0.06, 0.004, 0.022), loc=(0, -ld / 2 - 0.002, 0.015)), M["frieze"], lid_bone)
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
    "dagger": build_dagger,
    "sword": build_sword,
    "khopesh": build_khopesh,
    "epsilon_axe": build_epsilon_axe,
    "duckbill_axe": build_duckbill_axe,
    "mace": build_mace,
    "spear": build_spear,
    "bow": build_bow,
    "composite_bow": build_composite_bow,
    "sling": build_sling,
    "throwing_stick": build_throwing_stick,
    "javelin": build_javelin,
    "arrow": build_arrow,
    "sling_stone": build_sling_stone,
    "potion_flask": build_potion_flask,
    "potion_lotus": build_potion_lotus,
    "potion_pilgrim": build_potion_pilgrim,
    "potion_canopic": build_potion_canopic,
    "potion_cobra": build_potion_cobra,
    "potion_ankh": build_potion_ankh,
    "chest": build_chest,
    "amulet_strength": build_amulet_strength,
    "amulet_armor": build_amulet_armor,
    "amulet_health": build_amulet_health,
    "amulet_poison": build_amulet_poison,
    "amulet_traps": build_amulet_traps,
    "amulet_blunt": build_amulet_blunt,
    "amulet_slash": build_amulet_slash,
    "amulet_pierce": build_amulet_pierce,
    "amulet_regeneration": build_amulet_regeneration,
    "amulet_venom": build_amulet_venom,
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
        obj["low"] = low  # the draw frames are shifted the same (bow_frames)
        obj.data.set_sharp_from_angle(angle=math.radians(50))
        obj.location = (xs[name], 0, 0)
        common.uv_unwrap(obj, b.tags, boost={"gem": ((0, 0, 0), 1.6)})
        if bake:
            out = tex_dir or bpy.app.tempdir or "/tmp"
            ao = 0.05 if name != "chest" else 0.15
            potions = [(stem, palette) for stem, model, palette in POTIONS if model == name]
            if potions:  # a texture per potion on the vessel, no texture of its own; it shows the first
                texs = []
                for stem, palette in potions:
                    materials(palette)
                    texs.append(common.bake_texture(obj, os.path.join(out, stem + ".png"), TEX_SIZE, "item_" + stem, ao_distance=ao))
                materials()
                tex = texs[0]
            else:
                stem = FILES.get(name, name)
                tex = common.bake_texture(obj, os.path.join(out, stem + ".png"), TEX_SIZE, "item_" + stem, ao_distance=ao)
            common.use_baked_material(obj, tex)
            obj["texture"] = tex.name
        (x0, x1), (y0, y1), (z0, z1) = extents(obj)
        print("item_{}: {} verts, {} tris, x {:.3f}..{:.3f}, y {:.3f}..{:.3f}, z {:.3f}..{:.3f}".format(
            name, len(obj.data.vertices), common.tri_count(obj), x0, x1, y0, y1, z0, z1))
        objs[name] = obj
    return objs


def bow_frames(obj, path, g, name="bow"):
    """Writes a bow's BOW_FRAMES draw frames to path: one shape key per frame on the baked (fully drawn) bow."""
    M = materials()
    obj.shape_key_add(name="Basis")
    keys = []
    for k in range(BOW_FRAMES):
        b = Builder()
        BUILDERS[name](b, M, draw=k / (BOW_FRAMES - 1))
        key = obj.shape_key_add(name="draw%d" % k)
        for i, v in enumerate(b.verts):
            key.data[i].co = V(v) - V((0, 0, obj["low"]))
        keys.append(key)
    frames, normals, uvs = [], [], None
    try:
        for key in keys:
            for other in keys:
                other.value = 1.0 if other is key else 0.0
            bpy.context.view_layer.update()
            pos, nrm, uv = g["_sample"](obj, bpy.context.evaluated_depsgraph_get())
            frames.append(pos)
            normals.append(nrm)
            uvs = uvs or uv
    finally:
        obj.shape_key_clear()
    stats = g["md3"].write_md3(path, frames, normals, uvs, name=os.path.splitext(os.path.basename(path))[0])
    print("Exported {} -> {}: {verts} verts, {tris} tris, {frames} frames, {bytes} bytes".format(obj.name, path, **stats))


def export(objs, models_dir=None):
    models_dir = models_dir or os.path.join(REPO, "models", "items")
    p = os.path.join(REPO, "tools", "blender", "md3_export.py")
    g = {"__file__": p, "__name__": "md3_export"}
    exec(open(p).read(), g)
    for name, obj in objs.items():
        loc = obj.location.copy()
        obj.location = (0, 0, 0)
        bpy.context.view_layer.update()
        path = os.path.join(models_dir, "%s.md3" % FILES.get(name, name))
        if name in BOWS:
            bow_frames(obj, path, g, name)
        else:
            g["export_md3"](obj, path, 0, 0)
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
