"""Procedural Egyptian cobra (Naja haje): mesh, rig, baked texture and the .md3 animations (move, attack, die, idle,
rise, spit).

    MCP:  p = ".../tools/blender/models/cobra.py"; g = {"__file__": p, "__name__": "cobra"}
          exec(open(p).read(), g); g["build"]()          # then g["export"]()
    CLI:  blender -b --python tools/blender/models/cobra.py -- [--export] [--measure]

Two textures share one UV layout: cobra.png (sand brown, Naja haje) and cobra_giant.png (black-necked: near black,
pale cream throat band, amber eyes). Set COBRA_TEX=giant to show the giant texture on the built object.
cobra_apep.png (Apep, the boss: red-black, gold-green scale edges, yellow eyes, a dark red hood with pale eye-spots):
bake it alone with --boss-texture; COBRA_TEX=apep shows it.

Blender space: Z up, the cobra faces +Y (game uses rotA = 180, as the scorpion), its right side is +X.
The body is one tube along a chain of spine joints (tail tip to the back of the head), each joint a bone with its
own frame (the vertices blend between the two joints round them). A pose is the heading (yaw), elevation and roll
along the body, integrated from the tail so the length never changes (as in worm.py), plus a hood amount that
spreads the neck joints sideways and flattens them (bone scale). The head is rigid on its own bone, the lower jaw
on a hinge, the forked tongue slides out of the mouth. Every frame the pose is moved so a fixed spine joint (the base
of the raised front) stays put, blended towards the coil's centre in the idle pose, and lifted so the lowest vertex
touches the floor (numpy copy of the skinning).
--measure prints each clip's frame 0 extents and the spit clip's mouth position per frame.
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

COLL = "cobra_new"
CLIPS = [
    ("cobra_move", "", 24),
    ("cobra_attack", "_att", 22),
    ("cobra_die", "_die", 30),
    ("cobra_idle", "_idle", 32),
    ("cobra_rise", "_rise", 24),
    ("cobra_spit", "_spit", 18),
]
LOOPS = ("cobra_move", "cobra_attack", "cobra_idle")
TEX_SIZE = 1024
REVIEW_VIEW = {"target": (0, -0.2, 0.4), "ortho": 2.6, "res": (560, 420)}

COL = {
    "back": (0.30, 0.20, 0.09),
    "flank": (0.42, 0.30, 0.14),
    "band": (0.13, 0.085, 0.04),
    "belly": (0.72, 0.62, 0.40),
    "throat": (0.05, 0.035, 0.025),
    "head": (0.22, 0.15, 0.07),
    "eye": (0.01, 0.008, 0.006),
    "mouth": (0.70, 0.38, 0.38),
    "fang": (0.85, 0.82, 0.70),
    "tongue": (0.08, 0.03, 0.035),
}
# The giant cobra (cobra_giant.png, same UVs): black-necked, near black with faint darker bands, a pale cream throat
# band under the hood, amber eyes.
COL_GIANT = {
    "back": (0.035, 0.03, 0.026),
    "flank": (0.06, 0.05, 0.04),
    "band": (0.012, 0.01, 0.009),
    "belly": (0.30, 0.27, 0.21),
    "throat": (0.78, 0.72, 0.56),
    "head": (0.03, 0.025, 0.02),
    "eye": (0.75, 0.42, 0.04),
    "mouth": (0.55, 0.30, 0.32),
    "fang": (0.85, 0.82, 0.70),
    "tongue": (0.05, 0.02, 0.025),
}
# Apep (cobra_apep.png, same UVs, a boss): deep red-black, gold-green scale edges ("edge"), glowing yellow eyes, a dark
# red hood ("hood") with a pale eye-spot on its back ("spot").
COL_APEP = {
    "back": (0.07, 0.008, 0.007),
    "flank": (0.11, 0.015, 0.012),
    "band": (0.025, 0.004, 0.004),
    "belly": (0.22, 0.06, 0.035),
    "throat": (0.03, 0.006, 0.005),
    "head": (0.08, 0.01, 0.008),
    "eye": (1.0, 0.85, 0.08),
    "mouth": (0.60, 0.12, 0.10),
    "fang": (0.88, 0.80, 0.55),
    "tongue": (0.04, 0.01, 0.01),
    "edge": (0.55, 0.50, 0.12),
    "hood": (0.30, 0.025, 0.02),
    "spot": (0.85, 0.80, 0.62),
}

LB = 2.9  # body length, tail tip (s = 0) to the back of the head (s = LB)
N = 56  # spine segments; joints 0 .. N
DS = LB / N
RAISED = 1.05  # the raised front: from the base joint to the head
A = N - round(RAISED / DS)  # base joint of the raised front: the anchor of the reared poses
BASE_Y = 0.15  # where the anchor joint stands (y)
JAW_HINGE = V((0, 0.0, -0.022))
MOUTH = V((0, 0.185, -0.02))  # the mouth's front, head-bone rest space (spit release point)


def joint_s(k):
    return k * DS


# ---------------------------------------------------------------- body shape


def radius(s):
    """Half width of the body at arc length s from the tail tip."""
    keys = [(0.0, 0.007), (0.25, 0.026), (0.7, 0.054), (1.3, 0.075), (1.8, 0.078), (2.2, 0.069), (2.5, 0.058), (2.75, 0.052), (LB, 0.054)]
    for (s0, r0), (s1, r1) in zip(keys, keys[1:]):
        if s <= s1:
            return r0 + (r1 - r0) * smoothstep(s0, s1, s)
    return keys[-1][1]


TOP, BELLY = 0.92, 0.62  # half height above / below the centre line, times the half width


def ring_point(th, rx):
    sn = math.sin(th)
    ry = rx * (TOP if sn >= 0 else BELLY)
    cs = math.cos(th)
    # A flat belly with rounded edges.
    k = 1.0 if sn >= 0 else 1.0 + 0.12 * sn * sn * abs(cs)
    return V((rx * cs * k, 0, ry * sn))


def hood_profile(s):
    """0..1: how much the joint at s spreads with the hood (broadest a little behind the head)."""
    d = LB - s
    return smoothstep(0.03, 0.15, d) * (1 - smoothstep(0.24, 0.52, d))


# ---------------------------------------------------------------- materials


def _math(nt, op, a=None, b=None, value=None):
    n = nt.nodes.new("ShaderNodeMath")
    n.operation = op
    for i, x in enumerate((a, b)):
        if x is None:
            continue
        if isinstance(x, (int, float)):
            n.inputs[i].default_value = x
        else:
            nt.links.new(x, n.inputs[i])
    return n.outputs[0]


def _mix(nt, fac, c1, c2):
    m = nt.nodes.new("ShaderNodeMix")
    m.data_type = "RGBA"
    nt.links.new(fac, m.inputs["Factor"])
    for sock, c in ((m.inputs[6], c1), (m.inputs[7], c2)):
        if isinstance(c, tuple):
            sock.default_value = (*c, 1)
        else:
            nt.links.new(c, sock)
    return m.outputs[2]


def body_material(name, col_set, banded=True):
    """Pattern u = angle round the body (0.25 the back, 0.75 the belly), v = arc length from the tail tip: sand brown
    back with darker cross bands, a pale belly with transverse scutes, a dark throat band under the hood."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nt = mat.node_tree
    for node in list(nt.nodes):
        if node.type not in ("OUTPUT_MATERIAL", "BSDF_PRINCIPLED"):
            nt.nodes.remove(node)
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Roughness"].default_value = 0.45
    uv = nt.nodes.new("ShaderNodeUVMap")
    uv.uv_map = "pattern"
    sep = nt.nodes.new("ShaderNodeSeparateXYZ")
    nt.links.new(uv.outputs["UV"], sep.inputs[0])
    u, v = sep.outputs["X"], sep.outputs["Y"]
    up = _math(nt, "SINE", _math(nt, "MULTIPLY", u, 2 * math.pi))  # +1 back, -1 belly
    backness = _math(nt, "MULTIPLY_ADD", up, 0.5)  # 1 back, 0 belly
    backness.node.inputs[2].default_value = 0.5
    tone = nt.nodes.new("ShaderNodeValToRGB")
    els = tone.color_ramp.elements
    els[0].position, els[0].color = 0.22, (*col_set["belly"], 1)
    els[1].position, els[1].color = 0.62, (*col_set["back"], 1)
    els.new(0.40).color = (*col_set["flank"], 1)
    nt.links.new(backness, tone.inputs["Fac"])
    col = tone.outputs["Color"]
    belly = _math(nt, "LESS_THAN", backness, 0.3)
    if banded:
        # Cross bands on the back and flanks, period 0.21, soft edged.
        ph = _math(nt, "FRACT", _math(nt, "MULTIPLY", v, 1 / 0.21))
        tri = _math(nt, "ABSOLUTE", _math(nt, "SUBTRACT", ph, 0.5))
        bandf = _math(nt, "MULTIPLY", _math(nt, "SUBTRACT", 1.0, _smooth(nt, tri, 0.12, 0.22)), _math(nt, "SUBTRACT", 1.0, belly))
        col = _mix(nt, _math(nt, "MULTIPLY", bandf, 0.6), col, col_set["band"])
        # Transverse belly scutes.
        sc = _math(nt, "FRACT", _math(nt, "MULTIPLY", v, 1 / 0.028))
        seam = _math(nt, "MULTIPLY", _math(nt, "GREATER_THAN", sc, 0.82), belly)
        col = _mix(nt, _math(nt, "MULTIPLY", seam, 0.35), col, col_set["band"])
        # The dark throat band: under the neck, behind the hood's broadest part.
        d = _math(nt, "SUBTRACT", LB, v)
        win = _math(nt, "MULTIPLY", _smooth(nt, d, 0.10, 0.15), _math(nt, "SUBTRACT", 1.0, _smooth(nt, d, 0.30, 0.36)))
        under = _math(nt, "LESS_THAN", backness, 0.42)
        col = _mix(nt, _math(nt, "MULTIPLY", win, under), col, col_set["throat"])
        if "hood" in col_set:  # the back of the hood in its own colour, a pale ring (the eye-spot) on it
            top = _smooth(nt, backness, 0.45, 0.65)
            col = _mix(nt, _math(nt, "MULTIPLY", win, top), col, col_set["hood"])
            for side in (-1, 1):
                du = _math(nt, "MULTIPLY", _math(nt, "SUBTRACT", u, 0.25 + side * 0.075), 1.4)
                dd = _math(nt, "SUBTRACT", d, 0.21)
                dist = _math(nt, "SQRT", _math(nt, "ADD", _math(nt, "MULTIPLY", du, du), _math(nt, "MULTIPLY", dd, dd)))
                ring = _math(nt, "MULTIPLY", _math(nt, "SUBTRACT", 1.0, _smooth(nt, dist, 0.045, 0.055)), _smooth(nt, dist, 0.022, 0.03))
                col = _mix(nt, ring, col, col_set["spot"])
    # Scale granulation.
    coord = nt.nodes.new("ShaderNodeTexCoord")
    vor = nt.nodes.new("ShaderNodeTexVoronoi")
    vor.feature = "DISTANCE_TO_EDGE"
    vor.inputs["Scale"].default_value = 70.0
    nt.links.new(coord.outputs["Object"], vor.inputs["Vector"])
    edge = _smooth(nt, vor.outputs["Distance"], 0.0, 0.08)
    gran = _math(nt, "MULTIPLY_ADD", edge, 0.3)
    gran.node.inputs[2].default_value = 0.72
    m = nt.nodes.new("ShaderNodeVectorMath")
    m.operation = "MULTIPLY"
    nt.links.new(col, m.inputs[0])
    comb = nt.nodes.new("ShaderNodeCombineXYZ")
    for i in range(3):
        nt.links.new(gran, comb.inputs[i])
    nt.links.new(comb.outputs[0], m.inputs[1])
    out = m.outputs[0]
    if "edge" in col_set:  # scale edges in their own colour
        rim = _math(nt, "SUBTRACT", 1.0, _smooth(nt, vor.outputs["Distance"], 0.0, 0.025))
        out = _mix(nt, _math(nt, "MULTIPLY", rim, 0.5), out, col_set["edge"])
    nt.links.new(out, bsdf.inputs["Base Color"])
    return mat


def _smooth(nt, x, e0, e1):
    """smoothstep(e0, e1, x) as nodes."""
    mr = nt.nodes.new("ShaderNodeMapRange")
    mr.interpolation_type = "SMOOTHSTEP"
    nt.links.new(x, mr.inputs["Value"])
    mr.inputs["From Min"].default_value = e0
    mr.inputs["From Max"].default_value = e1
    return mr.outputs["Result"]


def materials(col_set=COL):
    specs = {
        "eye": ("solid", "eye"),
        "mouth": ("solid", "mouth"),
        "fang": ("solid", "fang"),
        "tongue": ("solid", "tongue"),
    }
    M = {k: common.make_material("cobra_" + k, spec, col_set) for k, spec in specs.items()}
    M["body"] = body_material("cobra_body", col_set)
    M["head"] = body_material("cobra_head", col_set, banded=False)
    return M


# ---------------------------------------------------------------- parts


def build_body(b, M):
    n = 18
    ss = [i * 0.018 for i in range(int(LB / 0.018) + 1)] + [LB, LB + 0.035]
    rings = []
    for s in ss:
        rx = radius(min(s, LB))
        rings.append([ring_point(2 * math.pi * j / n, rx) + V((0, s - LB, 0)) for j in range(n)])
    verts = [p for r in rings for p in r]
    faces, fuv = [], []
    for i in range(len(rings) - 1):
        for j in range(n):
            j2 = (j + 1) % n
            faces.append([i * n + j, i * n + j2, (i + 1) * n + j2, (i + 1) * n + j])
            fuv.append([(j / n, ss[i]), ((j + 1) / n, ss[i]), ((j + 1) / n, ss[i + 1]), (j / n, ss[i + 1])])
    tip = len(verts)
    verts.append(V((0, -LB - 0.008, 0)))
    for j in range(n):
        faces.append([tip, (j + 1) % n, j])
        fuv.append([(0.5, 0), ((j + 1) / n, 0), (j / n, 0)])

    def weights(co):
        s = co.y + LB
        if s >= LB:
            return {"head": 1.0}
        k = min(max(s / DS, 0.0), N - 1e-6)
        k0 = int(k)
        t = k - k0
        return {"s%02d" % k0: 1 - t, "s%02d" % (k0 + 1): t}

    b.add((verts, faces, fuv), M["body"], weights)


def head_shape(th, a):
    sn = math.sin(th)
    k = 1.0 if sn >= 0 else 0.55  # flat underside
    return k * (1.0 - 0.12 * abs(math.cos(th)) * max(sn, 0))  # slightly flat crown


def build_head(b, M):
    up = V((0, 0, 1))
    # Upper head: broad behind the eyes (the jaw muscles), a rounded snout. Rings along +Y, centre a little up.
    keys = [((0, -0.03, 0.004), 0.050, 0.046), ((0, 0.02, 0.008), 0.066, 0.050), ((0, 0.08, 0.008), 0.060, 0.044),
            ((0, 0.14, 0.004), 0.045, 0.034), ((0, 0.178, 0.0), 0.026, 0.022), ((0, 0.192, -0.002), 0.008, 0.008)]
    b.add(tube(keys, sub=3, n=16, ref=up, shape=head_shape), M["head"], "head", "head")
    # Mouth roof (pink), just under the flat underside.
    roof = [((0, 0.0, -0.019), 0.050, 0.004), ((0, 0.09, -0.019), 0.050, 0.004), ((0, 0.17, -0.016), 0.022, 0.004)]
    b.add(tube(roof, sub=2, n=10, ref=up), M["mouth"], "head", "head")
    for s in (1, -1):
        b.add(ellipsoid((0.052 * s, 0.10, 0.024), (0.016, 0.018, 0.015), n=10, rings=6), M["eye"], "head", "head")
        # Fangs at the front of the upper jaw, pointing down and a little back.
        base = V((0.020 * s, 0.150, -0.020))
        b.add(common.cone(base, V((0, -0.25, -1)), 0.034, 0.0055, n=6), M["fang"], "head", "head")
    # Lower jaw: two thin rami meeting at the chin, a pink floor between.
    jaw = [((0, -0.01, -0.030), 0.052, 0.014), ((0, 0.07, -0.030), 0.046, 0.013), ((0, 0.15, -0.028), 0.030, 0.011),
           ((0, 0.18, -0.027), 0.014, 0.008)]
    b.add(tube(jaw, sub=3, n=12, ref=up, shape=lambda th, a: 1.0 if math.sin(th) < 0 else 0.45), M["head"], "jaw", "head")
    floor = [((0, 0.0, -0.022), 0.044, 0.003), ((0, 0.15, -0.022), 0.024, 0.003)]
    b.add(tube(floor, sub=2, n=8, ref=up), M["mouth"], "jaw", "head")
    # Forked tongue, inside the mouth at rest (slides forward through the lip notch).
    tongue = [((0, 0.02, -0.024), 0.006, 0.003), ((0, 0.11, -0.024), 0.004, 0.0025)]
    b.add(tube(tongue, sub=2, n=6, ref=up), M["tongue"], "tongue", "head")
    for s in (1, -1):
        fork = [((0, 0.11, -0.024), 0.003, 0.002), ((0.012 * s, 0.15, -0.026), 0.0015, 0.0015)]
        b.add(tube(fork, sub=2, n=5, ref=up), M["tongue"], "tongue", "head")


# ---------------------------------------------------------------- rig


def build_rig(coll):
    arm = bpy.data.armatures.new("cobra_rig")
    rig = bpy.data.objects.new("cobra_rig", arm)
    coll.objects.link(rig)
    arm.display_type = "STICK"
    rig.show_in_front = True
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    eb = arm.edit_bones

    def bone(name, head):
        bn = eb.new(name)
        bn.head = V(head)
        bn.tail = V(head) + V((0, 0.04, 0))
        bn.align_roll(V((0, 0, 1)))  # local x = +X, y = +Y, z = +Z: rest frames are plain translations

    for k in range(N + 1):
        bone("s%02d" % k, (0, joint_s(k) - LB, 0))
    bone("head", (0, 0, 0))
    bone("jaw", JAW_HINGE)
    bone("tongue", (0, 0.02, -0.024))
    bpy.ops.object.mode_set(mode="OBJECT")
    for pb in rig.pose.bones:
        pb.rotation_mode = "QUATERNION"
    return rig


# ---------------------------------------------------------------- poses

# Pose parameters (degrees, tiles of the model): wave amplitude and phase of the slither, lean of the raised column,
# neck bend forward, head pitch (+ up), sway of the raised front, hood 0..1, jaw opening, tongue 0..1 out, collapse
# (0 flat .. 1 raised), roll of the front and of the rear (die), writhe (die), coil 0..1 (blend to the idle coil),
# shift (anchor forward), breath (idle swell), yaw of the whole raised front.
REST = dict(wave=50.0, phase=0.0, lean=84.0, neck=86.0, head=0.0, sway=0.0, hood=0.5, jaw=3.0, tongue=0.0, collapse=1.0,
            rollf=0.0, rollr=0.0, writhe=0.0, wphase=0.0, coil=0.0, shift=0.0, breath=0.0, fyaw=0.0, lift=0.0)


def ground_slope(s):
    """Elevation (radians) that keeps the flat belly on the floor while the body thickens from the tail."""
    h = 0.004
    return math.atan((radius(s + h) - radius(max(s - h, 0))) * BELLY / (2 * h))


def coil_angles():
    """Yaw and elevation per joint of the coiled resting pose: a flat spiral from the tail inside, the front climbing
    over the outer coil, the head resting on top facing +Y."""
    sc = 2.12  # end of the spiral
    r0, r1 = 0.11, 0.40
    yaw, theta = [], 0.0
    prev = 0.0
    for k in range(N + 1):
        s = joint_s(k)
        if s <= sc:
            r = r0 + (r1 - r0) * s / sc
            kappa = 1 / r
        else:
            kappa = 1 / r1 + (1 / 0.24 - 1 / r1) * smoothstep(sc, sc + 0.25, s) - (1 / 0.24 + 1.0) * smoothstep(LB - 0.32, LB - 0.1, s)
        if k:
            theta += 0.5 * (prev + kappa) * DS
        prev = kappa
        yaw.append(theta)
    off = yaw[-1]
    yaw = [-(y - off) for y in yaw]  # turning to the left (-X) as it goes forward; the head at yaw 0
    el = []
    for k in range(N + 1):
        s = joint_s(k)
        e = ground_slope(s) if s < sc else 0.0
        e += math.radians(34) * smoothstep(sc + 0.02, sc + 0.12, s) * (1 - smoothstep(sc + 0.24, sc + 0.36, s))
        e -= math.radians(10) * smoothstep(LB - 0.2, LB, s)
        el.append(e)
    return yaw, el


COIL = coil_angles()


def angles(p):
    """Yaw, elevation, roll (radians) and hood factor per joint."""
    yaw, el, roll, hood = [], [], [], []
    sa = joint_s(A)
    for k in range(N + 1):
        s = joint_s(k)
        u = s / LB
        rs = smoothstep(sa - 0.14, sa + 0.12, s)  # 0 on the floor, 1 in the raised front
        ns = smoothstep(LB - 0.36, LB - 0.08, s)  # the neck bends forward
        hs = smoothstep(LB - 0.08, LB, s)
        amp = math.radians(p["wave"]) * (1 - smoothstep(sa - 0.6, sa - 0.05, s)) * (0.6 + 0.4 * smoothstep(0, 0.8, s))
        y = amp * math.sin(2 * math.pi * (1.55 * u + p["phase"]))
        y += math.radians(p["sway"]) * rs * smoothstep(sa, LB, s)
        y += math.radians(p["fyaw"]) * smoothstep(sa - 0.5, LB - 0.1, s)
        y += math.radians(p["writhe"]) * math.sin(2 * math.pi * (2.3 * u - p["wphase"])) * (0.4 + 0.6 * u)
        e = ground_slope(s) * (1 - rs)
        e += p["collapse"] * (math.radians(p["lean"]) * rs - math.radians(p["neck"]) * ns + math.radians(p["head"]) * hs)
        c = p["coil"]
        if c > 0:
            y = (1 - c) * y + c * COIL[0][k]
            e = (1 - c) * e + c * COIL[1][k]
        yaw.append(y)
        el.append(e)
        roll.append(math.radians(p["rollf"]) * smoothstep(sa - 0.5, sa + 0.3, s) + math.radians(p["rollr"]) * (1 - smoothstep(0.6, sa, s)))
        hood.append(p["hood"] * hood_profile(s))
    return yaw, el, roll, hood


def joint_frames(p):
    """Per joint: (position, rotation 3x3 with columns side / tangent / up). Integrated from the tail, then placed."""
    yaw, el, roll, hood = angles(p)

    def direction(y, e):
        return V((math.sin(y) * math.cos(e), math.cos(y) * math.cos(e), math.sin(e)))

    pts = [V((0, 0, 0))]
    for k in range(N):
        d = direction(0.5 * (yaw[k] + yaw[k + 1]), 0.5 * (el[k] + el[k + 1]))
        pts.append(pts[-1] + d * DS)
    rots = []
    for k in range(N + 1):
        t = direction(yaw[k], el[k])
        side = V((math.cos(yaw[k]), -math.sin(yaw[k]), 0))
        up = side.cross(t)
        r = roll[k]
        side, up = side * math.cos(r) + up * math.sin(r), up * math.cos(r) - side * math.sin(r)
        rots.append(Matrix((side, t, up)).transposed())
    # Placement: the base joint of the raised front at (0, BASE_Y), blended towards the coil centre on the origin.
    anchor = V((0, BASE_Y + p["shift"], 0)) - pts[A]
    c = p["coil"]
    if c > 0:
        cen = sum(pts, V()) / len(pts)
        anchor = anchor * (1 - c) + (V((0.0, -0.05, 0)) - cen) * c
    anchor.z = 0
    pts = [q + anchor for q in pts]
    return pts, rots, hood


def bone_deforms(p):
    """Armature-space deformation matrices (pose @ rest^-1) per bone, before the floor fix."""
    pts, rots, hood = joint_frames(p)
    D = {}
    for k in range(N + 1):
        h = hood[k]
        br = 1 + p["breath"]
        S = Matrix.Diagonal((br * (1 + 3.4 * h), 1, br * (1 - 0.45 * h), 1))
        M = rots[k].to_4x4() @ S
        M.translation = pts[k]
        D["s%02d" % k] = M @ Matrix.Translation(V((0, joint_s(k) - LB, 0))).inverted()
    head = rots[N].to_4x4()
    head.translation = pts[N]
    D["head"] = head
    D["jaw"] = head @ Matrix.Translation(JAW_HINGE) @ Matrix.Rotation(-math.radians(p["jaw"]), 4, "X") @ Matrix.Translation(-JAW_HINGE)
    D["tongue"] = head @ Matrix.Translation(V((0, 0.085 * p["tongue"], -0.004 * p["tongue"])))
    return D


class Skin:
    """numpy linear blend skinning of every vertex, for the floor contact and the measurements."""

    def __init__(self, b):
        import numpy as np

        self.np = np
        self.bones = sorted({n for w in b.weights for n in w})
        col = {n: j for j, n in enumerate(self.bones)}
        self.P = np.array([[*v, 1.0] for v in b.verts])
        self.W = np.zeros((len(b.verts), len(self.bones)))
        for r, w in enumerate(b.weights):
            tot = sum(w.values())
            for n, x in w.items():
                self.W[r, col[n]] = x / tot

    def points(self, D):
        np = self.np
        out = np.zeros((len(self.P), 3))
        for j, n in enumerate(self.bones):
            m = np.array(D[n])[:3]
            out += self.W[:, j : j + 1] * (self.P @ m.T)
        return out


def key_frame(rig, skin, p, frame_no):
    D = bone_deforms(p)
    lift = -float(skin.points(D)[:, 2].min()) + p["lift"]
    S = Matrix.Translation(V((0, 0, lift)))
    D = {n: S @ m for n, m in D.items()}
    for bn in rig.data.bones:
        ml = bn.matrix_local
        pb = rig.pose.bones[bn.name]
        pb.matrix_basis = ml.inverted() @ D[bn.name] @ ml
        pb.keyframe_insert("rotation_quaternion", frame=frame_no)
        pb.keyframe_insert("location", frame=frame_no)
        pb.keyframe_insert("scale", frame=frame_no)
    return D


def move_params(t):
    """Slither at phase t in [0, 1): waves run back along the body on the floor, the raised front sways a little and
    flicks the tongue."""
    p = dict(REST)
    w = 2 * math.pi * t
    p.update(phase=t, sway=3.0 * math.sin(w), head=2.0 * math.sin(2 * w), neck=86.0 + 2.0 * math.sin(w + 1.0),
             tongue=max(0.0, math.sin(2 * math.pi * (t - 0.55) / 0.3)) if 0.55 <= t <= 0.85 else 0.0)
    return p


READY = move_params(0.0)


def idle_params(t):
    p = dict(READY)
    w = 2 * math.pi * t
    flick = max(0.0, math.sin(2 * math.pi * (t - 0.5) / 0.22)) if 0.5 <= t <= 0.72 else 0.0
    p.update(coil=1.0, hood=0.0, jaw=0.0, breath=0.035 * (0.5 - 0.5 * math.cos(w)), tongue=flick)
    return p


def interpolate(keys, frame_no, base=None, linear=()):
    full = []
    for f, over in keys:
        p = dict(base or READY)
        p.update(over)
        full.append((f, p))
    for (f0, p0), (f1, p1) in zip(full, full[1:]):
        if f0 <= frame_no <= f1:
            t = smoothstep(f0, f1, frame_no)
            lt = (frame_no - f0) / (f1 - f0)
            return {k: p0[k] + (p1[k] - p0[k]) * (lt if k in linear else t) for k in p0}
    return full[-1][1]


# Rears back with the hood spread, strikes forward and down with the mouth wide open, snaps shut, recoils.
ATTACK = [
    (0, {}),
    (5, dict(lean=94, neck=92, head=14, hood=1.0, jaw=10, shift=-0.06)),
    (8, dict(lean=96, neck=88, head=16, hood=1.0, jaw=30, shift=-0.08)),
    (11, dict(lean=34, neck=8, head=-22, hood=1.0, jaw=70, shift=0.24)),
    (13, dict(lean=30, neck=4, head=-24, hood=1.0, jaw=0, shift=0.28)),
    (16, dict(lean=50, neck=36, head=-10, hood=0.9, jaw=0, shift=0.14)),
    (19, dict(lean=80, neck=82, head=2, hood=0.7, jaw=4, shift=0.03)),
    (22, {}),
]

# Recoils, writhes, the raised front falls, rolls belly up and lies still.
DIE = [
    (0, {}),
    (3, dict(lean=96, neck=70, head=30, hood=1.0, jaw=60, wave=10, writhe=10, wphase=0.0)),
    (6, dict(lean=70, neck=60, head=10, hood=0.8, jaw=40, wave=8, writhe=24, wphase=0.25, fyaw=25)),
    (9, dict(lean=55, neck=40, head=-10, hood=0.6, jaw=55, wave=6, writhe=-26, wphase=0.5, fyaw=-30, rollf=30)),
    (13, dict(lean=30, neck=20, head=-5, hood=0.4, jaw=45, wave=4, writhe=24, wphase=0.75, fyaw=50, rollf=70, rollr=20, collapse=0.6)),
    (17, dict(lean=10, neck=0, head=0, hood=0.2, jaw=35, wave=14, writhe=-18, wphase=1.0, fyaw=85, rollf=115, rollr=40, collapse=0.25)),
    (21, dict(lean=0, neck=0, head=0, hood=0.1, jaw=30, wave=22, writhe=14, wphase=1.2, fyaw=100, rollf=125, rollr=50, collapse=0.0)),
    (25, dict(lean=0, neck=0, head=0, hood=0.1, jaw=32, wave=24, writhe=12, wphase=1.3, fyaw=104, rollf=128, rollr=52, collapse=0.0)),
    (29, dict(lean=0, neck=0, head=0, hood=0.1, jaw=32, wave=24, writhe=12, wphase=1.3, fyaw=104, rollf=128, rollr=52, collapse=0.0)),
]

# From the coil: the front rises out of the coil, the body unwinds behind it, the hood spreads; ends on move frame 0.
RISE = [
    (0, dict(coil=1.0, hood=0.0, jaw=0.0, tongue=0.0)),
    (4, dict(coil=0.9, hood=0.0, jaw=0.0, head=20, tongue=0.0)),
    (12, dict(coil=0.35, hood=0.3, jaw=4, lean=92, head=10, tongue=0.0)),
    (17, dict(coil=0.06, hood=0.95, jaw=12, lean=92, neck=90, head=8, tongue=0.0)),
    (20, dict(coil=0.0, hood=0.8, jaw=6, lean=86, tongue=0.0)),
    (23, {}),
]

SPIT_RELEASE = 7
# The hood flares, the head draws back and up, jerks forward with the mouth open (the venom leaves at SPIT_RELEASE),
# settles back to move frame 0.
SPIT = [
    (0, {}),
    (4, dict(hood=1.0, lean=92, neck=92, head=12, jaw=6, shift=-0.04)),
    (SPIT_RELEASE, dict(hood=1.0, lean=78, neck=82, head=-2, jaw=40, shift=0.04)),
    (9, dict(hood=1.0, lean=80, neck=84, head=-2, jaw=34, shift=0.03)),
    (13, dict(hood=0.85, lean=84, neck=86, head=2, jaw=8, shift=0.0)),
    (17, {}),
]

MOUTH_TRACK = {}  # frame -> mouth front (armature space) in the spit clip


def clip_params(name, f, frames):
    if name == "cobra_move":
        return move_params(f / frames)
    if name == "cobra_idle":
        return idle_params(f / frames)
    keys = {"cobra_attack": ATTACK, "cobra_die": DIE, "cobra_rise": RISE, "cobra_spit": SPIT}[name]
    if name == "cobra_rise":
        return interpolate(keys, f, linear=())
    return interpolate(keys, f)


def make_actions(rig, skin):
    rig.animation_data_create()
    acts = {}
    for name, suffix, frames in CLIPS:
        act = bpy.data.actions.get(name)
        if act:
            bpy.data.actions.remove(act)
        act = bpy.data.actions.new(name)
        act.use_fake_user = True
        rig.animation_data.action = act
        last = frames if name in LOOPS else frames - 1  # loops key frame N = frame 0
        for f in range(last + 1):
            D = key_frame(rig, skin, clip_params(name, f, frames), f)
            if name == "cobra_spit":
                MOUTH_TRACK[f] = D["head"] @ MOUTH
        acts[name] = act
    return acts


def measure(skin):
    """Frame 0 extents of every clip, the reference clip's half sizes and the spit clip's mouth path."""
    np = skin.np
    ref = None
    for name, _, frames in CLIPS:
        D = bone_deforms(clip_params(name, 0, frames))
        P = skin.points(D)
        P[:, 2] -= P[:, 2].min()
        lo, hi = P.min(axis=0), P.max(axis=0)
        if ref is None:
            ref = (lo, hi)
        print("MEASURE {}: height {:.3f} length (y) {:.3f} [{:.3f}..{:.3f}] width (x) {:.3f} [{:.3f}..{:.3f}]".format(
            name, hi[2], hi[1] - lo[1], lo[1], hi[1], hi[0] - lo[0], lo[0], hi[0]))
    lo, hi = ref
    centre = (lo + hi) / 2
    print("MEASURE reference: halfX {:.3f} halfZ (y) {:.3f}, centre x {:.3f} y {:.3f}".format((hi[0] - lo[0]) / 2, (hi[1] - lo[1]) / 2, centre[0], centre[1]))
    height = hi[2]
    for f in sorted(MOUTH_TRACK):
        m = MOUTH_TRACK[f]
        print("MOUTH spit f{:02d}: forward {:.3f} from the centre (y {:.3f}), height {:.3f} = {:.3f} of the reference height{}".format(
            f, m.y - centre[1], m.y, m.z, m.z / height, "  <- release" if f == SPIT_RELEASE else ""))
    _ = np


# ---------------------------------------------------------------- entry points

SKIN = None


def build(bake=True, tex_path=None, giant_path=None):
    global SKIN
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    M = materials()
    rig = build_rig(coll)
    b = Builder()
    build_body(b, M)
    build_head(b, M)
    obj = common.finish_mesh(b, coll, "cobra_new")
    common.uv_unwrap(obj, b.tags, {"head": (V((0, 0.08, 0)), 2.2)})
    if bake:
        tmp = bpy.app.tempdir or "/tmp"
        tex = common.bake_texture(obj, tex_path or os.path.join(tmp, "cobra_preview.png"), TEX_SIZE, "cobra", ao_distance=0.1)
        materials(COL_GIANT)
        giant = common.bake_texture(obj, giant_path or os.path.join(tmp, "cobra_giant_preview.png"), TEX_SIZE, "cobra_giant", ao_distance=0.1)
        if os.environ.get("COBRA_TEX") == "apep":
            materials(COL_APEP)
            tex = common.bake_texture(obj, os.path.join(tmp, "cobra_apep_preview.png"), TEX_SIZE, "cobra_apep", ao_distance=0.1)
        common.use_baked_material(obj, giant if os.environ.get("COBRA_TEX") == "giant" else tex)
    common.rig_object(obj, rig)
    SKIN = Skin(b)
    acts = make_actions(rig, SKIN)
    rig.animation_data.action = acts["cobra_move"]
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 0, CLIPS[0][2] - 1
    scene.frame_set(0)
    print("cobra: {} verts, {} tris".format(len(obj.data.vertices), common.tri_count(obj)))
    return obj, rig


def export(models_dir=None):
    common.export_files(bpy.data.objects["cobra_new"], bpy.data.objects["cobra_rig"], "cobra", CLIPS, "monsters", models_dir)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    if "--boss-texture" in args:
        obj, _ = build(bake=False)
        materials(COL_APEP)
        common.bake_texture(obj, os.path.join(REPO, "textures", "monsters", "cobra_apep.png"), TEX_SIZE, "cobra_apep", ao_distance=0.1)
    elif "--export" in args:
        tex_dir = os.path.join(REPO, "textures", "monsters")
        build(tex_path=os.path.join(tex_dir, "cobra.png"), giant_path=os.path.join(tex_dir, "cobra_giant.png"))
        export()
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "cobra.blend"))
    else:
        build(bake=False)
    if "--measure" in args:
        measure(SKIN)
