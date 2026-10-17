"""Procedural treasure containers (plan treasure-containers), besides the wooden chest (items.py, the mimic copies it):
a painted coffer with a gabled lid, a reed basket, a set of four canopic jars, a large sealed pottery jar and an offering
table with the item under a linen cover.

    MCP:  p = ".../tools/blender/models/containers.py"; g = {"__file__": p, "__name__": "containers"}
          exec(open(p).read(), g); g["build"](bake=False)      # then g["export"](objs)
    CLI:  blender -b --python tools/blender/models/containers.py -- [--export] [--bake] [--only jar,table]

Blender space: Z up, real sizes in metres like the items (the engine centres each on its frame 0, scales its largest
side to Item::scale: CONTAINER_DEFS in src/world/containers.h, which takes UNITS_PER_M x the largest side printed here,
so all containers share the chest's metres). Each faces -Y (the camera at rotA 0); the engine turns it to its stance.

Frames: the opening clip, FRAMES[name] frames, one shape key each on the baked object: frame 0 closed (the item hidden),
the last frame open (the item shows, the engine draws it standing at the cell's centre, on the table's top for the
table: ITEM_LIFT). The coffer's lid swings back on its hinge like the chest's, the basket's lifts off and leans against
its side, the canopic jars' heads lift off one after the other and stand on the floor in front, the jar smashes (shards fly and
lie on the floor round its foot, the mud stopper drops; the engine adds the dust), the table's linen cover slides off
backwards to the floor.

Textures: one 512 PNG per container (media/textures/items/container_<name>.png), albedo x ambient occlusion like the
items, baked on BAKE_POSE (the closed jar and table: their outsides; the coffer, basket and jars open: their insides).
"""

import importlib
import math
import os
import random
import sys

import bpy
from mathutils import Matrix, Quaternion
from mathutils import Vector as V

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import common  # noqa: E402
import decor  # noqa: E402
import items  # noqa: E402

importlib.reload(common)
importlib.reload(decor)
importlib.reload(items)
from common import REPO, Builder, ellipsoid, lathe, smoothstep, transform, tube  # noqa: E402
from decor import arc_len, box, place, prism, rot  # noqa: E402

COLL = "containers_new"
CONTAINERS = ["coffer", "basket", "canopic", "jar", "table"]
# Frames of the opening clip, over CONTAINER_DEFS' openMs (~17 a second).
FRAMES = {"coffer": 12, "basket": 11, "canopic": 16, "jar": 10, "table": 14}
BAKE_POSE = {"coffer": 1.0, "basket": 1.0, "canopic": 1.0, "jar": 0.0, "table": 0.0}
UNITS_PER_M = 8.0 / 0.834  # the chest: 8 world units over its 0.834 m width (items.py)
TEX_SIZE = 512
SPACING = 1.6  # containers are spread along X in the scene (bake only; export is at the origin)

COL = {
    "plaster": (0.80, 0.74, 0.60),
    "ebony": (0.05, 0.035, 0.03),
    "blue": (0.05, 0.20, 0.55),
    "red": (0.60, 0.10, 0.05),
    "yellow": (0.80, 0.58, 0.14),
    "green": (0.10, 0.42, 0.30),
    "lining": (0.30, 0.05, 0.04),
    "reed": (0.66, 0.52, 0.28),
    "reed_dark": (0.40, 0.28, 0.12),
    "reed_red": (0.45, 0.13, 0.06),
    "terracotta": (0.52, 0.24, 0.11),
    "terracotta_dark": (0.26, 0.11, 0.05),
    "ochre": (0.66, 0.44, 0.16),
    "black": (0.03, 0.02, 0.015),
    "mud": (0.36, 0.30, 0.22),
    "mud_dark": (0.22, 0.18, 0.12),
    "alabaster": (0.84, 0.78, 0.64),
    "alabaster_dark": (0.66, 0.59, 0.45),
    "lapis": (0.06, 0.12, 0.45),
    "gold": (0.85, 0.60, 0.16),
    "wood": (0.40, 0.22, 0.10),
    "wood_dark": (0.22, 0.11, 0.05),
    "limestone": (0.74, 0.68, 0.55),
    "limestone_dark": (0.56, 0.50, 0.39),
    "linen": (0.86, 0.82, 0.70),
    "linen_dark": (0.70, 0.64, 0.50),
    "bread": (0.62, 0.38, 0.14),
    "fig": (0.30, 0.12, 0.16),
}


def materials():
    st = decor.stripes
    specs = {
        "plaster": ("solid", "plaster"),
        "roof": st("u", 0.08, [(5, "blue"), (1, "yellow")]),
        "ebony": ("solid", "ebony"),
        "panel": st("u", 0.06, [(2, "blue"), (1, "yellow"), (2, "red"), (1, "yellow"), (2, "green"), (1, "yellow")]),
        "frieze": st("v", 0.012, [(1, "yellow"), (2, "blue"), (1, "yellow"), (2, "red")]),
        "blue": ("solid", "blue"),
        "red": ("solid", "red"),
        "yellow": ("solid", "yellow"),
        "lining": ("solid", "lining"),
        "reed": st("v", 0.018, [(3, "reed"), (1, "reed_dark")], "LINEAR"),
        "reed_band": st("u", 1 / 24, [(1, "reed_red"), (1, "reed_dark"), (2, "reed")]),
        "reed_dark": ("solid", "reed_dark"),
        "clay": ("solid", "terracotta"),
        "clay_in": ("solid", "terracotta_dark"),
        "clay_band": st("v", 0.012, [(1, "black"), (2, "ochre"), (1, "black"), (3, "terracotta")]),
        "mud": st("u", 1 / 9, [(3, "mud"), (1, "mud_dark")], "LINEAR"),
        "seal": ("solid", "mud_dark"),
        "alabaster": st("v", 0.02, [(4, "alabaster"), (1, "alabaster_dark")], "LINEAR"),
        "glyphs": st("u", 1 / 30, [(3, "alabaster"), (1, "black"), (2, "alabaster"), (1, "lapis")]),
        "wig": st("u", 1 / 18, [(1, "lapis"), (1, "gold")]),
        "face": ("solid", "alabaster"),
        "black": ("solid", "black"),
        "gold": ("solid", "gold"),
        "wood": st("u", 1 / 10, [(3, "wood"), (1, "wood_dark")], "LINEAR"),
        "limestone": st("v", 0.04, [(4, "limestone"), (1, "limestone_dark")], "LINEAR"),
        "limestone_plain": ("solid", "limestone"),
        "carving": st("u", 0.05, [(2, "limestone"), (1, "limestone_dark")]),
        "linen": st("u", 0.03, [(4, "linen"), (1, "linen_dark")], "LINEAR"),
        "linen_hem": st("v", 0.01, [(2, "linen"), (1, "blue")]),
        "bread": ("solid", "bread"),
        "fig": ("solid", "fig"),
    }
    return {name: common.make_material("cont_" + name, spec, COL) for name, spec in specs.items()}


# ---------------------------------------------------------------- helpers


def part():
    return Builder()


def merge_moved(b, p, m):
    """Adds part p to b with every vertex moved by the 4x4 matrix m."""
    q = Builder()
    q.verts = [m @ V(v) for v in p.verts]
    q.faces, q.fuv, q.fmat, q.weights, q.tags, q.mats = p.faces, p.fuv, p.fmat, p.weights, p.tags, p.mats
    items.merge(b, q)


def about(pivot, r3):
    """4x4: rotate by the 3x3 r3 round pivot."""
    pv = V(pivot)
    return Matrix.Translation(pv) @ r3.to_4x4() @ Matrix.Translation(-pv)


def ease_out(t):
    return 1 - (1 - t) ** 3


def revolve(b, M, profile, mats, n=20, z_mod=None, m=None):
    for mat, geo in lathe(profile, arc_len(profile), n, mats, z_mod).items():
        if m is not None:
            geo = transform(geo, lambda v: m @ V(v))
        b.add(geo, M[mat], "root")


def hinged_lid(t, closed_pause=0.12, swing=100.0):
    """Lid angle (degrees) at t 0..1: a pause at the latch, then a swing back slowing to rest (as items.chest_lid)."""
    if t < closed_pause:
        return 2.0 * math.sin(math.pi * t / closed_pause)
    return swing * ease_out((t - closed_pause) / (1 - closed_pause))


# ---------------------------------------------------------------- coffer

COFFER_W, COFFER_D, COFFER_H, COFFER_LEG = 0.66, 0.40, 0.30, 0.06
GABLE_H = 0.13


def build_coffer(b, M, opening=1.0):
    """A white-plastered coffer on four ebony legs, framed in ebony, painted panels (blue, red, green between yellow
    bars) on the front and sides, a yellow and blue frieze under the rim, a red lining; the lid a gabled shrine roof
    with painted ends, hinged at the back. Two knobs, on the lid and on the front, for the cord that tied it shut."""
    w, d, h, leg = COFFER_W, COFFER_D, COFFER_H, COFFER_LEG
    t = 0.022
    z0, z1 = leg, leg + h
    # Walls and floor.
    for sx, sy, x, y in ((w, t, 0, -d / 2 + t / 2), (w, t, 0, d / 2 - t / 2), (t, d - 2 * t, -w / 2 + t / 2, 0), (t, d - 2 * t, w / 2 - t / 2, 0)):
        b.add(place(box(sx, sy, h), loc=(x, y, (z0 + z1) / 2)), M["plaster"], "root")
    b.add(place(box(w - 2 * t, d - 2 * t, 0.02), loc=(0, 0, z0 + 0.01)), M["lining"], "root")
    for sx, sy, x, y in ((w - 2 * t, 0.003, 0, -d / 2 + t + 0.0015), (w - 2 * t, 0.003, 0, d / 2 - t - 0.0015),
                         (0.003, d - 2 * t, -w / 2 + t + 0.0015, 0), (0.003, d - 2 * t, w / 2 - t - 0.0015, 0)):
        b.add(place(box(sx, sy, h - 0.03), loc=(x, y, z0 + 0.02 + (h - 0.03) / 2)), M["lining"], "root")
    # Legs and the ebony frame: corner posts, the rails at the rim and the base.
    for sx in (-1, 1):
        for sy in (-1, 1):
            b.add(place(box(0.04, 0.04, leg + h), loc=(sx * (w / 2 - 0.012), sy * (d / 2 - 0.012), (leg + h) / 2)), M["ebony"], "root")
    for z in (z0 + 0.012, z1 - 0.012):
        b.add(place(box(w + 0.006, 0.014, 0.024), loc=(0, -d / 2 - 0.004, z)), M["ebony"], "root")
        b.add(place(box(w + 0.006, 0.014, 0.024), loc=(0, d / 2 + 0.004, z)), M["ebony"], "root")
        for sx in (-1, 1):
            b.add(place(box(0.014, d + 0.006, 0.024), loc=(sx * (w / 2 + 0.004), 0, z)), M["ebony"], "root")
    # Painted panels and the frieze, a hair proud of the plaster.
    fy = -d / 2 - 0.0025
    for x in (-0.17, 0.17):
        b.add(place(box(0.26, 0.003, 0.15), loc=(x, fy, z0 + 0.12)), M["panel"], "root", "panel")
    b.add(place(box(w - 0.07, 0.003, 0.03), loc=(0, fy, z1 - 0.045)), M["frieze"], "root")
    for sx in (-1, 1):
        b.add(place(box(0.003, d - 0.08, 0.15), loc=(sx * (w / 2 + 0.0025), 0, z0 + 0.12)), M["panel"], "root", "panel")
    # The front knob.
    b.add(ellipsoid((0, -d / 2 - 0.02, z1 - 0.09), (0.018, 0.018, 0.018), n=10, rings=5), M["ebony"], "root")
    b.add(place(box(0.012, 0.022, 0.012), loc=(0, -d / 2 - 0.01, z1 - 0.09)), M["ebony"], "root")
    # Lid, closed: a frame and the gabled roof along X, painted ends, a knob on its front slope.
    lid = part()
    lw, ld = w + 0.03, d + 0.03
    lid.add(place(box(lw, ld, 0.025), loc=(0, 0, 0.0125)), M["ebony"], "root")
    roof = [(-ld / 2 + 0.01, 0.025), (ld / 2 - 0.01, 0.025), (0.0, 0.025 + GABLE_H)]
    lid.add(items.prism_x(roof, lw - 0.02), M["roof"], "root")
    lid.add(place(box(lw - 0.08, ld - 0.08, 0.004), loc=(0, 0, -0.002)), M["lining"], "root")
    for sx in (-1, 1):  # the painted gable ends
        tri = [(p[0] * 0.86, 0.025 + (p[1] - 0.025) * 0.86) for p in roof]
        end = items.prism_x(tri, 0.004)
        lid.add(transform(end, lambda v, sx=sx: V((v.x + sx * (lw / 2 - 0.008), v.y, v.z))), M["red"], "root")
    # Ridge beam and the slopes' yellow edges.
    lid.add(items.prism_x([(-0.012, 0.025 + GABLE_H - 0.01), (0.012, 0.025 + GABLE_H - 0.01), (0.0, 0.025 + GABLE_H + 0.012)], lw), M["ebony"], "root")
    slope = math.atan2(GABLE_H, ld / 2 - 0.01)
    for sy in (-1, 1):
        mid = V((0, sy * (ld / 4), 0.025 + GABLE_H / 2))
        strip = place(box(lw - 0.06, 0.04, 0.004), rot(x=sy * math.degrees(slope)), mid + V((0, 0, 0.003)))
        lid.add(strip, M["frieze"], "root")
    knob = V((0, -ld / 4, 0.025 + GABLE_H / 2 + 0.012))
    lid.add(ellipsoid(knob, (0.016, 0.016, 0.016), n=10, rings=5), M["ebony"], "root")
    # Hinged on the back top edge, swung back.
    hinge = V((0, d / 2 + 0.015, z1))
    m = about(hinge, rot(x=-hinged_lid(opening))) @ Matrix.Translation(V((0, 0, z1)))
    merge_moved(b, lid, m)


# ---------------------------------------------------------------- basket

BASKET_R, BASKET_H = 0.27, 0.34


def basket_lid():
    """The lid at the basket's rim (z = 0 at its underside): a low dome of coiled reed, a dark band, a knob."""
    lid = part()
    M = materials_cache()
    prof = [(0.0, 0.0), (BASKET_R + 0.018, 0.0), (BASKET_R + 0.02, 0.012), (BASKET_R + 0.012, 0.05),
            (BASKET_R - 0.03, 0.09), (0.12, 0.115), (0.04, 0.125), (0.0, 0.126)]
    revolve(lid, M, prof, ["reed_dark", "reed_dark", "reed_band", "reed", "reed", "reed", "reed"], n=24)
    revolve(lid, M, [(0.0, 0.124), (0.028, 0.124), (0.032, 0.14), (0.022, 0.16), (0.0, 0.165)], ["reed_dark"] * 4, n=12)
    return lid


def build_basket(b, M, opening=1.0):
    """A round basket of coiled reed, wider at the mouth, a red and dark zigzag band, two loop handles, the lid a low
    dome with a knob. The lid lifts, swings aside and comes down leaning against the basket's right side."""
    r, h = BASKET_R, BASKET_H
    prof = [(0.0, 0.0), (r - 0.04, 0.0), (r - 0.025, 0.02), (r - 0.01, 0.12), (r, 0.22), (r + 0.006, h - 0.02), (r + 0.01, h),
            (r - 0.012, h), (r - 0.02, h - 0.02), (r - 0.03, 0.22), (r - 0.04, 0.12), (r - 0.055, 0.03), (0.0, 0.03)]
    mats = ["reed_dark", "reed", "reed", "reed_band", "reed", "reed_dark", "reed_dark", "reed", "reed", "reed", "reed", "reed_dark"]
    revolve(b, M, prof, mats, n=28)
    for sx in (-1, 1):  # loop handles under the rim
        pts = [V((sx * (r - 0.004), -0.05, h - 0.06)), V((sx * (r + 0.03), -0.03, h - 0.045)), V((sx * (r + 0.035), 0.03, h - 0.045)),
               V((sx * (r - 0.004), 0.05, h - 0.06))]
        b.add(tube([(p, 0.008, 0.008) for p in pts], sub=3, n=6, ref=V((0, 0, 1))), M["reed_dark"], "root")
    lid = basket_lid()
    # The way off: up, over to the right, down onto its rim on the floor, leaning on the basket.
    t = opening
    up = smoothstep(0.0, 0.35, t)
    over = smoothstep(0.25, 0.85, t)
    lean = smoothstep(0.4, 1.0, t)
    rest = V((r + 0.12, -0.02, 0.0))  # where its rim's centre comes to stand, beside the basket
    start = V((0, 0, h))
    pos = start.lerp(rest, over) + V((0, 0, 0.16 * up * (1 - over) + 0.16 * math.sin(math.pi * over) * 0.6))
    # Tipped onto its edge (its top facing right, away from the basket), leaning back on the basket's side.
    tilt = rot(y=72 * lean) @ rot(x=-10 * lean)
    m = Matrix.Translation(pos) @ tilt.to_4x4()
    merge_moved(b, lid, m)


# ---------------------------------------------------------------- canopic jars

JAR_K = 2.6  # decor.py's canopic jar and heads, tile units, to metres
JAR_TOP = 0.108 * JAR_K
CANOPIC_X = (-0.46, -0.23, 0.23, 0.46)
CANOPIC_HEADS = ("human", "baboon", "jackal", "falcon")  # Imsety, Hapy, Duamutef, Qebehsenuef
HEAD_MATS = {"lid": "face", "wig": "wig", "black": "black"}
SLEDGE_H = 0.05


def canopic_jar(b, M, x):
    prof = [(0.0, 0.0), (0.03, 0.0), (0.034, 0.006), (0.04, 0.04), (0.042, 0.075), (0.036, 0.1), (0.03, 0.108), (0.0, 0.108)]
    prof = [(rr * JAR_K, z * JAR_K) for rr, z in prof]
    mats = ["alabaster", "alabaster", "alabaster", "glyphs", "alabaster", "alabaster", "alabaster"]
    revolve(b, M, prof, mats, n=20, m=Matrix.Translation(V((x, 0, SLEDGE_H))))


def canopic_head(kind):
    """A head lid in metres, base at z = 0, facing -Y."""
    p = part()
    M = materials_cache()
    for geo, mat in decor.canopic_head(kind):
        p.add(transform(geo, lambda v: V(v) * JAR_K), M[HEAD_MATS.get(mat, "face")], "root")
    return p


def build_canopic(b, M, opening=1.0):
    """Four alabaster canopic jars with a band of glyphs on a low wooden sledge, a gap in the middle for the item; the
    heads of the four sons of Horus as lids (human, baboon, jackal, falcon). Opening, the heads lift off one after the
    other and are set down on the floor in front of their jars."""
    b.add(place(box(1.18, 0.3, SLEDGE_H), loc=(0, 0, SLEDGE_H / 2)), M["wood"], "root")
    for sx in (-1, 1):  # the sledge's runners curl up at the ends
        b.add(place(box(0.05, 0.3, 0.03), loc=(sx * 0.6, 0, SLEDGE_H + 0.005)), M["wood"], "root")
    for k, (x, kind) in enumerate(zip(CANOPIC_X, CANOPIC_HEADS)):
        canopic_jar(b, M, x)
        head = canopic_head(kind)
        t = smoothstep(0.0, 1.0, (opening - 0.12 * k) / 0.64)
        lift = math.sin(math.pi * min(t / 0.7, 1.0)) * 0.12 if t < 0.7 else 0.0
        seated = V((x, 0, SLEDGE_H + JAR_TOP))
        down = V((x * 1.08, -0.3, 0.0))  # set down on the floor in front of its jar, facing the camera
        pos = seated.lerp(down, smoothstep(0.15, 1.0, t)) + V((0, 0, lift))
        r = rot(x=-25 * math.sin(math.pi * t)) @ rot(z=(k - 1.5) * 10 * t)
        merge_moved(b, head, Matrix.Translation(pos) @ r.to_4x4())


# ---------------------------------------------------------------- sealed jar

JAR_PROFILE = [(0.12, 0.0), (0.16, 0.06), (0.22, 0.2), (0.245, 0.34), (0.235, 0.46), (0.19, 0.56), (0.12, 0.62),
               (0.1, 0.66), (0.115, 0.69)]
JAR_WALL = 0.018
JAR_COLS = 12  # shards round
JAR_BASE_ROWS = 1  # the foot stays standing, its top edge broken
SHARD_SEED = 7


def jar_radius(z):
    for (r0, z0), (r1, z1) in zip(JAR_PROFILE, JAR_PROFILE[1:]):
        if z <= z1:
            return r0 + (r1 - r0) * (z - z0) / (z1 - z0)
    return JAR_PROFILE[-1][0]


def jar_rows():
    """z of the seams between rows of shards: the foot, then irregular rows up to the neck."""
    return [0.0, 0.16, 0.25, 0.34, 0.42, 0.5, 0.57, 0.63, 0.69]


def shard(th0, th1, z0, z1, jit):
    """One piece of the wall: the outer and inner faces over (th0..th1, z0..z1), its corners jittered (jit: the
    seam's own wobble, shared with the neighbours), as a closed hexahedron. Returns (verts, faces, uvs), its
    centre and its outward normal."""
    def pt(th, z, inner):
        r = jar_radius(z) - (JAR_WALL if inner else 0.0)
        return V((r * math.cos(th), r * math.sin(th), z))

    corners = [(th0 + jit[0], z0 + jit[1]), (th1 + jit[2], z0 + jit[3]), (th1 + jit[4], z1 + jit[5]), (th0 + jit[6], z1 + jit[7])]
    outer = [pt(th, z, False) for th, z in corners]
    inner = [pt(th, z, True) for th, z in corners]
    verts = outer + inner
    faces = [[0, 1, 2, 3], [7, 6, 5, 4], [0, 4, 5, 1], [1, 5, 6, 2], [2, 6, 7, 3], [3, 7, 4, 0]]
    c = sum(verts, V()) / 8
    thm = (th0 + th1) / 2
    nrm = V((math.cos(thm), math.sin(thm), 0))
    fixed = [common.orient(verts, f, (sum((verts[i] for i in f), V()) / 4) - c) for f in faces]
    fuv = [[((corners[i % 4][0]) * 0.25, verts[i].z) for i in f] for f in fixed]
    return (verts, fixed, fuv), c, nrm


def build_jar(b, M, opening=1.0):
    """A large storage jar of red-brown pottery, black and ochre bands at the shoulder, the mouth sealed with a dome
    of mud and a stamped seal. It smashes: the shards above the foot fly outwards and land lying flat round it, the
    upper ones farther; the stopper drops beside it. The foot stays standing, its top edge broken."""
    rnd = random.Random(SHARD_SEED)
    rows = jar_rows()
    n = JAR_COLS
    # Seam wobble shared by neighbouring shards: per vertical seam and row, per horizontal seam and column.
    th_jit = [[rnd.uniform(-0.18, 0.18) for _ in range(len(rows))] for _ in range(n)]
    z_jit = [[rnd.uniform(-0.03, 0.03) if 0 < r < len(rows) - 1 else 0.0 for r in range(len(rows))] for _ in range(n + 1)]
    stagger = [rnd.uniform(0, 2 * math.pi / n) for _ in rows]
    t = opening
    for r in range(len(rows) - 1):
        z0, z1 = rows[r], rows[r + 1]
        off = stagger[r] if r >= JAR_BASE_ROWS else 0.0
        for c in range(n):
            th0 = off + 2 * math.pi * c / n
            th1 = off + 2 * math.pi * (c + 1) / n
            jit = [th_jit[c][r], z_jit[c][r], th_jit[(c + 1) % n][r], z_jit[c + 1][r],
                   th_jit[(c + 1) % n][r + 1], z_jit[c + 1][r + 1], th_jit[c][r + 1], z_jit[c][r + 1]]
            if r < JAR_BASE_ROWS:
                jit = [0, 0, 0, 0, jit[4] * 0.5, jit[5], jit[6] * 0.5, jit[7]]  # level on the floor
            geo, centre, nrm = shard(th0, th1, z0, z1, jit)
            mat = "clay_band" if 0.42 <= (z0 + z1) / 2 <= 0.54 else ("clay_in" if r == len(rows) - 2 else "clay")
            p = part()
            p.add(geo, M[mat], "root")
            if r < JAR_BASE_ROWS:
                merge_moved(b, p, Matrix.Identity(4))
                continue
            # Flight: out along its normal and up a little, a spin, then lying on the floor, outside or inside up.
            dist = 0.05 + 0.2 * (z0 / rows[-1]) + rnd.uniform(0.0, 0.1)
            yaw = math.atan2(nrm.y, nrm.x) + rnd.uniform(-0.35, 0.35)
            land = V((math.cos(yaw) * (jar_radius(0.1) + dist), math.sin(yaw) * (jar_radius(0.1) + dist), JAR_WALL / 2 + 0.002))
            face_up = rnd.random() < 0.6  # the painted outside up
            # The rotation that lays it flat: its normal to +Z (or -Z), then a random turn on the floor.
            q_lay = nrm.rotation_difference(V((0, 0, 1 if face_up else -1)))
            q_end = Quaternion(V((0, 0, 1)), rnd.uniform(0, 2 * math.pi)) @ q_lay
            # Times: the top flies first, each a little apart.
            start = 0.05 * (len(rows) - 2 - r) / (len(rows) - 2) + rnd.uniform(0, 0.08)
            u = min(max((t - start) / (0.85 - start), 0.0), 1.0)
            fly = 1 - (1 - u) ** 1.6
            q = Quaternion().slerp(q_end, smoothstep(0.0, 0.9, u))
            pos = centre.lerp(land, fly)
            pos.z += math.sin(math.pi * fly) * (0.08 + 0.1 * z0)
            pos.z = max(pos.z, land.z) if u >= 1.0 else pos.z
            m = Matrix.Translation(pos) @ q.to_matrix().to_4x4() @ Matrix.Translation(-centre)
            merge_moved(b, p, m)
    # The mud stopper over the mouth, the seal pressed into its top; it drops and rolls to the left front.
    top = rows[-1]
    plug = part()
    prof = [(0.0, -0.03), (0.105, -0.03), (0.135, 0.0), (0.13, 0.03), (0.09, 0.07), (0.0, 0.085)]
    revolve(plug, M, prof, ["mud"] * 5, n=20)
    revolve(plug, M, [(0.0, 0.082), (0.04, 0.082), (0.042, 0.092), (0.0, 0.094)], ["seal"] * 3, n=14)
    u = smoothstep(0.0, 0.9, t)
    drop = V((-0.36, -0.22, 0.03))
    pos = V((0, 0, top)).lerp(drop, u)
    pos.z += math.sin(math.pi * u) * 0.12
    merge_moved(b, plug, Matrix.Translation(pos) @ rot(x=-70 * u, y=25 * u).to_4x4())


# ---------------------------------------------------------------- offering table

TABLE_TOP = 0.42  # the slab's top: the item stands there (ITEM_LIFT)
SLAB_W, SLAB_D, SLAB_H = 0.74, 0.46, 0.07


def cover(t):
    """The linen cover's vertices at t 0..1: a draped dome over the middle of the slab (rest), slid back off the
    table and fallen, crumpled flat, on the floor behind it (t = 1). Returns the surface (verts, faces, uvs)."""
    nu, nv = 14, 10
    rnd = random.Random(3)
    wob = [rnd.uniform(-1, 1) for _ in range((nu + 1) * (nv + 1))]
    rx, ry, hh = 0.25, 0.19, 0.2

    def rest(u, v, k):
        a, rr = 2 * math.pi * u, v  # polar grid: v = 0 the top, 1 the hem
        fold = math.sin(7 * a) * rr ** 2
        x, y = math.cos(a) * rx * rr * (1 + 0.08 * fold), math.sin(a) * ry * rr * (1 + 0.08 * fold)
        z = TABLE_TOP + hh * (1 - rr ** 2.2) ** 0.7 + 0.025 * fold * rr + 0.006
        return V((x, y, z))

    def fallen(u, v, k):
        a, rr = 2 * math.pi * u, v
        x = math.cos(a) * rx * 1.3 * rr + 0.04 * wob[k]
        y = SLAB_D / 2 + 0.2 + math.sin(a) * ry * 1.1 * rr + 0.03 * wob[k]
        z = 0.012 + 0.05 * (1 - rr) * (0.6 + 0.4 * wob[k]) ** 2
        return V((x, y, z))

    slide = smoothstep(0.0, 0.55, t)
    fall = smoothstep(0.45, 1.0, t)
    verts, faces, fuv = [], [], []
    for j in range(nv + 1):
        for i in range(nu + 1):
            k = j * (nu + 1) + i
            u, v = i / nu, max(j / nv, 0.02)
            p0, p1 = rest(u, v, k), fallen(u, v, k)
            back = p0 + V((0, (SLAB_D / 2 + 0.05) * slide, 0.08 * math.sin(math.pi * slide)))
            verts.append(back.lerp(p1, fall))
    for j in range(nv):
        for i in range(nu):
            a = j * (nu + 1) + i
            faces.append([a, a + nu + 1, a + nu + 2, a + 1])
            fuv.append([(i / nu, j / nv), (i / nu, (j + 1) / nv), ((i + 1) / nu, (j + 1) / nv), ((i + 1) / nu, j / nv)])
    return verts, faces, fuv


def build_table(b, M, opening=1.0):
    """A limestone offering table: a hetep slab with its spout to the front, two loaves and a bowl of figs carved and
    laid on it, on a round pedestal. The item stands in the middle under a linen cover, blue-hemmed; it slides off
    backwards and falls behind the table."""
    b.add(place(prism([(-0.16, -0.16), (0.16, -0.16), (0.16, 0.16), (-0.16, 0.16)], 0.04), loc=(0, 0, 0)), M["limestone"], "root")
    revolve(b, M, [(0.0, 0.04), (0.1, 0.04), (0.085, 0.08), (0.075, 0.3), (0.095, TABLE_TOP - SLAB_H), (0.0, TABLE_TOP - SLAB_H)],
            ["limestone"] * 5, n=20)
    z = TABLE_TOP - SLAB_H
    b.add(place(box(SLAB_W, SLAB_D, SLAB_H), loc=(0, 0, z + SLAB_H / 2)), M["limestone"], "root")
    b.add(place(box(0.2, 0.12, SLAB_H - 0.01), loc=(0, -SLAB_D / 2 - 0.06, z + (SLAB_H - 0.01) / 2)), M["limestone"], "root")
    b.add(place(box(0.05, 0.13, 0.004), loc=(0, -SLAB_D / 2 - 0.06, TABLE_TOP - 0.008)), M["limestone_plain"], "root")  # the channel
    b.add(place(box(SLAB_W - 0.06, 0.004, SLAB_H - 0.03), loc=(0, -SLAB_D / 2 - 0.002, z + SLAB_H / 2)), M["carving"], "root")
    # Offerings on the corners: two loaves, a bowl of figs.
    for x, y in ((-0.27, -0.12), (-0.27, 0.1)):
        b.add(ellipsoid((x, y, TABLE_TOP + 0.03), (0.07, 0.04, 0.035), n=12, rings=6), M["bread"], "root")
    revolve(b, M, [(0.0, TABLE_TOP), (0.05, TABLE_TOP), (0.08, TABLE_TOP + 0.045), (0.072, TABLE_TOP + 0.045), (0.0, TABLE_TOP + 0.02)],
            ["limestone_plain"] * 4, n=16, m=Matrix.Translation(V((0.27, 0.02, 0))))
    rnd = random.Random(9)
    for k in range(5):
        a = 2 * math.pi * k / 5
        b.add(ellipsoid((0.27 + 0.035 * math.cos(a), 0.02 + 0.035 * math.sin(a), TABLE_TOP + 0.045 + rnd.uniform(0, 0.01)), (0.022, 0.022, 0.02),
                        n=8, rings=4), M["fig"], "root")
    b.add(cover(opening), M["linen"], "root")


BUILDERS = {"coffer": build_coffer, "basket": build_basket, "canopic": build_canopic, "jar": build_jar, "table": build_table}
_MATS = {}


def materials_cache():
    if not _MATS:
        _MATS.update(materials())
    return _MATS


# ---------------------------------------------------------------- entry points


def build(bake=True, tex_dir=None, only=None):
    """Returns {name: obj}, each built at its BAKE_POSE; obj["texture"] = image name when baked."""
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    _MATS.clear()
    M = materials_cache()
    objs = {}
    for k, name in enumerate(n for n in CONTAINERS if not only or n in only):
        b = Builder()
        BUILDERS[name](b, M, opening=BAKE_POSE[name])
        obj = common.finish_mesh(b, coll, "container_" + name)
        obj.data.set_sharp_from_angle(angle=math.radians(50))
        obj.location = (k * SPACING, 0, 0)
        common.uv_unwrap(obj, b.tags)
        if bake:
            out = tex_dir or bpy.app.tempdir or "/tmp"
            stem = "container_" + name
            tex = common.bake_texture(obj, os.path.join(out, stem + ".png"), TEX_SIZE, "item_" + stem, ao_distance=0.12)
            common.use_baked_material(obj, tex)
            obj["texture"] = tex.name
        closed = Builder()
        BUILDERS[name](closed, M, opening=0.0)
        xs, ys, zs = ([v[a] for v in closed.verts] for a in range(3))
        size = max(max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs))
        print("container_{}: {} verts, {} tris; closed x {:.3f}..{:.3f} y {:.3f}..{:.3f} z {:.3f}..{:.3f}: scale {:.2f}".format(
            name, len(obj.data.vertices), common.tri_count(obj), min(xs), max(xs), min(ys), max(ys), min(zs), max(zs),
            size * UNITS_PER_M))
        if name == "table":
            print("container_table: item lift {:.2f}".format(TABLE_TOP * UNITS_PER_M))
        objs[name] = obj
    return objs


def write_frames(obj, path, g, name):
    """The opening clip, FRAMES[name] frames, as shape keys on the baked object, written to path."""
    M = materials_cache()
    count = FRAMES[name]
    obj.shape_key_add(name="Basis")
    keys = []
    for k in range(count):
        b = Builder()
        BUILDERS[name](b, M, opening=k / (count - 1))
        key = obj.shape_key_add(name="open%d" % k)
        if len(b.verts) != len(key.data):
            raise ValueError("{}: frame {} has {} verts, the mesh {}".format(name, k, len(b.verts), len(key.data)))
        for i, v in enumerate(b.verts):
            key.data[i].co = V(v)
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
    models_dir = models_dir or os.path.join(REPO, "media", "models", "items")
    p = os.path.join(REPO, "tools", "blender", "md3_export.py")
    g = {"__file__": p, "__name__": "md3_export"}
    exec(open(p).read(), g)
    for name, obj in objs.items():
        loc = obj.location.copy()
        obj.location = (0, 0, 0)
        bpy.context.view_layer.update()
        write_frames(obj, os.path.join(models_dir, "container_%s.md3" % name), g, name)
        obj.location = loc
    bpy.context.view_layer.update()


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    only = args[args.index("--only") + 1].split(",") if "--only" in args else None
    exporting = "--export" in args
    tex_dir = os.path.join(REPO, "media", "textures", "items")
    objs = build(bake=exporting or "--bake" in args, tex_dir=tex_dir if exporting else None, only=only)
    if exporting:
        export(objs)
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "containers.blend"))
