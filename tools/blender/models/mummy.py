"""Procedural mummy: a linen-wrapped corpse that lies in an open coffin until the player comes near, climbs out and
shambles after them. Mesh, rig, baked texture and six .md3 clips.

    MCP:  p = ".../tools/blender/models/mummy.py"; g = {"__file__": p, "__name__": "mummy"}
          exec(open(p).read(), g); g["build"](bake=False)      # then g["export"]()
    CLI:  blender -b --python tools/blender/models/mummy.py -- [--export] [--export-climb] [--coffin-check out_dir]

Built like anubis.py: Z up, facing +Y (game: towards the camera, drawn at rotA 180), right side +X (game -X).
Clips (MONSTER_CLIPS + the entombed extras, ENTOMBED_CLIPS in src/entities/character_model.h):
  mummy.md3       walk loop (the reference: frame 0's bounding box is centred on x = y = 0, lowest point on z = 0),
  mummy_att.md3   attack loop, one slow two-handed smash,
  mummy_die.md3   collapse into a heap of linen (plays once),
  mummy_idle.md3  dormant: on its back, head towards +X (game left), arms crossed, centred on x = y = 0, the back
                  IDLE_BACK world units above the floor (the coffin's inner floor is 0.48),
  mummy_rise.md3  climbs out (plays once): sits up, swings its legs over the front rim, drops onto its feet, two
                  stiff steps, raises the arms; the last frame is walk frame 0. The engine draws idle and rise
                  TOMB_DEPTH world units towards the back wall and slides that to 0 over rise t = SLIDE (smoothstep);
                  the rise clip is authored in the coffin's frame (the slide undone), so the feet do not skate.
  mummy_climb.md3 climbing a ladder (loops), back to the camera (drawn at rotA + 180): the engine sets the frame from
                  the height, one cycle per CLIMB["rungs"] rungs; the gripping hand and foot slide down at that rate
                  (see CLIMB). The build prints climbRise / climbGrip for KINDS.
World units: the engine scales walk frame 0's largest dimension H (metres) to SCALE world units (a tile is 40), so
1 world unit = H / SCALE metres (U below, set once the walk clip is built). The coffin (decor.py `coffin`, tile units)
is laid out around the dormant body centre: COFFIN in tile units, see coffin_box().
Loose bandage strips are short two-bone chains run by a small verlet simulation (gravity, lag, floor and coffin).
"""

import importlib
import math
import os
import sys

import bpy
from mathutils import Euler
from mathutils import Vector as V

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import common  # noqa: E402

importlib.reload(common)
from common import REPO, Builder, chain_weights, ellipsoid, smoothstep, tube  # noqa: E402

COLL = "mummy_new"
CLIPS = [("mummy_walk", "", 24), ("mummy_attack", "_att", 22), ("mummy_die", "_die", 30), ("mummy_idle", "_idle", 32), ("mummy_rise", "_rise", 26),
         ("mummy_climb", "_climb", 24)]
FRAMES = {name: n for name, _, n in CLIPS}
TEX_SIZE = 1024
REVIEW_VIEW = {"target": (0, 0.1, 0.9), "ortho": 2.4, "res": (360, 420)}

SCALE = 18.0  # KINDS scale (monster_kinds.cpp): world units of walk frame 0's largest dimension
TILE = 40.0  # world units per tile
TOMB_DEPTH = 14.4  # world units: the dormant body is drawn this far towards the back wall
SLIDE = (0.3, 0.75)  # rise t over which the engine slides TOMB_DEPTH to 0 (smoothstep)
IDLE_BACK = 0.6  # world units from the floor to the dormant mummy's back
# Coffin interior in tile units, relative to the dormant body centre (decor y = -0.14): x half length, y half depth,
# wall thickness, inner floor and rim height (decor.py build_coffin uses the same numbers around y = -0.14).
COFFIN = {"half_x": 0.30, "half_y": 0.08, "wall": 0.016, "floor": 0.012, "rim": 0.08}
U = {"m": 0.1}  # metres per world unit, set from walk frame 0 (H / SCALE)

COL = {
    "linen": (0.50, 0.43, 0.30),
    "linen_hi": (0.66, 0.59, 0.44),
    "linen_dark": (0.24, 0.19, 0.12),
    "stain": (0.28, 0.20, 0.10),
    "pitch": (0.035, 0.025, 0.018),
    "skin": (0.11, 0.07, 0.04),
    "eye": (0.85, 0.62, 0.18),
}


# ---------------------------------------------------------------- materials


def wrap_material(name, period, turn=1.0, base="linen", grime=1.0):
    """Spiral bandage: stripes along the pattern uv (v / period + u * turn), dark seams, grime and stains on top."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nt = mat.node_tree
    for node in list(nt.nodes):
        if node.type not in ("OUTPUT_MATERIAL", "BSDF_PRINCIPLED"):
            nt.nodes.remove(node)
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Roughness"].default_value = 0.85
    new, link = nt.nodes.new, nt.links.new
    uv = new("ShaderNodeUVMap")
    uv.uv_map = "pattern"
    sep = new("ShaderNodeSeparateXYZ")
    link(uv.outputs["UV"], sep.inputs[0])
    mv = new("ShaderNodeMath")
    mv.operation = "MULTIPLY"
    mv.inputs[1].default_value = 1.0 / period
    link(sep.outputs["Y"], mv.inputs[0])
    mu = new("ShaderNodeMath")
    mu.operation = "MULTIPLY_ADD"
    mu.inputs[1].default_value = turn
    link(sep.outputs["X"], mu.inputs[0])
    link(mv.outputs[0], mu.inputs[2])
    fr = new("ShaderNodeMath")
    fr.operation = "FRACT"
    link(mu.outputs[0], fr.inputs[0])
    ramp = new("ShaderNodeValToRGB")
    ramp.color_ramp.interpolation = "LINEAR"
    els = ramp.color_ramp.elements
    els[0].position, els[0].color = 0.0, (*COL["linen_dark"], 1)
    els[1].position, els[1].color = 1.0, (*COL["linen_dark"], 1)
    for pos, c in ((0.08, base), (0.45, "linen_hi"), (0.85, base), (0.95, "linen_dark")):
        els.new(pos).color = (*COL[c], 1)
    link(fr.outputs[0], ramp.inputs["Fac"])
    coord = new("ShaderNodeTexCoord")
    col = ramp.outputs["Color"]
    for scale, lo, hi, tint in ((14.0, 0.55, 1.0, None), (3.0, 0.3, 1.0, "stain")):
        noise = new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = scale
        noise.inputs["Detail"].default_value = 4.0
        link(coord.outputs["Object"], noise.inputs["Vector"])
        r = new("ShaderNodeMapRange")
        r.inputs["From Min"].default_value = 0.35
        r.inputs["From Max"].default_value = 0.65
        link(noise.outputs["Fac"], r.inputs["Value"])
        mix = new("ShaderNodeMix")
        mix.data_type = "RGBA"
        link(col, mix.inputs["A"])
        if tint:  # brown stains where the noise is low
            inv = new("ShaderNodeMath")
            inv.operation = "MULTIPLY_ADD"
            inv.inputs[1].default_value = -0.8 * grime
            inv.inputs[2].default_value = 0.8 * grime
            link(r.outputs["Result"], inv.inputs[0])
            link(inv.outputs[0], mix.inputs["Factor"])
            mix.blend_type = "MULTIPLY"
            mix.inputs["B"].default_value = (COL[tint][0] * 2.2, COL[tint][1] * 2.2, COL[tint][2] * 2.2, 1)
        else:  # fine grime
            r.inputs["To Min"].default_value = lo
            mix.blend_type = "MULTIPLY"
            mix.inputs["Factor"].default_value = grime
            link(r.outputs["Result"], mix.inputs["B"])
        col = mix.outputs["Result"]
    link(col, bsdf.inputs["Base Color"])
    return mat


def materials():
    M = {
        "wrap": wrap_material("mummy_wrap", 0.055, 1.0),
        "wrap_limb": wrap_material("mummy_wrap_limb", 0.04, -1.0),
        "wrap_head": wrap_material("mummy_wrap_head", 0.032, 1.0),
        "band": wrap_material("mummy_band", 0.5, 0.0, "linen", 1.0),
        "strip": wrap_material("mummy_strip", 0.09, 0.0, "linen_hi", 1.3),
    }
    for name in ("pitch", "skin", "eye"):
        M[name] = common.make_material("mummy_" + name, ("solid", name), COL)
    return M


# ---------------------------------------------------------------- skeleton


def side_name(bone, s):
    return bone + (".R" if s > 0 else ".L")


def joints(s):
    """Rest joint positions for side s (+1 right/+X, -1 left/-X)."""
    return {
        "hip": V((0.095 * s, 0.0, 0.92)),
        "knee": V((0.098 * s, 0.01, 0.50)),
        "ankle": V((0.098 * s, -0.01, 0.085)),
        "toe": V((0.10 * s, 0.15, 0.03)),
        "shoulder": V((0.19 * s, -0.01, 1.42)),
        "elbow": V((0.235 * s, -0.03, 1.14)),
        "wrist": V((0.255 * s, -0.01, 0.89)),
        "hand_end": V((0.26 * s, 0.0, 0.78)),
    }


PELVIS = V((0, 0, 0.93))  # pelvis bone head = hip centre, in root space
TORSO_CHAIN = ([(0, 0, 0.80), (0, 0, 1.04), (0, 0, 1.24), (0, 0, 1.46), (0, 0.01, 1.56), (0, 0.02, 1.82)], ["pelvis", "spine", "chest", "neck", "head"])
# Loose strips: (name, parent, anchor, segment lengths).
STRIPS = [
    ("strip_arm.R", "forearm.R", (0.272, -0.035, 1.10), (0.12, 0.12)),
    ("strip_arm.L", "forearm.L", (-0.262, -0.012, 0.97), (0.10, 0.10)),
    ("strip_waist", "pelvis", (-0.17, 0.03, 0.96), (0.14, 0.12)),
    ("strip_head", "head", (-0.083, 0.03, 1.64), (0.07, 0.065)),
]


def strip_bones(name):
    return name + ".0", name + ".1"


def build_rig(coll):
    arm = bpy.data.armatures.new("mummy_rig")
    rig = bpy.data.objects.new("mummy_rig", arm)
    coll.objects.link(rig)
    arm.display_type = "STICK"
    rig.show_in_front = True
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    eb = arm.edit_bones

    def bone(name, head, tail, parent=None, connect=False):
        b = eb.new(name)
        b.head, b.tail = V(head), V(tail)
        b.align_roll(V((1, 0, 0)).cross((b.tail - b.head).normalized()))
        if parent:
            b.parent = eb[parent]
            b.use_connect = connect

    bone("root", (0, 0, 0), (0, 0.3, 0))
    bone("pelvis", PELVIS, (0, 0, 1.04), "root")
    bone("spine", (0, 0, 1.04), (0, 0, 1.24), "pelvis", True)
    bone("chest", (0, 0, 1.24), (0, 0, 1.46), "spine", True)
    bone("neck", (0, 0, 1.46), (0, 0.01, 1.56), "chest", True)
    bone("head", (0, 0.01, 1.56), (0, 0.02, 1.80), "neck", True)
    for s in (1, -1):
        j = joints(s)
        n = lambda b: side_name(b, s)  # noqa: E731
        bone(n("thigh"), j["hip"], j["knee"], "pelvis")
        bone(n("shin"), j["knee"], j["ankle"], n("thigh"), True)
        bone(n("foot"), j["ankle"], j["toe"], n("shin"), True)
        bone(n("upperarm"), j["shoulder"], j["elbow"], "chest")
        bone(n("forearm"), j["elbow"], j["wrist"], n("upperarm"), True)
        bone(n("hand"), j["wrist"], j["hand_end"], n("forearm"), True)
    for name, parent, a, (l0, l1) in STRIPS:
        b0, b1 = strip_bones(name)
        a = V(a)
        bone(b0, a, a - V((0, 0, l0)), parent)
        bone(b1, a - V((0, 0, l0)), a - V((0, 0, l0 + l1)), b0, True)
    bpy.ops.object.mode_set(mode="OBJECT")
    for pb in rig.pose.bones:
        pb.rotation_mode = "XYZ"
    return rig


# ---------------------------------------------------------------- body parts


def band(c, axis, rx, ry, thick=0.007, n=14):
    """A bandage turn standing out of the wrapping: closed flat ring of radii rx, ry around axis through c."""
    c, axis = V(c), V(axis).normalized()
    u = V((1, 0, 0)) if abs(axis.x) < 0.9 else V((0, 1, 0))
    u = (u - u.dot(axis) * axis).normalized()
    w = axis.cross(u)
    pts = [c + u * (rx * math.cos(2 * math.pi * k / n)) + w * (ry * math.sin(2 * math.pi * k / n)) for k in range(n)]
    return tube([(p, thick, thick * 2.2) for p in pts], sub=1, n=4, ref=axis, closed=True)


def build_body(b, M):
    fwd = V((0, 1, 0))
    torso_w = chain_weights(*TORSO_CHAIN, 0.06)
    torso = [
        ((0, 0.0, 0.84), 0.10, 0.075),
        ((0, 0.0, 0.92), 0.145, 0.100),
        ((0, 0.0, 1.02), 0.128, 0.088),
        ((0, 0.003, 1.12), 0.124, 0.085),
        ((0, 0.008, 1.22), 0.142, 0.092),
        ((0, 0.01, 1.32), 0.163, 0.098),
        ((0, 0.0, 1.40), 0.178, 0.094),
        ((0, -0.005, 1.45), 0.138, 0.080),
        ((0, 0.0, 1.49), 0.064, 0.058),
        ((0, 0.01, 1.55), 0.052, 0.052),
        ((0, 0.015, 1.60), 0.050, 0.050),
    ]
    b.add(tube(torso, sub=2, n=18, ref=fwd, caps=(True, False), bulge=(0.03, 0)), M["wrap"], torso_w)
    for z, tilt, rx, ry in ((0.90, (0.12, 0.05, 1), 0.147, 0.103), (1.06, (-0.22, 0, 1), 0.130, 0.091), (1.20, (0.25, 0.08, 1), 0.143, 0.094),
                            (1.34, (-0.15, -0.1, 1), 0.168, 0.101)):
        b.add(band((0, 0.006, z), tilt, rx, ry), M["band"], torso_w)

    for s in (1, -1):
        j = joints(s)
        n = lambda bn: side_name(bn, s)  # noqa: E731
        x = lambda v: V((v[0] * s, v[1], v[2]))  # noqa: E731
        leg_w = chain_weights([x((0.098, 0, 1.07)), j["hip"], j["knee"], j["ankle"]], ["pelvis", n("thigh"), n("shin")], 0.06)
        leg = [
            (x((0.085, 0.0, 0.97)), 0.064, 0.068),
            (x((0.095, 0.004, 0.87)), 0.070, 0.073),
            (x((0.097, 0.01, 0.71)), 0.060, 0.063),
            (x((0.098, 0.012, 0.56)), 0.048, 0.050),
            (x((0.098, 0.008, 0.49)), 0.047, 0.049),
            (x((0.098, -0.004, 0.37)), 0.046, 0.047),
            (x((0.098, -0.01, 0.23)), 0.038, 0.040),
            (x((0.098, -0.012, 0.10)), 0.034, 0.036),
        ]
        b.add(tube(leg, sub=2, n=12, ref=fwd, caps=(False, True), bulge=(0, 0.01)), M["wrap_limb"], leg_w)
        for c, rx, ry, tilt in ((x((0.096, 0.008, 0.76)), 0.064, 0.067, x((0.2, 0, 1))), (x((0.098, -0.007, 0.30)), 0.045, 0.046, x((-0.25, 0.1, 1)))):
            b.add(band(c, tilt, rx, ry, 0.006, 12), M["band"], leg_w)

        def sole(th, a):
            return 0.78 if math.sin(th) < 0 else 1.0

        foot = [
            (x((0.098, -0.065, 0.047)), 0.031, 0.037),
            (x((0.098, -0.035, 0.057)), 0.039, 0.053),
            (x((0.098, 0.03, 0.048)), 0.044, 0.040),
            (x((0.10, 0.11, 0.035)), 0.045, 0.029),
            (x((0.10, 0.16, 0.027)), 0.035, 0.020),
        ]
        b.add(tube(foot, sub=2, n=10, ref=V((0, 0, 1)), shape=sole, bulge=(0.01, 0.012)), M["wrap_limb"],
              chain_weights([x((0.098, -0.01, 0.2)), j["ankle"], j["toe"]], [n("shin"), n("foot")], 0.03))

        arm_w = chain_weights([x((0.12, 0, 1.44)), j["shoulder"], j["elbow"], j["wrist"], j["hand_end"]], ["chest", n("upperarm"), n("forearm"), n("hand")], 0.05)
        arm = [
            (x((0.135, -0.005, 1.435)), 0.050, 0.052),
            (x((0.19, -0.01, 1.42)), 0.057, 0.057),
            (x((0.21, -0.02, 1.32)), 0.046, 0.049),
            (x((0.225, -0.028, 1.22)), 0.041, 0.043),
            (x((0.235, -0.03, 1.14)), 0.036, 0.039),
            (x((0.243, -0.024, 1.06)), 0.037, 0.038),
            (x((0.25, -0.015, 0.97)), 0.031, 0.030),
            (x((0.255, -0.01, 0.89)), 0.026, 0.025),
        ]
        b.add(tube(arm, sub=2, n=12, ref=fwd, caps=(True, False)), M["wrap_limb"], arm_w)
        for c, rx, ry, tilt in ((x((0.215, -0.023, 1.29)), 0.046, 0.048, x((0.3, 0, 1))), (x((0.247, -0.02, 1.01)), 0.035, 0.035, x((-0.3, 0.2, 1)))):
            b.add(band(c, tilt, rx, ry, 0.005, 12), M["band"], arm_w)

        # Wrapped claw: narrow palm, four bony fingers curled towards the palm (-s X), a thumb in front.
        w, e = j["wrist"], j["hand_end"]
        d = (e - w).normalized()
        palm_side = V((-s, 0, 0))
        b.add(tube([(w - d * 0.01, 0.022, 0.027), (w + d * 0.03, 0.025, 0.038), (w + d * 0.062, 0.022, 0.040)], sub=2, n=10, ref=fwd, bulge=(0, 0.006)),
              M["wrap_limb"], n("hand"))
        for k, fy in enumerate((-0.026, -0.009, 0.008, 0.025)):
            base = w + d * 0.058 + V((0, fy, 0))
            ln = 0.055 + 0.01 * (k in (1, 2))
            pts = [base, base + d * ln * 0.55 + palm_side * 0.006, base + d * ln * 0.9 + palm_side * 0.022, base + d * ln + palm_side * 0.04]
            b.add(tube([(p, r, r) for p, r in zip(pts, (0.010, 0.009, 0.008, 0.005))], sub=1, n=6, ref=palm_side), M["skin"], n("hand"))
        thumb = [w + d * 0.025 + V((0, 0.03, 0)), w + d * 0.05 + V((-0.006 * s, 0.045, 0)), w + d * 0.07 + V((-0.016 * s, 0.046, 0))]
        b.add(tube([(p, r, r) for p, r in zip(thumb, (0.011, 0.009, 0.006))], sub=1, n=6, ref=V((1, 0, 0))), M["skin"], n("hand"))


def build_head(b, M):
    fwd = V((0, 1, 0))
    head = [
        ((0, 0.015, 1.555), 0.046, 0.046),
        ((0, 0.03, 1.60), 0.062, 0.072),
        ((0, 0.03, 1.64), 0.074, 0.088),
        ((0, 0.02, 1.69), 0.084, 0.097),
        ((0, 0.01, 1.735), 0.082, 0.093),
        ((0, 0.0, 1.77), 0.064, 0.074),
        ((0, -0.005, 1.79), 0.034, 0.040),
    ]
    b.add(tube(head, sub=2, n=16, ref=fwd, caps=(False, True), bulge=(0, 0.012)), M["wrap_head"],
          lambda co: {"head": smoothstep(1.56, 1.61, co.z), "neck": 1 - smoothstep(1.56, 1.61, co.z)})
    b.add(ellipsoid((0, 0.112, 1.662), (0.014, 0.014, 0.026), n=8, rings=5), M["wrap_head"], "head")  # nose
    for z, tilt, rx, ry in ((1.745, (0, -0.15, 1), 0.083, 0.094), (1.615, (0.2, 0.25, 1), 0.068, 0.082)):
        b.add(band((0, 0.012 if z > 1.7 else 0.03, z), tilt, rx, ry, 0.006, 14), M["band"], "head")
    for s in (1, -1):
        sock = V((0.034 * s, 0.100, 1.697))
        b.add(ellipsoid(sock, (0.022, 0.014, 0.012), n=10, rings=5), M["pitch"], "head")
        b.add(ellipsoid(sock + V((0.002 * s, 0.010, 0)), (0.0075, 0.005, 0.0065), n=8, rings=4), M["eye"], "head")
    # Torn mouth: a dark gap with a dry lip.
    b.add(ellipsoid((0.012, 0.096, 1.612), (0.03, 0.012, 0.008), n=10, rings=4), M["pitch"], "head")
    b.add(ellipsoid((0.012, 0.101, 1.602), (0.024, 0.008, 0.004), n=8, rings=3), M["skin"], "head")


def build_strips(b, M):
    for name, _parent, a, (l0, l1) in STRIPS:
        b0, b1 = strip_bones(name)
        a = V(a)
        keys = [(a + V((0, 0, 0.01)), 0.024, 0.004), (a - V((0, 0, l0 * 0.5)), 0.024, 0.003), (a - V((0, 0, l0)), 0.021, 0.003),
                (a - V((0, 0, l0 + l1 * 0.6)), 0.017, 0.0025), (a - V((0, 0, l0 + l1)), 0.008, 0.002)]
        ref = V((0, 1, 0)) if abs(a.x) > 0.1 else V((1, 0, 0))
        b.add(tube(keys, sub=2, n=4, ref=ref), M["strip"], chain_weights([a + V((0, 0, 0.02)), a - V((0, 0, l0)), a - V((0, 0, l0 + l1))], [b0, b1], 0.02), "strip")


# ---------------------------------------------------------------- poses
# Pose = {bone: (rx, ry, rz) degrees}, plus "root_loc": (x, y, z) and "root_scale". Down-pointing limbs: +rx swings
# forward, +rz swings towards -X. Up-pointing bones (pelvis..head): +rx leans back. Root (rx, 0, rz): rx tips the whole
# body back about the feet, rz turns it (Euler XYZ = Rz @ Rx); (90, 0, 90) lies on the back with the head at +X.

READY = {  # arms reaching forward, hands drooping, hunched, head cocked: walk frame 0, attack frame 0, rise end
    "upperarm.R": (98, 0, -6), "forearm.R": (8, 0, 0), "hand.R": (-30, 0, 0),
    "upperarm.L": (92, 0, 8), "forearm.L": (14, 0, 0), "hand.L": (-24, 0, 0),
    "spine": (-4, 0, 0), "chest": (-8, 0, 0), "neck": (-4, 0, 0), "head": (-6, 4, 9),
}
CROSSED = {}  # arms crossed on the chest, solved in solve_crossed()


def pose(base=READY, **bones):
    p = {k: tuple(v) if not isinstance(v, (int, float)) else v for k, v in base.items()}
    for k, v in bones.items():
        p[k.replace("_R", ".R").replace("_L", ".L")] = v
    return p


def add(p, **bones):
    q = dict(p)
    for k, v in bones.items():
        k = k.replace("_R", ".R").replace("_L", ".L")
        q[k] = tuple(a + b for a, b in zip(q.get(k, (0, 0, 0)), v))
    return q


def is_strip(name):
    return name.startswith("strip_")


def set_pose(rig, p):
    for pb in rig.pose.bones:
        if is_strip(pb.name):
            continue
        pb.rotation_euler = [math.radians(a) for a in p.get(pb.name, (0, 0, 0))]
        pb.location = p.get("root_loc", (0, 0, 0)) if pb.name == "root" else (0, 0, 0)
        pb.scale = p.get("root_scale", (1, 1, 1)) if pb.name == "root" else (1, 1, 1)


def key_pose(rig, p, frame):
    set_pose(rig, p)
    for pb in rig.pose.bones:
        if is_strip(pb.name):
            continue
        pb.keyframe_insert("rotation_euler", frame=frame)
        if pb.name == "root":
            pb.keyframe_insert("location", frame=frame)
            pb.keyframe_insert("scale", frame=frame)


def aim(rig, name, target):
    """Turn bone name (shortest arc) so its tail points at target (armature space); returns its Euler degrees."""
    pb = rig.pose.bones[name]
    bpy.context.view_layer.update()
    m = pb.matrix.copy()
    q = m.col[1].to_3d().normalized().rotation_difference((V(target) - m.translation).normalized())
    new = (q.to_matrix() @ m.to_3x3()).to_4x4()
    new.translation = m.translation
    pb.matrix = new
    bpy.context.view_layer.update()
    return tuple(math.degrees(a) for a in pb.rotation_euler)


def solve_crossed(rig):
    """Osiride pose: forearms crossed high on the chest, fists at the opposite shoulders, the right arm in front."""
    set_pose(rig, {})
    for s in (1, -1):
        n = lambda bn: side_name(bn, s)  # noqa: E731
        CROSSED[n("upperarm")] = aim(rig, n("upperarm"), (0.215 * s, 0.10, 1.18))
        front = 0.18 if s > 0 else 0.152
        CROSSED[n("forearm")] = aim(rig, n("forearm"), (-0.07 * s, front, 1.35))
        CROSSED[n("hand")] = aim(rig, n("hand"), (-0.15 * s, front - 0.01, 1.40))
    set_pose(rig, {})


def walk_pose(t):
    """Stiff shamble at phase t (0 = passing pose): straight knees, the left foot dragged, a lurching sway."""
    def leg(ph, drag):
        c = math.cos(2 * math.pi * ph)
        swing = max(0.0, math.sin(2 * math.pi * (ph - 0.5)))
        th = 19 * c
        knee = -4 - (20 if not drag else 11) * swing
        foot = 6 * c - (8 if not drag else -6) * swing
        return th, knee, foot

    ph = t + 0.25
    thL, knL, ftL = leg(ph, True)
    thR, knR, ftR = leg(ph + 0.5, False)
    w = 2 * math.pi * ph
    c, sn = math.cos(w), math.sin(w)
    return add(pose(),
               thigh_L=(thL, 0, 0), shin_L=(knL, 0, 0), foot_L=(ftL, 0, 0),
               thigh_R=(thR, 0, 0), shin_R=(knR, 0, 0), foot_R=(ftR, 0, 0),
               pelvis=(-3, 7 * c, 4 * sn), spine=(0, -3 * c, -2 * sn), chest=(0, -4 * c, -2.5 * sn), head=(1.5 * math.sin(2 * w), 2 * c, 0),
               upperarm_R=(3 * math.sin(2 * w + 0.6), 0, 0), upperarm_L=(3 * math.sin(2 * w), 0, 0),
               hand_R=(4 * c, 0, 0), hand_L=(-4 * c, 0, 0))


def walk_poses():
    n = FRAMES["mummy_walk"]
    return [(f, walk_pose(f / n)) for f in range(0, n + 1, 2)]


def attack_poses():
    """One slow two-handed smash: arms up over the head, a lurch forward and down, a hold, back to the reach."""
    n = FRAMES["mummy_attack"]
    w0 = walk_pose(0.0)
    lift = add(w0, upperarm_R=(42, 0, 2), upperarm_L=(46, 0, -4), forearm_R=(12, 0, 0), forearm_L=(8, 0, 0), chest=(8, 0, 0), spine=(3, 0, 0), head=(4, 0, 0))
    windup = pose(w0, upperarm_R=(172, 0, -8), upperarm_L=(168, 0, 10), forearm_R=(36, 0, 0), forearm_L=(32, 0, 0), hand_R=(10, 0, 0), hand_L=(14, 0, 0),
                  pelvis=(2, 0, 0), spine=(8, 0, 0), chest=(12, 0, 0), neck=(4, 0, 0), head=(6, 0, 4),
                  thigh_L=(4, 0, 0), shin_L=(-6, 0, 0), thigh_R=(-12, 0, 0), shin_R=(-4, 0, 0), foot_R=(8, 0, 0))
    windup["root_loc"] = (0, -0.04, 0)
    smash = pose(w0, upperarm_R=(62, 0, -4), upperarm_L=(58, 0, 6), forearm_R=(6, 0, 0), forearm_L=(4, 0, 0), hand_R=(-30, 0, 0), hand_L=(-34, 0, 0),
                 pelvis=(-10, 0, 0), spine=(-14, 0, 0), chest=(-20, 0, 0), neck=(-6, 0, 0), head=(4, 0, 6),
                 thigh_L=(30, 0, 0), shin_L=(-24, 0, 0), foot_L=(-4, 0, 0), thigh_R=(-18, 0, 0), shin_R=(-6, 0, 0), foot_R=(14, 0, 0))
    smash["root_loc"] = (0, 0.12, 0)
    hold = add(smash, chest=(-3, 0, 0), spine=(-2, 0, 0), upperarm_R=(-4, 0, 0), upperarm_L=(-4, 0, 0), hand_R=(-6, 0, 0), hand_L=(-6, 0, 0))
    hold["root_loc"] = (0, 0.13, 0)
    back = add(w0, chest=(-4, 0, 0), thigh_L=(10, 0, 0), shin_L=(-8, 0, 0))
    back["root_loc"] = (0, 0.05, 0)
    return [(0, w0), (4, lift), (9, windup), (11, windup), (13, smash), (16, hold), (19, back), (n, w0)]


def die_poses():
    """Recoils, the knees give, folds forward onto the floor and sags into a heap of linen."""
    w0 = walk_pose(0.0)
    hit = pose(w0, upperarm_R=(120, 0, -30), upperarm_L=(110, 0, 34), forearm_R=(30, 0, 0), forearm_L=(36, 0, 0), hand_R=(20, 0, 0), hand_L=(16, 0, 0),
               spine=(8, 0, 0), chest=(14, 0, 0), neck=(8, 0, 0), head=(18, -10, -6), thigh_R=(6, 0, 0), shin_R=(-6, 0, 0))
    hit["root_loc"] = (0, -0.06, 0)
    buckle = pose({}, upperarm_R=(40, 0, -16), upperarm_L=(30, 0, 18), forearm_R=(30, 0, 0), forearm_L=(24, 0, 0),
                  pelvis=(-6, 0, 4), spine=(-6, 0, 0), chest=(-10, 0, 0), neck=(-8, 0, 0), head=(-10, 14, 10),
                  thigh_L=(40, 0, 0), shin_L=(-75, 0, 0), foot_L=(30, 0, 0), thigh_R=(36, 0, 0), shin_R=(-72, 0, 0), foot_R=(32, 0, 0))
    kneel = pose({}, upperarm_R=(20, 0, -12), upperarm_L=(14, 0, 14), forearm_R=(20, 0, 0), forearm_L=(18, 0, 0),
                 pelvis=(-14, 0, 4), spine=(-12, 0, 0), chest=(-14, 0, 0), neck=(-12, 0, 0), head=(-18, 16, 12),
                 thigh_L=(14, 0, 0), shin_L=(-102, 0, 0), foot_L=(-30, 0, 0), thigh_R=(10, 0, 0), shin_R=(-98, 0, 0), foot_R=(-32, 0, 0))
    kneel["root_loc"] = (0, 0.02, 0)
    fold = pose({}, upperarm_R=(95, 0, -20), upperarm_L=(100, 0, 24), forearm_R=(14, 0, 0), forearm_L=(20, 0, 0), hand_R=(-20, 0, 0),
                pelvis=(-55, 0, 6), spine=(-14, 0, 0), chest=(-16, 0, 0), neck=(-6, 0, 0), head=(4, 30, 10),
                thigh_L=(60, 0, 0), shin_L=(-120, 0, 0), foot_L=(-35, 0, 0), thigh_R=(56, 0, 0), shin_R=(-118, 0, 0), foot_R=(-35, 0, 0))
    fold["root_loc"] = (0, 0.20, 0)
    down = pose({}, upperarm_R=(140, 0, -38), upperarm_L=(130, 0, 42), forearm_R=(24, 0, 0), forearm_L=(36, 0, 0), hand_R=(-40, 0, 0), hand_L=(-30, 0, 0),
                pelvis=(-80, 0, 8), spine=(-12, 0, 4), chest=(-10, 0, 0), neck=(4, 0, 0), head=(10, 60, 0),
                thigh_L=(78, 0, 0), shin_L=(-140, 0, 0), foot_L=(-40, 0, 0), thigh_R=(74, 0, 0), shin_R=(-136, 0, 0), foot_R=(-40, 0, 0))
    down["root_loc"] = (0, 0.42, 0)
    heap = add(down, spine=(-4, 0, 6), chest=(-6, 0, -4), head=(4, 6, 0), upperarm_R=(6, 0, -8), upperarm_L=(4, 0, 10))
    heap["root_loc"] = (0, 0.46, 0)
    heap["root_scale"] = (1.15, 1.1, 0.62)
    sag = dict(heap)
    sag["root_scale"] = (1.08, 1.05, 0.78)
    last = FRAMES["mummy_die"] - 1
    return [(0, w0), (4, hit), (8, buckle), (11, kneel), (16, fold), (20, down), (25, sag), (last, heap)]


def idle_pose(t):
    """On its back (root (90, 0, 90): head at +X, face up), head propped on the headrest, arms crossed; a slow breath
    and once per loop a twitch of the right hand and the head."""
    w = 2 * math.pi * t
    twitch = math.exp(-((t - 0.62) / 0.04) ** 2)
    p = pose(CROSSED, root=(90, 0, 90), neck=(-26, 0, 0), head=(-20 - 3 * twitch, 0, -5 * twitch),
             chest=(1.2 * math.sin(w), 0, 0), spine=(0.6 * math.sin(w), 0, 0),
             foot_R=(4, 0, -4), foot_L=(2, 0, 5))
    p = add(p, hand_R=(14 * twitch, 0, 0), forearm_R=(4 * twitch, 0, 0))
    return p


# Climbing a ladder (model +Y = the ladder; the engine draws it at rotA + 180, back to the camera). The engine sets the
# frame from the height, one cycle per CLIMB["rungs"] rung spacings (a rung every tile / 12). Diagonal gait: a hand and
# the opposite foot grip and slide down linearly by R per cycle (they stay on the rungs while the engine lifts the
# body), the other pair lets go, draws back, rises stiffly and thrusts onto the rung R / 2 higher in the body frame.
# Root at a fixed height; the lowest foot of frame 0 stands on the floor (rung 0). Positions in metres, body frame.
CLIMB = {"rungs": 4, "grip_y": 0.40, "drop": 0.08}  # R in rungs; the hands' rung plane in front of the root
CLIMB_HAND = {"x": 0.30, "top": 5, "lift": 0.12, "wrist": V((0, -0.05, 0.06)), "dir": V((0, 0.55, -0.83))}  # top: rung index
CLIMB_FOOT = {"x": 0.17, "top": 2, "lift": 0.12, "back": 0.12}  # ankle `back` behind the rung plane (the ball of the foot on it)
CLIMB_POLE = {"forearm": V((1.0, -0.2, -0.6)), "shin": V((0.35, 1.0, 0.6))}  # elbows out and down, knees forward and up
CLIMB_PHASE = {"hand.L": 0.0, "foot.R": 0.0, "hand.R": 0.5, "foot.L": 0.5}  # v = 0: grips its top rung at frame 0
REF = {}  # walk frame 0 (model space, metres): "H" height, "S" largest extent, "C" bounding box centre y, "rig_y"


def rung():
    return TILE * U["m"] / 12


def climb_limb(v, top, lift):
    """Body-frame height and draw-back (0..1) of a limb at its phase v: grip 0..0.5 sliding down by R / 2 at the
    climbing rate, then let go, rise and thrust back in (0.5..1), a stiff three-part move."""
    R = CLIMB["rungs"] * rung()
    v %= 1.0
    if v < 0.5:
        return top - R * v, 0.0
    t = (v - 0.5) / 0.5
    return top - R / 2 + R / 2 * smoothstep(0.12, 0.85, t), smoothstep(0.0, 0.22, t) * (1 - smoothstep(0.72, 1.0, t))


def climb_targets(p, ankle_lift):
    """Body-frame wrist and ankle targets at cycle phase p, plus the reaching hands' draw-back / feet's toe droop."""
    r, out = rung(), {}
    for s in (1, -1):
        hand, foot = side_name("hand", s), side_name("foot", s)
        z, back = climb_limb(p + CLIMB_PHASE[hand], CLIMB_HAND["top"] * r, 1.0)
        out[hand] = (V((CLIMB_HAND["x"] * s, CLIMB["grip_y"] - CLIMB_HAND["lift"] * back, z)) + CLIMB_HAND["wrist"], back)
        z, back = climb_limb(p + CLIMB_PHASE[foot], CLIMB_FOOT["top"] * r, 1.0)
        droop = back * smoothstep(0.0, 0.12, z - (CLIMB_FOOT["top"] - CLIMB["rungs"] / 2) * r)  # toes droop once off the rung
        out[foot] = (V((CLIMB_FOOT["x"] * s, CLIMB["grip_y"] - CLIMB_FOOT["back"] - CLIMB_FOOT["lift"] * back, z + ankle_lift)), droop)
    return out


def two_bone(rig, upper, lower, target, pole):
    """Analytic two-bone IK: aim upper at the joint (bent towards pole, a direction) and lower at target."""
    bpy.context.view_layer.update()
    pu, pl = rig.pose.bones[upper], rig.pose.bones[lower]
    a, l1, l2 = pu.head.copy(), pu.bone.length, pl.bone.length
    d = target - a
    dist = min(max(d.length, abs(l1 - l2) + 1e-3), l1 + l2 - 1e-4)
    dn = d.normalized()
    x = (l1 * l1 + dist * dist - l2 * l2) / (2 * dist)
    side = (pole - dn * pole.dot(dn)).normalized()
    aim(rig, upper, a + dn * x + side * math.sqrt(max(l1 * l1 - x * x, 0.0)))
    aim(rig, lower, target)


def climb_pose(p):
    """FK trunk of the climb at phase p: stiff, leaning a little into the ladder, the head cocked up towards it and
    jerking towards the reaching hand."""
    w = 2 * math.pi * p
    q = pose({}, pelvis=(-4, 0, 3 * math.sin(w)), spine=(-3, 0, -1.5 * math.sin(w)), chest=(-4, 0, -2 * math.sin(w)),
             neck=(4, 0, 0), head=(10, 5, 8 * math.cos(w)))
    q["root_loc"] = (0, 0, -CLIMB["drop"])
    return q


def build_climb(rig, obj, act, skip):
    """Key every frame: FK trunk, IK hands and feet on the rungs; frame N = frame 0."""
    scene = bpy.context.scene
    n = FRAMES["mummy_climb"]
    rig.location = (0, REF["rig_y"], 0)
    set_pose(rig, {})
    bpy.context.view_layer.update()
    ankle_lift = rig.pose.bones["foot.R"].head.z - lowest(obj, skip)  # ankle over the sole, rest pose
    for f in range(n + 1):
        p = f / n
        set_pose(rig, climb_pose(p))
        for name, (target, back) in climb_targets(p, ankle_lift).items():
            s = 1 if name.endswith(".R") else -1
            if name.startswith("hand"):
                two_bone(rig, side_name("upperarm", s), side_name("forearm", s), target, CLIMB_POLE["forearm"] * V((s, 1, 1)))
                d = CLIMB_HAND["dir"].lerp(V((0, 0.2, -1)), back)  # the claw hooks over the rung, hangs while reaching
                aim(rig, name, target + d)
            else:
                two_bone(rig, side_name("thigh", s), side_name("shin", s), target, CLIMB_POLE["shin"] * V((s, 1, 1)))
                aim(rig, name, target + V((0.002 * s, 0.16, -0.055 - 0.08 * back)))  # sole level on the rung, toes droop
        for pb in rig.pose.bones:
            if is_strip(pb.name):
                continue
            pb.keyframe_insert("rotation_euler", frame=f)
            if pb.name == "root":
                pb.keyframe_insert("location", frame=f)
                pb.keyframe_insert("scale", frame=f)
    scene.frame_set(0)
    r, R, H = rung(), CLIMB["rungs"] * rung(), REF["H"]
    G = CLIMB["grip_y"] + REF["rig_y"]  # model space
    print("mummy climb: frames {}, tile {:.4f} m, rung {:.4f} m, R {:.4f} m ({} rungs), G {:.4f}, C {:.4f}, H {:.4f}, S_ref {:.4f}".format(
        n, TILE * U["m"], r, R, CLIMB["rungs"], G, REF["C"], H, REF["S"]))
    print("mummy climb: climbRise = {:.4f}, climbGrip = {:.4f}, frame 0 lowest {:.4f} m".format(R / H, (G - REF["C"]) / H, lowest(obj)))


# ---------------------------------------------------------------- strip simulation


def coffin_box():
    """Coffin solids in model space (metres, the dormant frame: body centre at the origin, +Y = towards the camera):
    [(min, max)] for the floor slab and the four walls, plus the inner floor height."""
    t = TILE * U["m"]
    hx, hy, w, fl, rim = (COFFIN[k] * t for k in ("half_x", "half_y", "wall", "floor", "rim"))
    boxes = [((-hx - w, -hy - w, -1.0), (hx + w, hy + w, fl)),  # floor slab
             ((-hx - w, hy, 0), (hx + w, hy + w, rim)), ((-hx - w, -hy - w, 0), (hx + w, -hy, rim)),  # front, back
             ((hx, -hy, 0), (hx + w, hy, rim)), ((-hx - w, -hy, 0), (-hx, hy, rim))]  # ends
    return boxes, fl


def collide(p, coffin):
    """Push a strip particle (dormant-frame coordinates) out of the floor and the coffin."""
    p = V(p)
    floor = 0.006
    if coffin:
        boxes, _fl = coffin_box()
        for lo, hi in boxes:
            if all(lo[i] <= p[i] <= hi[i] for i in range(2)):
                floor = max(floor, hi[2] + 0.008)
    p.z = max(p.z, floor)
    return p


def constrain(parent, p, length, coffin):
    """Put p at length from parent, out of the floor / coffin; a lifted point slides sideways to keep the length."""
    d = p - parent
    if d.length < 1e-6:
        d = V((0, 0, -1))
    p = parent + d.normalized() * length
    q = collide(p, coffin)
    if q.z > p.z + 1e-6:
        h = V((d.x, d.y, 0))
        h = h.normalized() if h.length > 1e-6 else V((1, 0, 0))
        r = math.sqrt(max(length * length - (q.z - parent.z) ** 2, 0.0))
        q = V((parent.x + h.x * r, parent.y + h.y * r, q.z))
    return q


def simulate_strips(rig, act, frames, cycles=1, shift=None, coffin=False, blend_to=None, state=None, start=None):
    """Verlet strips: each chain's tip and joint follow its anchor with lag under gravity. shift(f) = model-space offset
    of the coffin frame (the engine slide undone). Keys every frame of the last cycle; blend_to = (from_frame, eulers)
    eases the strips into given Euler angles by the last frame, start = eulers for frame 0. state carries the particles
    over from the clip before (it is updated in place); returns {frame: {bone: euler degrees}}."""
    scene = bpy.context.scene
    rig.animation_data.action = act
    g, damp = 9.8 / 14.0 ** 2, 0.80
    state = {} if state is None else state
    shift = shift or (lambda f: V())
    out = {}
    for c in range(cycles):
        for f in range(frames):
            scene.frame_set(f)
            off = rig.matrix_world.translation + shift(f)
            for name, _parent, _a, (l0, l1) in STRIPS:
                b0, b1 = strip_bones(name)
                pb0 = rig.pose.bones[b0]
                anchor = pb0.head + off
                if name not in state:
                    p1 = anchor - V((0, 0, l0))
                    p2 = p1 - V((0, 0, l1))
                    state[name] = [p1, p2, p1.copy(), p2.copy()]
                p1, p2, q1, q2 = state[name]
                n1 = p1 + (p1 - q1) * damp - V((0, 0, g))
                n2 = p2 + (p2 - q2) * damp - V((0, 0, g))
                for _ in range(4):
                    n1 = constrain(anchor, n1, l0, coffin)
                    n2 = constrain(n1, n2, l1, coffin)
                state[name] = [n1, n2, p1, p2]
                if c < cycles - 1:
                    continue
                for pb, a, tip in ((pb0, anchor, n1), (rig.pose.bones[b1], n1, n2)):
                    bpy.context.view_layer.update()
                    m = pb.matrix.copy()
                    q = m.col[1].to_3d().normalized().rotation_difference((tip - a).normalized())
                    new = (q.to_matrix() @ m.to_3x3().normalized()).to_4x4()
                    new.translation = m.translation
                    pb.matrix = new
                    pb.scale = (1, 1, 1)  # the setter takes up the shear of a squashed root (die)
                    bpy.context.view_layer.update()
                    if start and f == 0:
                        pb.rotation_euler = [math.radians(x) for x in start[pb.name]]
                    if blend_to and f >= blend_to[0]:
                        k = smoothstep(blend_to[0], frames - 1, f)
                        target = Euler([math.radians(x) for x in blend_to[1][pb.name]], "XYZ").to_quaternion()
                        pb.rotation_euler = pb.rotation_euler.to_quaternion().slerp(target, k).to_euler("XYZ", pb.rotation_euler)
                    pb.location = (0, 0, 0)
                    pb.keyframe_insert("rotation_euler", frame=f)
                    pb.keyframe_insert("location", frame=f)
                    pb.keyframe_insert("scale", frame=f)
                    out.setdefault(f, {})[pb.name] = tuple(math.degrees(x) for x in pb.rotation_euler)
    return out


# ---------------------------------------------------------------- clip assembly


def strip_verts(obj):
    groups = {g.index for g in obj.vertex_groups if is_strip(g.name)}
    return {v.index for v in obj.data.vertices if sum(ge.weight for ge in v.groups if ge.group in groups) > 0.5}


def evaluated(obj):
    ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mw = ev.matrix_world
    return [mw @ v.co for v in ev.data.vertices]


def lowest(obj, skip=()):
    return min(p.z for i, p in enumerate(evaluated(obj)) if i not in skip)


def bbox(obj, skip=()):
    pts = [p for i, p in enumerate(evaluated(obj)) if i not in skip]
    return V([min(p[i] for p in pts) for i in range(3)]), V([max(p[i] for p in pts) for i in range(3)])


def new_action(rig, name):
    act = bpy.data.actions.get(name)
    if act:
        bpy.data.actions.remove(act)
    act = bpy.data.actions.new(name)
    act.use_fake_user = True
    rig.animation_data.action = act
    return act


def make_cyclic(rig, act):
    from bpy_extras import anim_utils

    bag = anim_utils.action_get_channelbag_for_slot(act, rig.animation_data.action_slot)
    for fc in bag.fcurves:
        fc.modifiers.new("CYCLES")
        fc.update()


def floor_fix(rig, obj, frames, skip):
    scene = bpy.context.scene
    root = rig.pose.bones["root"]
    for f in frames:
        scene.frame_set(f)
        root.location.z -= lowest(obj, skip)
        root.keyframe_insert("location", frame=f)


def root_for_hip(rig, p, hip):
    """Root location that puts the hip centre (pelvis head) at model-space point hip for pose p."""
    r = Euler([math.radians(a) for a in p.get("root", (0, 0, 0))], "XYZ").to_matrix()
    return V(hip) - rig.matrix_world.translation - r @ PELVIS


def hip_of(rig, p):
    r = Euler([math.radians(a) for a in p.get("root", (0, 0, 0))], "XYZ").to_matrix()
    return rig.matrix_world.translation + V(p.get("root_loc", (0, 0, 0))) + r @ PELVIS


def slide(f):
    """Model-space offset of the coffin frame at rise frame f: the engine's slide towards the camera, undone."""
    t = f / (FRAMES["mummy_rise"] - 1)
    return V((0, -TOMB_DEPTH * U["m"] * smoothstep(SLIDE[0], SLIDE[1], t), 0))


def lerp_pose(keys, f):
    """Catmull-Rom through key poses (frame, pose); angles, root and the "hip" point."""
    frames = [k for k, _ in keys]
    i = max(j for j in range(len(keys)) if frames[j] <= f)
    if i == len(keys) - 1:
        return dict(keys[i][1])
    i0, i2, i3 = max(i - 1, 0), i + 1, min(i + 2, len(keys) - 1)
    t = (f - frames[i]) / (frames[i2] - frames[i])
    ps = [keys[j][1] for j in (i0, i, i2, i3)]
    out = {}
    for name in set().union(*ps):
        vals = [V(p.get(name, (1, 1, 1) if name == "root_scale" else (0, 0, 0))) for p in ps]
        out[name] = tuple(common._cr(*vals, t))
    return out


def rise_keys(rig, idle0, walk0):
    """Rise key poses in the coffin frame (dormant body centre at the origin, metres). "hip" = hip centre."""
    u = U["m"]
    t = TILE * u
    rim_y, rim_out, rim_z = COFFIN["half_y"] * t, (COFFIN["half_y"] + COFFIN["wall"]) * t, COFFIN["rim"] * t
    hip0 = hip_of(rig, idle0)
    end = hip_of(rig, walk0) - slide(FRAMES["mummy_rise"] - 1)
    seat_y = 0.5 * (rim_y + rim_out) + 0.01
    arms_low = dict(upperarm_R=(-12, 0, 4), upperarm_L=(-12, 0, -4), forearm_R=(30, 0, 0), forearm_L=(30, 0, 0), hand_R=(-30, 0, 0), hand_L=(-30, 0, 0))

    def k(base, hip, **bones):
        p = pose(base, **bones)
        p["hip"] = tuple(hip)
        return p

    sit_z = hip0.z + 0.07
    keys = [
        (0, dict(idle0, hip=tuple(hip0))),
        (2, k(CROSSED, (hip0.x, hip0.y, hip0.z + 0.02), root=(55, 0, 90), thigh_R=(35, 0, 0), thigh_L=(35, 0, 0), foot_R=(-20, 0, 0), foot_L=(-22, 0, 0),
              spine=(-8, 0, 0), chest=(-10, 0, 0), neck=(-10, 0, 0), head=(-12, 0, 0))),
        (4, k({}, (hip0.x * 0.8, hip0.y + 0.02, sit_z), root=(8, 0, 88), thigh_R=(84, 0, -4), thigh_L=(84, 0, 4), foot_R=(-15, 0, 0), foot_L=(-15, 0, 0),
              spine=(-6, 0, 0), chest=(-8, 0, 0), neck=(-6, 0, 0), head=(-4, 0, -6),
              upperarm_R=(38, 0, 4), upperarm_L=(34, 0, -4), forearm_R=(20, 0, 0), forearm_L=(18, 0, 0), hand_R=(-20, 0, 0), hand_L=(-20, 0, 0))),
        # Legs up in a V, pivoting on the seat towards the camera: the feet pass high over the front rim.
        (5, k({}, (hip0.x * 0.6, hip0.y + 0.04, sit_z), root=(22, 0, 70), thigh_R=(112, 0, -6), thigh_L=(108, 0, 6), shin_R=(-30, 0, 0), shin_L=(-34, 0, 0),
              foot_R=(-10, 0, 0), foot_L=(-10, 0, 0), spine=(-8, 0, 0), chest=(-10, 0, 0), neck=(-6, 0, 0), head=(-8, -10, 0), **arms_low)),
        (6, k({}, (hip0.x * 0.4, hip0.y + 0.05, sit_z), root=(24, 0, 36), thigh_R=(114, 0, -8), thigh_L=(110, 0, 8), shin_R=(-40, 0, 0), shin_L=(-44, 0, 0),
              foot_R=(-6, 0, 0), foot_L=(-6, 0, 0), spine=(-8, 0, 0), chest=(-10, 0, 0), neck=(-6, 0, 0), head=(-8, -6, 0), **arms_low)),
        (7, k({}, (hip0.x * 0.2, hip0.y + 0.07, sit_z - 0.02), root=(20, 0, 6), thigh_R=(112, 0, -6), thigh_L=(110, 0, 6), shin_R=(-70, 0, 0), shin_L=(-74, 0, 0),
              foot_R=(0, 0, 0), foot_L=(0, 0, 0), spine=(-10, 0, 0), chest=(-12, 0, 0), neck=(-6, 0, 0), head=(-6, 0, 0), **arms_low)),
        # Knees over the rim, shins down outside; it heaves itself onto the rim.
        (8, k({}, (hip0.x * 0.1, hip0.y + 0.10, sit_z + 0.05), root=(10, 0, 0), thigh_R=(104, 0, -5), thigh_L=(102, 0, 5), shin_R=(-104, 0, 0), shin_L=(-108, 0, 0),
              foot_R=(8, 0, 0), foot_L=(8, 0, 0), spine=(-14, 0, 0), chest=(-14, 0, 0), neck=(-6, 0, 0), head=(-4, 0, 0), **arms_low)),
        (9, k({}, (0.0, 0.5 * (hip0.y + 0.10 + seat_y) - 0.02, rim_z + 0.115), root=(-4, 0, 0), thigh_R=(96, 0, -4), thigh_L=(94, 0, 4), shin_R=(-104, 0, 0),
              shin_L=(-106, 0, 0), foot_R=(10, 0, 0), foot_L=(10, 0, 0), spine=(-14, 0, 0), chest=(-16, 0, 0), neck=(-4, 0, 0), head=(-2, 0, 0), **arms_low)),
        (10, k({}, (0.0, seat_y, rim_z + 0.10), root=(-10, 0, 0), thigh_R=(98, 0, -3), thigh_L=(96, 0, 3), shin_R=(-100, 0, 0), shin_L=(-102, 0, 0),
               foot_R=(6, 0, 0), foot_L=(6, 0, 0), spine=(-14, 0, 0), chest=(-16, 0, 0), neck=(-2, 0, 0), head=(0, 0, 4),
               upperarm_R=(10, 0, -12), upperarm_L=(10, 0, 12), forearm_R=(20, 0, 0), forearm_L=(20, 0, 0))),
    ]
    # Stands up over its feet, then two stiff steps (right, left) to the walk line, then straightens and reaches.
    feet_y = seat_y + 0.42
    stand_z = end.z - 0.03
    keys += [
        (12, k({}, (0.0, feet_y - 0.06, stand_z - 0.12), root=(0, 0, 0), thigh_R=(46, 0, -2), thigh_L=(44, 0, 2), shin_R=(-62, 0, 0), shin_L=(-60, 0, 0),
               foot_R=(16, 0, 0), foot_L=(16, 0, 0), pelvis=(-24, 0, 0), spine=(-8, 0, 0), chest=(-6, 0, 0), neck=(4, 0, 0), head=(6, 0, 4),
               upperarm_R=(28, 0, -10), upperarm_L=(26, 0, 10), forearm_R=(20, 0, 0), forearm_L=(20, 0, 0), hand_R=(-20, 0, 0), hand_L=(-20, 0, 0))),
        (14, k({}, (0.0, feet_y + 0.16, stand_z), root=(0, 0, 0), thigh_R=(22, 0, 0), shin_R=(-16, 0, 0), foot_R=(2, 0, 0), thigh_L=(-14, 0, 0), shin_L=(-6, 0, 0),
               foot_L=(10, 0, 0), pelvis=(-4, -6, -4), spine=(-4, 3, 2), chest=(-6, 3, 2), head=(-2, 0, 6),
               upperarm_R=(36, 0, -8), upperarm_L=(30, 0, 8), forearm_R=(16, 0, 0), forearm_L=(16, 0, 0), hand_R=(-22, 0, 0), hand_L=(-22, 0, 0))),
        (16, k({}, (0.0, 0.5 * (feet_y + 0.16 + end.y), stand_z + 0.02), root=(0, 0, 0), thigh_L=(22, 0, 0), shin_L=(-16, 0, 0), foot_L=(2, 0, 0),
               thigh_R=(-14, 0, 0), shin_R=(-6, 0, 0), foot_R=(10, 0, 0), pelvis=(-4, 6, 4), spine=(-4, -3, -2), chest=(-6, -3, -2), head=(-4, 2, 8),
               upperarm_R=(48, 0, -8), upperarm_L=(44, 0, 8), forearm_R=(14, 0, 0), forearm_L=(16, 0, 0), hand_R=(-24, 0, 0), hand_L=(-24, 0, 0))),
        (19, k(walk_pose(0.0), (end.x, end.y, end.z), upperarm_R=(56, 0, -6), upperarm_L=(52, 0, 8), hand_R=(-30, 0, 0), hand_L=(-26, 0, 0),
               chest=(-2, 0, 0), head=(-2, 0, 4))),
        (22, k(walk_pose(0.0), (end.x, end.y, end.z), upperarm_R=(86, 0, -6), upperarm_L=(80, 0, 8), chest=(-12, 0, 0), head=(-10, 6, 12))),
        (FRAMES["mummy_rise"] - 1, dict(walk0, hip=tuple(end))),
    ]
    return keys


def build_rise(rig, obj, act, idle0, walk0, skip):
    """Key the rise every frame: pose from the keys, root from the hip point (coffin frame -> model space), then
    keep the body off the coffin floor (sitting up) / on the floor (standing)."""
    scene = bpy.context.scene
    n = FRAMES["mummy_rise"]
    keys = rise_keys(rig, idle0, walk0)
    inner = COFFIN["floor"] * TILE * U["m"]
    root = rig.pose.bones["root"]
    for f in range(n):
        if f == 0:
            p = dict(idle0)
        elif f == n - 1:
            p = dict(walk0)
        else:
            p = lerp_pose(keys, f)
            p["root_loc"] = tuple(root_for_hip(rig, p, V(p.pop("hip")) + slide(f)))
        p.pop("hip", None)
        key_pose(rig, p, f)
        if 0 < f < n - 1:
            scene.frame_set(f)
            low = lowest(obj, skip)
            if f <= 8:
                fix = max(0.0, inner + 0.004 - low)  # never through the coffin floor
            elif f <= 11:
                fix = max(0.0, -low)  # feet hanging down to the floor, never through it
            else:
                fix = -low  # standing: on the floor
            root.location.z += fix
            root.keyframe_insert("location", frame=f)


def make_actions(rig, obj):
    scene = bpy.context.scene
    rig.animation_data_create()
    skip = strip_verts(obj)
    acts = {}
    solve_crossed(rig)

    # Walk first: it is the reference (frame 0 centred on x = y = 0, H = its largest dimension).
    n = FRAMES["mummy_walk"]
    act = acts["mummy_walk"] = new_action(rig, "mummy_walk")
    for f, p in walk_poses():
        key_pose(rig, p, f)
    make_cyclic(rig, act)
    floor_fix(rig, obj, range(n + 1), skip)
    walk_strips = simulate_strips(rig, act, n, cycles=3)
    scene.frame_set(0)
    lo, hi = bbox(obj)
    rig.location = (-(lo.x + hi.x) / 2, -(lo.y + hi.y) / 2, 0)
    bpy.context.view_layer.update()
    lo, hi = bbox(obj)
    H = max(hi - lo)
    U["m"] = H / SCALE
    REF.update(H=hi.z - lo.z, S=H, C=(lo.y + hi.y) / 2, rig_y=rig.location.y)
    walk0 = walk_pose(0.0)
    walk0["root_loc"] = tuple(rig.pose.bones["root"].location)
    print("mummy: walk frame 0 {:.3f} x {:.3f} x {:.3f} m, H = {:.3f} m, 1 world unit = {:.4f} m".format(*(hi - lo), H, U["m"]))

    n = FRAMES["mummy_attack"]
    act = acts["mummy_attack"] = new_action(rig, "mummy_attack")
    for f, p in attack_poses():
        key_pose(rig, p, f)
    make_cyclic(rig, act)
    floor_fix(rig, obj, range(n + 1), skip)
    simulate_strips(rig, act, n, cycles=3)

    n = FRAMES["mummy_die"]
    act = acts["mummy_die"] = new_action(rig, "mummy_die")
    for f, p in die_poses():
        key_pose(rig, p, f)
    floor_fix(rig, obj, range(n), skip)
    simulate_strips(rig, act, n)

    # Idle: centred on x = y = 0 (frame 0), the back IDLE_BACK world units above the floor.
    n = FRAMES["mummy_idle"]
    act = acts["mummy_idle"] = new_action(rig, "mummy_idle")
    idle0 = idle_pose(0.0)
    key_pose(rig, idle0, 0)
    scene.frame_set(0)
    lo, hi = bbox(obj, skip)
    loc = V(rig.pose.bones["root"].location) - V(((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, lo.z - IDLE_BACK * U["m"]))
    idle0["root_loc"] = tuple(loc)
    for f in range(0, n + 1, 2):
        p = idle_pose(f / n)
        p["root_loc"] = tuple(loc)
        key_pose(rig, p, f)
    make_cyclic(rig, act)
    state = {}
    idle_strips = simulate_strips(rig, act, n, cycles=3, coffin=True, state=state)

    n = FRAMES["mummy_rise"]
    act = acts["mummy_rise"] = new_action(rig, "mummy_rise")
    build_rise(rig, obj, act, idle0, walk0, skip)
    simulate_strips(rig, act, n, shift=lambda f: -slide(f), coffin=True, blend_to=(19, walk_strips[0]), state=state, start=idle_strips[0])

    n = FRAMES["mummy_climb"]
    act = acts["mummy_climb"] = new_action(rig, "mummy_climb")
    build_climb(rig, obj, act, skip)
    make_cyclic(rig, act)
    simulate_strips(rig, act, n, cycles=3)
    return acts


def report(rig, obj):
    scene = bpy.context.scene
    for name, _, n in CLIPS:
        rig.animation_data.action = bpy.data.actions[name]
        lows = []
        for f in range(n):
            scene.frame_set(f)
            lows.append(lowest(obj))
        print("MINZ {}: {}".format(name, " ".join("{:.3f}".format(z) for z in lows)))
    rig.animation_data.action = bpy.data.actions["mummy_idle"]
    scene.frame_set(0)
    lo, hi = bbox(obj)
    u = U["m"]
    print("idle frame 0 (world units): x {:.2f}..{:.2f}, y {:.2f}..{:.2f}, z {:.2f}..{:.2f}".format(lo.x / u, hi.x / u, lo.y / u, hi.y / u, lo.z / u, hi.z / u))
    head = [p for p, v in zip(evaluated(obj), obj.data.vertices) if obj.vertex_groups["head"].index in {ge.group for ge in v.groups if ge.weight > 0.9}]
    back = min(head, key=lambda p: p.z)
    k = 1.0 / (TILE * u)
    print("idle head back: decor x {:.3f}, {:.4f} tiles above the inner floor (decor.py COFFIN_HEADREST)".format(-back.x * k, back.z * k - COFFIN["floor"]))


# ---------------------------------------------------------------- entry points


def build(bake=True, tex_path=None):
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    M = materials()
    rig = build_rig(coll)
    b = Builder()
    build_body(b, M)
    first = len(b.verts)
    build_head(b, M)
    b.tags[first:] = ["head"] * (len(b.verts) - first)
    build_strips(b, M)
    obj = common.finish_mesh(b, coll, "mummy_new")
    common.uv_unwrap(obj, b.tags, {"head": ((0, 0.03, 1.67), 1.8)})
    if bake:
        tex = common.bake_texture(obj, tex_path or os.path.join(bpy.app.tempdir or "/tmp", "mummy_preview.png"), TEX_SIZE, "mummy")
        common.use_baked_material(obj, tex)
    common.rig_object(obj, rig)
    acts = make_actions(rig, obj)
    rig.animation_data.action = acts["mummy_walk"]
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 0, FRAMES["mummy_walk"] - 1
    scene.frame_set(0)
    print("mummy: {} verts, {} tris".format(len(obj.data.vertices), common.tri_count(obj)))
    return obj, rig


def export(models_dir=None):
    common.export_files(bpy.data.objects["mummy_new"], bpy.data.objects["mummy_rig"], "mummy", CLIPS, "monsters", models_dir)


def coffin_check(out_dir, frames=(0, 3, 5, 6, 7, 8, 9, 10, 12, 14, 16, 19, 25)):
    """Coffin prop (decor.py) + the dormant / rising mummy in decor tile units at the engine's placement; renders
    game-camera and side views and counts mummy vertices inside the coffin's solids per rise frame."""
    os.makedirs(out_dir, exist_ok=True)
    obj, rig = bpy.data.objects["mummy_new"], bpy.data.objects["mummy_rig"]
    p = os.path.join(HERE, "decor.py")
    g = {"__file__": p, "__name__": "decor"}
    exec(open(p).read(), g)
    coffin = g["build"](bake=False, only=["coffin"])["coffin"]
    coffin.location = (0, 0, 0)
    env = bpy.data.objects.get("decor_env")
    k = 1.0 / (TILE * U["m"])  # tile units per metre
    holder = bpy.data.objects.new("mummy_holder", None)
    bpy.context.scene.collection.objects.link(holder)
    holder.rotation_euler = (0, 0, math.pi)  # rotA 180
    holder.scale = (k, k, k)
    rig.parent = holder
    scene = bpy.context.scene
    walk_y = -0.5
    n = FRAMES["mummy_rise"]

    def place(t):
        holder.location = (0, walk_y + TOMB_DEPTH / TILE * (1 - smoothstep(SLIDE[0], SLIDE[1], t)), 0)
        bpy.context.view_layer.update()

    boxes, _fl = coffin_box()
    frames_pts = []
    for clip, fr in (("mummy_idle", (0,)), ("mummy_rise", range(n))):
        rig.animation_data.action = bpy.data.actions[clip]
        for f in fr:
            t = f / (n - 1) if clip == "mummy_rise" else 0.0
            scene.frame_set(f)
            place(t)
            # Back to the dormant frame (metres, body centre at the origin, +Y towards the camera).
            frames_pts.append((clip, f, [V((-p.x / k, -(p.y + 0.14) / k, p.z / k)) for p in evaluated(obj)]))
    # Every frame and the halfway point to the next one (the engine blends frames linearly).
    checks = list(frames_pts)
    checks += [("between", f0 + 0.5, [a.lerp(b, 0.5) for a, b in zip(p0, p1)]) for (_c, f0, p0), (_c1, _f1, p1) in zip(frames_pts[1:], frames_pts[2:])]
    for clip, f, pts in checks:
        inside = [[i for i, q in enumerate(pts) if all(lo[j] + 1e-4 < q[j] < hi[j] - 1e-4 for j in range(3))] for lo, hi in boxes]
        parts = sorted({max(obj.data.vertices[i].groups, key=lambda ge: ge.weight).group for hit in inside for i in hit})
        print("COFFIN {} {:4.1f}: inside floor/front/back/ends {} {} lowest {:.2f} wu".format(
            clip, f, [len(h) for h in inside], [obj.vertex_groups[gi].name for gi in parts], min(q.z for q in pts) / U["m"]))

    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x, scene.render.resolution_y = 640, 400
    if scene.world is None:
        scene.world = bpy.data.worlds.new("World")
    scene.world.color = (0.25, 0.25, 0.25)
    sun = bpy.data.objects.new("check_sun", bpy.data.lights.new("check_sun", "SUN"))
    sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(60), 0, math.radians(-20))
    scene.collection.objects.link(sun)
    cam = bpy.data.objects.new("check_cam", bpy.data.cameras.new("check_cam"))
    scene.collection.objects.link(cam)
    scene.camera = cam
    cam.data.type = "ORTHO"
    if env:
        env.hide_render = False
    views = {"game": (80, 0, V((0, -0.25, 0.22)), 1.0), "side": (90, 90, V((0, -0.3, 0.22)), 1.0), "top": (20, 0, V((0, -0.2, 0.1)), 1.0),
             "close": (78, 0, V((0, -0.14, 0.08)), 0.75)}
    for clip, fr in (("mummy_idle", (0,)), ("mummy_rise", frames)):
        rig.animation_data.action = bpy.data.actions[clip]
        for f in fr:
            scene.frame_set(f)
            place(f / (n - 1) if clip == "mummy_rise" else 0.0)
            for vname, (tilt, yaw, centre, scale) in views.items():
                if vname in ("top", "close") and clip == "mummy_rise" and f > 9:
                    continue
                cam.data.ortho_scale = scale
                rot = Euler((math.radians(tilt), 0, math.radians(yaw)), "XYZ")
                cam.rotation_euler = rot
                cam.location = centre + rot.to_matrix() @ V((0, 0, 6))
                scene.render.filepath = os.path.join(out_dir, "{}_{}_{:02d}.png".format(vname, clip.split("_")[1], f))
                bpy.ops.render.render(write_still=True)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    build(bake="--export" in args or "--bake" in args, tex_path=os.path.join(REPO, "textures", "monsters", "mummy.png") if "--export" in args else None)
    report(bpy.data.objects["mummy_rig"], bpy.data.objects["mummy_new"])
    if "--export" in args:
        export()
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "mummy.blend"))
    if "--export-climb" in args:  # only mummy_climb.md3 (the UVs do not need the bake)
        common.export_files(bpy.data.objects["mummy_new"], bpy.data.objects["mummy_rig"], "mummy", [c for c in CLIPS if c[0] == "mummy_climb"], "monsters")
    if "--coffin-check" in args:
        coffin_check(args[args.index("--coffin-check") + 1])
