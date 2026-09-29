"""Procedural mimic: a monster that passes for the treasure chest item until the player comes near.

    MCP:  p = ".../tools/blender/models/mimic.py"; g = {"__file__": p, "__name__": "mimic"}
          exec(open(p).read(), g); g["build"](bake=False)      # then g["export"]()
    CLI:  blender -b --python tools/blender/models/mimic.py -- [--export] [--bake]

Blender space: Z up, the chest faces -Y (game rotA 0), like the item. The shell is the chest of items.py
(build_chest: same geometry, and the UVs and pixels of textures/items/treasure_chest.png copied into the left half
of mimic.png), so the rest pose is vertex for vertex the item: the lid open towards the back over a heap of gold.
Everything else hides inside it at rest: lower fangs and gums in the walls (slide up out of the rim), upper fangs
lying flat in the lid frame (fold down out of its underside), the tongue and a fleshy mouth floor under the gold
heap (the heap sinks into the box when it wakes). Rigid bones posed by deformation matrices (as in bat.py),
the legs ("base") shear under the box ("body") as it twists and sways; per-frame floor fix.

Clips (engine ~14 fps): walk (`mimic.md3`, loops on the awake pose: lid snaps, the box sways and twists), attack
(`_att`, loops from the same awake pose: gape, lunge towards -Y with the tongue lashing out, snap, shake), die (`_die`,
plays once from the awake pose: shudders, tongue limp, the lid slams, the gold comes back up under it and the lid
falls open; the last frame is the rest pose again), idle (`_idle`, loops: the chest, still, but the lid dips and
the box breathes once per loop; frame 0 = the chest item, the engine's normalization reference for the mimic,
AMBUSH_CLIPS in src/entities/character_model.h).

Texture: 1024 x 512, left half = treasure_chest.png unchanged, right half = the mouth parts (AO baked on their own).
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
sys.path.insert(0, os.path.join(HERE, ".."))
import common  # noqa: E402
import items  # noqa: E402
import md3  # noqa: E402

importlib.reload(common)
importlib.reload(items)
from common import REPO, Builder, _cr, chain_weights, ellipsoid, smoothstep, tube  # noqa: E402

COLL = "mimic_new"
CLIPS = [("mimic_walk", "", 32), ("mimic_attack", "_att", 22), ("mimic_die", "_die", 30), ("mimic_idle", "_idle", 42)]
MOUTH_TEX = 512  # the mouth half of the 1024 x 512 texture
REVIEW_VIEW = {"target": (0, -0.2, 0.42), "ortho": 2.0, "res": (440, 400)}
CHEST_MD3 = os.path.join(REPO, "models", "items", "treasure_chest.md3")
CHEST_PNG = os.path.join(REPO, "textures", "items", "treasure_chest.png")

W, D, H, T = items.CHEST_W, items.CHEST_D, items.CHEST_H, items.CHEST_WALL
Z0 = items.CHEST_LEG
Z1 = Z0 + H  # box rim
LW, LD = W + 0.02, D + 0.02  # lid frame
HINGE = V((0, D / 2 + 0.01, Z1))
LID_REST = items.LID_OPEN

COL = {
    "gum": (0.34, 0.03, 0.045),
    "flesh": (0.30, 0.035, 0.04),
    "flesh_dark": (0.14, 0.012, 0.018),
    "throat": (0.025, 0.004, 0.006),
    "tooth": (0.86, 0.80, 0.63),
    "tooth_root": (0.52, 0.40, 0.26),
    "tongue": (0.62, 0.15, 0.18),
    "tongue_dark": (0.36, 0.05, 0.08),
}


def materials():
    specs = {
        "gum": ("solid", "gum"),
        "flesh": ("stripes", "v", 1 / 9, [(2, "flesh"), (1, "flesh_dark")], "LINEAR"),
        "throat": ("solid", "throat"),
        "tooth": ("stripes", "v", 0.2, [(1, "tooth_root"), (3, "tooth"), (6, "tooth")], "LINEAR"),
        "tongue": ("stripes", "u", 1.0, [(1.6, "tongue"), (0.8, "tongue_dark"), (5.6, "tongue")], "LINEAR"),
    }
    return {name: common.make_material("mimic_" + name, spec, COL) for name, spec in specs.items()}


# ---------------------------------------------------------------- matrices


def rx(d):
    return Matrix.Rotation(math.radians(d), 4, "X")


def ry(d):
    return Matrix.Rotation(math.radians(d), 4, "Y")


def rz(d):
    return Matrix.Rotation(math.radians(d), 4, "Z")


def tr(x, y=0.0, z=0.0):
    return Matrix.Translation(V((x, y, z)) if not isinstance(x, V) else x)


def about(pivot, m):
    return tr(V(pivot)) @ m @ tr(-V(pivot))


LID_LOCAL = tr(HINGE) @ rx(-LID_REST) @ tr(0, -LD / 2, 0)  # closed-lid coordinates (items.build_chest) -> rest


# ---------------------------------------------------------------- mouth parts (hidden at rest)

LOW_EXT = 0.105  # how far the lower fangs and gums slide up
GUM_Z = Z1 - 0.1  # gum centres at rest (inside the walls)
FRONT_Y = -D / 2 + T / 2  # mid-plane of the front wall
SIDE_X = W / 2 - T / 2
LOW_FRONT = [(0.095, 0.075), (0.15, 0.092), (0.205, 0.068), (0.26, 0.08), (0.315, 0.062)]  # (|x|, length)
LOW_SIDE = [(-0.15, 0.07), (-0.09, 0.085), (-0.03, 0.066), (0.03, 0.075), (0.09, 0.064), (0.15, 0.058)]  # (y, length)
FANG_Y, FANG_Z = -LD / 2 + 0.018, 0.015  # front fold line in closed-lid coordinates
FANG_X = LW / 2 - 0.018  # side fold lines
UP_FRONT = [(0.0, 0.08), (0.065, 0.098), (0.12, 0.075), (0.175, 0.085), (0.23, 0.07), (0.285, 0.078)]  # (|x|, length)
UP_SIDE = [(-0.13, 0.072), (-0.07, 0.08), (-0.01, 0.068), (0.05, 0.075), (0.11, 0.062)]  # (y, length)
FANG_FOLD = 86.0  # degrees at full extension

NT = 7  # tongue bones
TONGUE_Z = 0.232
TONGUE_REST = [V((0, 0.16, TONGUE_Z)), V((0, 0.05, TONGUE_Z)), V((0, -0.05, TONGUE_Z)), V((0, -0.16, TONGUE_Z))]
FLOOR_Z = 0.205


def fang(base, direction, across, length, w, t):
    """Flattened, slightly curved fang from base along direction; w along the row, t across it."""
    d = V(direction).normalized()
    keys = [(base, w, t), (base + d * length * 0.35, w * 0.82, t * 0.85), (base + d * length * 0.75, w * 0.4, t * 0.5), (base + d * length, 0.0012, 0.0012)]
    return tube(keys, sub=2, n=8, ref=V(across), caps=(True, False))


def build_lower(b, M):
    """Fangs standing in scalloped gums inside the front and side walls (bone "teeth")."""
    z = GUM_Z
    for ax, length in LOW_FRONT:
        for s in (-1, 1):
            base = V((s * ax, FRONT_Y, z))
            b.add(fang(base, (0, 0.04, 1), (0, 1, 0), length, 0.013, 0.0072), M["tooth"], "teeth", "mouth")
            b.add(ellipsoid(base, (0.03, 0.0092, 0.021), n=12, rings=5, ref=V((0, 1, 0))), M["gum"], "teeth", "mouth")
    for y, length in LOW_SIDE:
        for s in (-1, 1):
            base = V((s * SIDE_X, y, z))
            b.add(fang(base, (-s * 0.04, 0, 1), (1, 0, 0), length, 0.012, 0.0072), M["tooth"], "teeth", "mouth")
            b.add(ellipsoid(base, (0.0092, 0.03, 0.021), n=12, rings=5, ref=V((0, 1, 0))), M["gum"], "teeth", "mouth")


def build_upper(b, M):
    """Fangs lying flat inside the lid frame, hinged at its front and side edges (bones fang_f / fang_l / fang_r)."""
    for ax, length in UP_FRONT:
        for s in ((-1, 1) if ax else (1,)):
            base = LID_LOCAL @ V((s * ax, FANG_Y, FANG_Z))
            tip = LID_LOCAL @ V((s * ax, FANG_Y + 1, FANG_Z)) - LID_LOCAL @ V((s * ax, FANG_Y, FANG_Z))
            across = LID_LOCAL.to_3x3() @ V((0, 0, 1))
            b.add(fang(base, tip, across, length, 0.014 if ax != 0.065 else 0.016, 0.0072), M["tooth"], "fang_f", "mouth")
    for y, length in UP_SIDE:
        for s, bone in ((1, "fang_r"), (-1, "fang_l")):
            base = LID_LOCAL @ V((s * FANG_X, y, FANG_Z))
            d = LID_LOCAL.to_3x3() @ V((-s, 0, 0))
            across = LID_LOCAL.to_3x3() @ V((0, 0, 1))
            b.add(fang(base, d, across, length, 0.012, 0.0072), M["tooth"], bone, "mouth")


def tongue_joints():
    return resample(TONGUE_REST, NT)


def build_tongue(b, M):
    J = tongue_joints()
    names = ["t%d" % k for k in range(NT)]

    def groove(th, a):
        d = math.atan2(math.sin(th - math.pi / 2), math.cos(th - math.pi / 2))
        return 1.0 - 0.28 * math.exp(-(d / 0.35) ** 2)

    keys = []
    for k, p in enumerate(J):
        u = k / NT
        keys.append((p, 0.05 - 0.004 * u - 0.022 * smoothstep(0.55, 1.0, u), 0.014 - 0.004 * u))
    b.add(tube(keys, sub=3, n=16, ref=V((0, 0, 1)), shape=groove, bulge=(0.0, 0.02)), M["tongue"], chain_weights(J, names, blend=0.025), "mouth")


def build_floor(b, M):
    """Fleshy mouth floor under the gold, ribbed like a palate, the gullet behind the tongue root (bone "body")."""
    mx, my = W / 2 - T - 0.004, D / 2 - T - 0.004

    def pt(u, v):
        x, y = mx * (2 * u - 1), my * (2 * v - 1)
        q = (x / mx) ** 2 + (y / my) ** 2
        return (x, y, FLOOR_Z - 0.018 * max(0.0, 1 - q))

    grid = surface(pt, 16, 10)
    b.add(grid, M["flesh"], "body", "mouth")
    b.add(ellipsoid((0, 0.15, FLOOR_Z + 0.001), (0.075, 0.05, 0.006), n=14, rings=4), M["throat"], "body", "mouth")


def surface(fn, nu, nv):
    verts = [V(fn(i / nu, j / nv)) for j in range(nv + 1) for i in range(nu + 1)]
    faces, fuv = [], []
    for j in range(nv):
        for i in range(nu):
            a = j * (nu + 1) + i
            faces.append([a, a + 1, a + nu + 2, a + nu + 1])
            fuv.append([(i / nu, j / nv), ((i + 1) / nu, j / nv), ((i + 1) / nu, (j + 1) / nv), (i / nu, (j + 1) / nv)])
    return verts, faces, fuv


def resample(points, count, dense=16):
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
    for a, c in zip(fine, fine[1:]):
        arc.append(arc[-1] + (c - a).length)
    out, j = [], 0
    for k in range(count + 1):
        target = arc[-1] * k / count
        while j < len(arc) - 2 and arc[j + 1] < target:
            j += 1
        t = (target - arc[j]) / max(arc[j + 1] - arc[j], 1e-9)
        out.append(fine[j].lerp(fine[j + 1], min(max(t, 0.0), 1.0)))
    return out


# ---------------------------------------------------------------- shell


def build_shell(M):
    """The item chest; its "root" parts go to the box (body), the legs blend down to the floor (base)."""
    b = Builder()
    items.build_chest(b, M, lid_bone="lid", heap_bone="heap")
    low = min(v.z for v in b.verts)
    b.verts = [V(v) - V((0, 0, low)) for v in b.verts]
    for i, (v, w) in enumerate(zip(b.verts, b.weights)):
        if "root" in w:
            b.weights[i] = {"base" if v.z < 0.001 else "body": 1.0}  # the feet stay on the floor, the legs shear
    return b


def chest_uvs(mesh, first_tris):
    """Per-loop UVs of the first first_tris loop triangles, read from treasure_chest.md3 (checked against positions)."""
    frames, _, uvs = md3.read_md3(CHEST_MD3)
    pos = frames[0]
    if len(uvs) != 3 * first_tris:
        raise ValueError("treasure_chest.md3 has {} triangles, the shell {}".format(len(uvs) // 3, first_tris))
    mesh.calc_loop_triangles()
    out = {}
    worst = 0.0
    for i, tri in enumerate(mesh.loop_triangles[:first_tris]):
        for c in range(3):
            co = mesh.vertices[tri.vertices[c]].co
            g = pos[3 * i + c]
            worst = max(worst, abs(co.x - g[0]), abs(co.z - g[1]), abs(-co.y - g[2]))
            out[tri.loops[c]] = uvs[3 * i + c]
    if worst > 1e-3:
        raise ValueError("shell does not match treasure_chest.md3 (off by {:.4f})".format(worst))
    return out


# ---------------------------------------------------------------- rig

BONES = ["base", "body", "lid", "heap", "teeth", "fang_f", "fang_l", "fang_r"] + ["t%d" % k for k in range(NT)]


def build_rig(coll):
    arm = bpy.data.armatures.new("mimic_rig")
    rig = bpy.data.objects.new("mimic_rig", arm)
    coll.objects.link(rig)
    arm.display_type = "STICK"
    rig.show_in_front = True
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    eb = arm.edit_bones
    heads = {"base": V((0, 0, 0)), "body": V((0, 0, 0.2)), "lid": HINGE, "heap": V((0, 0, 0.3)), "teeth": V((0, FRONT_Y, Z1)),
             "fang_f": HINGE, "fang_l": HINGE, "fang_r": HINGE}
    for name, h in heads.items():  # +Y bones, roll 0: bone space = world axes
        e = eb.new(name)
        e.head, e.tail = h, h + V((0, 0.08, 0))
        e.roll = 0.0
    J = tongue_joints()
    for k in range(NT):
        e = eb.new("t%d" % k)
        e.head, e.tail = J[k], J[k + 1]
        e.align_roll(V((0, 0, 1)))
    bpy.ops.object.mode_set(mode="OBJECT")
    for pb in rig.pose.bones:
        pb.rotation_mode = "QUATERNION"
    return rig


# ---------------------------------------------------------------- poses

REST = dict(yaw=0.0, roll=0.0, pitch=0.0, fwd=0.0, sway=0.0, lift=0.0, bx=0.0, bz=0.0, lid=LID_REST, teeth=0.0, fang=0.0, heap=1.0,
            heapz=0.0, tongue=0, wag=0.0)

# Tongue shapes: control points in body space (resampled along the bones); REST lies flat under the gold.
TONGUES = [
    TONGUE_REST,
    [V((0, 0.15, 0.236)), V((0, 0.0, 0.40)), V((0, -0.22, 0.475)), V((0, -0.31, 0.30))],  # 1 lolling over the front rim
    [V((0, 0.14, 0.25)), V((0, 0.03, 0.45)), V((0, -0.11, 0.6)), V((0, -0.22, 0.54))],  # 2 reared, curled up
    [V((0, 0.14, 0.25)), V((0, -0.06, 0.47)), V((0, -0.36, 0.52)), V((0, -0.64, 0.47))],  # 3 lashing out
    [V((0, 0.16, 0.24)), V((0, 0.06, 0.29)), V((0, -0.04, 0.31)), V((0, -0.14, 0.28))],  # 4 pulled in (snap)
    [V((0, 0.15, 0.236)), V((0.0, -0.06, 0.41)), V((0.03, -0.25, 0.445)), V((0.07, -0.30, 0.13))],  # 5 limp, hanging
]


def tongue_points(p):
    """Control points blended from p["tongue"] (float index into TONGUES), wagged sideways by p["wag"] (metres at the tip)."""
    i = min(int(p["tongue"]), len(TONGUES) - 2)
    t = p["tongue"] - i
    pts = [a.lerp(c, t) for a, c in zip(TONGUES[i], TONGUES[i + 1])]
    return [q + V((p["wag"] * k * k / 9, 0, 0)) for k, q in enumerate(pts)]


def deformations(p, fix=0.0):
    """Deformation matrix (armature space) of every bone for pose parameters p."""
    G = tr(0, 0, fix)
    move = G @ tr(p["sway"], -p["fwd"], p["lift"])
    pitch = p["pitch"]
    tilt = about((0, -D / 2 if pitch > 0 else D / 2 + 0.01, 0), rx(pitch))
    roll = p["roll"]
    side = about((W / 2 if roll > 0 else -W / 2, 0, 0), ry(roll))
    R = move @ rz(p["yaw"]) @ side @ tilt  # rigid part of the body
    S = Matrix.Diagonal((1 + p["bx"], 1 + p["bx"], 1 + p["bz"], 1.0))

    def rigid_at(point):
        """Rigid transform that follows the scaled body at point (keeps rigid parts free of shear)."""
        return R @ tr(S @ V(point) - V(point))

    # The feet lag the box: a quarter of its twist, most of its tilt (the back feet leave the floor in the lunge).
    feet_tilt = about((0, -D / 2 if pitch > 0 else D / 2 + 0.01, 0), rx(0.8 * pitch))
    feet_roll = about((W / 2 if roll > 0 else -W / 2, 0, 0), ry(0.6 * roll))
    Dm = {"base": move @ rz(0.25 * p["yaw"]) @ feet_roll @ feet_tilt, "body": R @ S}
    lid = rigid_at(HINGE) @ about(HINGE, rx(LID_REST - p["lid"]))
    Dm["lid"] = lid
    fold = FANG_FOLD * p["fang"]
    Dm["fang_f"] = lid @ LID_LOCAL @ about((0, FANG_Y, FANG_Z), rx(-fold)) @ LID_LOCAL.inverted()
    Dm["fang_r"] = lid @ LID_LOCAL @ about((FANG_X, 0, FANG_Z), ry(-fold)) @ LID_LOCAL.inverted()
    Dm["fang_l"] = lid @ LID_LOCAL @ about((-FANG_X, 0, FANG_Z), ry(fold)) @ LID_LOCAL.inverted()
    Dm["teeth"] = rigid_at((0, 0, Z1)) @ tr(0, 0, LOW_EXT * p["teeth"])
    g = p["heap"]
    Dm["heap"] = R @ S @ tr(0, 0, -0.24 * (1 - g) + p["heapz"]) @ about((0, 0, 0.3), Matrix.Scale(0.75 + 0.25 * g, 4))
    J = tongue_joints()
    P = resample(tongue_points(p), NT)
    root = rigid_at(J[0])
    for k in range(NT):
        r = J[k + 1] - J[k]
        q = P[k + 1] - P[k]
        rot = r.rotation_difference(q).to_matrix().to_4x4()
        u = r.normalized()
        s = q.length / r.length
        stretch = Matrix.Identity(4)
        for a in range(3):
            for c in range(3):
                stretch[a][c] += (s - 1) * u[a] * u[c]
        Dm["t%d" % k] = root @ tr(P[k]) @ rot @ stretch @ tr(-J[k])
    return Dm


class Skin:
    """numpy linear blend skinning of every vertex (floor contact and measurements)."""

    def __init__(self, b):
        import numpy as np

        self.np = np
        col = {n: j for j, n in enumerate(BONES)}
        self.P = np.array([[*v, 1.0] for v in b.verts])
        self.W = np.zeros((len(b.verts), len(BONES)))
        for r, w in enumerate(b.weights):
            tot = sum(w.values())
            for n, x in w.items():
                self.W[r, col[n]] = x / tot

    def coords(self, Dm):
        np = self.np
        out = np.zeros((len(self.P), 3))
        for j, n in enumerate(BONES):
            if self.W[:, j].any():
                out += self.W[:, j:j + 1] * (self.P @ np.array(Dm[n])[:3].T)
        return out


def pose(skin, p):
    """Deformations for p lifted just enough that nothing goes through the floor."""
    Dm = deformations(p)
    low = float(skin.coords(Dm)[:, 2].min())
    return deformations(p, -low) if low < 0 else Dm


def key_frame(rig, Dm, f):
    for bn in rig.data.bones:
        pb = rig.pose.bones[bn.name]
        pb.matrix_basis = bn.matrix_local.inverted() @ Dm[bn.name] @ bn.matrix_local
        for path in ("location", "rotation_quaternion", "scale"):
            pb.keyframe_insert(path, frame=f)


def blend(keys, f):
    """keys = [(frame, overrides)] on top of the previous key; smoothstep between keys."""
    full, cur = [], dict(REST)
    for k, over in keys:
        cur = dict(cur, **over)
        full.append((k, cur))
    for k, p in full:
        if k == f:
            return dict(p)
    for (f0, p0), (f1, p1) in zip(full, full[1:]):
        if f0 <= f <= f1:
            t = smoothstep(f0, f1, f)
            return {k: p0[k] + (p1[k] - p0[k]) * t for k in p0}
    return dict(full[-1][1])


AWAKE = dict(lid=LID_REST, teeth=1.0, fang=1.0, heap=0.0, tongue=1.0)


def walk_params(f, n):
    """Awake loop from and back to the awake pose (= attack frame 0): four snaps of the lid, the box swaying and
    twisting. Every oscillation is periodic over the clip and zero at frame 0, so the loop is seamless."""
    keys = [
        (0, AWAKE),
        (3, dict(lid=28)),
        (5, dict(lid=100)),
        (8, dict(lid=22)),
        (10, dict(lid=108)),
        (14, dict(lid=24)),
        (17, dict(lid=112)),
        (21, dict(lid=30)),
        (23, dict(lid=110)),
        (27, dict(lid=26)),
        (29, dict(lid=100)),
        (n, AWAKE),
    ]
    p = blend(keys, f)
    w = 2 * math.pi * f / n
    snap = max(0.0, (LID_REST - p["lid"]) / 80.0)  # 1 when the lid slams down
    p["yaw"] = 9 * math.sin(w * 2) + 3 * math.sin(w * 5)
    p["roll"] = 4 * math.sin(w * 3)
    p["sway"] = 0.02 * math.sin(w * 2)
    p["pitch"] = 4 * snap
    p["bz"] = -0.035 * snap + 0.015 * math.sin(w * 6)
    p["bx"] = 0.015 * snap
    p["wag"] = 0.06 * math.sin(w * 4)
    return p


ATTACK = [
    (0, AWAKE),
    (4, dict(lid=126, pitch=-9, bz=0.04, tongue=2.0, teeth=1.0)),
    (6, dict(lid=132, pitch=-11, bz=0.05, tongue=2.0)),
    (8, dict(lid=128, pitch=13, fwd=0.14, lift=0.03, bz=-0.01, tongue=3.0)),
    (10, dict(lid=16, pitch=11, fwd=0.17, lift=0.0, bz=-0.06, bx=0.03, tongue=4.0)),
    (11, dict(lid=20, yaw=12, roll=3, bz=-0.04, bx=0.02)),
    (12, dict(lid=15, yaw=-11, roll=-3, bz=-0.05)),
    (13, dict(lid=19, yaw=7, roll=2, bz=-0.03)),
    (15, dict(lid=45, yaw=0, roll=0, pitch=4, fwd=0.09, bz=0.0, bx=0.0, tongue=4.0)),
    (18, dict(lid=95, pitch=0, fwd=0.02, tongue=1.0)),
    (22, AWAKE),
]

DIE = [
    (0, AWAKE),
    (2, dict(lid=128, pitch=-7, yaw=6, tongue=2.4, bz=0.03)),
    (3, dict(lid=100, yaw=-7, roll=3, pitch=-5)),
    (4, dict(lid=118, yaw=7, roll=-3, tongue=2.0)),
    (5, dict(lid=90, yaw=-6, roll=3, bz=0.0)),
    (6, dict(lid=110, yaw=6, roll=-2, tongue=5.0)),
    (7, dict(lid=80, yaw=-5, roll=2)),
    (8, dict(lid=100, yaw=4, roll=-2)),
    (9, dict(lid=75, yaw=-3, roll=1, pitch=-2)),
    (11, dict(lid=85, yaw=0, roll=0, pitch=0, tongue=5.0)),
    (13, dict(lid=40, tongue=4.0, teeth=0.7, fang=0.8)),
    (15, dict(lid=10, tongue=0.0, teeth=0.05, fang=0.0, bz=0.0)),
    (16, dict(lid=0, teeth=0.0, bz=-0.035, bx=0.012)),
    (17, dict(lid=6, bz=0.012, bx=0.0)),
    (18, dict(lid=0, bz=0.0, heap=0.0)),
    (22, dict(heap=1.0)),
    (23, dict(lid=3)),
    (24, dict(lid=0)),
    (27, dict(lid=112)),
    (28, dict(lid=103)),
    (29, dict(REST)),
]


def idle_params(f, n):
    """The chest, still; once per loop the lid dips a hair and the box swells, as if it breathed."""
    p = dict(REST)
    br = math.sin(math.pi * min(max((f - 24) / 14, 0.0), 1.0)) ** 2
    p.update(lid=LID_REST - 4.5 * br, bz=0.012 * br, bx=0.005 * br, heapz=0.004 * br)
    return p


def clip_params(name, f, n):
    if name == "mimic_walk":
        return walk_params(f, n)
    if name == "mimic_attack":
        return blend(ATTACK, f)
    if name == "mimic_die":
        return blend(DIE, f)
    return idle_params(f, n)


def make_actions(rig, skin):
    rig.animation_data_create()
    acts = {}
    for name, _, frames in CLIPS:
        act = bpy.data.actions.get(name)
        if act:
            bpy.data.actions.remove(act)
        act = bpy.data.actions.new(name)
        act.use_fake_user = True
        rig.animation_data.action = act
        for f in range(frames):
            key_frame(rig, pose(skin, clip_params(name, f, frames)), f)
        acts[name] = act
    return acts


def report(skin, shell_count):
    np = skin.np
    rest = skin.coords(deformations(REST))
    lo, hi = rest[:shell_count].min(axis=0), rest[:shell_count].max(axis=0)
    alo, ahi = rest.min(axis=0), rest.max(axis=0)
    print("mimic rest: shell x {:.4f}..{:.4f} y {:.4f}..{:.4f} z {:.4f}..{:.4f}; all parts x {:.4f}..{:.4f} y {:.4f}..{:.4f} z {:.4f}..{:.4f}".format(
        lo[0], hi[0], lo[1], hi[1], lo[2], hi[2], alo[0], ahi[0], alo[1], ahi[1], alo[2], ahi[2]))
    for name, _, frames in CLIPS:
        c = [skin.coords(pose(skin, clip_params(name, f, frames))) for f in range(frames)]
        front = min(float(x[:, 1].min()) for x in c)
        print("mimic {}: min z {:.4f}, front y {:.3f} (rest {:.3f}), max |coord| {:.3f}".format(
            name, min(float(x[:, 2].min()) for x in c), front, float(lo[1]), max(float(np.abs(x).max()) for x in c)))


# ---------------------------------------------------------------- entry points


def merge(b, other):
    items.merge(b, other)


def build(bake=True, tex_path=None):
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    M = items.materials()
    M.update(materials())
    shell = build_shell(M)
    mouth = Builder()
    build_lower(mouth, M)
    build_upper(mouth, M)
    build_tongue(mouth, M)
    build_floor(mouth, M)

    # Mouth parts on their own: UVs and (hidden inside the chest at rest) AO baked without the shell around them.
    mobj = common.finish_mesh(mouth, coll, "mimic_mouth")
    common.uv_unwrap(mobj, mouth.tags)
    muv = [tuple(d.uv) for d in mobj.data.uv_layers["UVMap"].data]
    mouth_px = None
    if bake:
        import numpy as np

        img = common.bake_texture(mobj, os.path.join(bpy.app.tempdir or "/tmp", "mimic_mouth.png"), MOUTH_TEX, "mimic_mouth", ao_distance=0.05)
        mouth_px = np.array(img.pixels[:]).reshape(MOUTH_TEX, MOUTH_TEX, 4)
    data = mobj.data
    bpy.context.view_layer.objects.active = None
    bpy.data.objects.remove(mobj)
    bpy.data.meshes.remove(data)
    bpy.context.view_layer.update()

    b = Builder()
    merge(b, shell)
    merge(b, mouth)
    obj = common.finish_mesh(b, coll, "mimic_new")
    obj.data.set_sharp_from_angle(angle=math.radians(50))
    mesh = obj.data
    uv = mesh.uv_layers.new(name="UVMap")
    mesh.uv_layers.active = uv
    uv.active_render = True
    shell_loops = sum(len(f) for f in shell.faces)
    shell_tris = sum(len(f) - 2 for f in shell.faces)
    cuv = chest_uvs(mesh, shell_tris)
    flat = []
    for li in range(len(mesh.loops)):
        if li < shell_loops:
            u, v = cuv[li]
            flat += [u * 0.5, v]
        else:
            u, v = muv[li - shell_loops]
            flat += [0.5 + u * 0.5, v]
    uv.data.foreach_set("uv", flat)

    if bake:
        import numpy as np

        chest = bpy.data.images.load(CHEST_PNG, check_existing=False)
        cw, ch = chest.size
        chest_px = np.array(chest.pixels[:]).reshape(ch, cw, 4)
        out = np.ones((MOUTH_TEX, 2 * MOUTH_TEX, 4))
        out[:, :cw] = chest_px
        out[:, MOUTH_TEX:] = mouth_px
        bpy.data.images.remove(chest)
        tex = bpy.data.images.get("mimic_tex") or bpy.data.images.new("mimic_tex", 2 * MOUTH_TEX, MOUTH_TEX, alpha=False)
        tex.scale(2 * MOUTH_TEX, MOUTH_TEX)
        tex.pixels = out.ravel().tolist()
        tex.filepath_raw = tex_path or os.path.join(bpy.app.tempdir or "/tmp", "mimic_preview.png")
        tex.file_format = "PNG"
        tex.save()
        common.use_baked_material(obj, tex)

    rig = build_rig(coll)
    common.rig_object(obj, rig)
    skin = Skin(b)
    acts = make_actions(rig, skin)
    report(skin, len(shell.verts))
    rig.animation_data.action = acts["mimic_walk"]
    scene = bpy.context.scene
    scene.render.fps = 14
    scene.frame_start, scene.frame_end = 0, CLIPS[0][2] - 1
    scene.frame_set(0)
    print("mimic: {} verts, {} tris".format(len(obj.data.vertices), common.tri_count(obj)))
    return obj, rig


def export(models_dir=None):
    common.export_files(bpy.data.objects["mimic_new"], bpy.data.objects["mimic_rig"], "mimic", CLIPS, "monsters", models_dir)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    exporting = "--export" in args
    build(bake=exporting or "--bake" in args, tex_path=os.path.join(REPO, "textures", "monsters", "mimic.png") if exporting else None)
    if exporting:
        export()
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "mimic.blend"))
