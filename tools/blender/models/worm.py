"""Procedural man-eating worm: mesh, rig, baked texture and the three .md3 animations.

    MCP:  p = ".../tools/blender/models/worm.py"; g = {"__file__": p, "__name__": "worm"}
          exec(open(p).read(), g); g["build"]()          # then g["export"]()
    CLI:  blender -b --python tools/blender/models/worm.py -- [--export]

Blender space: Z up, the worm faces -Y (game uses rotA = 0), tail towards +Y.
The spine is posed procedurally from a curve (elevation, heading, roll per bone), so every
animation frame is keyed; three hinged jaws open around a toothed, round mouth.
"""

import importlib
import math
import os
import sys

import bpy
from mathutils import Matrix, Quaternion
from mathutils import Vector as V

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import common  # noqa: E402

importlib.reload(common)
from common import REPO, Builder, chain_weights, smoothstep, tube  # noqa: E402

COLL = "worm_new"
CLIPS = [("worm_walk", "", 32), ("worm_attack", "_att", 32), ("worm_die", "_die", 40)]
TEX_SIZE = 1024
REVIEW_VIEW = {"target": (0, 2.0, 0.7), "ortho": 5.8, "res": (560, 320)}

L = 4.6  # body length, tail tip (s = 0) to mouth rim (s = L)
N = 16  # spine bones, tail to head
RIDGE = 0.2  # body segment length
JAWS = [90, 210, 330]  # centre angle of each jaw around the mouth (0 = +X, 90 = up)
HINGE_R, HINGE_Y, JAW_LEN = 0.285, -0.04, 0.40

COL = {
    "belly": (0.50, 0.34, 0.28),
    "flank": (0.28, 0.13, 0.12),
    "back": (0.11, 0.055, 0.065),
    "back_dark": (0.06, 0.03, 0.04),
    "gum": (0.55, 0.16, 0.15),
    "red": (0.38, 0.025, 0.03),
    "dark_red": (0.09, 0.0, 0.01),
    "black": (0.01, 0.0, 0.0),
    "chitin": (0.07, 0.045, 0.045),
    "chitin_light": (0.22, 0.14, 0.11),
    "ivory": (0.78, 0.72, 0.55),
}


# ---------------------------------------------------------------- body shape


def radius(s):
    """Base body radius at arc length s from the tail tip."""
    keys = [(0.0, 0.045), (0.3, 0.12), (0.8, 0.21), (1.5, 0.29), (2.5, 0.345), (3.5, 0.36), (4.2, 0.345), (L, 0.31)]
    for (s0, r0), (s1, r1) in zip(keys, keys[1:]):
        if s <= s1:
            return r0 + (r1 - r0) * smoothstep(s0, s1, s)
    return keys[-1][1]


def ridge(s):
    """Segment bulge: 1 mid-segment, dips in the grooves between segments."""
    if s < 0.3 or s > L - 0.1:
        return 1.0
    return 0.93 + 0.07 * math.sin(math.pi * ((s / RIDGE) % 1.0)) ** 0.6


def centre_z(s):
    return 0.85 * radius(s)  # belly (flattened to 0.85) rests on the floor


def belly(th):
    return 1.0 if math.sin(th) >= 0 else 1.0 - 0.15 * math.sin(th) ** 2


MOUTH_Z = centre_z(L)


def revolve(rings, vs, n=14, shape=belly, caps=(False, False), bulge=(0.0, 0.0)):
    """Surface of revolution around lines parallel to Y. rings = [(y, r, zc)], vs = pattern v per ring.

    Normals face outward while the profile runs towards -Y and inward once it turns back,
    so one profile can go along the outside, over the lip and down the throat.
    """
    verts, faces, fuv = [], [], []
    for y, r, z in rings:
        for j in range(n):
            th = 2 * math.pi * j / n
            k = shape(th) if shape else 1.0
            verts.append(V((r * k * math.cos(th), y, z + r * k * math.sin(th))))
    for i in range(len(rings) - 1):
        for j in range(n):
            j2 = (j + 1) % n
            faces.append([i * n + j, i * n + j2, (i + 1) * n + j2, (i + 1) * n + j])
            fuv.append([(j / n, vs[i]), ((j + 1) / n, vs[i]), ((j + 1) / n, vs[i + 1]), (j / n, vs[i + 1])])
    for end in (0, 1):
        if not caps[end]:
            continue
        i = 0 if end == 0 else len(rings) - 1
        y, _, z = rings[i]
        ci = len(verts)
        verts.append(V((0, y + (bulge[0] if end == 0 else bulge[1]), z)))
        for j in range(n):
            a, b = i * n + j, i * n + (j + 1) % n
            faces.append([ci, b, a] if end == 0 else [ci, a, b])
            fuv.append([(0.5, vs[i])] * 3)
    return verts, faces, fuv


def orient(verts, face, expected):
    """Return face with winding flipped if its normal points away from expected."""
    a, b, c = (verts[i] for i in face[:3])
    return face if (b - a).cross(c - a).dot(expected) >= 0 else face[::-1]


def cone(base, direction, length, r, n=6):
    d = V(direction).normalized()
    ref = V((0, 0, 1)) if abs(d.z) < 0.9 else V((1, 0, 0))
    return tube([(base - d * 0.01, r, r), (base + d * length * 0.5, r * 0.55, r * 0.55), (base + d * length, 0.002, 0.002)],
                sub=1, n=n, ref=ref, caps=(True, False))


# ---------------------------------------------------------------- materials


def body_material():
    """Dark mottled back fading to a pale belly (pattern u), darker grooves between segments (pattern v = s)."""
    mat = bpy.data.materials.get("worm_body") or bpy.data.materials.new("worm_body")
    nt = mat.node_tree
    for node in list(nt.nodes):
        if node.type not in ("OUTPUT_MATERIAL", "BSDF_PRINCIPLED"):
            nt.nodes.remove(node)
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Roughness"].default_value = 0.5
    new = nt.nodes.new
    uv = new("ShaderNodeUVMap")
    uv.uv_map = "pattern"
    sep = new("ShaderNodeSeparateXYZ")
    nt.links.new(uv.outputs["UV"], sep.inputs[0])

    # u: 0.25 = top, 0.75 = bottom -> sin(2 pi u) runs +1 (back) .. -1 (belly).
    ang = new("ShaderNodeMath")
    ang.operation = "MULTIPLY"
    ang.inputs[1].default_value = 2 * math.pi
    sn = new("ShaderNodeMath")
    sn.operation = "SINE"
    remap = new("ShaderNodeMath")
    remap.operation = "MULTIPLY_ADD"
    remap.inputs[1].default_value = 0.5
    remap.inputs[2].default_value = 0.5
    nt.links.new(sep.outputs["X"], ang.inputs[0])
    nt.links.new(ang.outputs[0], sn.inputs[0])
    nt.links.new(sn.outputs[0], remap.inputs[0])
    tone = new("ShaderNodeValToRGB")
    els = tone.color_ramp.elements
    els[0].position, els[0].color = 0.0, (*COL["belly"], 1)
    els[1].position, els[1].color = 1.0, (*COL["back_dark"], 1)
    els.new(0.35).color = (*COL["flank"], 1)
    els.new(0.7).color = (*COL["back"], 1)
    nt.links.new(remap.outputs[0], tone.inputs["Fac"])

    div = new("ShaderNodeMath")
    div.operation = "MULTIPLY"
    div.inputs[1].default_value = 1.0 / RIDGE
    fr = new("ShaderNodeMath")
    fr.operation = "FRACT"
    groove = new("ShaderNodeValToRGB")
    ge = groove.color_ramp.elements
    ge[0].position, ge[0].color = 0.0, (0.35, 0.35, 0.35, 1)
    ge[1].position, ge[1].color = 1.0, (0.35, 0.35, 0.35, 1)
    ge.new(0.14).color = (1, 1, 1, 1)
    ge.new(0.86).color = (1, 1, 1, 1)
    nt.links.new(sep.outputs["Y"], div.inputs[0])
    nt.links.new(div.outputs[0], fr.inputs[0])
    nt.links.new(fr.outputs[0], groove.inputs["Fac"])

    coord = new("ShaderNodeTexCoord")
    noise = new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 7.0
    noise.inputs["Detail"].default_value = 4.0
    mott = new("ShaderNodeValToRGB")
    me = mott.color_ramp.elements
    me[0].position, me[0].color = 0.35, (0.6, 0.6, 0.6, 1)
    me[1].position, me[1].color = 0.7, (1.15, 1.15, 1.15, 1)
    nt.links.new(coord.outputs["Object"], noise.inputs["Vector"])
    nt.links.new(noise.outputs["Fac"], mott.inputs["Fac"])

    m1 = new("ShaderNodeVectorMath")
    m1.operation = "MULTIPLY"
    m2 = new("ShaderNodeVectorMath")
    m2.operation = "MULTIPLY"
    nt.links.new(tone.outputs["Color"], m1.inputs[0])
    nt.links.new(groove.outputs["Color"], m1.inputs[1])
    nt.links.new(m1.outputs[0], m2.inputs[0])
    nt.links.new(mott.outputs["Color"], m2.inputs[1])
    nt.links.new(m2.outputs[0], bsdf.inputs["Base Color"])
    return mat


def materials():
    specs = {
        "throat": ("stripes", "v", 0.85, [(1, "gum"), (1.5, "red"), (3, "dark_red"), (3, "black")], "LINEAR"),
        "jaw_out": ("stripes", "v", 0.07, [(3, "chitin"), (1, "chitin_light")], "LINEAR"),
        "jaw_in": ("solid", "gum"),
        "tooth": ("solid", "ivory"),
        "chitin": ("solid", "chitin"),
    }
    M = {name: common.make_material("worm_" + name, spec, COL) for name, spec in specs.items()}
    M["body"] = body_material()
    return M


# ---------------------------------------------------------------- rig


def spine_joints():
    return [V((0, L - k * L / N, centre_z(k * L / N))) for k in range(N + 1)]


def jaw_frame(k):
    """Hinge point, petal mid point and hinge axis of jaw k."""
    phi = math.radians(JAWS[k])
    hinge = V((HINGE_R * math.cos(phi), HINGE_Y, MOUTH_Z + HINGE_R * math.sin(phi)))
    axis = V((-math.sin(phi), 0, math.cos(phi)))
    return hinge, petal_point(phi, 0.0, 0.5), axis


def build_rig(coll):
    arm = bpy.data.armatures.new("worm_rig")
    rig = bpy.data.objects.new("worm_rig", arm)
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
        b = eb.new(name)
        b.head, b.tail = V(head), V(tail)
        b.align_roll(V(x_axis).cross((b.tail - b.head).normalized()))
        if parent:
            b.parent = eb[parent]
            b.use_connect = connect

    J = spine_joints()
    bone("root", (0, 0, 0), (0, -0.3, 0), (1, 0, 0))
    for k in range(N):
        bone("s%02d" % k, J[k], J[k + 1], (1, 0, 0), "root" if k == 0 else "s%02d" % (k - 1), k > 0)
    for k in range(len(JAWS)):
        hinge, mid, axis = jaw_frame(k)
        bone("jaw%d" % k, hinge, mid, axis, "s%02d" % (N - 1))
    bpy.ops.object.mode_set(mode="OBJECT")
    for pb in rig.pose.bones:
        pb.rotation_mode = "QUATERNION"
    return rig


def jaw_open_sign(k):
    """+1 or -1: which rotation about the hinge swings jaw k away from the mouth axis."""
    hinge, mid, axis = jaw_frame(k)
    tip = petal_point(math.radians(JAWS[k]), 0.0, 1.0) - hinge
    moved = Matrix.Rotation(math.radians(30), 3, axis) @ tip + hinge
    return 1 if V((moved.x, 0, moved.z - MOUTH_Z)).length > V((tip.x + hinge.x, 0, tip.z + hinge.z - MOUTH_Z)).length else -1


# ---------------------------------------------------------------- parts


def petal_point(phi_c, a, v, inner=False):
    """Jaw surface: a in [-1, 1] across the jaw, v in [0, 1] hinge -> tip; closed jaws form a pointed dome."""
    half = math.radians(56) * (0.9 if inner else 1.0)
    phi = phi_c + a * half  # sectors keep their width so closed jaws meet along seams
    rho = HINGE_R * math.cos(v * math.pi / 2) ** 0.85
    y = HINGE_Y - JAW_LEN * math.sin(v * math.pi / 2)
    if inner:
        rho = max(rho - (0.035 * (1 - v) + 0.008), 0.0)
        y += 0.02 * (1 - v)
    return V((rho * math.cos(phi), y, MOUTH_Z + rho * math.sin(phi)))


def build_body(b, M):
    J = spine_joints()
    weights = chain_weights(J, ["s%02d" % k for k in range(N)], blend=L / N / 2)
    # Outside: tail tip -> mouth rim, one ring every quarter segment.
    ss = [i * RIDGE / 4 for i in range(int(round(L / (RIDGE / 4)))) ] + [L]
    rings = [(L - s, radius(s) * ridge(s), centre_z(s)) for s in ss]
    b.add(revolve(rings, ss, caps=(True, False), bulge=(0.03, 0)), M["body"], weights)
    # Lip and throat: over the rim and back into the body, ending in darkness.
    lip = [(0.0, 0.31), (-0.025, 0.305), (-0.04, 0.29), (-0.045, 0.27), (-0.035, 0.25), (-0.01, 0.24), (0.05, 0.235), (0.15, 0.22), (0.3, 0.19),
           (0.45, 0.15), (0.55, 0.11), (0.6, 0.06)]
    arc = [0.0]
    for (y0, r0), (y1, r1) in zip(lip, lip[1:]):
        arc.append(arc[-1] + math.hypot(y1 - y0, r1 - r0))
    b.add(revolve([(y, r, MOUTH_Z) for y, r in lip], arc, shape=None, caps=(False, True), bulge=(0, 0.02)), M["throat"], weights, "head")
    # Throat teeth: rings of hooks pointing inward and down the gullet.
    for y, r, count, length, off in ((0.06, 0.232, 12, 0.085, 0.0), (0.2, 0.207, 10, 0.07, 0.5), (0.34, 0.178, 8, 0.055, 0.25)):
        for i in range(count):
            th = 2 * math.pi * (i + off) / count
            radial = V((math.cos(th), 0, math.sin(th)))
            base = V((0, y, MOUTH_Z)) + radial * r
            b.add(cone(base, -radial + V((0, 0.55, 0)), length, 0.02), M["tooth"], weights, "head")
    # Bristles along the flanks, spines along the back, stinger on the tail.
    s = 0.5
    while s < L - 0.45:
        for th, length, rr, dorsal in ((math.radians(-15), 0.07, 0.012, False), (math.radians(195), 0.07, 0.012, False), (math.pi / 2, 0.1, 0.018, True)):
            if dorsal and int(round(s / RIDGE)) % 2:
                continue
            k = belly(th)
            radial = V((math.cos(th), 0, math.sin(th)))
            base = V((0, L - s, centre_z(s))) + radial * radius(s) * k * 0.98
            b.add(cone(base, radial + V((0, 0.55, 0)), length, rr), M["chitin"], weights)
        s += RIDGE
    b.add(cone(V((0, L - 0.02, centre_z(0.02))), V((0, 1, 0.15)), 0.18, 0.035), M["chitin"], weights)


def build_jaws(b, M):
    na, nv = 7, 7
    for k, deg in enumerate(JAWS):
        phi = math.radians(deg)
        verts, faces, fuv, mats = [], [], [], []
        grid = {}
        for side in ("out", "in"):
            for i in range(nv):
                v = 0.97 * i / (nv - 1)
                for j in range(na):
                    a = -1 + 2 * j / (na - 1)
                    grid[side, i, j] = len(verts)
                    verts.append(petal_point(phi, a, v, side == "in"))
        tip = len(verts)
        verts.append(V((0, HINGE_Y - JAW_LEN, MOUTH_Z)))
        radial = V((math.cos(phi), 0, math.sin(phi)))
        tangent = V((-math.sin(phi), 0, math.cos(phi)))

        def add(face, expected, mat, uv):
            faces.append(orient(verts, face, expected))
            if faces[-1] is not face:
                uv = uv[::-1]
            fuv.append(uv)
            mats.append(mat)

        for i in range(nv - 1):
            for j in range(na - 1):
                uv = [(j / na, i * 0.07), ((j + 1) / na, i * 0.07), ((j + 1) / na, (i + 1) * 0.07), (j / na, (i + 1) * 0.07)]
                out = [grid["out", i, j], grid["out", i, j + 1], grid["out", i + 1, j + 1], grid["out", i + 1, j]]
                add(out, radial - V((0, 0.6, 0)), "jaw_out", uv)
                inn = [grid["in", i, j], grid["in", i, j + 1], grid["in", i + 1, j + 1], grid["in", i + 1, j]]
                add(inn, -radial + V((0, 0.6, 0)), "jaw_in", uv)
            for j, sgn in ((0, -1), (na - 1, 1)):
                strip = [grid["out", i, j], grid["out", i + 1, j], grid["in", i + 1, j], grid["in", i, j]]
                add(strip, tangent * sgn, "jaw_out", [(0, i * 0.07), (0, (i + 1) * 0.07), (0.1, (i + 1) * 0.07), (0.1, i * 0.07)])
        for j in range(na - 1):
            strip = [grid["out", 0, j], grid["out", 0, j + 1], grid["in", 0, j + 1], grid["in", 0, j]]
            add(strip, V((0, 1, 0)), "jaw_out", [(j / na, 0), ((j + 1) / na, 0), ((j + 1) / na, 0.02), (j / na, 0.02)])
            last = nv - 1
            add([grid["out", last, j], grid["out", last, j + 1], tip], radial - V((0, 1, 0)), "jaw_out", [(j / na, 0.45), ((j + 1) / na, 0.45), (0.5, 0.5)])
            add([grid["in", last, j], grid["in", last, j + 1], tip], -radial, "jaw_in", [(j / na, 0.45), ((j + 1) / na, 0.45), (0.5, 0.5)])
        for j, sgn in ((0, -1), (na - 1, 1)):
            add([grid["out", nv - 1, j], tip, grid["in", nv - 1, j]], tangent * sgn, "jaw_out", [(0, 0.45), (0, 0.5), (0.1, 0.45)])
        bone = "jaw%d" % k
        for mat in ("jaw_out", "jaw_in"):
            sel = [i for i, m in enumerate(mats) if m == mat]
            b.add((verts, [faces[i] for i in sel], [fuv[i] for i in sel]), M[mat], bone, "head")
        # Hooked teeth on the inside of each jaw.
        for v, a_list, length in ((0.2, (-0.6, 0.0, 0.6), 0.1), (0.42, (-0.4, 0.4), 0.085), (0.63, (0.0,), 0.07)):
            for a in a_list:
                base = petal_point(phi, a, v, True)
                inward = V((0, base.y, MOUTH_Z)) - base
                b.add(cone(base, inward.normalized() + V((0, 0.5, 0)), length, 0.022), M["tooth"], bone, "head")


# ---------------------------------------------------------------- animation

REST = {"rise": 38, "bend": 38, "head": -12, "wave": 0, "lat": 0, "phase": 0, "hyaw": 0, "curl": 0, "roll": 0, "lift": 0, "shift": 0, "jaw": 0}


def walk_params(t):
    """Crawl cycle at phase t in [0, 1): retrograde ground waves, side sway, head bob, tasting jaws."""
    p = dict(REST)
    p.update(wave=12, lat=11, phase=t, hyaw=7 * math.sin(2 * math.pi * t), rise=38 + 4 * math.sin(4 * math.pi * t),
             jaw=2.5 + 2.5 * math.sin(4 * math.pi * t))
    return p


READY = walk_params(0.0)


def curve(p):
    """Per-bone (elevation, heading, roll) in radians for pose parameters p."""
    out = []
    for k in range(N):
        u = (k + 0.5) / N
        ground = 1 - smoothstep(0.5, 0.65, u)
        el = p["rise"] * smoothstep(0.55, 0.72, u) - p["bend"] * smoothstep(0.78, 0.92, u) + p["head"] * smoothstep(0.9, 1.0, u)
        el += p["wave"] * ground * math.sin(2 * math.pi * (2.2 * u + p["phase"]))
        yaw = p["lat"] * math.sin(2 * math.pi * (1.3 * u + p["phase"])) * (0.35 + 0.65 * (1 - u))
        yaw += p["hyaw"] * smoothstep(0.75, 1.0, u) + p["curl"] * (u - 0.5)
        out.append((math.radians(el), math.radians(yaw), math.radians(p["roll"])))
    return out


def spine_targets(p, anchor_k):
    """Armature-space matrices for every spine bone, resting on the floor."""
    J = spine_joints()
    pts = [V((0, 0, 0))]
    dirs = []
    for k, (el, yaw, roll) in enumerate(curve(p)):
        seg = J[k + 1] - J[k]
        el += math.asin(max(-1.0, min(1.0, seg.z / seg.length)))  # rest slope near the thin tail
        d = V((math.sin(yaw) * math.cos(el), -math.cos(yaw) * math.cos(el), math.sin(el)))
        dirs.append((d, roll))
        pts.append(pts[-1] + d * seg.length)
    shift = V((J[anchor_k].x - pts[anchor_k].x, J[anchor_k].y - pts[anchor_k].y - p["shift"], 0))
    # Belly is flattened to 0.85 r; rolled onto its side the round flank touches the floor instead.
    side = abs(math.sin(math.radians(p["roll"])))
    bristle = 0.07 * side  # flank bristles point down when rolled
    floor = min(pts[k].z - radius(k * L / N) * (0.85 + 0.15 * side) - bristle for k in range(N + 1))
    shift.z = -floor + p["lift"]
    mats = []
    for k, (d, roll) in enumerate(dirs):
        up = V((0, 0, 1)) if abs(d.z) < 0.99 else V((0, 1, 0))
        x0 = up.cross(d).normalized()
        z0 = x0.cross(d)
        x = x0 * math.cos(roll) + z0 * math.sin(roll)
        m = Matrix((x, d, x.cross(d))).transposed().to_4x4()
        m.translation = pts[k] + shift
        mats.append(m)
    return mats


def jaw_matrices(rig, head_target, p):
    """Posed armature-space matrices of the jaws for a given head bone matrix."""
    bones = rig.data.bones
    head_rest = bones["s%02d" % (N - 1)].matrix_local
    out = []
    for k in range(len(JAWS)):
        rest = bones["jaw%d" % k].matrix_local
        rot = Quaternion((1, 0, 0), math.radians(p["jaw"] * (1.0, 0.92, 1.06)[k]) * jaw_open_sign(k))
        out.append((head_target @ head_rest.inverted() @ rest @ rot.to_matrix().to_4x4(), rest, rot))
    return out


def key_frame(rig, p, frame, anchor_k):
    bones = rig.data.bones
    targets = spine_targets(p, anchor_k)
    # Open jaws must not sink into the floor: lift the worm so the lowest jaw tip touches it.
    tips = [petal_point(math.radians(JAWS[k]), a, 0.97) for k in range(len(JAWS)) for a in (-1, 0, 1)]
    low = min((m @ rest.inverted() @ tip).z for (m, rest, _), k3 in zip(jaw_matrices(rig, targets[-1], p), range(3)) for tip in tips[k3 * 3:k3 * 3 + 3])
    if low < 0:
        p = dict(p, lift=p["lift"] - low)
        targets = spine_targets(p, anchor_k)
    parent_pose = parent_rest = bones["root"].matrix_local
    for k, target in enumerate(targets):
        name = "s%02d" % k
        rest = bones[name].matrix_local
        rig.pose.bones[name].matrix_basis = (parent_rest.inverted() @ rest).inverted() @ parent_pose.inverted() @ target
        rig.pose.bones[name].keyframe_insert("rotation_quaternion", frame=frame)
        if k == 0:
            rig.pose.bones[name].keyframe_insert("location", frame=frame)
        parent_pose, parent_rest = target, rest
    for k, (_, _, rot) in enumerate(jaw_matrices(rig, targets[-1], p)):
        pb = rig.pose.bones["jaw%d" % k]
        pb.rotation_quaternion = rot  # jaws differ slightly so they don't move as one piece
        pb.keyframe_insert("rotation_quaternion", frame=frame)


def interpolate(keys, frame):
    """Eased interpolation between (frame, overrides) keys, overrides applied on top of READY."""
    full = []
    for f, over in keys:
        p = dict(READY)
        p.update(over)
        full.append((f, p))
    for (f0, p0), (f1, p1) in zip(full, full[1:]):
        if f0 <= frame <= f1:
            t = smoothstep(f0, f1, frame)
            return {k: p0[k] + (p1[k] - p0[k]) * t for k in p0}
    return full[-1][1]


ATTACK = [
    (0, {}),
    (9, dict(rise=72, bend=82, head=20, jaw=95, wave=0, lat=0, hyaw=0, shift=-0.35)),
    (13, dict(rise=34, bend=44, head=-8, jaw=115, wave=0, lat=0, hyaw=0, shift=0.70)),
    (15, dict(rise=32, bend=44, head=-12, jaw=110, wave=0, lat=0, hyaw=0, shift=0.80)),
    (17, dict(rise=32, bend=42, head=-8, jaw=0, wave=0, lat=0, hyaw=0, shift=0.76)),
    (19, dict(rise=34, bend=42, head=-6, jaw=0, wave=0, lat=0, hyaw=18, shift=0.72)),
    (21, dict(rise=34, bend=42, head=-6, jaw=0, wave=0, lat=0, hyaw=-16, shift=0.68)),
    (23, dict(rise=35, bend=40, head=-8, jaw=2, wave=0, lat=0, hyaw=10, shift=0.55)),
    (26, dict(rise=36, bend=38, head=-10, jaw=3, wave=0, lat=0, hyaw=0, shift=0.28)),
    (32, {}),
]

DIE = [
    (0, {}),
    (3, dict(rise=62, bend=50, head=35, jaw=70, wave=0, lat=0, hyaw=0)),
    (6, dict(rise=55, bend=50, head=25, jaw=40, wave=4, lat=25, phase=0.1, hyaw=-25, roll=15)),
    (9, dict(rise=50, bend=48, head=15, jaw=80, wave=4, lat=-25, phase=0.2, hyaw=25, roll=-20, curl=20)),
    (12, dict(rise=40, bend=40, head=10, jaw=30, wave=4, lat=30, phase=0.3, hyaw=-30, roll=25, curl=-15)),
    (15, dict(rise=28, bend=28, head=0, jaw=70, wave=3, lat=-20, phase=0.4, hyaw=20, roll=-10, curl=25)),
    (19, dict(rise=0, bend=0, head=-5, jaw=50, wave=2, lat=5, phase=0.45, hyaw=5, roll=50, curl=45, lift=0.12)),
    (22, dict(rise=0, bend=0, head=0, jaw=45, wave=0, lat=0, phase=0.45, hyaw=0, roll=70, curl=55)),
    (25, dict(rise=0, bend=0, head=0, jaw=42, wave=0, lat=0, phase=0.45, hyaw=0, roll=72, curl=58, lift=0.04)),
    (28, dict(rise=0, bend=0, head=0, jaw=40, wave=0, lat=0, phase=0.45, hyaw=8, roll=75, curl=60)),
    (31, dict(rise=0, bend=0, head=8, jaw=50, wave=0, lat=0, phase=0.45, hyaw=8, roll=75, curl=60)),
    (34, dict(rise=0, bend=0, head=0, jaw=38, wave=0, lat=4, phase=0.45, hyaw=6, roll=75, curl=60)),
    (39, dict(rise=0, bend=0, head=0, jaw=38, wave=0, lat=0, phase=0.45, hyaw=6, roll=75, curl=60)),
]


def make_actions(rig):
    rig.animation_data_create()
    acts = {}
    for name, suffix, frames in CLIPS:
        act = bpy.data.actions.get(name)
        if act:
            bpy.data.actions.remove(act)
        act = bpy.data.actions.new(name)
        act.use_fake_user = True
        rig.animation_data.action = act
        last = frames if name != "worm_die" else frames - 1  # loops key frame N = frame 0
        for f in range(last + 1):
            if name == "worm_walk":
                key_frame(rig, walk_params(f / frames), f, N // 2)
            elif name == "worm_attack":
                key_frame(rig, interpolate(ATTACK, f), f, 2)  # tail planted, front lunges
            else:
                key_frame(rig, interpolate(DIE, f), f, N // 2)
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
    build_body(b, M)
    build_jaws(b, M)
    obj = common.finish_mesh(b, coll, "worm_new")
    common.uv_unwrap(obj, b.tags, {"head": ((0, -0.1, MOUTH_Z), 1.6)})
    if bake:
        tex = common.bake_texture(obj, tex_path or os.path.join(bpy.app.tempdir or "/tmp", "worm_preview.png"), TEX_SIZE, "worm", ao_distance=0.25)
        common.use_baked_material(obj, tex)
    common.rig_object(obj, rig)
    acts = make_actions(rig)
    rig.animation_data.action = acts["worm_walk"]
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 0, CLIPS[0][2] - 1
    scene.frame_set(0)
    print("worm: {} verts, {} tris".format(len(obj.data.vertices), common.tri_count(obj)))
    return obj, rig


def export(models_dir=None):
    common.export_files(bpy.data.objects["worm_new"], bpy.data.objects["worm_rig"], "worm", CLIPS, "monsters", models_dir)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    build(tex_path=os.path.join(REPO, "textures", "monsters", "worm.png") if "--export" in args else None)
    if "--export" in args:
        export()
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "worm.blend"))
