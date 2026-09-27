"""Procedural tomb lotus: a man-eating blue lotus growing out of a cracked, painted Egyptian jar.

    MCP:  p = ".../tools/blender/models/plant.py"; g = {"__file__": p, "__name__": "plant"}
          exec(open(p).read(), g); g["build"]()          # then g["export"]()
    CLI:  blender -b --python tools/blender/models/plant.py -- [--export]

Blender space: Z up, the bloom faces -Y (game uses rotA = 0). The plant never moves: the walk
file is an idle loop. A cobra-like stalk carries the bloom (petals hinge open around a fanged maw),
thorny vines spill over the jar rim onto the floor. Everything is FK: per-bone Euler angles from
pose parameters, every frame keyed; poses that would push the bloom through the floor are eased
off by bisection (lunge in the attack, droop while dying).
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
from common import REPO, Builder, _cr, chain_weights, cone, ellipsoid, lathe, orient, perp, smoothstep, tube  # noqa: E402

COLL = "plant_new"
CLIPS = [("plant_walk", "", 32), ("plant_attack", "_att", 26), ("plant_die", "_die", 36)]
TEX_SIZE = 1024
REVIEW_VIEW = {"target": (0, -0.4, 1.5), "ortho": 4.4, "res": (440, 480)}

N = 10  # stalk bones
NV = 8  # bones per vine
PETALS = 8  # per ring (inner + outer)

COL = {
    "terracotta": (0.42, 0.14, 0.06),
    "terracotta_dark": (0.22, 0.07, 0.03),
    "ochre": (0.62, 0.40, 0.12),
    "blue": (0.04, 0.16, 0.45),
    "red": (0.42, 0.05, 0.03),
    "black": (0.015, 0.01, 0.008),
    "soil": (0.05, 0.03, 0.015),
    "stalk": (0.035, 0.13, 0.06),
    "stalk_hi": (0.10, 0.26, 0.09),
    "vine": (0.07, 0.11, 0.035),
    "leaf": (0.05, 0.20, 0.06),
    "leaf_vein": (0.13, 0.32, 0.10),
    "thorn": (0.16, 0.06, 0.04),
    "cream": (0.80, 0.78, 0.64),
    "sky": (0.32, 0.46, 0.85),
    "lotus": (0.07, 0.16, 0.68),
    "violet": (0.17, 0.04, 0.38),
    "gum": (0.55, 0.12, 0.20),
    "throat": (0.30, 0.02, 0.05),
    "ivory": (0.80, 0.74, 0.58),
    "gold": (0.85, 0.58, 0.10),
}


# ---------------------------------------------------------------- helpers


def resample(points, count, dense=12):
    """Catmull-Rom through points, returned as count + 1 points evenly spaced by arc length."""
    pts = [V(p) for p in points]
    m = len(pts)
    fine = []
    for i in range(m - 1):
        k = [pts[max(i - 1, 0)], pts[i], pts[i + 1], pts[min(i + 2, m - 1)]]
        for s in range(dense):
            fine.append(_cr(*k, s / dense))
    fine.append(pts[-1])
    arc = [0.0]
    for a, b in zip(fine, fine[1:]):
        arc.append(arc[-1] + (b - a).length)
    out, j = [], 0
    for k in range(count + 1):
        target = arc[-1] * k / count
        while j < len(arc) - 2 and arc[j + 1] < target:
            j += 1
        t = (target - arc[j]) / max(arc[j + 1] - arc[j], 1e-9)
        out.append(fine[j].lerp(fine[j + 1], min(max(t, 0.0), 1.0)))
    return out


# ---------------------------------------------------------------- materials


def stripes(axis, period, bands, interp="CONSTANT"):
    return ("stripes", axis, period, bands, interp)


def materials():
    specs = {
        "jar": ("solid", "terracotta"),
        "jar_in": ("solid", "terracotta_dark"),
        "frieze": stripes("u", 1 / 18, [(2, "blue"), (1, "ochre"), (2, "red"), (1, "ochre")]),
        "band": stripes("v", 0.07, [(1, "black"), (3, "ochre"), (1, "black"), (3, "blue")]),
        "lip": stripes("v", 0.5, [(1, "ochre")]),
        "soil": ("solid", "soil"),
        "stalk": stripes("u", 1 / 8, [(3, "stalk"), (1, "stalk_hi")], "LINEAR"),
        "vine": ("solid", "vine"),
        "leaf": stripes("u", 1 / 14, [(5, "leaf"), (1, "leaf_vein")], "LINEAR"),
        "thorn": ("solid", "thorn"),
        "petal": stripes("v", 1.0, [(2, "cream"), (2, "sky"), (4, "lotus"), (2, "violet")], "LINEAR"),
        "sepal": stripes("v", 1.0, [(3, "stalk"), (2, "lotus"), (3, "lotus"), (2, "violet")], "LINEAR"),
        "throat": stripes("v", 0.34, [(1, "gum"), (2, "throat"), (2, "black")], "LINEAR"),
        "green": ("solid", "stalk"),
        "tooth": ("solid", "ivory"),
        "gold": ("solid", "gold"),
    }
    return {name: common.make_material("plant_" + name, spec, COL) for name, spec in specs.items()}


# ---------------------------------------------------------------- jar and floor

NOTCH = math.radians(-50)  # broken rim at the front right


def notch_depth(th):
    d = math.atan2(math.sin(th - NOTCH), math.cos(th - NOTCH))
    f = max(0.0, 1 - abs(d) / math.radians(28))
    return 0.2 * f * (0.65 + 0.35 * abs(math.sin(9 * th)))


def build_jar(b, M):
    prof = [(0.0, 0.0), (0.40, 0.0), (0.47, 0.05), (0.56, 0.2), (0.60, 0.36), (0.58, 0.52), (0.52, 0.68), (0.44, 0.8), (0.39, 0.87),
            (0.40, 0.93), (0.45, 0.97), (0.46, 1.0), (0.42, 1.02), (0.37, 1.0), (0.34, 0.92), (0.33, 0.82), (0.30, 0.76), (0.0, 0.76)]
    mats = ["jar", "jar", "jar", "jar", "frieze", "band", "jar", "jar", "band", "lip", "lip", "lip", "lip", "jar_in", "jar_in", "jar_in", "soil"]
    vs = [0.0]
    for (r0, z0), (r1, z1) in zip(prof, prof[1:]):
        vs.append(vs[-1] + math.hypot(r1 - r0, z1 - z0))

    def z_mod(th, r, z):
        return z - notch_depth(th) * smoothstep(0.78, 1.0, z) * (z - 0.78) / 0.24 if z > 0.78 else z

    for mat, geo in lathe(prof, vs, 44, mats, z_mod).items():
        b.add(geo, M[mat], "root")
    # Shards of the broken rim lying on the floor.
    for pos, rot, size in (((0.55, -0.62, 0.0), 20, 0.16), ((0.78, -0.35, 0.0), 75, 0.11), ((0.30, -0.80, 0.0), 140, 0.09)):
        c = V(pos)
        pts = [c + V((size * math.cos(math.radians(rot + a)), size * 0.7 * math.sin(math.radians(rot + a)), 0.0)) for a in (0, 110, 200, 290)]
        verts = pts + [p + V((0, 0, 0.035)) for p in pts]
        faces = [[0, 1, 2, 3], [4, 5, 6, 7]] + [[i, (i + 1) % 4, 4 + (i + 1) % 4, 4 + i] for i in range(4)]
        ctr = c + V((0, 0, 0.017))
        faces = [orient(verts, f, (sum((verts[i] for i in f), V()) / len(f)) - ctr) for f in faces]
        uv = [[(0.1, 0.1)] * len(f) for f in faces]
        b.add((verts, faces, uv), M["jar"], "root")


def lily_pad(b, M, centre, radius, notch):
    """Flat round leaf with a notch, veins radiating from the stem."""
    nphi, rings = 28, [0.0, 0.35, 0.7, 1.0]
    c = V(centre)
    verts, faces, fuv, idx = [], [], [], {}
    gap = math.radians(14)
    for side, dz in (("top", 0.024), ("bot", 0.004)):
        idx[side, 0] = len(verts)
        verts.append(c + V((0, 0, dz + (0.004 if side == "top" else 0))))
        for i, p in enumerate(rings[1:], 1):
            for j in range(nphi + 1):
                phi = notch + gap + (2 * math.pi - 2 * gap) * j / nphi
                r = radius * p * (1 + 0.04 * math.sin(5 * phi))
                idx[side, i, j] = len(verts)
                verts.append(c + V((r * math.cos(phi), r * math.sin(phi), dz + 0.03 * p ** 4)))

    def add(face, expected, uv):
        f = orient(verts, face, expected)
        faces.append(f)
        fuv.append(uv if f is face else uv[::-1])

    last = len(rings) - 1
    for side, e in (("top", V((0, 0, 1))), ("bot", V((0, 0, -1)))):
        for j in range(nphi):
            u0, u1 = j / nphi, (j + 1) / nphi
            add([idx[side, 0], idx[side, 1, j], idx[side, 1, j + 1]], e, [(u0, 0), (u0, 0.1), (u1, 0.1)])
            for i in range(1, last):
                add([idx[side, i, j], idx[side, i, j + 1], idx[side, i + 1, j + 1], idx[side, i + 1, j]], e, [(u0, i), (u1, i), (u1, i + 1), (u0, i + 1)])
    for j in range(nphi):
        phi = notch + gap + (2 * math.pi - 2 * gap) * (j + 0.5) / nphi
        add([idx["top", last, j], idx["top", last, j + 1], idx["bot", last, j + 1], idx["bot", last, j]], V((math.cos(phi), math.sin(phi), 0)),
            [(0, 0), (0.01, 0), (0.01, 0.01), (0, 0.01)])
    for j, sgn in ((0, -1), (nphi, 1)):
        phi = notch + gap + (2 * math.pi - 2 * gap) * j / nphi
        side_n = V((-math.sin(phi), math.cos(phi), 0)) * -sgn
        for i in range(1, last):
            add([idx["top", i, j], idx["top", i + 1, j], idx["bot", i + 1, j], idx["bot", i, j]], side_n, [(0, 0), (0.01, 0), (0.01, 0.01), (0, 0.01)])
        add([idx["top", 0], idx["top", 1, j], idx["bot", 1, j], idx["bot", 0]], side_n, [(0, 0), (0.01, 0), (0.01, 0.01), (0, 0.01)])
    b.add((verts, faces, fuv), M["leaf"], "root")


# ---------------------------------------------------------------- stalk and bloom


def stalk_joints():
    """Rest curve: rises out of the jar, arches over like a rearing cobra, bloom facing forward and down."""
    ctrl = [(0, 0.05, 0.55), (0, 0.08, 1.0), (0, 0.15, 1.5), (0, 0.14, 1.95), (0, 0.02, 2.35), (0, -0.2, 2.58), (0, -0.44, 2.47)]
    return resample(ctrl, N)


def stalk_radius(u):
    return 0.15 - 0.05 * u + 0.03 * smoothstep(0.85, 1.0, u)


def bloom_frame():
    J = stalk_joints()
    axis = (J[N] - J[N - 1]).normalized()
    u1 = V((1, 0, 0))
    return J[N], axis, u1, axis.cross(u1)


def petal_rest(ring, k):
    """Base point, direction, across and inward vectors of petal k in ring 0 (inner) / 1 (outer)."""
    O, A, u1, u2 = bloom_frame()
    phi = 2 * math.pi * (k + 0.5 * ring) / PETALS
    R = math.cos(phi) * u1 + math.sin(phi) * u2
    base = O + A * (0.16 - 0.04 * ring) + R * (0.23 + 0.04 * ring)
    beta = math.radians(40 + 18 * ring)
    D = (math.cos(beta) * A + math.sin(beta) * R).normalized()
    C = A.cross(R).normalized()
    inward = (-R - (-R).dot(D) * D).normalized()
    return base, D, C, inward


PETAL_LEN, PETAL_W = (0.62, 0.66), (0.16, 0.17)


def build_petal(b, M, ring, k):
    base, D, C, inward = petal_rest(ring, k)
    na, nv, thick = 5, 7, 0.016
    L, W = PETAL_LEN[ring], PETAL_W[ring]

    def point(a, v, back):
        w = W * (0.35 + 0.65 * math.sin(math.pi * v ** 0.75)) * max(1 - v ** 4, 0.0) ** 0.5
        p = base + D * (L * v) + C * (a * w) + inward * (0.05 * a * a * math.sin(math.pi * min(v * 1.3, 1)) - 0.07 * v * v)
        return p - inward * thick * (1 - 0.6 * v) if back else p

    verts, grid = [], {}
    for side in ("front", "back"):
        for i in range(nv):
            for j in range(na):
                grid[side, i, j] = len(verts)
                verts.append(point(-1 + 2 * j / (na - 1), 0.98 * i / (nv - 1), side == "back"))
    tip = len(verts)
    verts.append(base + D * L - inward * 0.07)
    faces, fuv = [], []

    def add(face, expected, uv):
        f = orient(verts, face, expected)
        faces.append(f)
        fuv.append(uv if f is face else uv[::-1])

    for i in range(nv - 1):
        v0, v1 = i / (nv - 1), (i + 1) / (nv - 1)
        for j in range(na - 1):
            u0, u1 = j / (na - 1), (j + 1) / (na - 1)
            uv = [(u0, v0), (u1, v0), (u1, v1), (u0, v1)]
            add([grid["front", i, j], grid["front", i, j + 1], grid["front", i + 1, j + 1], grid["front", i + 1, j]], inward, uv)
            add([grid["back", i, j], grid["back", i, j + 1], grid["back", i + 1, j + 1], grid["back", i + 1, j]], -inward, uv)
        for j, sgn in ((0, -1), (na - 1, 1)):
            add([grid["front", i, j], grid["front", i + 1, j], grid["back", i + 1, j], grid["back", i, j]], C * sgn, [(0, v0), (0, v1), (0, v1), (0, v0)])
    last = nv - 1
    for j in range(na - 1):
        u0, u1 = j / (na - 1), (j + 1) / (na - 1)
        add([grid["front", 0, j], grid["front", 0, j + 1], grid["back", 0, j + 1], grid["back", 0, j]], -D, [(u0, 0), (u1, 0), (u1, 0), (u0, 0)])
        add([grid["front", last, j], grid["front", last, j + 1], tip], inward + D, [(u0, 0.98), (u1, 0.98), (0.5, 1.0)])
        add([grid["back", last, j], grid["back", last, j + 1], tip], -inward + D, [(u0, 0.98), (u1, 0.98), (0.5, 1.0)])
    for j, sgn in ((0, -1), (na - 1, 1)):
        add([grid["front", last, j], tip, grid["back", last, j]], C * sgn, [(0, 0.98), (0, 1.0), (0, 0.98)])
    b.add((verts, faces, fuv), M["petal" if ring == 0 else "sepal"], "petal%d_%d" % (ring, k))


def build_stalk(b, M):
    J = stalk_joints()
    names = ["s%02d" % k for k in range(N)]
    weights = chain_weights(J, names, blend=0.08)

    def ribs(th, a):
        return 1.0 + 0.05 * math.cos(8 * th)

    keys = [(J[0] - V((0, 0, 0.1)), stalk_radius(0) * 1.1, stalk_radius(0) * 1.1)]
    keys += [(J[k], stalk_radius(k / N), stalk_radius(k / N)) for k in range(N + 1)]
    b.add(tube(keys, sub=3, n=14, ref=V((1, 0, 0)), shape=ribs, caps=(False, False)), M["stalk"], weights)
    # Thorns spiralling up the stalk, pointing back down towards the jar.
    for k in range(1, N):
        for off in (0.0, 0.5):
            u = (k + off) / N
            i = min(int(u * N), N - 1)
            p = J[i].lerp(J[i + 1], u * N - i)
            t = (J[i + 1] - J[i]).normalized()
            x = perp(t)
            ang = 2.4 * (2 * k + off * 2)
            radial = (Matrix.Rotation(ang, 3, t) @ x).normalized()
            base = p + radial * stalk_radius(u) * 0.92
            b.add(cone(base, radial - t * 0.6, 0.11 - 0.03 * u, 0.028), M["thorn"], weights)


def build_bloom(b, M):
    O, A, u1, u2 = bloom_frame()
    bone = "head"
    # Receptacle behind the petals.
    rec = [(O - A * 0.12, stalk_radius(1) * 0.95, stalk_radius(1) * 0.95), (O + A * 0.02, 0.2, 0.2), (O + A * 0.12, 0.26, 0.26), (O + A * 0.17, 0.25, 0.25)]
    b.add(tube(rec, sub=2, n=18, ref=u1, caps=(False, False)), M["green"], bone)
    # Maw: over the rim and down into the throat.
    lip = [(0.17, 0.25), (0.2, 0.235), (0.2, 0.2), (0.17, 0.17), (0.1, 0.14), (0.02, 0.1), (-0.05, 0.05), (-0.08, 0.0)]
    verts = []
    n = 18
    for d, r in lip:
        for j in range(n):
            phi = 2 * math.pi * j / n
            verts.append(O + A * d + (math.cos(phi) * u1 + math.sin(phi) * u2) * r)
    faces, fuv = [], []
    arc = [0.0]
    for (d0, r0), (d1, r1) in zip(lip, lip[1:]):
        arc.append(arc[-1] + math.hypot(d1 - d0, r1 - r0))
    for i in range(len(lip) - 1):
        for j in range(n):
            j2 = (j + 1) % n
            phi = 2 * math.pi * (j + 0.5) / n
            radial = math.cos(phi) * u1 + math.sin(phi) * u2
            face = [i * n + j, i * n + j2, (i + 1) * n + j2, (i + 1) * n + j]
            f = orient(verts, face, (radial if i == 0 else -radial) + A * 0.4)
            faces.append(f)
            uv = [(j / n, arc[i]), ((j + 1) / n, arc[i]), ((j + 1) / n, arc[i + 1]), (j / n, arc[i + 1])]
            fuv.append(uv if f is face else uv[::-1])
    b.add((verts, faces, fuv), M["throat"], bone)
    # Fangs: an outer ring on the lip, a second ring down the throat, all hooked inwards.
    for d, r, count, length, off, rr in ((0.19, 0.205, 12, 0.13, 0.0, 0.026), (0.1, 0.14, 9, 0.09, 0.5, 0.02)):
        for i in range(count):
            phi = 2 * math.pi * (i + off) / count
            radial = math.cos(phi) * u1 + math.sin(phi) * u2
            b.add(cone(O + A * d + radial * r, -radial + A * 0.25, length, rr), M["tooth"], bone)
    # Golden stamens around the maw.
    for i in range(16):
        phi = 2 * math.pi * (i + 0.25) / 16
        radial = math.cos(phi) * u1 + math.sin(phi) * u2
        base = O + A * 0.17 + radial * 0.24
        d = (A + radial * 0.7).normalized()
        b.add(tube([(base, 0.01, 0.01), (base + d * 0.13, 0.007, 0.007)], sub=1, n=5, ref=perp(d), caps=(False, False)), M["gold"], bone)
        b.add(ellipsoid(base + d * 0.15, (0.016, 0.016, 0.026), n=6, rings=3, axis=d, ref=perp(d)), M["gold"], bone)
    for ring in (0, 1):
        for k in range(PETALS):
            build_petal(b, M, ring, k)


# ---------------------------------------------------------------- vines

VINES = [math.radians(-125), math.radians(-40), math.radians(115)]  # azimuths (-90 = front)


def vine_joints(az):
    c, s = math.cos(az), math.sin(az)
    ctrl = [(0.22, 0.72), (0.36, 0.98), (0.47, 1.03), (0.58, 0.85), (0.66, 0.5), (0.68, 0.25), (0.78, 0.075), (0.98, 0.055), (1.18, 0.042), (1.30, 0.06),
            (1.34, 0.13)]
    return resample([(r * c, r * s, z) for r, z in ctrl], NV)


def vine_radius(u):
    return 0.065 - 0.043 * u


def build_vines(b, M):
    for v, az in enumerate(VINES):
        J = vine_joints(az)
        names = ["v%d_%d" % (v, k) for k in range(NV)]
        weights = chain_weights(J, names, blend=0.06)
        dense = resample(J, NV * 4, dense=6)
        keys = [(p, vine_radius(i / (NV * 4)), vine_radius(i / (NV * 4))) for i, p in enumerate(dense)]
        b.add(tube(keys, sub=1, n=9, ref=V((0, 0, 1)), caps=(True, True), bulge=(0.01, 0.02)), M["vine"], weights)
        for i in range(3, NV * 4 - 1, 2):
            p, q = dense[i], dense[i + 1]
            t = (q - p).normalized()
            side = t.cross(V((0, 0, 1)))
            side = side.normalized() if side.length > 1e-3 else perp(t)
            sgn = 1 if i % 4 == 1 else -1
            radial = (side * sgn + V((0, 0, 0.6))).normalized()
            b.add(cone(p + radial * vine_radius(i / (NV * 4)) * 0.9, radial - t * 0.4, 0.07, 0.016), M["thorn"], weights)


# ---------------------------------------------------------------- rig


def build_rig(coll):
    arm = bpy.data.armatures.new("plant_rig")
    rig = bpy.data.objects.new("plant_rig", arm)
    coll.objects.link(rig)
    arm.display_type = "STICK"
    rig.show_in_front = True
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    eb = arm.edit_bones

    def bone(name, head, tail, x_axis, parent=None, connect=False):
        e = eb.new(name)
        e.head, e.tail = V(head), V(tail)
        e.align_roll(V(x_axis).cross((e.tail - e.head).normalized()))
        if parent:
            e.parent = eb[parent]
            e.use_connect = connect

    bone("root", (0, 0, 0), (0, 0.3, 0), (1, 0, 0))
    J = stalk_joints()
    for k in range(N):
        bone("s%02d" % k, J[k], J[k + 1], (1, 0, 0), "root" if k == 0 else "s%02d" % (k - 1), k > 0)
    O, A, u1, _ = bloom_frame()
    bone("head", O, O + A * 0.3, u1, "s%02d" % (N - 1), True)
    for ring in (0, 1):
        for k in range(PETALS):
            base, D, C, _ = petal_rest(ring, k)
            bone("petal%d_%d" % (ring, k), base, base + D * 0.3, C, "head")
    for v, az in enumerate(VINES):
        J = vine_joints(az)
        normal = V((-math.sin(az), math.cos(az), 0))  # horizontal, across the vine's vertical plane
        for k in range(NV):
            bone("v%d_%d" % (v, k), J[k], J[k + 1], normal, "root" if k == 0 else "v%d_%d" % (v, k - 1), k > 0)
    bpy.ops.object.mode_set(mode="OBJECT")
    for pb in rig.pose.bones:
        pb.rotation_mode = "XYZ"
    return rig


def rot_sign(rig, name, measure):
    """+1 if a positive local X rotation increases measure(tip - head) for this bone, else -1."""
    bone = rig.data.bones[name]
    m = bone.matrix_local.to_3x3()
    vec = bone.tail_local - bone.head_local
    turned = m @ Matrix.Rotation(0.2, 3, "X") @ m.inverted() @ vec
    return 1 if measure(turned) > measure(vec) else -1


# ---------------------------------------------------------------- animation

REST = dict(sway=0.0, side=0.0, ph=0.0, rear=0.0, lunge=0.0, droop=0.0, turn=0.0, hp=0.0, hy=0.0, shake=0.0,
            open=0.0, wilt=0.0, vlift=0.0, vsway=0.0, vph=0.0, vstrike=0.0, vcurl=0.0)


def dist(total, weights):
    s = sum(weights) or 1.0
    return [total * w / s for w in weights]


def stalk_angles(p):
    """Per stalk bone (x, z) Euler degrees. +x curls the stalk forward/down."""
    us = [(k + 0.5) / N for k in range(N)]
    wave = [p["sway"] * (0.4 + u) * math.sin(2 * math.pi * p["ph"] - 2.2 * u) for u in us]
    side = [p["side"] * (0.3 + u) * math.sin(2 * math.pi * p["ph"] + 0.7 - 1.8 * u) for u in us]
    x = [sum(parts) for parts in zip(
        wave, dist(-p["rear"], [1 - u for u in us]), dist(0.7 * p["rear"], [u * u for u in us]),
        dist(p["lunge"], [1.2 - u for u in us]), dist(-0.8 * p["lunge"], [u * u for u in us]), dist(p["droop"], [math.exp(-((u - 0.4) / 0.25) ** 2) for u in us]))]
    z = [a + b + c for a, b, c in zip(side, dist(p["turn"], [1.0] * N), dist(p["shake"], [smoothstep(0.5, 1.0, u) for u in us]))]
    return list(zip(x, z))


def set_pose(rig, p, signs):
    pose = rig.pose.bones
    for k, (x, z) in enumerate(stalk_angles(p)):
        pose["s%02d" % k].rotation_euler = (math.radians(x), 0, math.radians(z))
    pose["head"].rotation_euler = (math.radians(p["hp"]), 0, math.radians(p["hy"]))
    for ring in (0, 1):
        for k in range(PETALS):
            name = "petal%d_%d" % (ring, k)
            # Wilting petals flop open, the lower ones most (gravity).
            _, D, _, _ = petal_rest(ring, k)
            sag = p["wilt"] * (1.0 + 0.6 * ring) * (0.6 + 0.4 * max(0.0, -D.z))
            jitter = 1 + 0.08 * math.sin(3.1 * k + ring)
            pose[name].rotation_euler = (math.radians((p["open"] * (0.8 + 0.4 * ring) * jitter + sag) * signs[name]), 0, 0)
    for v in range(len(VINES)):
        for k in range(NV):
            u = (k + 0.5) / NV
            name = "v%d_%d" % (v, k)
            floor = smoothstep(0.35, 0.5, u)  # bones lying on the floor may only lift
            lift = p["vlift"] * floor * (0.3 + u) * (1 + 0.25 * math.sin(2 * math.pi * (p["vph"] + v / 3)))
            lift += p["vcurl"] * smoothstep(0.6, 1.0, u)
            sway = p["vsway"] * floor * math.sin(2 * math.pi * (p["vph"] + v / 3) - 3 * u)
            # Strike: sweep the vine towards the front (-Y).
            az = VINES[v]
            toward = math.degrees(math.atan2(math.sin(-math.pi / 2 - az), math.cos(-math.pi / 2 - az)))
            sweep = p["vstrike"] * floor * max(-1.0, min(1.0, toward / 60))
            pose[name].rotation_euler = (math.radians(lift * signs[name]), 0, math.radians(sway + sweep * signs[name + "z"]))


def lowest_z(obj):
    bpy.context.view_layer.update()
    ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    return min(v.co.z for v in ev.data.vertices)


def fit_floor(rig, obj, p, signs, key, clearance=0.005):
    """Scale p[key] down until nothing goes through the floor."""
    set_pose(rig, p, signs)
    if lowest_z(obj) >= -clearance:
        return p
    lo, hi = 0.0, 1.0
    full = p[key]
    for _ in range(12):
        mid = (lo + hi) / 2
        set_pose(rig, dict(p, **{key: full * mid}), signs)
        if lowest_z(obj) >= -clearance:
            lo = mid
        else:
            hi = mid
    return dict(p, **{key: full * lo})


def idle_params(t):
    p = dict(REST)
    w = 2 * math.pi * t
    p.update(sway=1.8, side=1.6, ph=t, hp=3 * math.sin(w + 0.8), hy=4 * math.sin(w), open=6 * math.sin(2 * w), vlift=10, vsway=10, vph=t,
             vcurl=8 + 6 * math.sin(w))
    return p


READY = idle_params(0.0)


def interpolate(keys, frame, linear=("ph", "vph")):
    full = []
    for f, over in keys:
        p = dict(READY)
        p.update(over)
        full.append((f, p))
    for (f0, p0), (f1, p1) in zip(full, full[1:]):
        if f0 <= frame <= f1:
            t = smoothstep(f0, f1, frame)
            lt = (frame - f0) / (f1 - f0)
            return {k: p0[k] + (p1[k] - p0[k]) * (lt if k in linear else t) for k in p0}
    return full[-1][1]


ATTACK = [
    (0, {}),
    (7, dict(rear=55, hp=-18, open=38, vlift=22, vcurl=14, vph=0.15, sway=0, side=0)),
    (10, dict(rear=62, hp=-22, open=48, vlift=26, vcurl=16, vph=0.25, sway=0, side=0)),
    (13, dict(rear=0, lunge=70, hp=-18, open=-28, vlift=12, vstrike=45, vcurl=5, vph=0.35, sway=0, side=0)),
    (15, dict(lunge=74, hp=-16, open=-32, shake=16, vlift=8, vstrike=50, vph=0.45, sway=0, side=0)),
    (17, dict(lunge=72, hp=-16, open=-32, shake=-16, vlift=8, vstrike=45, vph=0.55, sway=0, side=0)),
    (20, dict(lunge=35, hp=-6, open=4, vlift=12, vstrike=15, vph=0.7, sway=0, side=0)),
    (26, dict(vph=1.0)),
]

DIE = [
    (0, {}),
    (3, dict(rear=40, hp=-28, open=55, vlift=35, vcurl=20, sway=0, side=0)),
    (6, dict(rear=25, turn=28, hp=-15, open=45, shake=20, vlift=25, vsway=25, vph=0.3, sway=0, side=0)),
    (9, dict(rear=18, turn=-26, hp=-10, open=58, shake=-20, vlift=20, vsway=25, vph=0.6, sway=0, side=0)),
    (12, dict(rear=5, lunge=15, droop=30, turn=16, open=62, wilt=10, vlift=10, vsway=15, vph=0.9, sway=0, side=0)),
    (16, dict(droop=90, lunge=20, turn=14, hp=10, open=66, wilt=25, vlift=4, vsway=10, vph=1.1, vcurl=0, sway=0, side=0)),
    (21, dict(droop=140, lunge=25, turn=16, hp=20, open=70, wilt=40, vlift=0, vsway=-20, vph=1.25, vcurl=-4, sway=0, side=0)),
    (25, dict(droop=160, lunge=30, turn=18, hp=24, open=72, wilt=50, vlift=0, vsway=-35, vph=1.25, vcurl=-6, sway=0, side=0)),
    (28, dict(droop=162, lunge=30, turn=18, hp=14, open=72, wilt=50, vlift=6, vsway=-35, vph=1.25, vcurl=-6, sway=0, side=0)),
    (31, dict(droop=164, lunge=30, turn=18, hp=26, open=74, wilt=55, vlift=0, vsway=-38, vph=1.25, vcurl=-6, sway=0, side=0)),
    (35, dict(droop=165, lunge=30, turn=18, hp=26, open=75, wilt=58, vlift=0, vsway=-40, vph=1.25, vcurl=-6, sway=0, side=0)),
]


def make_actions(rig, obj):
    measure_open = {}
    O, A, _, _ = bloom_frame()
    for ring in (0, 1):
        for k in range(PETALS):
            measure_open["petal%d_%d" % (ring, k)] = lambda vec: -vec.normalized().dot(A)  # away from the bloom axis
    signs = {name: rot_sign(rig, name, m) for name, m in measure_open.items()}
    for v, az in enumerate(VINES):
        toward_front = V((0, -1, 0))
        for k in range(NV):
            name = "v%d_%d" % (v, k)
            signs[name] = rot_sign(rig, name, lambda vec: vec.normalized().z)
            bone = rig.data.bones[name]
            m = bone.matrix_local.to_3x3()
            vec = bone.tail_local - bone.head_local
            turned = m @ Matrix.Rotation(0.2, 3, "Z") @ m.inverted() @ vec
            signs[name + "z"] = 1 if turned.normalized().dot(toward_front) > vec.normalized().dot(toward_front) else -1

    # Solve every pose first (no action assigned, so the depsgraph shows the manual pose).
    rig.animation_data_create()
    rig.animation_data.action = None
    solved = {}
    for name, _, frames in CLIPS:
        poses = []
        last = frames if name != "plant_die" else frames - 1  # loops key frame N = frame 0
        for f in range(last + 1):
            if name == "plant_walk":
                p = idle_params(f / frames)
            elif name == "plant_attack":
                p = fit_floor(rig, obj, interpolate(ATTACK, f), signs, "lunge")
            else:
                p = fit_floor(rig, obj, interpolate(DIE, f), signs, "droop")
            poses.append((f, p))
        solved[name] = poses
    acts = {}
    for name, _, _ in CLIPS:
        act = bpy.data.actions.get(name)
        if act:
            bpy.data.actions.remove(act)
        act = bpy.data.actions.new(name)
        act.use_fake_user = True
        rig.animation_data.action = act
        for f, p in solved[name]:
            set_pose(rig, p, signs)
            for pb in rig.pose.bones:
                pb.keyframe_insert("rotation_euler", frame=f)
        acts[name] = act
    return acts


# ---------------------------------------------------------------- entry points


def build(bake=True, tex_path=None):
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    M = materials()
    rig = build_rig(coll)
    b = Builder()
    build_jar(b, M)
    for centre, radius, notch in (((-0.95, -0.45, 0), 0.34, 2.2), ((0.85, 0.55, 0), 0.3, -2.4), ((-0.55, 0.85, 0), 0.24, -0.9)):
        lily_pad(b, M, centre, radius, notch)
    build_stalk(b, M)
    build_vines(b, M)
    first = len(b.verts)
    build_bloom(b, M)
    b.tags[first:] = ["head"] * (len(b.verts) - first)
    obj = common.finish_mesh(b, coll, "plant_new")
    O, _, _, _ = bloom_frame()
    common.uv_unwrap(obj, b.tags, {"head": (O, 1.5)})
    if bake:
        tex = common.bake_texture(obj, tex_path or os.path.join(bpy.app.tempdir or "/tmp", "plant_preview.png"), TEX_SIZE, "plant", ao_distance=0.2)
        common.use_baked_material(obj, tex)
    common.rig_object(obj, rig)
    acts = make_actions(rig, obj)
    rig.animation_data.action = acts["plant_walk"]
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 0, CLIPS[0][2] - 1
    scene.frame_set(0)
    print("plant: {} verts, {} tris".format(len(obj.data.vertices), common.tri_count(obj)))
    return obj, rig


def export(models_dir=None):
    common.export_files(bpy.data.objects["plant_new"], bpy.data.objects["plant_rig"], "plant", CLIPS, "monsters", models_dir)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    build(tex_path=os.path.join(REPO, "textures", "monsters", "plant.png") if "--export" in args else None)
    if "--export" in args:
        export()
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "plant.blend"))
