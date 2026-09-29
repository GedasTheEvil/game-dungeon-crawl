"""Procedural golden scarab (Scarabaeus sacer, plus the giant scarab texture): mesh, rig, baked textures and the
.md3 animations (walk, attack, die, and jump: the giant scarab's leap over pits and traps, plays once, the game
moves the body).

    MCP:  p = ".../tools/blender/models/scarab.py"; g = {"__file__": p, "__name__": "scarab"}
          exec(open(p).read(), g); g["build"]()          # then g["export"]()
    CLI:  blender -b --python tools/blender/models/scarab.py -- [--export]

Blender space: Z up, the beetle faces +Y (game uses rotA = 180), its right side is +X.
Rigid parts (body, head, elytra, antennae, leg segments) each follow one bone. Every frame is
posed procedurally: the body moves as a rigid transform and the six legs are solved with
two-bone IK towards foot targets (planted on the floor or given in body space).
Two textures share one UV layout: scarab.png (gold and lapis) and scarab_giant.png (obsidian and carnelian, red eyes).
Set SCARAB_TEX=giant to show the giant texture on the built object (review renders with --bake).
"""

import importlib
import math
import os
import sys

import bpy
from mathutils import Matrix
from mathutils import Vector as V

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import common  # noqa: E402

importlib.reload(common)
from common import REPO, Builder, ellipsoid, smoothstep, tube  # noqa: E402

COLL = "scarab_new"
CLIPS = [("scarab_walk", "", 24), ("scarab_attack", "_att", 26), ("scarab_die", "_die", 32), ("scarab_jump", "_jump", 10)]
TEX_SIZE = 1024
REVIEW_VIEW = {"target": (0, 0, 0.45), "ortho": 3.6, "res": (520, 400)}

COL = {
    "gold": (0.80, 0.52, 0.12),
    "gold_hi": (0.95, 0.74, 0.28),
    "gold_leg": (0.62, 0.38, 0.07),
    "gold_dark": (0.40, 0.22, 0.035),
    "bronze": (0.16, 0.08, 0.02),
    "lapis": (0.03, 0.08, 0.42),
    "carnelian": (0.55, 0.07, 0.02),
    "eye": (0.30, 0.01, 0.005),
    "black": (0.01, 0.008, 0.005),
}
COL_GIANT = dict(COL, **{
    "gold": (0.055, 0.045, 0.05),
    "gold_hi": (0.32, 0.09, 0.035),
    "gold_leg": (0.045, 0.035, 0.035),
    "gold_dark": (0.018, 0.013, 0.016),
    "bronze": (0.03, 0.01, 0.005),
    "lapis": (0.42, 0.05, 0.02),
    "carnelian": (0.75, 0.22, 0.02),
    "eye": (0.85, 0.05, 0.01),
})

# Elytra: base (v = 0) behind the pronotum to the tip (v = 1).
EL_Y0, EL_Y1, EL_W, EL_H, EL_Z = 0.27, -0.98, 0.76, 0.42, 0.52
EL_TH = -0.42  # the shell wraps from the suture (pi/2) down under the side margin to this angle
PIVOT = V((0, -0.1, 0.5))  # body rotations happen around this point
NECK = V((0, 0.78, 0.47))
HEAD_C = V((0, 0.98, 0.45))  # clypeus plate centre
FOOT_Z = 0.035  # height of the foot joint above the floor


def leg_defs():
    """Rest (neutral stance) layout of the six legs. s = +1 right, -1 left."""
    legs = []
    for s in (1, -1):
        sfx = ".R" if s > 0 else ".L"
        legs.append(dict(name="1" + sfx, s=s, hip=V((0.30 * s, 0.66, 0.40)), foot=V((0.88 * s, 1.12, FOOT_Z)), a=0.46, b=0.62,
                         bend=V((0.3 * s, 0.2, 1)), tdir=None, tlen=0.0, curled=V((0.36 * s, 0.98, 0.10)), phase=0.0 if s < 0 else 0.5))
        legs.append(dict(name="2" + sfx, s=s, hip=V((0.36 * s, 0.14, 0.36)), foot=V((1.08 * s, 0.06, FOOT_Z)), a=0.50, b=0.62,
                         bend=V((0.35 * s, 0, 1)), tdir=V((0.55 * s, -0.84, -0.06)), tlen=0.34, curled=V((0.40 * s, 0.20, 0.06)),
                         phase=0.5 if s < 0 else 0.0))
        legs.append(dict(name="3" + sfx, s=s, hip=V((0.30 * s, -0.14, 0.34)), foot=V((0.92 * s, -1.08, FOOT_Z)), a=0.58, b=0.78,
                         bend=V((0.3 * s, -0.2, 1)), tdir=V((0.30 * s, -0.95, -0.06)), tlen=0.38, curled=V((0.36 * s, -0.50, 0.06)),
                         phase=0.0 if s < 0 else 0.5))
    return legs


LEGS = leg_defs()


# ---------------------------------------------------------------- math helpers


def frame(origin, y, x_hint):
    """4x4 matrix: Y along y, X = x_hint made perpendicular, origin as translation."""
    y = V(y).normalized()
    x = (V(x_hint) - V(x_hint).dot(y) * y).normalized()
    m = Matrix((x, y, x.cross(y))).transposed().to_4x4()
    m.translation = origin
    return m


def solve_leg(hip, foot, a, b, bend):
    """Two-bone IK: knee position and the leg plane normal."""
    d = foot - hip
    c = min(max(d.length, abs(a - b) + 1e-3), (a + b) * 0.999)
    d.normalize()
    e = (bend - bend.dot(d) * d).normalized()
    ca = (a * a + c * c - b * b) / (2 * a * c)
    knee = hip + a * (ca * d + math.sqrt(max(0.0, 1 - ca * ca)) * e)
    return knee, d.cross(e), hip + d * c


def rot_about(pivot, rot3):
    return Matrix.Translation(pivot) @ rot3.to_4x4() @ Matrix.Translation(-pivot)


def euler(pitch=0.0, roll=0.0, yaw=0.0):
    """Degrees. pitch: nose up (+), roll: right side down (+), yaw: turn left (+)."""
    return Matrix.Rotation(math.radians(yaw), 3, "Z") @ Matrix.Rotation(math.radians(roll), 3, "Y") @ Matrix.Rotation(math.radians(pitch), 3, "X")


def orient(verts, face, expected):
    a, b, c = (verts[i] for i in face[:3])
    return face if (b - a).cross(c - a).dot(expected) >= 0 else face[::-1]


def cone(base, direction, length, r, n=6):
    d = V(direction).normalized()
    ref = V((0, 0, 1)) if abs(d.z) < 0.9 else V((1, 0, 0))
    return tube([(base - d * 0.01, r, r), (base + d * length * 0.5, r * 0.55, r * 0.55), (base + d * length, 0.002, 0.002)],
                sub=1, n=n, ref=ref, caps=(True, False))


# ---------------------------------------------------------------- materials


def stripes(axis, period, bands, interp="CONSTANT"):
    return ("stripes", axis, period, bands, interp)


def materials(pal=COL):
    ely = [(1.5, "lapis"), (1, "gold_dark")] + [(6, "gold"), (1, "gold_dark")] * 8 + [(3, "gold_hi"), (3, "lapis"), (10, "gold_dark")]
    specs = {
        "elytra": stripes("u", 1.0, ely),
        "elytra_in": ("solid", "bronze"),
        # Pronotum tube: u 0 / 0.5 = side margins (lapis inlay), 0.75 = underside.
        "pronotum": stripes("u", 1.0, [(1, "lapis"), (1, "gold_hi"), (44, "gold"), (2, "gold_hi"), (4, "lapis"), (2, "gold_hi"), (44, "gold_dark"), (2, "lapis")]),
        "abdomen": stripes("v", 0.13, [(4, "gold_dark"), (1, "bronze")]),
        "clypeus": stripes("v", 0.36, [(26, "gold"), (4, "gold_hi"), (6, "gold_dark")]),
        "gold": ("solid", "gold"),
        "leg": ("solid", "gold_leg"),
        "dark": ("solid", "gold_dark"),
        "bronze": ("solid", "bronze"),
        "lapis": ("solid", "lapis"),
        "sun": ("solid", "carnelian"),
        "eye": ("solid", "eye"),
    }
    return {name: common.make_material("scarab_" + name, spec, pal) for name, spec in specs.items()}


# ---------------------------------------------------------------- parts


def elytron_point(s, a, v, inset=0.0):
    """a: 0 suture -> 1 under the side margin, v: 0 base -> 1 tip."""
    y = EL_Y0 + (EL_Y1 - EL_Y0) * v
    w = EL_W * (1 - v ** 3.0) ** 0.6 * (0.96 + 0.04 * math.sin(math.pi * min(v / 0.5, 1.0)))
    h = EL_H * (1 - v ** 2.6) ** 0.55 * (0.9 + 0.1 * math.sin(math.pi * min(v / 0.7, 1.0)))
    z0 = EL_Z - 0.16 * v * v
    th = math.pi / 2 - a * (math.pi / 2 - EL_TH)
    w, h = max(w - inset, 0.005), max(h - inset, 0.005)
    return V((s * (w * math.cos(th) + 0.006), y, z0 + h * math.sin(th)))


def build_elytron(b, M, s):
    na, nv = 16, 20
    verts, grid = [], {}
    for side, inset in (("out", 0.0), ("in", 0.03)):
        for i in range(nv):
            v = 0.99 * (i / (nv - 1)) ** 0.9
            for j in range(na):
                grid[side, i, j] = len(verts)
                verts.append(elytron_point(s, j / (na - 1), v, inset))
    length = EL_Y0 - EL_Y1
    parts = {"elytra": ([], []), "elytra_in": ([], [])}

    def add(mat, face, expected, uv):
        f = orient(verts, face, expected)
        parts[mat][0].append(f)
        parts[mat][1].append(uv if f is face else uv[::-1])

    def axis_pt(i):
        p = verts[grid["out", i, 0]]
        return V((0, p.y, EL_Z - 0.16 * ((EL_Y0 - p.y) / length) ** 2))

    for i in range(nv - 1):
        v0, v1 = i / (nv - 1) * length, (i + 1) / (nv - 1) * length
        for j in range(na - 1):
            u0, u1 = j / (na - 1), (j + 1) / (na - 1)
            uv = [(u0, v0), (u1, v0), (u1, v1), (u0, v1)]
            q = [grid["out", i, j], grid["out", i, j + 1], grid["out", i + 1, j + 1], grid["out", i + 1, j]]
            add("elytra", q, verts[q[0]] - axis_pt(i), uv)
            q = [grid["in", i, j], grid["in", i, j + 1], grid["in", i + 1, j + 1], grid["in", i + 1, j]]
            add("elytra_in", q, axis_pt(i) - verts[q[0]], uv)
        # Suture edge (lapis line) and the under-margin edge.
        add("elytra", [grid["out", i, 0], grid["out", i + 1, 0], grid["in", i + 1, 0], grid["in", i, 0]], V((-s, 0, 0.3)), [(0.0, v0), (0.0, v1), (0.01, v1), (0.01, v0)])
        last = na - 1
        add("elytra", [grid["out", i, last], grid["out", i + 1, last], grid["in", i + 1, last], grid["in", i, last]], V((0.3 * s, 0, -1)),
            [(0.99, v0), (0.99, v1), (1.0, v1), (1.0, v0)])
    for j in range(na - 1):
        u0, u1 = j / (na - 1), (j + 1) / (na - 1)
        add("elytra", [grid["out", 0, j], grid["out", 0, j + 1], grid["in", 0, j + 1], grid["in", 0, j]], V((0, 1, 0)), [(u0, 0), (u1, 0), (u1, 0.01), (u0, 0.01)])
        e = nv - 1
        add("elytra", [grid["out", e, j], grid["out", e, j + 1], grid["in", e, j + 1], grid["in", e, j]], V((0, -1, 0)),
            [(u0, length), (u1, length), (u1, length), (u0, length)])
    bone = "ely" + (".R" if s > 0 else ".L")
    for mat, (faces, fuv) in parts.items():
        b.add((verts, faces, fuv), M[mat], bone)


def build_body(b, M):
    up, fwd = V((0, 0, 1)), V((0, 1, 0))

    def flat_belly(th, a):
        sn = math.sin(th)
        return 1.0 if sn >= 0 else 1.0 - 0.28 * sn * sn

    # Pronotum and prothorax, back to front.
    pron = [((0, 0.17, 0.55), 0.34, 0.16), ((0, 0.25, 0.56), 0.60, 0.24), ((0, 0.34, 0.565), 0.70, 0.27), ((0, 0.48, 0.565), 0.73, 0.28),
            ((0, 0.62, 0.555), 0.68, 0.26), ((0, 0.73, 0.535), 0.55, 0.21), ((0, 0.80, 0.50), 0.38, 0.14)]
    b.add(tube(pron, sub=3, n=28, ref=up, shape=flat_belly, bulge=(0.04, 0.05)), M["pronotum"], "body")
    # Sun disk inlay on the pronotum: carnelian cabochon in a gold ring.
    sun_c = V((0, 0.47, 0.842))
    b.add(ellipsoid(sun_c, (0.17, 0.15, 0.035), n=18, rings=6), M["sun"], "body")
    ring = [sun_c + V((0.185 * math.cos(2 * math.pi * i / 24), 0.165 * math.sin(2 * math.pi * i / 24), -0.004 - 0.01 * math.cos(2 * math.pi * i / 24) ** 2))
            for i in range(24)]
    b.add(common.loft(ring, [(0.022, 0.018)] * 24, n=8, ref=up, closed=True), M["gold"], "body")
    # Abdomen under the elytra, segment rings along its length.
    b.add(ellipsoid((0, -0.30, 0.47), (0.58, 0.22, 0.56), n=22, rings=14, axis=V((0, -1, 0)), ref=up), M["abdomen"], "body")
    # Meso/metathorax underside between pronotum and abdomen, coxae under every hip.
    b.add(ellipsoid((0, 0.05, 0.43), (0.40, 0.12, 0.34), n=16, rings=8, axis=fwd, ref=up), M["dark"], "body")
    for leg in LEGS:
        h = leg["hip"]
        b.add(ellipsoid(h - V((0.03 * leg["s"], 0, 0)), (0.10, 0.08, 0.09), n=10, rings=6, axis=V((leg["s"], 0, 0)), ref=up), M["dark"], "body")
    for s in (1, -1):
        build_elytron(b, M, s)


def clypeus_radius(phi):
    """Outline of the head plate; phi from +Y towards +X. Six rake teeth across the front."""
    c = math.cos(phi)
    deg = math.degrees(math.atan2(math.sin(phi), c))
    r = 0.29 + 0.05 * c + 0.035 * math.sin(phi) ** 2 * (c < 0.2)
    for tooth, amp in ((7.5, 0.075), (30, 0.08), (52.5, 0.06)):
        for sgn in (1, -1):
            r += amp * max(0.0, 1 - abs(deg - sgn * tooth) / 8.5)
    return r


def build_head(b, M):
    nphi, rho = 96, [0.0, 0.45, 0.75, 0.92, 1.0]
    verts, faces, fuv, mats = [], [], [], []
    idx = {}

    def z_top(p):
        return 0.012 + 0.07 * (1 - p * p) + 0.02 * p ** 6

    def z_bot(p):
        return -0.012 - 0.055 * (1 - p * p) ** 0.7

    for side, zf in (("top", z_top), ("bot", z_bot)):
        idx[side, 0] = len(verts)
        verts.append(V((0, 0, zf(0.0))))
        for i, p in enumerate(rho[1:], 1):
            for j in range(nphi):
                phi = 2 * math.pi * j / nphi
                r = clypeus_radius(phi) * p
                idx[side, i, j] = len(verts)
                verts.append(V((r * math.sin(phi), r * math.cos(phi), zf(p))))

    def add(face, expected, uv):
        f = orient(verts, face, expected)
        faces.append(f)
        fuv.append(uv if f is face else uv[::-1])

    last = len(rho) - 1
    for side, exp in (("top", V((0, 0, 1))), ("bot", V((0, 0, -1)))):
        for j in range(nphi):
            j2 = (j + 1) % nphi
            u0, u1 = j / nphi, (j + 1) / nphi
            add([idx[side, 0], idx[side, 1, j], idx[side, 1, j2]], exp, [(0.5, 0), (u0, rho[1] * 0.36), (u1, rho[1] * 0.36)])
            for i in range(1, last):
                add([idx[side, i, j], idx[side, i, j2], idx[side, i + 1, j2], idx[side, i + 1, j]], exp,
                    [(u0, rho[i] * 0.36), (u1, rho[i] * 0.36), (u1, rho[i + 1] * 0.36), (u0, rho[i + 1] * 0.36)])
    for j in range(nphi):
        j2 = (j + 1) % nphi
        phi = 2 * math.pi * (j + 0.5) / nphi
        add([idx["top", last, j], idx["top", last, j2], idx["bot", last, j2], idx["bot", last, j]], V((math.sin(phi), math.cos(phi), 0)),
            [(j / nphi, 0.355), ((j + 1) / nphi, 0.355), ((j + 1) / nphi, 0.36), (j / nphi, 0.36)])
    tilt = rot_about(V((0, 0, 0)), Matrix.Rotation(math.radians(-19), 3, "X"))
    verts = [HEAD_C + tilt @ v for v in verts]
    b.add((verts, faces, fuv), M["clypeus"], "head")
    # Head capsule under the plate, compound eyes on the sides.
    b.add(ellipsoid((0, 0.88, 0.40), (0.27, 0.12, 0.2), n=16, rings=8, axis=V((0, 1, 0)), ref=V((0, 0, 1))), M["dark"], "head")
    for s in (1, -1):
        b.add(ellipsoid((0.285 * s, 0.84, 0.405), (0.07, 0.06, 0.08), n=12, rings=6, axis=V((0, 1, 0)), ref=V((0, 0, 1))), M["eye"], "head")


def antenna_points(s):
    base = V((0.20 * s, 1.0, 0.37))
    return base, base + V((0.13 * s, 0.13, -0.04))


def build_antennae(b, M):
    for s in (1, -1):
        base, end = antenna_points(s)
        bone = "ant" + (".R" if s > 0 else ".L")
        b.add(tube([(base, 0.018, 0.018), ((base + end) / 2 + V((0, 0, 0.01)), 0.015, 0.015), (end, 0.02, 0.02)], sub=2, n=8,
                   ref=V((0, 0, 1)), bulge=(0.005, 0.0)), M["bronze"], bone)
        d = (end - base).normalized()
        for k, fan in enumerate((-22, 0, 22)):
            ax = Matrix.Rotation(math.radians(fan), 3, V((0, 0, 1))) @ d
            c = end + ax * 0.05 + V((0, 0, (k - 1) * 0.022))
            b.add(ellipsoid(c, (0.028, 0.012, 0.055), n=10, rings=5, axis=ax, ref=V((0, 0, 1))), M["dark"], bone)


def rest_leg(leg):
    knee, normal, foot = solve_leg(leg["hip"], leg["foot"], leg["a"], leg["b"], leg["bend"])
    return knee, normal, foot


def build_legs(b, M):
    for leg in LEGS:
        s, hip, name = leg["s"], leg["hip"], leg["name"]
        knee, normal, foot = rest_leg(leg)
        fd = (knee - hip).normalized()
        td = (foot - knee).normalized()
        lat = V((s, 0, 0))
        front = name[0] == "1"
        thick = {"1": 0.095, "2": 0.075, "3": 0.082}[name[0]]
        # Femur: flattened in the leg plane, thickest in the middle.
        fem = [(hip - fd * 0.03, thick * 0.7, thick * 0.55), (hip + fd * leg["a"] * 0.4, thick, thick * 0.72),
               (knee - fd * 0.02, thick * 0.72, thick * 0.55), (knee + fd * 0.03, thick * 0.55, thick * 0.45)]
        b.add(tube(fem, sub=2, n=10, ref=normal, bulge=(0.02, 0.015)), M["leg"], "femur" + name)
        b.add(ellipsoid(knee, (thick * 0.62, thick * 0.62, thick * 0.62), n=10, rings=5), M["dark"], "tibia" + name)
        if front:
            # Digging tibia: flat blade, four teeth on the outer edge.
            side = (lat - lat.dot(td) * td).normalized()
            blade_n = td.cross(side)
            tib = [(knee - td * 0.02, 0.055, 0.045), (knee + td * leg["b"] * 0.35, 0.068, 0.036), (knee + td * leg["b"] * 0.75, 0.09, 0.032),
                   (foot, 0.085, 0.03)]
            b.add(tube(tib, sub=2, n=10, ref=blade_n, bulge=(0.01, 0.02)), M["leg"], "tibia" + name)  # rx (wide) runs along side
            for k, t in enumerate((0.45, 0.62, 0.8, 0.98)):
                p = knee + td * leg["b"] * t
                w = 0.068 + 0.022 * t
                b.add(cone(p + side * w * 0.8, side + td * 0.55, 0.07 + 0.03 * k, 0.03), M["bronze"], "tibia" + name)
            b.add(cone(foot - td * 0.02, td + V((0, 0, -0.2)), 0.09, 0.025), M["bronze"], "tibia" + name)
            continue
        tib = [(knee - td * 0.02, 0.042, 0.042), (knee + td * leg["b"] * 0.5, 0.05, 0.047), (foot - td * 0.01, 0.062, 0.056), (foot + td * 0.02, 0.05, 0.045)]
        b.add(tube(tib, sub=2, n=10, ref=normal, bulge=(0.01, 0.012)), M["leg"], "tibia" + name)
        back = (-normal if normal.dot(V((0, -1, 0))) < 0 else normal)
        for t in (0.35, 0.55, 0.75):
            p = knee + td * leg["b"] * t
            b.add(cone(p + back * 0.04, back + td * 0.8, 0.08, 0.017), M["bronze"], "tibia" + name)
        for off in (-1, 1):
            b.add(cone(foot + normal * 0.03 * off, td + normal * 0.25 * off, 0.1, 0.016), M["bronze"], "tibia" + name)
        # Tarsus: five beads and two claws, lying on the floor.
        t_dir = leg["tdir"].normalized()
        p = foot.copy()
        bone = "tarsus" + name
        for k in range(5):
            r = 0.04 - 0.005 * k
            seg = leg["tlen"] / 5
            c = p + t_dir * seg * 0.5
            b.add(ellipsoid(c, (r, r * 0.85, seg * 0.62), n=8, rings=4, axis=t_dir, ref=V((0, 0, 1))), M["leg"], bone)
            p = p + t_dir * seg
        for off in (-1, 1):
            b.add(cone(p, t_dir + normal * 0.5 * off + V((0, 0, -0.35)), 0.07, 0.012), M["bronze"], bone)


# ---------------------------------------------------------------- rig


def rigid_bones():
    """name -> (head, tail, parent) for bones whose X axis is world X at rest."""
    bones = {
        "root": ((0, 0, 0), (0, 0.3, 0), None),
        "body": (PIVOT, PIVOT + V((0, 0.5, 0)), "root"),
        "head": (NECK, NECK + V((0, 0.35, 0)), "body"),
    }
    for s in (1, -1):
        sfx = ".R" if s > 0 else ".L"
        hinge = V((0.05 * s, EL_Y0, EL_Z + EL_H * 0.9))
        bones["ely" + sfx] = (hinge, hinge + V((0, -0.5, 0)), "body")
        base, end = antenna_points(s)
        bones["ant" + sfx] = (base, end, "head")
    return bones


def build_rig(coll):
    arm = bpy.data.armatures.new("scarab_rig")
    rig = bpy.data.objects.new("scarab_rig", arm)
    coll.objects.link(rig)
    arm.display_type = "STICK"
    rig.show_in_front = True
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    eb = arm.edit_bones

    def bone(name, head, tail, x_axis, parent):
        e = eb.new(name)
        e.head, e.tail = V(head), V(tail)
        e.align_roll(V(x_axis).cross((e.tail - e.head).normalized()))
        if parent:
            e.parent = eb[parent]

    for name, (head, tail, parent) in rigid_bones().items():
        bone(name, head, tail, (1, 0, 0), parent)
    for leg in LEGS:
        knee, normal, foot = rest_leg(leg)
        n = leg["name"]
        bone("femur" + n, leg["hip"], knee, normal, "body")
        bone("tibia" + n, knee, foot, normal, "femur" + n)
        if leg["tdir"] is not None:
            bone("tarsus" + n, foot, foot + leg["tdir"].normalized() * leg["tlen"], normal, "tibia" + n)
    bpy.ops.object.mode_set(mode="OBJECT")
    for pb in rig.pose.bones:
        pb.rotation_mode = "QUATERNION"
    return rig


# ---------------------------------------------------------------- animation

REST = dict(x=0.0, y=0.0, z=0.0, pitch=0.0, roll=0.0, yaw=0.0, hp=0.0, hy=0.0, ely=0.0, ant=0.0,
            plant=1.0, fplant=1.0, fup=0.0, ffwd=0.0, fout=0.0, curl=0.0, flail=0.0, fphase=0.0)
STRIDE, STEP_H = 0.42, 0.14


def body_matrix(p):
    return Matrix.Translation(V((p["x"], p["y"], p["z"]))) @ rot_about(PIVOT, euler(p["pitch"], p["roll"], p["yaw"]))


def gait(leg, t):
    """Walk: foot position (armature space) and swing amount for phase t of the cycle."""
    ph = (t + leg["phase"]) % 1.0
    base = leg["foot"]
    if ph < 0.5:  # stance: foot slides back under the moving body
        k = ph / 0.5
        return base + V((0, STRIDE * (0.5 - k), 0)), 0.0
    k = (ph - 0.5) / 0.5
    lift = math.sin(math.pi * k)
    return base + V((0, STRIDE * (smoothstep(0, 1, k) - 0.5), STEP_H * lift)), lift


def walk_params(t):
    p = dict(REST)
    w = 2 * math.pi * t
    p.update(z=0.012 * math.cos(2 * w), roll=1.5 * math.sin(w), yaw=1.5 * math.cos(w), pitch=0.8 * math.sin(2 * w),
             hp=2 * math.sin(2 * w + 1), hy=3 * math.sin(w), ant=8 * math.sin(2 * w + 0.5), ely=0.0)
    p["feet"] = {leg["name"]: gait(leg, t) for leg in LEGS}
    return p


READY = walk_params(0.0)
READY_FEET = {k: v[0].copy() for k, v in READY["feet"].items()}


def leg_targets(p, T):
    """Foot target (armature space) and swing amount per leg for non-walk frames."""
    out = {}
    for leg in LEGS:
        n, s = leg["name"], leg["s"]
        front = n[0] == "1"
        planted = READY_FEET[n]
        free = planted.copy()
        if front:
            free += V((p["fout"] * s, p["ffwd"], p["fup"]))
        free = free.lerp(leg["curled"], p["curl"])
        ph = 2 * math.pi * (p["fphase"] + {"1": 0.0, "2": 0.33, "3": 0.66}[n[0]] + (0.17 if s > 0 else 0.0))
        free += V((0.5 * s * math.sin(ph), 0.7 * math.cos(ph * 1.3), 0.6 * math.sin(ph + 1))) * p["flail"]
        w = p["fplant"] if front else p["plant"]
        target = planted.lerp(T @ free, 1 - w)
        if p["roll"] < 45:
            target.z = max(target.z, FOOT_Z)
        out[n] = (target, 1 - w)
    return out


def pose_matrices(rig, p):
    """Armature-space pose matrices of every bone for parameters p."""
    bones = rig.data.bones
    rest = {b.name: b.matrix_local.copy() for b in bones}
    T = body_matrix(p)
    R = T.to_3x3()
    head_m = T @ rot_about(NECK, euler(p["hp"], 0, p["hy"]))
    mats = {"root": rest["root"], "body": T @ rest["body"], "head": head_m @ rest["head"]}
    for s in (1, -1):
        sfx = ".R" if s > 0 else ".L"
        hinge = bones["ely" + sfx].head_local
        # Lift the back of the elytron and swing it outwards.
        open_m = rot_about(hinge, Matrix.Rotation(math.radians(-0.6 * p["ely"] * s), 3, "Y") @ Matrix.Rotation(math.radians(-p["ely"]), 3, "X"))
        mats["ely" + sfx] = T @ open_m @ rest["ely" + sfx]
        base = bones["ant" + sfx].head_local
        mats["ant" + sfx] = head_m @ rot_about(base, euler(p["ant"], 0, -0.5 * p["ant"] * s)) @ rest["ant" + sfx]
    feet = p["feet"] if "feet" in p else leg_targets(p, T)
    for leg in LEGS:
        n = leg["name"]
        knee0, normal0, foot0 = rest_leg(leg)
        target, swing = feet[n]
        hip = T @ leg["hip"]
        knee, normal, foot = solve_leg(hip, target, leg["a"], leg["b"], R @ leg["bend"])
        f0 = frame(leg["hip"], knee0 - leg["hip"], normal0)
        mats["femur" + n] = frame(hip, knee - hip, normal) @ f0.inverted() @ rest["femur" + n]
        t0 = frame(knee0, foot0 - knee0, normal0)
        mats["tibia" + n] = frame(knee, foot - knee, normal) @ t0.inverted() @ rest["tibia" + n]
        if leg["tdir"] is not None:
            tr0 = frame(foot0, leg["tdir"], normal0)
            ground = leg["tdir"].normalized()
            ground_body = R @ ground
            hang = (ground_body + (foot - knee).normalized() * 0.8).normalized()
            planted = V((ground.x, ground.y, 0)).normalized() + V((0, 0, ground.z))
            k = min(1.0, swing)
            tdir = planted.lerp(hang, k)
            mats["tarsus" + n] = frame(foot, tdir, normal) @ tr0.inverted() @ rest["tarsus" + n]
    return mats


def key_frame(rig, p, frame_no):
    mats = pose_matrices(rig, p)
    bones = rig.data.bones
    for b in bones:
        pose = mats[b.name]
        if b.parent:
            rel = b.parent.matrix_local.inverted() @ b.matrix_local
            basis = rel.inverted() @ mats[b.parent.name].inverted() @ pose
        else:
            basis = b.matrix_local.inverted() @ pose
        pb = rig.pose.bones[b.name]
        pb.matrix_basis = basis
        pb.keyframe_insert("rotation_quaternion", frame=frame_no)
        pb.keyframe_insert("location", frame=frame_no)


def interpolate(keys, frame_no, linear=("fphase",)):
    full = []
    for f, over in keys:
        p = {k: v for k, v in READY.items() if k != "feet"}
        p.update(over)
        full.append((f, p))
    for (f0, p0), (f1, p1) in zip(full, full[1:]):
        if f0 <= frame_no <= f1:
            t = smoothstep(f0, f1, frame_no)
            lt = (frame_no - f0) / (f1 - f0)
            return {k: p0[k] + (p1[k] - p0[k]) * (lt if k in linear else t) for k in p0}
    return full[-1][1]


ATTACK = [
    (0, {}),
    (6, dict(pitch=20, y=-0.12, z=0.10, hp=14, ely=14, ant=25, fplant=0, fup=0.55, ffwd=-0.05, fout=0.12)),
    (9, dict(pitch=24, y=-0.14, z=0.13, hp=18, ely=18, ant=30, fplant=0, fup=0.62, ffwd=0.05, fout=0.14)),
    (12, dict(pitch=-7, y=0.30, z=-0.04, hp=-18, ely=10, ant=5, fplant=0, fup=0.03, ffwd=0.45, fout=-0.02)),
    (14, dict(pitch=-8, y=0.32, z=-0.03, hp=-16, ely=8, ant=0, fplant=0, fup=0.0, ffwd=0.46, fout=-0.02)),
    (17, dict(pitch=-5, y=0.26, z=-0.03, hp=-10, hy=12, ely=6, fplant=0, fup=0.02, ffwd=0.05, fout=0.04)),
    (20, dict(pitch=-2, y=0.14, hp=-4, hy=-8, ely=3, fplant=0.3, fup=0.04, ffwd=0.0)),
    (26, {}),
]

DIE = [
    (0, {}),
    (3, dict(pitch=14, y=-0.12, z=0.05, hp=18, ely=12, ant=-20, fplant=0.3, fup=0.25)),
    (6, dict(pitch=6, y=-0.15, roll=22, hp=10, ely=10, ant=-10, plant=0.6, fplant=0, fup=0.3, flail=0.12, fphase=0.3)),
    (9, dict(roll=85, x=0.25, y=-0.15, hp=5, ely=8, plant=0, fplant=0, flail=0.18, fphase=0.7)),
    (12, dict(roll=150, x=0.45, y=-0.15, hp=-5, ely=8, plant=0, fplant=0, flail=0.2, fphase=1.1)),
    (14, dict(roll=180, x=0.5, y=-0.15, hp=-10, ely=10, plant=0, fplant=0, flail=0.2, fphase=1.4)),
    (17, dict(roll=180, x=0.5, y=-0.15, hp=-8, ely=10, plant=0, fplant=0, flail=0.22, fphase=2.0)),
    (20, dict(roll=180, x=0.5, y=-0.15, hp=-12, ely=10, plant=0, fplant=0, flail=0.18, fphase=2.6, curl=0.2)),
    (23, dict(roll=180, x=0.5, y=-0.15, hp=-12, ely=12, plant=0, fplant=0, flail=0.12, fphase=3.2, curl=0.45)),
    (26, dict(roll=180, x=0.5, y=-0.15, hp=-14, ely=12, plant=0, fplant=0, flail=0.06, fphase=3.7, curl=0.75, ant=-20)),
    (29, dict(roll=180, x=0.5, y=-0.15, hp=-14, ely=12, plant=0, fplant=0, flail=0.03, fphase=4.2, curl=0.9, ant=-30)),
    (31, dict(roll=180, x=0.5, y=-0.15, hp=-15, ely=12, plant=0, fplant=0, flail=0.0, fphase=4.5, curl=1.0, ant=-35)),
]

# Leap (one-shot): crouch, spring off with the elytra flung open, legs tucked, reach down, landing crouch. The game
# moves the body along the arc (Monster::UpdateJump: feet leave the floor at 22 %, touch it at 83 % of the clip),
# so the pose stays on the floor and ends where it started.
JUMP = [
    (0, {}),
    (2, dict(z=-0.06, pitch=-6, hp=-6, ely=6, ant=-10)),
    (3, dict(z=0.02, pitch=12, hp=6, ely=16, ant=15, fplant=0, fup=0.10, ffwd=0.10, plant=0.3)),
    (4, dict(pitch=6, ely=24, ant=25, fplant=0, fup=0.12, ffwd=0.18, plant=0, curl=0.3)),
    (6, dict(pitch=0, ely=26, ant=25, fplant=0, fup=0.08, ffwd=0.18, plant=0, curl=0.35)),
    (7, dict(pitch=-8, ely=16, ant=10, fplant=0, fup=0.02, ffwd=0.10, plant=0, curl=0.1)),
    (8, dict(z=-0.05, pitch=-5, hp=-6, ely=10, fplant=1, plant=0.6)),
    (9, dict(z=-0.03, pitch=-2, ely=3)),
]


def lowest_z(obj):
    ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mw = ev.matrix_world
    return min((mw @ v.co).z for v in ev.data.vertices)


def make_actions(rig, obj):
    rig.animation_data_create()
    scene = bpy.context.scene
    acts = {}
    for name, suffix, frames in CLIPS:
        act = bpy.data.actions.get(name)
        if act:
            bpy.data.actions.remove(act)
        act = bpy.data.actions.new(name)
        act.use_fake_user = True
        rig.animation_data.action = act
        last = frames if name not in ("scarab_die", "scarab_jump") else frames - 1  # loops key frame N = frame 0
        for f in range(last + 1):
            if name == "scarab_walk":
                key_frame(rig, walk_params(f / frames), f)
            else:
                key_frame(rig, interpolate({"scarab_attack": ATTACK, "scarab_die": DIE, "scarab_jump": JUMP}[name], f), f)
        acts[name] = act
    # Feet on the floor at frame 0 of the walk (shared by all files).
    rig.animation_data.action = acts["scarab_walk"]
    scene.frame_set(0)
    rig.location.z -= lowest_z(obj)
    # Dying: the body rolls onto its back, keep the lowest point on the floor every frame.
    rig.animation_data.action = acts["scarab_die"]
    root = rig.pose.bones["root"]
    for f in range(CLIPS[2][2]):
        scene.frame_set(f)
        low = lowest_z(obj)
        if f > 2:
            root.location.z -= low
            root.keyframe_insert("location", frame=f)
    # Leaping: the hanging tarsi would dip through the floor early in the arc, lift the root above it.
    rig.animation_data.action = acts["scarab_jump"]
    for f in range(CLIPS[3][2]):
        scene.frame_set(f)
        low = lowest_z(obj)
        if low < 0:
            root.location.z -= low
            root.keyframe_insert("location", frame=f)
    return acts


# ---------------------------------------------------------------- entry points


def build(bake=True, tex_path=None, giant_path=None):
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    M = materials()
    rig = build_rig(coll)
    b = Builder()
    build_body(b, M)
    first = len(b.verts)
    build_head(b, M)
    build_antennae(b, M)
    b.tags[first:] = ["head"] * (len(b.verts) - first)
    build_legs(b, M)
    obj = common.finish_mesh(b, coll, "scarab_new")
    common.uv_unwrap(obj, b.tags, {"head": (HEAD_C, 1.5)})
    if bake:
        tmp = bpy.app.tempdir or "/tmp"
        tex = common.bake_texture(obj, tex_path or os.path.join(tmp, "scarab_preview.png"), TEX_SIZE, "scarab", ao_distance=0.2)
        materials(COL_GIANT)
        giant = common.bake_texture(obj, giant_path or os.path.join(tmp, "scarab_giant_preview.png"), TEX_SIZE, "scarab_giant", ao_distance=0.2)
        common.use_baked_material(obj, giant if os.environ.get("SCARAB_TEX") == "giant" else tex)
    common.rig_object(obj, rig)
    acts = make_actions(rig, obj)
    rig.animation_data.action = acts["scarab_walk"]
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 0, CLIPS[0][2] - 1
    scene.frame_set(0)
    print("scarab: {} verts, {} tris".format(len(obj.data.vertices), common.tri_count(obj)))
    return obj, rig


def export(models_dir=None):
    common.export_files(bpy.data.objects["scarab_new"], bpy.data.objects["scarab_rig"], "scarab", CLIPS, "monsters", models_dir)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    tex_dir = os.path.join(REPO, "textures", "monsters")
    if "--export" in args:
        build(tex_path=os.path.join(tex_dir, "scarab.png"), giant_path=os.path.join(tex_dir, "scarab_giant.png"))
    else:
        build()
    if "--export" in args:
        export()
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "scarab.blend"))
