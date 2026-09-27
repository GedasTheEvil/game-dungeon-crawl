"""Procedural corridor decorations: ten static props scattered along the back wall of empty floor cells, plus the
wall torch (placed separately by the engine, see Dungeon::scatterTorches).

    MCP:  p = ".../tools/blender/models/decor.py"; g = {"__file__": p, "__name__": "decor"}
          exec(open(p).read(), g); g["build"](bake=False)      # then g["export"]()
    CLI:  blender -b --python tools/blender/models/decor.py -- [--export] [--review out.png] [--only web,sand]

Blender space: Z up, 1 unit = 1 tile (the engine draws them at glScale 40 without Centrify, so real sizes
survive). Origin = floor level, horizontal centre of the tile, on the back wall plane; props extend
towards -Y (the camera). The spiderweb is modelled in the upper left corner (x = -0.5, z = 1); the engine
mirrors it for the right corner. The player is ~0.37 tall.

Textures: one 512 PNG per prop (textures/decorations/decor_<name>.png) with the lighting baked in (a sun from the
camera side and above, plus ambient occlusion). The engine draws the props textured only, without the toon
pass: they never turn relative to the camera, and the toon ramp would wash out dark details (eye sockets, charcoal).
"""

import importlib
import math
import os
import random
import sys

import bpy
from mathutils import Euler
from mathutils import Vector as V

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import common  # noqa: E402

importlib.reload(common)
from common import REPO, Builder, cone, ellipsoid, lathe, orient, perp, smoothstep, transform, tube  # noqa: E402

COLL = "decor_new"
PROPS = ["web", "pottery", "canopic", "rubble", "sand", "skeleton", "brazier", "lamp", "scrolls", "ushabti", "torch"]
TEX_SIZE = 512
# Enlargement per prop so it reads next to the player (~0.5 tile tall in game).
# Kept inside x = +-0.42 and y >= -0.43 (the player walks at y = -0.5); the engine's DECOR_JITTER uses the slack.
PROP_SCALE = {"web": 1.2, "pottery": 1.4, "canopic": 1.5, "rubble": 1.3, "skeleton": 1.2, "brazier": 1.35, "lamp": 1.9, "scrolls": 1.6,
              "ushabti": 1.9}
SCALE_PIVOT = {"web": (-0.5, 0.0, 1.0)}  # the web grows out of its corner
SPACING = 1.2  # props are spread along X in the scene (bake/review only; export is at the origin)

COL = {
    "silk": (0.82, 0.82, 0.78),
    "spider": (0.03, 0.025, 0.02),
    "terracotta": (0.42, 0.14, 0.06),
    "terracotta_dark": (0.20, 0.07, 0.03),
    "ochre": (0.62, 0.40, 0.12),
    "black": (0.015, 0.01, 0.008),
    "alabaster": (0.80, 0.74, 0.58),
    "lid": (0.70, 0.60, 0.42),
    "blue": (0.04, 0.16, 0.45),
    "sandstone": (0.55, 0.42, 0.25),
    "sandstone_dark": (0.36, 0.26, 0.14),
    "sand": (0.72, 0.56, 0.32),
    "bone": (0.74, 0.68, 0.52),
    "bone_dark": (0.40, 0.34, 0.22),
    "bronze": (0.32, 0.19, 0.07),
    "verdigris": (0.12, 0.30, 0.22),
    "char": (0.03, 0.025, 0.02),
    "ember": (0.35, 0.08, 0.02),
    "wood": (0.30, 0.18, 0.08),
    "ash": (0.36, 0.34, 0.31),
    "papyrus": (0.76, 0.63, 0.38),
    "papyrus_dark": (0.46, 0.34, 0.16),
    "ink": (0.05, 0.04, 0.03),
    "reed": (0.52, 0.40, 0.17),
    "reed_dark": (0.30, 0.21, 0.08),
    "faience": (0.05, 0.40, 0.38),
    "red": (0.42, 0.05, 0.03),
    "date": (0.16, 0.06, 0.03),
    "bread": (0.60, 0.38, 0.15),
    "gold": (0.85, 0.58, 0.10),
    "iron": (0.07, 0.065, 0.06),
    "pitch": (0.05, 0.035, 0.02),
    "linen_burnt": (0.26, 0.17, 0.09),
}


def stripes(axis, period, bands, interp="CONSTANT"):
    return ("stripes", axis, period, bands, interp)


def materials():
    specs = {
        "silk": ("solid", "silk"),
        "spider": ("solid", "spider"),
        "clay": ("solid", "terracotta"),
        "clay_in": ("solid", "terracotta_dark"),
        "clay_band": stripes("v", 0.012, [(1, "black"), (2, "ochre"), (1, "black"), (3, "terracotta")]),
        "alabaster": ("solid", "alabaster"),
        "alabaster_band": stripes("v", 0.01, [(1, "blue"), (1, "alabaster"), (1, "blue")]),
        "lid": ("solid", "lid"),
        "wig": stripes("u", 1 / 16, [(1, "blue"), (1, "gold")]),
        "black": ("solid", "black"),
        "stone": ("solid", "sandstone"),
        "stone_dark": ("solid", "sandstone_dark"),
        "sand": ("solid", "sand"),
        "bone": ("solid", "bone"),
        "bone_dark": ("solid", "bone_dark"),
        "bronze": stripes("v", 0.03, [(3, "bronze"), (1, "verdigris")], "LINEAR"),
        "char": stripes("u", 1 / 7, [(3, "char"), (1, "ember")], "LINEAR"),
        "bronze_plain": ("solid", "bronze"),
        "wood": stripes("u", 1 / 9, [(2, "wood"), (1, "reed_dark")], "LINEAR"),
        "ash": ("solid", "ash"),
        "papyrus": stripes("u", 1 / 10, [(4, "papyrus"), (1, "papyrus_dark")], "LINEAR"),
        "papyrus_ink": stripes("v", 0.014, [(1, "ink"), (2, "papyrus"), (1, "papyrus")]),
        "red": ("solid", "red"),
        "reed": stripes("v", 0.008, [(1, "reed"), (1, "reed_dark")], "LINEAR"),
        "faience": stripes("v", 0.009, [(3, "faience"), (1, "ink")]),
        "date": ("solid", "date"),
        "bread": ("solid", "bread"),
        "iron": ("solid", "iron"),
        "pitch": ("solid", "pitch"),
        "linen": stripes("v", 0.012, [(2, "linen_burnt"), (1, "pitch")], "LINEAR"),
    }
    return {name: common.make_material("decor_" + name, spec, COL) for name, spec in specs.items()}


# ---------------------------------------------------------------- geometry helpers


def rot(x=0.0, y=0.0, z=0.0):
    return Euler((math.radians(x), math.radians(y), math.radians(z)), "XYZ").to_matrix()


def place(geo, r=None, loc=(0, 0, 0)):
    t = V(loc)
    return transform(geo, lambda v: (r @ V(v) if r else V(v)) + t)


def ground(geo, lift=0.0):
    """Shift geo so its lowest point sits at z = lift."""
    low = min(v.z for v in geo[0])
    return transform(geo, lambda v: V(v) + V((0, 0, lift - low)))


def prism(pts, h):
    """Convex polygon pts (x, y) extruded from z = 0 to z = h."""
    n = len(pts)
    verts = [V((x, y, 0.0)) for x, y in pts] + [V((x, y, h)) for x, y in pts]
    c = sum(verts, V()) / len(verts)
    raw = [list(range(n)), list(range(n, 2 * n))] + [[i, (i + 1) % n, n + (i + 1) % n, n + i] for i in range(n)]
    faces, fuv = [], []
    for f in raw:
        fc = sum((verts[i] for i in f), V()) / len(f)
        f2 = orient(verts, f, fc - c)
        faces.append(f2)
        fuv.append([(verts[i].x, verts[i].y + verts[i].z) for i in f2])
    return verts, faces, fuv


def box(sx, sy, sz):
    return place(prism([(-sx / 2, -sy / 2), (sx / 2, -sy / 2), (sx / 2, sy / 2), (-sx / 2, sy / 2)], sz), loc=(0, 0, -sz / 2))


def blob(r, seed, n=5, jitter=0.35):
    """Irregular convex-ish polygon of radius r (x, y) for shards and patches."""
    rnd = random.Random(seed)
    return [(r * (1 - jitter * rnd.random()) * math.cos(2 * math.pi * (k + 0.3 * rnd.random()) / n),
             r * (1 - jitter * rnd.random()) * math.sin(2 * math.pi * (k + 0.3 * rnd.random()) / n)) for k in range(n)]


def rod(a, b, r0, r1=None, n=8, caps=(True, True)):
    a, b = V(a), V(b)
    r1 = r0 if r1 is None else r1
    return tube([(a, r0, r0), (b, r1, r1)], sub=1, n=n, ref=perp((b - a).normalized()), caps=caps)


def curve(points, r, n=6, ref=V((0, 0, 1)), r_end=None):
    pts = [V(p) for p in points]
    r_end = r if r_end is None else r_end
    keys = [(p, r + (r_end - r) * i / (len(pts) - 1), r + (r_end - r) * i / (len(pts) - 1)) for i, p in enumerate(pts)]
    return tube(keys, sub=3, n=n, ref=ref)


def rock(c, r, seed, amp=0.22, n=9, rings=6):
    """Lumpy ellipsoid, flattened where it would go below the floor."""
    rnd = random.Random(seed)
    waves = [(V((rnd.gauss(0, 1), rnd.gauss(0, 1), rnd.gauss(0, 1))).normalized(), rnd.uniform(0, 6.28), rnd.uniform(2.0, 4.5)) for _ in range(4)]
    c = V(c)

    def fn(v):
        d = V(v) - c
        u = V((d.x / r[0], d.y / r[1], d.z / r[2]))
        un = u.normalized() if u.length > 1e-9 else V((0, 0, 1))
        k = 1 + amp * sum(math.sin(fr * un.dot(a) + ph) for a, ph, fr in waves) / len(waves)
        p = c + d * k
        p.z = max(p.z, 0.0)
        return p

    return transform(ellipsoid(c, r, n, rings), fn)


def arc_len(profile):
    vs = [0.0]
    for (r0, z0), (r1, z1) in zip(profile, profile[1:]):
        vs.append(vs[-1] + math.hypot(r1 - r0, z1 - z0))
    return vs


def revolve(b, M, profile, mats, n=16, z_mod=None, r=None, loc=(0, 0, 0), tag="body"):
    for mat, geo in lathe(profile, arc_len(profile), n, mats, z_mod).items():
        b.add(place(geo, r, loc), M[mat], "root", tag)


def revolve_geo(profile, n=16, z_mod=None):
    """Single-material lathe returned as one geo (for ground() before adding)."""
    return lathe(profile, arc_len(profile), n, ["x"] * (len(profile) - 1), z_mod)["x"]


def surface(fn, nu, nv):
    """Grid surface; fn(u, v) -> point, u should run along +x and v along +y so faces point up (+z)."""
    verts = [V(fn(i / nu, j / nv)) for j in range(nv + 1) for i in range(nu + 1)]
    faces, fuv = [], []
    for j in range(nv):
        for i in range(nu):
            a = j * (nu + 1) + i
            faces.append([a, a + 1, a + nu + 2, a + nu + 1])
            fuv.append([(i / nu, j / nv), ((i + 1) / nu, j / nv), ((i + 1) / nu, (j + 1) / nv), (i / nu, (j + 1) / nv)])
    return verts, faces, fuv


def jag(th, seed, depth):
    return depth * (math.sin(3 * th + seed) + 0.6 * math.sin(7 * th + 2 * seed) + 0.4 * math.sin(11 * th + 3 * seed)) / 2


# ---------------------------------------------------------------- props


def build_web(b, M):
    """Corner web in the upper left corner of the back wall, with a small spider."""
    y, r = -0.006, 0.0032
    corner = V((-0.5 + 0.004, y, 1.0 - 0.004))
    angles = [0, 14, 28, 43, 58, 74, 90]  # from the ceiling (+x) down the side wall (-z)
    lengths = [0.30, 0.34, 0.37, 0.38, 0.36, 0.33, 0.29]

    def pt(i, f):
        a = math.radians(angles[i])
        return corner + V((lengths[i] * f * math.cos(a), 0, -lengths[i] * f * math.sin(a)))

    for i in range(len(angles)):
        b.add(rod(pt(i, 0.0), pt(i, 1.0), r, n=3), M["silk"], "root")
    torn = {(5, 3), (6, 3), (6, 4), (7, 3), (7, 4), (3, 1)}
    for k, f in enumerate([0.1, 0.18, 0.27, 0.37, 0.48, 0.6, 0.73, 0.87]):
        for i in range(len(angles) - 1):
            if (k, i) in torn:
                continue
            p, q = pt(i, f), pt(i + 1, f)
            mid = corner + ((p + q) / 2 - corner) * 0.94
            b.add(rod(p, mid, r * 0.8, n=3), M["silk"], "root")
            b.add(rod(mid, q, r * 0.8, n=3), M["silk"], "root")
    # Torn strands hanging down from the hole.
    for i, f, drop in ((3, 0.73, 0.09), (4, 0.87, 0.13)):
        p = pt(i, f)
        b.add(curve([p, p + V((0.01, 0, -drop * 0.5)), p + V((0.006, 0, -drop))], r * 0.8, n=3, ref=V((0, 1, 0))), M["silk"], "root")
    # Spider sitting on the web.
    s = pt(2, 0.45) + V((0, -0.008, 0))
    b.add(ellipsoid(s + V((0, 0, 0.018)), (0.013, 0.009, 0.017), n=8, rings=5), M["spider"], "root")
    b.add(ellipsoid(s, (0.009, 0.007, 0.009), n=8, rings=4), M["spider"], "root")
    for side in (-1, 1):
        for k in range(4):
            a = math.radians(-60 + 40 * k)
            knee = s + V((side * 0.018 * math.cos(a), -0.008, 0.016 * math.sin(a) + 0.004))
            foot = s + V((side * 0.03 * math.cos(a), -0.002, 0.03 * math.sin(a) - 0.004))
            b.add(rod(s, knee, 0.0022, 0.0018, n=4), M["spider"], "root")
            b.add(rod(knee, foot, 0.0018, 0.0012, n=4), M["spider"], "root")


def build_pottery(b, M):
    """Amphora broken open at the shoulder lying along the wall, its neck and shards scattered in front."""
    prof = [(0.0, 0.0), (0.018, 0.0), (0.024, 0.012), (0.05, 0.04), (0.064, 0.08), (0.066, 0.11), (0.058, 0.145), (0.048, 0.165),
            (0.043, 0.165), (0.052, 0.145), (0.059, 0.11), (0.057, 0.08), (0.044, 0.045), (0.0, 0.03)]
    mats = ["clay", "clay", "clay", "clay", "clay_band", "clay", "clay", "clay", "clay_in", "clay_in", "clay_in", "clay_in", "clay_in"]

    def z_mod(th, r, z):
        if z > 0.16:
            return z + jag(th, 1.0, 0.012) - 0.045 * math.exp(-((th - 1.2) / 0.5) ** 2)
        return z

    lie = rot(y=90) @ rot(z=25)
    lie = rot(z=-12) @ lie
    for mat, geo in lathe(prof, arc_len(prof), 20, mats, z_mod).items():
        b.add(place(geo, lie, (-0.12, -0.085, 0.063)), M[mat], "root")
    # Neck with the rim and one handle stub, lying on its side.
    neck = [(0.032, 0.0), (0.024, 0.015), (0.02, 0.045), (0.027, 0.056), (0.03, 0.064), (0.024, 0.066), (0.016, 0.056), (0.015, 0.02),
            (0.025, 0.002), (0.032, 0.0)]
    neck_mats = ["clay", "clay", "clay", "clay_band", "clay", "clay_in", "clay_in", "clay_in", "clay_in"]

    def neck_mod(th, r, z):
        return z + jag(th, 2.5, 0.008) if z < 0.003 else z

    lie2 = rot(z=62) @ rot(y=90)
    for mat, geo in lathe(neck, arc_len(neck), 14, neck_mats, neck_mod).items():
        b.add(place(geo, lie2, (0.13, -0.19, 0.03)), M[mat], "root")
    b.add(place(curve([(0.02, 0, 0.05), (0.045, 0, 0.045), (0.05, 0, 0.02)], 0.006, n=6, ref=V((0, 1, 0))), lie2, (0.13, -0.19, 0.03)), M["clay"], "root")
    # Shards.
    rnd = random.Random(11)
    for k, (x, y, size) in enumerate(((0.02, -0.16, 0.035), (0.07, -0.12, 0.028), (-0.03, -0.22, 0.03), (0.19, -0.1, 0.024), (-0.2, -0.18, 0.022),
                                      (0.09, -0.25, 0.02))):
        geo = prism(blob(size, 20 + k), 0.007)
        geo = place(geo, rot(rnd.uniform(-12, 12), rnd.uniform(-12, 12), rnd.uniform(0, 360)), (x, y, 0.0))
        b.add(ground(geo), M["clay" if k % 3 else "clay_band"], "root")


def canopic_head(kind):
    """Lid of a canopic jar, base at z = 0, facing -Y. Returns [(geo, mat)]."""
    parts = []
    collar = [(0.0, 0.0), (0.033, 0.0), (0.036, 0.006), (0.03, 0.012), (0.0, 0.012)]
    parts.append((revolve_geo(collar, 16), "lid"))
    if kind == "human":
        parts.append((ellipsoid((0, -0.002, 0.04), (0.021, 0.021, 0.027), n=12, rings=6), "lid"))
        parts.append((ellipsoid((0, 0.005, 0.043), (0.027, 0.024, 0.029), n=12, rings=6), "wig"))
    elif kind == "baboon":
        parts.append((ellipsoid((0, 0, 0.036), (0.024, 0.022, 0.024), n=12, rings=6), "lid"))
        parts.append((ellipsoid((0, -0.02, 0.028), (0.012, 0.014, 0.01), n=10, rings=5), "lid"))
    elif kind == "jackal":
        parts.append((ellipsoid((0, 0, 0.034), (0.02, 0.02, 0.022), n=12, rings=6), "black"))
        parts.append((cone(V((0, -0.012, 0.03)), V((0, -1, -0.25)), 0.035, 0.011, n=8), "black"))
        for s in (-1, 1):
            parts.append((cone(V((s * 0.011, 0.002, 0.048)), V((s * 0.2, 0.05, 1)), 0.028, 0.008, n=6), "black"))
    else:  # falcon
        parts.append((ellipsoid((0, 0, 0.036), (0.022, 0.022, 0.024), n=12, rings=6), "lid"))
        parts.append((cone(V((0, -0.018, 0.034)), V((0, -1, -0.8)), 0.018, 0.008, n=6), "black"))
        for s in (-1, 1):
            parts.append((ellipsoid((s * 0.012, -0.016, 0.042), (0.005, 0.004, 0.004), n=6, rings=3), "black"))
    return parts


def canopic_jar():
    prof = [(0.0, 0.0), (0.03, 0.0), (0.034, 0.006), (0.04, 0.04), (0.042, 0.075), (0.036, 0.1), (0.03, 0.108), (0.0, 0.108)]
    mats = ["alabaster", "alabaster", "alabaster", "alabaster_band", "alabaster", "alabaster", "alabaster"]
    return [(geo, mat) for mat, geo in lathe(prof, arc_len(prof), 16, mats).items()]


def build_canopic(b, M):
    """Four canopic jars against the wall; the last one toppled, its falcon lid rolled away."""
    for x, kind, yaw in ((-0.24, "human", 0), (-0.13, "baboon", 8), (-0.02, "jackal", -6)):
        for geo, mat in canopic_jar():
            b.add(place(geo, rot(z=yaw), (x, -0.07, 0)), M[mat], "root")
        for geo, mat in canopic_head(kind):
            b.add(place(geo, rot(z=yaw), (x, -0.07, 0.108)), M[mat], "root")
    tip = rot(z=-20) @ rot(y=95)
    jar = canopic_jar()
    low = min(v.z for geo, _ in jar for v in place(geo, tip)[0])
    for geo, mat in jar:
        b.add(place(geo, tip, (0.08, -0.1, 0.002 - low)), M[mat], "root")
    lid = rot(z=150) @ rot(x=-70)
    low = min(v.z for geo, _ in canopic_head("falcon") for v in place(geo, lid)[0])
    for geo, mat in canopic_head("falcon"):
        b.add(place(geo, lid, (0.24, -0.17, 0.001 - low)), M[mat], "root")


def build_rubble(b, M):
    """Collapsed stones piled against the wall, a fallen dressed block and a patch of dust."""
    b.add(place(prism(blob(0.26, 3, n=9, jitter=0.2), 0.003), rot(z=5), (0.0, -0.1, 0.0)), M["sand"], "root")
    stones = [((-0.08, -0.06, 0.045), (0.075, 0.06, 0.05), 1), ((0.06, -0.05, 0.04), (0.065, 0.05, 0.045), 2), ((-0.01, -0.035, 0.1), (0.05, 0.04, 0.035), 3),
              ((-0.17, -0.12, 0.018), (0.032, 0.026, 0.022), 4), ((0.15, -0.13, 0.015), (0.026, 0.022, 0.018), 5), ((0.02, -0.15, 0.012), (0.022, 0.018, 0.014), 6),
              ((-0.05, -0.2, 0.008), (0.013, 0.012, 0.01), 7), ((0.09, -0.21, 0.008), (0.012, 0.011, 0.009), 8), ((-0.22, -0.05, 0.02), (0.035, 0.03, 0.026), 9),
              ((0.2, -0.2, 0.006), (0.01, 0.009, 0.008), 10), ((-0.12, -0.17, 0.01), (0.016, 0.014, 0.012), 12)]
    for c, r, seed in stones:
        b.add(rock(c, r, seed), M["stone" if seed % 3 else "stone_dark"], "root")
    block = place(box(0.12, 0.07, 0.06), rot(x=6, y=-9, z=22), (0.2, -0.08, 0.0))
    b.add(ground(block), M["stone_dark"], "root")


def build_sand(b, M):
    """Wind-blown sand heaped against the back wall, tapering to nothing at the tile edges."""
    half, depth, height = 0.47, 0.3, 0.085

    def fn(u, v):
        x = -half + 2 * half * u
        reach = depth * (0.78 + 0.22 * math.sin(5.3 * x + 0.7))
        y = -reach * (1 - v)  # v = 0 front edge, v = 1 at the wall
        t = v
        taper = 1 - smoothstep(0.28, half, abs(x))
        h = height * t ** 1.8 * (0.75 + 0.25 * math.sin(6.1 * x + 1.3)) * taper
        h += 0.003 * math.sin(55 * y + 9 * x) * t * taper
        return (x, y, max(h, 0.0))

    b.add(surface(fn, 36, 10), M["sand"], "root")
    for c, r, seed in (((-0.12, -0.14, 0.004), (0.02, 0.016, 0.014), 31), ((0.17, -0.09, 0.012), (0.024, 0.02, 0.016), 32)):
        b.add(rock(c, r, seed), M["stone"], "root")


def build_skeleton(b, M):
    """Remains of an earlier explorer slumped against the wall, one knee up, skull lolling to the side."""
    bone, dark = M["bone"], M["bone_dark"]

    def knob(c, r):
        b.add(ellipsoid(c, (r, r, r), n=8, rings=4), bone, "root")

    def long_bone(a, c, r):
        b.add(rod(a, c, r, n=7), bone, "root")
        knob(a, r * 1.5)
        knob(c, r * 1.5)

    b.add(ellipsoid((0, -0.06, 0.026), (0.036, 0.021, 0.02), n=12, rings=5), bone, "root")
    spine = [V((0, -0.058, 0.045)).lerp(V((0, -0.02, 0.15)), k / 7) for k in range(8)]
    for p in spine:
        b.add(ellipsoid(p, (0.009, 0.008, 0.006), n=8, rings=4), dark, "root")
    for k in range(5):
        z = 0.09 + 0.013 * k
        base = V((0, -0.02 - 0.038 * (1 - (z - 0.045) / 0.105), z))
        w = 0.035 - 0.004 * abs(k - 2.5)
        for s in (-1, 1):
            pts = [base + V((s * w * math.sin(math.radians(a)), -0.05 * (1 - math.cos(math.radians(a))) / 2, -0.012 * a / 150)) for a in (0, 35, 70, 105, 140)]
            b.add(curve(pts, 0.0032, n=5), bone, "root")
    b.add(ellipsoid((0, -0.065, 0.1), (0.006, 0.004, 0.025), n=6, rings=4), bone, "root")
    for s in (-1, 1):
        b.add(rod((s * 0.004, -0.028, 0.152), (s * 0.045, -0.026, 0.152), 0.003, n=5), bone, "root")
    # Skull, built upright facing -Y, then tipped forward and to the side.
    skull = [(ellipsoid((0, 0, 0), (0.025, 0.029, 0.027), n=12, rings=7), bone), (ellipsoid((0, -0.016, -0.014), (0.017, 0.016, 0.012), n=10, rings=5), bone),
             (ellipsoid((0, -0.012, -0.03), (0.016, 0.014, 0.006), n=10, rings=4), bone), (ellipsoid((0, -0.03, -0.012), (0.004, 0.003, 0.005), n=6, rings=3), M["black"])]
    for s in (-1, 1):
        skull.append((ellipsoid((s * 0.01, -0.025, 0.0), (0.0075, 0.005, 0.0065), n=8, rings=4), M["black"]))
    for geo, mat in skull:
        b.add(place(geo, rot(z=-15) @ rot(x=-28, y=24), (0.016, -0.04, 0.19)), mat, "root")
    # Arms hang down, hands on the floor.
    for s in (-1, 1):
        shoulder, elbow, wrist = V((s * 0.048, -0.03, 0.148)), V((s * 0.062, -0.08, 0.075)), V((s * 0.078, -0.135, 0.01))
        long_bone(shoulder, elbow, 0.0045)
        for off in (-0.003, 0.003):
            b.add(rod(elbow + V((off, 0, 0)), wrist + V((off, 0, 0)), 0.0028, n=5), bone, "root")
        knob(wrist, 0.005)
        for f in range(4):
            a = math.radians(-100 + 25 * f) if s > 0 else math.radians(-80 - 25 * f)
            tip = wrist + V((0.028 * math.cos(a), 0.028 * math.sin(a), -0.004))
            b.add(rod(wrist, tip, 0.0018, n=4), bone, "root")
    # Legs: left knee drawn up, right leg stretched out.
    for s, knee, ankle in ((-1, V((-0.05, -0.18, 0.085)), V((-0.058, -0.28, 0.012))), (1, V((0.06, -0.19, 0.014)), V((0.07, -0.31, 0.012)))):
        hip = V((s * 0.026, -0.07, 0.022))
        long_bone(hip, knee, 0.0055)
        long_bone(knee, ankle, 0.0045)
        b.add(ellipsoid(ankle + V((0, -0.022, -0.004)), (0.011, 0.026, 0.006), n=8, rings=4), bone, "root")
    b.add(ellipsoid((-0.02, -0.26, 0.004), (0.004, 0.02, 0.004), n=6, rings=3), bone, "root")


def build_brazier(b, M):
    """Cold bronze tripod brazier: a shallow bowl of ash and charcoal on three legs with lion paws."""
    c = V((0.0, -0.12, 0.0))
    rim_z, rim_r = 0.17, 0.075
    bowl = [(0.0, 0.12), (0.03, 0.12), (0.06, 0.135), (rim_r, 0.16), (rim_r + 0.006, rim_z), (rim_r - 0.004, rim_z), (0.062, 0.158), (0.0, 0.158)]
    revolve(b, M, bowl, ["bronze", "bronze", "bronze", "bronze", "bronze", "ash", "ash"], n=24, loc=c)
    b.add(ellipsoid(c + V((0, 0, 0.116)), (0.012, 0.012, 0.008), n=8, rings=4), M["bronze"], "root")
    for k in range(3):
        a = math.radians(90 + 120 * k)
        d = V((math.cos(a), math.sin(a), 0))
        top = c + d * (rim_r * 0.8) + V((0, 0, 0.14))
        knee = c + d * (rim_r * 1.15) + V((0, 0, 0.07))
        foot = c + d * (rim_r * 1.05) + V((0, 0, 0.012))
        b.add(curve([top, knee, foot], 0.008, n=7, ref=V((0, 0, 1)).cross(d), r_end=0.006), M["bronze_plain"], "root")
        b.add(ellipsoid(foot + d * 0.006 + V((0, 0, -0.004)), (0.012, 0.014, 0.009), n=8, rings=4, axis=V((0, 0, 1)), ref=d), M["bronze_plain"], "root")
    # Charcoal heaped in the ash, a few lumps glowing faintly.
    for k, (dx, dy, r) in enumerate(((0.0, 0.0, 0.022), (0.03, 0.012, 0.016), (-0.028, 0.018, 0.017), (0.014, -0.03, 0.015), (-0.02, -0.024, 0.013),
                                      (0.04, -0.018, 0.011))):
        b.add(rock(c + V((dx, dy, 0.158)), (r, r * 0.9, r * 0.7), 60 + k, amp=0.3, n=7, rings=4), M["char" if k % 3 else "black"], "root")
    # Spilled ash on the floor.
    b.add(place(prism(blob(0.07, 44, n=8, jitter=0.3), 0.002), loc=c + V((0.01, -0.02, 0))), M["ash"], "root")
    for k, (dx, dy) in enumerate(((0.08, -0.06), (-0.07, -0.08))):
        b.add(rock(c + V((dx, dy, 0.004)), (0.008, 0.007, 0.006), 70 + k, n=6, rings=3), M["char"], "root")


def build_lamp(b, M):
    """Offerings: a clay oil lamp with a charred wick, a bowl of dates, flatbread and a small jug."""
    lamp = [(0.0, 0.0), (0.02, 0.0), (0.028, 0.008), (0.027, 0.016), (0.012, 0.021), (0.007, 0.017), (0.0, 0.017)]
    yaw = rot(z=-130)
    revolve(b, M, lamp, ["clay"] * 3 + ["clay_band", "clay", "clay_in"], n=16, r=yaw, loc=(-0.1, -0.12, 0))
    spout_a, spout_b = V((0.018, 0, 0.01)), V((0.046, 0, 0.014))
    b.add(place(rod(spout_a, spout_b, 0.008, 0.006, n=8), yaw, (-0.1, -0.12, 0)), M["clay"], "root")
    b.add(place(rod(spout_b, spout_b + V((0.008, 0, 0.012)), 0.0025, n=5), yaw, (-0.1, -0.12, 0)), M["char"], "root")
    b.add(place(curve([(-0.02, 0, 0.012), (-0.035, 0, 0.02), (-0.028, 0, 0.004)], 0.004, n=5, ref=V((0, 1, 0))), yaw, (-0.1, -0.12, 0)), M["clay"], "root")
    bowl = [(0.0, 0.0), (0.03, 0.0), (0.05, 0.02), (0.055, 0.03), (0.05, 0.03), (0.045, 0.022), (0.025, 0.008), (0.0, 0.008)]
    revolve(b, M, bowl, ["clay", "clay", "clay_band", "clay", "clay_in", "clay_in", "clay_in"], n=20, loc=(0.08, -0.1, 0))
    rnd = random.Random(5)
    for k in range(6):
        a = 2 * math.pi * k / 6 + rnd.uniform(-0.3, 0.3)
        rr = 0.02 if k else 0.0
        c = V((0.08 + rr * math.cos(a), -0.1 + rr * math.sin(a), 0.016 + (0.006 if k == 0 else 0)))
        b.add(place(ellipsoid((0, 0, 0), (0.007, 0.011, 0.006), n=8, rings=4), rot(z=rnd.uniform(0, 180)), c), M["date"], "root")
    bread = [(0.0, 0.0), (0.026, 0.0), (0.028, 0.004), (0.022, 0.008), (0.0, 0.009)]
    revolve(b, M, bread, ["bread"] * 4, n=16, r=rot(x=8), loc=(0.17, -0.17, 0.003))
    jug = [(0.0, 0.0), (0.018, 0.0), (0.025, 0.02), (0.026, 0.04), (0.016, 0.06), (0.011, 0.07), (0.015, 0.08), (0.012, 0.081), (0.008, 0.07), (0.0, 0.066)]
    revolve(b, M, jug, ["clay", "clay", "clay_band", "clay", "clay", "clay", "clay", "clay_in", "clay_in"], n=14, loc=(-0.005, -0.045, 0))


# Tip of the torch head (fire origin in the engine: TORCH_FIRE in src/world/dungeon_decor.cpp).
TORCH_TILT = 10.0  # degrees, top leans out of the wall (towards -Y)
TORCH_BASE, TORCH_TOP = V((0.0, -0.045, 0.40)), V((0.0, -0.085, 0.63))


def build_torch(b, M):
    """Wall torch: an iron bracket on the back wall holding a tilted wooden shaft with a pitch-soaked linen head."""
    tilt = rot(x=TORCH_TILT)
    axis = (TORCH_TOP - TORCH_BASE).normalized()
    b.add(place(box(0.05, 0.012, 0.12), loc=(0, -0.006, 0.50)), M["iron"], "root")
    for z in (0.455, 0.545):
        b.add(ellipsoid((0, -0.013, z), (0.007, 0.004, 0.007), n=8, rings=3), M["iron"], "root")
    ring_c = TORCH_BASE + axis * ((-0.06 - TORCH_BASE.y) / axis.y)
    b.add(rod((0, -0.012, 0.515), ring_c + V((0, 0.016, 0.004)), 0.006, n=6), M["iron"], "root")
    b.add(curve([(0, -0.012, 0.455), (0, -0.03, 0.46), ring_c + V((0, 0.012, -0.012))], 0.005, n=6, ref=V((1, 0, 0))), M["iron"], "root")
    band = [(0.015, -0.012), (0.02, -0.012), (0.02, 0.012), (0.015, 0.012)]
    revolve(b, M, band, ["iron"] * 3, n=16, r=tilt, loc=ring_c)
    b.add(rod(TORCH_BASE, TORCH_TOP, 0.011, 0.014, n=10), M["wood"], "root")
    head = [(0.013, -0.012), (0.022, 0.0), (0.027, 0.03), (0.025, 0.062), (0.016, 0.082), (0.0, 0.09)]
    revolve(b, M, head, ["linen", "linen", "linen", "pitch", "pitch"], n=16, r=tilt, loc=TORCH_TOP)


def scroll(b, M, length, r, r_mat, loc):
    ax = r_mat @ V((1, 0, 0))
    c = V(loc)
    a, e = c - ax * length / 2, c + ax * length / 2
    keys = [(a, r * 1.1, r * 1.1), (a + ax * 0.006, r, r), (e - ax * 0.006, r, r), (e, r * 1.1, r * 1.1)]
    b.add(tube(keys, sub=1, n=12, ref=perp(ax), bulge=(0.002, 0.002)), M["papyrus"], "root")
    b.add(tube([(c - ax * 0.004, r * 1.08, r * 1.08), (c + ax * 0.004, r * 1.08, r * 1.08)], sub=1, n=12, ref=perp(ax)), M["red"], "root")


def build_scrolls(b, M):
    """Papyrus scrolls spilling out of a broken reed basket, one partly unrolled on the floor."""
    basket = [(0.0, 0.0), (0.04, 0.0), (0.046, 0.02), (0.047, 0.065), (0.042, 0.065), (0.041, 0.02), (0.0, 0.006)]

    def rim(th, r, z):
        return z + jag(th, 4.0, 0.01) - 0.02 * math.exp(-((th - 2.0) / 0.6) ** 2) if z > 0.06 else z

    lie = rot(z=-35) @ rot(y=90)
    for mat, geo in lathe(basket, arc_len(basket), 18, ["reed"] * 6, rim).items():
        b.add(place(geo, lie, (0.1, -0.07, 0.047)), M[mat], "root")
    scroll(b, M, 0.13, 0.013, rot(z=12), (-0.13, -0.07, 0.013))
    scroll(b, M, 0.11, 0.011, rot(z=-22), (-0.1, -0.12, 0.011))
    scroll(b, M, 0.12, 0.012, rot(z=4), (-0.12, -0.095, 0.036))
    scroll(b, M, 0.1, 0.011, rot(z=-60), (0.2, -0.14, 0.011))
    # Unrolled sheet: roll at the back, free end curling up towards the camera.
    length, width = 0.15, 0.075
    base = V((0.0, -0.1, 0.0))
    yaw = rot(z=18)

    def sheet(u, v):
        x = -width / 2 + width * u
        s = length * (1 - v)  # v = 1 at the roll
        curl = 0.02 * smoothstep(0.75, 1.0, s / length) ** 2
        return base + yaw @ V((x, -s, 0.002 + curl))

    b.add(surface(sheet, 4, 12), M["papyrus_ink"], "root")
    scroll(b, M, width + 0.01, 0.01, yaw, base + V((0, 0, 0.01)))


def ushabti(part="all"):
    """Mummiform funerary figurine, standing on z = 0, facing -Y. part: all / lower / upper."""
    keys = [((0, 0, 0.0), 0.011, 0.008), ((0, 0, 0.012), 0.012, 0.009), ((0, 0, 0.03), 0.013, 0.01), ((0, 0, 0.046), 0.015, 0.011), ((0, 0, 0.054), 0.012, 0.009),
            ((0, 0, 0.058), 0.008, 0.007)]
    if part == "lower":
        keys = keys[:3] + [((0, 0, 0.034), 0.013, 0.01)]
    elif part == "upper":
        keys = [((0, 0, 0.032), 0.013, 0.01)] + keys[3:]
    parts = [(tube([(V(p), rx, ry) for p, rx, ry in keys], sub=2, n=10, bulge=(0.002, 0.002)), "faience")]
    if part != "lower":
        parts.append((ellipsoid((0, 0.002, 0.066), (0.012, 0.011, 0.013), n=10, rings=5), "black"))
        parts.append((ellipsoid((0, -0.006, 0.064), (0.007, 0.006, 0.009), n=8, rings=4), "faience"))
        for s in (-1, 1):
            parts.append((rod((s * 0.008, -0.009, 0.042), (-s * 0.004, -0.012, 0.036), 0.003, n=5), "faience"))
    return parts


def build_ushabti(b, M):
    """Five faience ushabti: two still standing, two fallen over, one broken in half."""

    def add(parts, r, loc, lie=False):
        geos = [(place(geo, r), mat) for geo, mat in parts]
        low = min(v.z for geo, _ in geos for v in geo[0]) if lie else 0.0
        for geo, mat in geos:
            b.add(place(geo, None, V(loc) + V((0, 0, -low))), M[mat], "root")

    add(ushabti(), rot(z=5), (-0.2, -0.05, 0))
    add(ushabti(), rot(z=-10), (-0.12, -0.06, 0))
    add(ushabti(), rot(z=30) @ rot(x=-90), (-0.02, -0.13, 0), lie=True)
    add(ushabti(), rot(z=-75) @ rot(x=-90, y=15), (0.07, -0.07, 0), lie=True)
    add(ushabti("lower"), rot(z=12), (0.18, -0.05, 0))
    add(ushabti("upper"), rot(z=140) @ rot(x=-90), (0.2, -0.15, 0), lie=True)


BUILDERS = {
    "web": build_web,
    "pottery": build_pottery,
    "canopic": build_canopic,
    "rubble": build_rubble,
    "sand": build_sand,
    "skeleton": build_skeleton,
    "brazier": build_brazier,
    "lamp": build_lamp,
    "scrolls": build_scrolls,
    "ushabti": build_ushabti,
    "torch": build_torch,
}


# ---------------------------------------------------------------- texture


def bake_lit(obj, out_path, size=TEX_SIZE):
    """Bake colour x light (sun from the camera side and above, sky for ambient occlusion) into one PNG."""
    import numpy as np

    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 96
    if scene.world is None:
        scene.world = bpy.data.worlds.new("World")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (1, 1, 1, 1)
    scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.45
    sun = bpy.data.objects.get("decor_sun")
    if sun is None:
        sun = bpy.data.objects.new("decor_sun", bpy.data.lights.new("decor_sun", "SUN"))
        scene.collection.objects.link(sun)
    sun.data.energy = 0.75
    sun.data.angle = math.radians(10)
    sun.rotation_euler = (math.radians(50), 0, 0)  # shines towards +Y (into the wall) and down

    img = bpy.data.images.new(obj.name + "_bake", size, size, alpha=False)
    for m in obj.data.materials:
        node = m.node_tree.nodes.get("bake_target") or m.node_tree.nodes.new("ShaderNodeTexImage")
        node.name = "bake_target"
        node.image = img
        m.node_tree.nodes.active = node
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.bake(type="DIFFUSE", pass_filter={"COLOR", "DIRECT", "INDIRECT"}, margin=6, use_clear=True)
    for m in obj.data.materials:
        m.node_tree.nodes.remove(m.node_tree.nodes["bake_target"])
    px = np.clip(np.array(img.pixels[:]), 0.0, 1.0)
    tex = bpy.data.images.get(obj.name + "_tex") or bpy.data.images.new(obj.name + "_tex", size, size, alpha=False)
    tex.pixels = px.tolist()
    tex.filepath_raw = out_path
    tex.file_format = "PNG"
    tex.save()
    bpy.data.images.remove(img)
    return tex


# ---------------------------------------------------------------- entry points


def build_env(coll):
    """Floor and back wall under the whole line-up: occluders for the AO bake and a backdrop for reviews."""
    half = SPACING * len(PROPS) / 2 + 0.5
    verts = [(-half, -1.0, 0), (half, -1.0, 0), (half, 0, 0), (-half, 0, 0), (-half, 0, 1.2), (half, 0, 1.2)]
    mesh = bpy.data.meshes.new("decor_env")
    mesh.from_pydata(verts, [], [[0, 1, 2, 3], [3, 2, 5, 4]])
    obj = bpy.data.objects.new("decor_env", mesh)
    coll.objects.link(obj)
    return obj


def build(bake=True, tex_dir=None, only=None):
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    env = bpy.data.meshes.get("decor_env")
    if env:
        bpy.data.meshes.remove(env)
    M = materials()
    build_env(coll)
    objs = {}
    for k, name in enumerate(PROPS):
        if only and name not in only:
            continue
        b = Builder()
        BUILDERS[name](b, M)
        pivot = V(SCALE_PIVOT.get(name, (0, 0, 0)))
        b.verts = [pivot + (V(v) - pivot) * PROP_SCALE.get(name, 1.0) for v in b.verts]
        obj = common.finish_mesh(b, coll, "decor_" + name)
        obj.data.set_sharp_from_angle(angle=math.radians(50))
        obj.location.x = (k - (len(PROPS) - 1) / 2) * SPACING
        common.uv_unwrap(obj, b.tags)
        if bake:
            tex = bake_lit(obj, os.path.join(tex_dir or bpy.app.tempdir or "/tmp", "decor_%s.png" % name))
            common.use_baked_material(obj, tex)
        xs = [v.co.x for v in obj.data.vertices]
        print("decor_{}: {} verts, {} tris, x {:.2f}..{:.2f}, y >= {:.2f}, z <= {:.2f}".format(
            name, len(xs), common.tri_count(obj), min(xs), max(xs), min(v.co.y for v in obj.data.vertices), max(v.co.z for v in obj.data.vertices)))
        objs[name] = obj
    return objs


def export(objs, models_dir=None):
    models_dir = models_dir or os.path.join(REPO, "models", "decorations")
    p = os.path.join(REPO, "tools", "blender", "md3_export.py")
    g = {"__file__": p, "__name__": "md3_export"}
    exec(open(p).read(), g)
    for name, obj in objs.items():
        x = obj.location.x
        obj.location.x = 0.0
        bpy.context.view_layer.update()
        g["export_md3"](obj, os.path.join(models_dir, "decor_%s.md3" % name), 0, 0)
        obj.location.x = x
    bpy.context.view_layer.update()


def review(path, objs):
    """Front view of the line-up (the game camera looks straight at the back wall)."""
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    n = len(objs)
    rows = (n + 4) // 5
    scene.render.resolution_x, scene.render.resolution_y = 1500, 390 * rows
    if scene.world is None:
        scene.world = bpy.data.worlds.new("World")
    scene.world.color = (0.25, 0.25, 0.25)
    for o in list(scene.collection.objects):
        if o.name.startswith("review_"):
            bpy.data.objects.remove(o)
    sun = bpy.data.objects.new("review_sun", bpy.data.lights.new("review_sun", "SUN"))
    sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(60), 0, math.radians(-20))
    scene.collection.objects.link(sun)
    cam = bpy.data.objects.new("review_cam", bpy.data.cameras.new("review_cam"))
    scene.collection.objects.link(cam)
    scene.camera = cam
    cam.data.type = "ORTHO"
    # Grid of cells: 5 per row, each 1 tile wide. Stack rows by moving props temporarily.
    saved = {name: o.location.copy() for name, o in objs.items()}
    for i, (name, o) in enumerate(objs.items()):
        o.location = (i % 5 - 2, 0, -1.3 * (i // 5))
    cam.data.ortho_scale = 5.0
    tilt = math.radians(80)
    centre = V((0, 0, 0.5 - 0.65 * (rows - 1)))
    cam.rotation_euler = (tilt, 0, 0)
    cam.location = centre - V((0, math.sin(tilt), -math.cos(tilt))) * 6
    env = bpy.data.objects.get("decor_env")
    env.hide_render = True
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    env.hide_render = False
    for name, o in objs.items():
        o.location = saved[name]


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    only = args[args.index("--only") + 1].split(",") if "--only" in args else None
    exporting = "--export" in args
    objs = build(bake=exporting or "--bake" in args, tex_dir=os.path.join(REPO, "textures", "decorations") if exporting else None, only=only)
    if "--review" in args:
        review(args[args.index("--review") + 1], objs)
    if exporting:
        export(objs)
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "decor.blend"))
