"""Procedural Anubis for the game: mesh, rig, baked texture and the three .md3 animations.

Everything is generated from code, so this file is the model source. Run in Blender 5:
    MCP:  p = ".../tools/blender/models/anubis.py"; g = {"__file__": p, "__name__": "anubis"}
          exec(open(p).read(), g); g["build"]()          # then g["export"]() to write game files
    CLI:  blender -b --python tools/blender/models/anubis.py -- [--export] [--export-climb] [--boss-texture]
          (--export-climb writes only anubis_climb.md3; ANUBIS_LADDER=1 adds a ladder stand-in for review renders)

Blender space: Z up, the model faces +Y, its right side is +X. Units are metres (~2 m tall);
the engine rescales by the largest dimension, so only proportions matter.
Everything lives in the collection "anubis_new"; rebuilding replaces it.
Two textures share one UV layout: anubis.png (gold and lapis) and anubis_boss.png (the finale's boss: obsidian skin,
carnelian and gold, a blood-red kilt, burning eyes; bake it alone with --boss-texture).
"""

import math
import os
import sys

import importlib

import bpy
from mathutils import Matrix
from mathutils import Vector as V

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import common  # noqa: E402

importlib.reload(common)
from common import REPO, Builder, chain_weights, ellipsoid, loft, smoothstep, transform, tube  # noqa: E402
COLL = "anubis_new"
FRAMES = 26  # frames per animation file; engine plays ~14 fps
CLIMB = ("anubis_climb", "_climb", 24)
CLIPS = [("anubis_walk", "", FRAMES), ("anubis_attack", "_att", FRAMES), ("anubis_die", "_die", FRAMES), CLIMB]
TEX_SIZE = 1024

# Linear-space colours.
COL = {
    "skin": (0.030, 0.030, 0.038),
    "skin_hi": (0.060, 0.058, 0.070),
    "gold": (0.80, 0.50, 0.10),
    "gold_dark": (0.45, 0.25, 0.04),
    "lapis": (0.03, 0.08, 0.42),
    "turq": (0.04, 0.42, 0.38),
    "red": (0.50, 0.08, 0.03),
    "linen": (0.82, 0.78, 0.64),
    "linen_dark": (0.58, 0.54, 0.42),
    "white": (0.85, 0.85, 0.80),
    "black": (0.008, 0.008, 0.008),
    "bronze": (0.22, 0.12, 0.04),
}

# The Anubis boss: darker skin, carnelian where the lapis was, gold for the turquoise, a crimson kilt, fiery eyes.
COL_BOSS = dict(COL, **{
    "skin": (0.012, 0.010, 0.016),
    "skin_hi": (0.030, 0.024, 0.036),
    "lapis": (0.32, 0.03, 0.015),
    "turq": (0.85, 0.55, 0.10),
    "red": (0.02, 0.04, 0.30),
    "linen": (0.26, 0.035, 0.03),
    "linen_dark": (0.12, 0.015, 0.012),
    "white": (1.0, 0.42, 0.04),
})


def materials(pal=COL):
    stripes = lambda axis, period, bands: ("stripes", axis, period, bands)  # noqa: E731
    specs = {
        "skin": ("solid", "skin"),
        "gold": ("solid", "gold"),
        "black": ("solid", "black"),
        "white": ("solid", "white"),
        "wig": stripes("v", 0.036, [(1, "gold"), (2, "lapis")]),
        "wig_cap": stripes("u", 1 / 36, [(1, "gold"), (2, "lapis")]),
        # Collar bands, inner -> outer on the top face (u 0..0.5), mirrored underneath.
        "collar": stripes("u", 1.0, [(3, "gold"), (6, "lapis"), (2, "gold"), (6, "turq"), (2, "gold"), (6, "red"), (2, "gold"), (8, "lapis"), (6, "gold"), (3, "gold_dark"),
                                     (3, "gold_dark"), (6, "gold"), (8, "lapis"), (2, "gold"), (6, "red"), (2, "gold"), (6, "turq"), (2, "gold"), (6, "lapis"), (3, "gold")]),
        "kilt": stripes("u", 1 / 44, [(3, "linen"), (1, "linen_dark")]),
        "belt": stripes("u", 1 / 28, [(3, "gold"), (1, "lapis"), (1, "red"), (1, "lapis")]),
        "apron": stripes("u", 1 / 8, [(2, "gold"), (1, "lapis"), (1, "gold"), (2, "red"), (1, "gold"), (1, "lapis")]),
        "band": stripes("v", 0.03, [(1, "gold"), (1, "lapis"), (1, "gold")]),
        "staff": stripes("v", 0.30, [(10, "bronze"), (1, "gold"), (1, "gold_dark"), (1, "gold")]),
    }
    return {name: common.make_material("anubis_" + name, spec, pal) for name, spec in specs.items()}


# ---------------------------------------------------------------- skeleton


def side_name(bone, s):
    return bone + (".R" if s > 0 else ".L")


def joints(s):
    """Rest joint positions for side s (+1 right/+X, -1 left/-X)."""
    return {
        "hip": V((0.10 * s, 0.0, 0.93)),
        "knee": V((0.10 * s, 0.008, 0.50)),
        "ankle": V((0.10 * s, -0.012, 0.085)),
        "toe": V((0.103 * s, 0.14, 0.025)),
        "shoulder": V((0.20 * s, 0.0, 1.43)),
        "elbow": V((0.25 * s, -0.025, 1.15)),
        "wrist": V((0.275 * s, 0.0, 0.905)),
        "hand_end": V((0.284 * s, 0.006, 0.80)),
    }


def build_rig(coll):
    arm = bpy.data.armatures.new("anubis_rig")
    rig = bpy.data.objects.new("anubis_rig", arm)
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
        # Local X = world +X for every bone: rx swings limbs forward (down bones) / back (up bones).
        b.align_roll(V((1, 0, 0)).cross((b.tail - b.head).normalized()))
        if parent:
            b.parent = eb[parent]
            b.use_connect = connect

    bone("root", (0, 0, 0), (0, 0.3, 0))
    bone("pelvis", (0, 0, 0.95), (0, 0, 1.05), "root")
    bone("spine", (0, 0, 1.05), (0, 0, 1.25), "pelvis", True)
    bone("chest", (0, 0, 1.25), (0, 0, 1.48), "spine", True)
    bone("neck", (0, 0, 1.48), (0, 0.02, 1.64), "chest", True)
    bone("head", (0, 0.02, 1.64), (0, 0.02, 1.86), "neck", True)
    for s in (1, -1):
        j = joints(s)
        n = lambda b: side_name(b, s)  # noqa: E731
        bone(n("thigh"), j["hip"], j["knee"], "pelvis")
        bone(n("shin"), j["knee"], j["ankle"], n("thigh"), True)
        bone(n("foot"), j["ankle"], j["toe"], n("shin"), True)
        bone(n("upperarm"), j["shoulder"], j["elbow"], "chest")
        bone(n("forearm"), j["elbow"], j["wrist"], n("upperarm"), True)
        bone(n("hand"), j["wrist"], j["hand_end"], n("forearm"), True)
    bpy.ops.object.mode_set(mode="OBJECT")
    for pb in rig.pose.bones:
        pb.rotation_mode = "XYZ"
    return rig


def add_staff_bone(rig, head, tail):
    """Sceptre bone under the right hand, so the sceptre can be dropped when dying."""
    bpy.context.view_layer.objects.active = rig
    bpy.ops.object.mode_set(mode="EDIT")
    b = rig.data.edit_bones.new("staff")
    b.head, b.tail = V(head), V(tail)
    b.align_roll(V((1, 0, 0)).cross((b.tail - b.head).normalized()))
    b.parent = rig.data.edit_bones["hand.R"]
    bpy.ops.object.mode_set(mode="OBJECT")
    rig.pose.bones["staff"].rotation_mode = "XYZ"


# ---------------------------------------------------------------- body parts


def build_body(b, M):
    up, fwd = V((0, 0, 1)), V((0, 1, 0))

    def torso_shape(th, a):
        # a: chest amount -> pectoral bulge in front, flatter back.
        front = max(0.0, math.sin(th))
        return 1.0 + 0.07 * a * front ** 2 * (0.4 + 0.6 * abs(math.cos(th))) - 0.03 * a * max(0.0, -math.sin(th))

    torso = [
        ((0, 0.0, 0.86), 0.10, 0.07, 0),
        ((0, 0.0, 0.92), 0.148, 0.100, 0),
        ((0, 0.0, 1.02), 0.138, 0.094, 0),
        ((0, 0.0, 1.12), 0.134, 0.094, 0.1),
        ((0, 0.005, 1.22), 0.154, 0.104, 0.5),
        ((0, 0.01, 1.32), 0.178, 0.112, 1.0),
        ((0, 0.0, 1.405), 0.192, 0.102, 0.6),
        ((0, -0.005, 1.455), 0.148, 0.084, 0),
        ((0, 0.0, 1.50), 0.068, 0.064, 0),
        ((0, 0.01, 1.57), 0.055, 0.057, 0),
        ((0, 0.02, 1.66), 0.050, 0.052, 0),
    ]
    b.add(tube(torso, sub=2, n=18, ref=fwd, shape=torso_shape, caps=(True, False), bulge=(0.03, 0)), M["skin"],
          chain_weights([(0, 0, 0.80), (0, 0, 1.05), (0, 0, 1.25), (0, 0, 1.48), (0, 0.01, 1.64), (0, 0.02, 1.9)],
                        ["pelvis", "spine", "chest", "neck", "head"], 0.06))

    for s in (1, -1):
        j = joints(s)
        n = lambda bn: side_name(bn, s)  # noqa: E731
        x = lambda v: V((v[0] * s, v[1], v[2]))  # noqa: E731

        def calf(th, a):
            return 1.0 + 0.12 * a * max(0.0, -math.sin(th)) ** 2

        leg = [
            (x((0.088, 0.0, 0.98)), 0.066, 0.070, 0),
            (x((0.10, 0.004, 0.88)), 0.073, 0.076, 0),
            (x((0.10, 0.01, 0.72)), 0.064, 0.067, 0),
            (x((0.10, 0.012, 0.56)), 0.049, 0.051, 0),
            (x((0.10, 0.008, 0.48)), 0.045, 0.047, 0),
            (x((0.10, -0.004, 0.37)), 0.047, 0.048, 1),
            (x((0.10, -0.01, 0.23)), 0.038, 0.040, 0.3),
            (x((0.10, -0.012, 0.10)), 0.029, 0.031, 0),
        ]
        b.add(tube(leg, sub=2, n=12, ref=fwd, shape=calf, caps=(False, True), bulge=(0, 0.01)), M["skin"],
              chain_weights([x((0.10, 0, 1.08)), j["hip"], j["knee"], j["ankle"]], ["pelvis", n("thigh"), n("shin")], 0.06))

        def sole(th, a):
            return 0.75 if math.sin(th) < 0 else 1.0

        foot = [
            (x((0.10, -0.065, 0.045)), 0.028, 0.034),
            (x((0.10, -0.035, 0.055)), 0.036, 0.050),
            (x((0.10, 0.03, 0.045)), 0.041, 0.036),
            (x((0.102, 0.11, 0.030)), 0.043, 0.024),
            (x((0.104, 0.165, 0.022)), 0.036, 0.016),
        ]
        b.add(tube(foot, sub=2, n=10, ref=up, shape=sole, bulge=(0.01, 0.012)), M["skin"],
              chain_weights([x((0.10, -0.012, 0.2)), j["ankle"], j["toe"]], [n("shin"), n("foot")], 0.03))
        b.add(tube([(x((0.10, -0.012, 0.135)), 0.034, 0.036), (x((0.10, -0.012, 0.115)), 0.037, 0.039), (x((0.10, -0.012, 0.095)), 0.034, 0.036)],
                   sub=1, n=12, caps=(False, False)), M["band"], n("shin"))

        arm = [
            (x((0.14, 0.0, 1.445)), 0.054, 0.056),
            (x((0.20, 0.0, 1.43)), 0.062, 0.062),
            (x((0.225, -0.01, 1.33)), 0.050, 0.053),
            (x((0.24, -0.02, 1.23)), 0.045, 0.047),
            (x((0.25, -0.025, 1.15)), 0.037, 0.040),
            (x((0.26, -0.018, 1.07)), 0.041, 0.042),
            (x((0.27, -0.008, 0.98)), 0.034, 0.032),
            (x((0.275, 0.0, 0.905)), 0.027, 0.026),
        ]
        b.add(tube(arm, sub=2, n=12, ref=fwd, caps=(True, False)), M["skin"],
              chain_weights([x((0.12, 0, 1.44)), j["shoulder"], j["elbow"], j["wrist"], j["hand_end"]],
                            ["chest", n("upperarm"), n("forearm"), n("hand")], 0.05))
        # Arm band and bracelet.
        b.add(tube([(x((0.222, -0.009, 1.345)), 0.055, 0.058), (x((0.227, -0.011, 1.32)), 0.058, 0.061), (x((0.232, -0.013, 1.295)), 0.054, 0.057)],
                   sub=1, n=12, caps=(False, False)), M["band"], n("upperarm"))
        b.add(tube([(x((0.268, -0.006, 0.965)), 0.036, 0.035), (x((0.271, -0.004, 0.94)), 0.038, 0.037), (x((0.274, -0.002, 0.915)), 0.034, 0.033)],
                   sub=1, n=12, caps=(False, False)), M["gold"], n("forearm"))

        # Fist: palm faces the thigh, so it is thin across X and wide front-back.
        w, e = j["wrist"], j["hand_end"]
        d = (e - w).normalized()
        fist = [(w - d * 0.01, 0.024, 0.030), (w + d * 0.025, 0.030, 0.042), (w + d * 0.06, 0.034, 0.048), (w + d * 0.09, 0.032, 0.044), (w + d * 0.108, 0.020, 0.028)]
        b.add(tube(fist, sub=2, n=10, ref=fwd, bulge=(0, 0.006)), M["skin"], n("hand"))
        thumb = [(w + d * 0.03 + V((0, 0.03, 0)), 0.013, 0.013), (w + d * 0.06 + V((-0.004 * s, 0.045, 0)), 0.012, 0.012), (w + d * 0.08 + V((-0.008 * s, 0.047, 0)), 0.010, 0.010)]
        b.add(tube(thumb, sub=1, n=8, ref=V((1, 0, 0)), bulge=(0.004, 0.006)), M["skin"], n("hand"))


def build_head(b, M):
    up, fwd = V((0, 0, 1)), V((0, 1, 0))
    skull = [
        ((0, -0.115, 1.745), 0.022, 0.022),
        ((0, -0.10, 1.745), 0.066, 0.070),
        ((0, -0.06, 1.75), 0.092, 0.095),
        ((0, -0.01, 1.752), 0.097, 0.097),
        ((0, 0.04, 1.738), 0.084, 0.080),
        ((0, 0.085, 1.712), 0.057, 0.055),
        ((0, 0.13, 1.692), 0.043, 0.043),
        ((0, 0.18, 1.677), 0.035, 0.036),
        ((0, 0.23, 1.665), 0.028, 0.030),
        ((0, 0.265, 1.658), 0.021, 0.023),
        ((0, 0.284, 1.655), 0.012, 0.014),
    ]
    b.add(tube(skull, sub=2, n=16, ref=up, bulge=(0.008, 0.004)), M["skin"], "head")
    jaw = [((0, 0.02, 1.664), 0.052, 0.032), ((0, 0.09, 1.648), 0.040, 0.022), ((0, 0.17, 1.638), 0.028, 0.016), ((0, 0.235, 1.633), 0.018, 0.011), ((0, 0.258, 1.633), 0.008, 0.006)]
    b.add(tube(jaw, sub=2, n=10, ref=up, bulge=(0.0, 0.003)), M["skin"], "head")
    b.add(ellipsoid((0, 0.284, 1.662), (0.013, 0.012, 0.011), n=8, rings=5), M["black"], "head")

    wig_cap = [((0, -0.125, 1.75), 0.03, 0.03), ((0, -0.105, 1.75), 0.076, 0.081), ((0, -0.06, 1.756), 0.100, 0.103), ((0, -0.015, 1.758), 0.104, 0.104), ((0, 0.012, 1.755), 0.101, 0.100)]
    b.add(tube(wig_cap, sub=2, n=18, ref=up, caps=(True, False), bulge=(0.01, 0)), M["wig_cap"], "head")
    wig_back = [((0, -0.09, 1.80), 0.100, 0.03), ((0, -0.125, 1.70), 0.118, 0.03), ((0, -0.125, 1.60), 0.124, 0.03), ((0, -0.105, 1.52), 0.118, 0.024)]
    b.add(tube(wig_back, sub=2, n=12, ref=V((0, -1, 0)), bulge=(0.01, 0.01)), M["wig"], "head")

    for s in (1, -1):
        x = lambda v: V((v[0] * s, v[1], v[2]))  # noqa: E731

        def concave(th, a):
            return 1.0 if math.sin(th) < 0 else 0.35 + 0.65 * abs(math.cos(th))

        ear = [(x((0.05, -0.02, 1.80)), 0.046, 0.019), (x((0.056, -0.024, 1.86)), 0.043, 0.015), (x((0.063, -0.028, 1.92)), 0.033, 0.011),
               (x((0.070, -0.032, 1.98)), 0.017, 0.008), (x((0.075, -0.035, 2.025)), 0.003, 0.003)]
        b.add(tube(ear, sub=2, n=10, ref=fwd, shape=concave, caps=(False, True)), M["skin"], "head")
        inner = [(x((0.052, -0.015, 1.83)), 0.030, 0.004), (x((0.058, -0.019, 1.88)), 0.028, 0.004), (x((0.064, -0.023, 1.93)), 0.020, 0.003), (x((0.070, -0.027, 1.985)), 0.006, 0.002)]
        b.add(tube(inner, sub=2, n=8, ref=fwd), M["gold"], "head")

        # Egyptian eye: gold liner, white, dark pupil.
        b.add(ellipsoid(x((0.050, 0.076, 1.748)), (0.008, 0.032, 0.015), n=10, rings=5), M["gold"], "head")
        b.add(ellipsoid(x((0.054, 0.079, 1.749)), (0.006, 0.022, 0.0105), n=10, rings=5), M["white"], "head")
        b.add(ellipsoid(x((0.0585, 0.082, 1.749)), (0.004, 0.0085, 0.0085), n=8, rings=4), M["black"], "head")

        lappet = [(x((0.085, -0.035, 1.78)), 0.046, 0.014), (x((0.102, -0.018, 1.68)), 0.050, 0.014), (x((0.115, 0.02, 1.585)), 0.052, 0.014),
                  (x((0.114, 0.072, 1.50)), 0.050, 0.013), (x((0.104, 0.108, 1.40)), 0.045, 0.012)]
        b.add(tube(lappet, sub=2, n=10, ref=V((0.5 * s, 1, 0.2)), bulge=(0.004, 0.004)), M["wig"],
              lambda co: {"head": smoothstep(1.50, 1.64, co.z), "chest": 1 - smoothstep(1.50, 1.64, co.z)})


def build_clothes(b, M):
    fwd = V((0, 1, 0))
    # Broad collar: flat strip around the neck, sheared so it drapes down and out.
    path, secs = [], []
    for i in range(32):
        a = 2 * math.pi * i / 32
        path.append(V((0.142 * math.cos(a), 0.012 + 0.108 * math.sin(a), 1.452 - 0.015 * max(0.0, math.sin(a)))))
        secs.append((0.072, 0.007, None, 0.85))
    b.add(loft(path, secs, n=10, ref=V((0, 0, 1)), closed=True), M["collar"], "chest")

    def kilt_w(co):
        t = smoothstep(0.95, 0.62, co.z)
        side = min(max(co.x / 0.12, -1.0), 1.0)
        k = 0.7 * t * abs(side)
        return {"pelvis": 1 - k, ("thigh.R" if side > 0 else "thigh.L"): k}

    kilt = [((0, 0.005, 1.03), 0.150, 0.102), ((0, 0.01, 0.95), 0.178, 0.126), ((0, 0.02, 0.82), 0.192, 0.146), ((0, 0.03, 0.66), 0.216, 0.170), ((0, 0.03, 0.62), 0.219, 0.173)]
    b.add(tube(kilt, sub=2, n=28, ref=fwd, caps=(False, False)), M["kilt"], kilt_w)
    belt = [((0, 0.005, 0.99), 0.158, 0.108), ((0, 0.005, 1.025), 0.161, 0.111), ((0, 0.005, 1.06), 0.152, 0.103)]
    b.add(tube(belt, sub=1, n=28, ref=fwd, caps=(False, False)), M["belt"], "pelvis")
    apron = [((0, 0.128, 1.0), 0.048, 0.006), ((0, 0.148, 0.90), 0.060, 0.006), ((0, 0.172, 0.78), 0.078, 0.006), ((0, 0.20, 0.645), 0.098, 0.006), ((0, 0.205, 0.61), 0.100, 0.006)]
    b.add(tube(apron, sub=2, n=8, ref=fwd), M["apron"], kilt_w)


def build_props(b, M, rig):
    """Sceptre and ankh are modelled in the ready pose, then moved to rest space of their hand."""
    set_pose(rig, READY)
    bpy.context.view_layer.update()

    def to_rest(bone_name, geo):
        pb = rig.pose.bones[bone_name]
        mat = pb.bone.matrix_local @ pb.matrix.inverted()
        return transform(geo, lambda v: mat @ v)

    def fist_centre(s):
        pb = rig.pose.bones[side_name("hand", s)]
        j = joints(s)
        rest = j["wrist"] + (j["hand_end"] - j["wrist"]).normalized() * 0.06
        return pb.matrix @ pb.bone.matrix_local.inverted() @ rest

    # Was-sceptre, vertical through the right fist.
    p = fist_centre(1)
    pb = rig.pose.bones["hand.R"]
    STAFF["hook"] = ((pb.bone.matrix_local @ pb.matrix.inverted()).to_3x3() @ V((0, 1, 0))).normalized()  # rest space
    bottom, top = 0.05, 1.90
    STAFF["grip"] = p.z - bottom
    staff_head = to_rest("hand.R", ([p, p + V((0, 0, 0.3))], [], []))[0]
    shaft = [(V((p.x, p.y, bottom + 0.10)), 0.013, 0.013), (V((p.x, p.y, 1.0)), 0.016, 0.016), (V((p.x, p.y, top)), 0.015, 0.015)]
    b.add(to_rest("hand.R", tube(shaft, sub=6, n=10, caps=(False, False))), M["staff"], "staff")
    head = [(V((p.x, p.y - 0.01, top - 0.01)), 0.017, 0.017), (V((p.x, p.y + 0.01, top + 0.03)), 0.020, 0.024), (V((p.x, p.y + 0.06, top + 0.035)), 0.014, 0.018),
            (V((p.x, p.y + 0.11, top + 0.02)), 0.008, 0.010)]
    b.add(to_rest("hand.R", tube(head, sub=2, n=10, ref=V((0, 0, 1)), bulge=(0.005, 0.004))), M["gold"], "staff")
    for e in (1, -1):
        prong = [(V((p.x, p.y, bottom + 0.11)), 0.014, 0.014), (V((p.x + 0.012 * e, p.y, bottom + 0.05)), 0.010, 0.010), (V((p.x + 0.03 * e, p.y, bottom)), 0.007, 0.007)]
        b.add(to_rest("hand.R", tube(prong, sub=2, n=8, bulge=(0, 0.003))), M["gold"], "staff")

    # Ankh hanging from the left fist, held by its loop, facing forward.
    p = fist_centre(-1)
    lc = p - V((0, 0, 0.015))
    loop_path = [lc + V((0.028 * math.sin(2 * math.pi * i / 16), 0, 0.042 * math.cos(2 * math.pi * i / 16))) for i in range(16)]
    b.add(to_rest("hand.L", loft(loop_path, [(0.008, 0.008)] * 16, n=8, ref=V((0, 1, 0)), closed=True)), M["gold"], "hand.L")
    bar_z = lc.z - 0.045
    bar = [(V((p.x - 0.055, p.y, bar_z)), 0.009, 0.007), (V((p.x, p.y, bar_z)), 0.008, 0.007), (V((p.x + 0.055, p.y, bar_z)), 0.009, 0.007)]
    b.add(to_rest("hand.L", tube(bar, sub=2, n=8, ref=V((0, 1, 0)), bulge=(0.003, 0.003))), M["gold"], "hand.L")
    stem = [(V((p.x, p.y, bar_z)), 0.008, 0.007), (V((p.x, p.y, bar_z - 0.10)), 0.013, 0.008)]
    b.add(to_rest("hand.L", tube(stem, sub=3, n=8, ref=V((0, 1, 0)), bulge=(0, 0.002))), M["gold"], "hand.L")
    set_pose(rig, {})
    add_staff_bone(rig, *staff_head)


# ---------------------------------------------------------------- animation
# Pose = {bone: (rx, ry, rz) degrees}, plus "root_loc": (x, y, z).
# Down-pointing limbs: +rx swings forward. Up-pointing bones (pelvis..head): +rx leans back, ry twists.

READY = {
    "upperarm.R": (12, 0, -4), "forearm.R": (78, 0, 0), "hand.R": (0, 0, 0),
    "upperarm.L": (-4, 0, 6), "forearm.L": (14, 0, 0),
}


STAFF = {"grip": 0.9, "hook": V((0, 1, 0))}  # sceptre bottom -> fist distance and rest-space hook direction (build_props)


def pose(base=READY, **bones):
    p = {k: tuple(v) for k, v in base.items()}
    for k, v in bones.items():
        p[k.replace("_R", ".R").replace("_L", ".L")] = v
    return p


def set_pose(rig, p):
    for pb in rig.pose.bones:
        pb.rotation_euler = [math.radians(a) for a in p.get(pb.name, (0, 0, 0))]
        pb.location = p.get("root_loc", (0, 0, 0)) if pb.name == "root" else (0, 0, 0)


def key_pose(rig, p, frame):
    set_pose(rig, p)
    for pb in rig.pose.bones:
        pb.keyframe_insert("rotation_euler", frame=frame)
        if pb.name in ("root", "staff"):
            pb.keyframe_insert("location", frame=frame)


def walk_poses():
    def step(t, s):
        """Leg pose for phase t (0 = this leg's heel strike, 0.5 = toe-off/mid swing)."""
        # Thigh swings forward -> back through the stance, knee bends in swing.
        th = 17 * math.cos(2 * math.pi * t)
        swing = max(0.0, math.sin(2 * math.pi * (t - 0.5)))
        knee = -6 - 42 * swing
        foot = 8 * math.cos(2 * math.pi * t) - 10 * swing
        return th, knee, foot

    poses = []
    for f in range(0, FRAMES + 1, 2):
        t = f / FRAMES
        thL, knL, ftL = step(t, -1)
        thR, knR, ftR = step((t + 0.5) % 1, 1)
        bob = -0.018 * abs(math.cos(2 * math.pi * t)) + 0.006
        twist = 6 * math.cos(2 * math.pi * t)
        p = pose(
            thigh_L=(thL, 0, 0), shin_L=(knL, 0, 0), foot_L=(ftL, 0, 0),
            thigh_R=(thR, 0, 0), shin_R=(knR, 0, 0), foot_R=(ftR, 0, 0),
            pelvis=(-3, twist, 2 * math.cos(2 * math.pi * t)),
            spine=(-2, -twist * 0.5, 0), chest=(-2, -twist * 0.6, -2 * math.cos(2 * math.pi * t)),
            neck=(0, 0, 0), head=(2, twist * 0.1, 0),
            # Left arm swings against the left leg; staff arm barely moves.
            upperarm_L=(-4 - 20 * math.cos(2 * math.pi * t), 0, 6), forearm_L=(18 + 8 * math.sin(2 * math.pi * t), 0, 0),
            upperarm_R=(12 + 4 * math.cos(2 * math.pi * t), 0, -4), forearm_R=(78, 0, 0),
        )
        p["root_loc"] = (0, 0, bob)
        poses.append((f, p))
    return poses


def attack_poses():
    ready = pose()
    windup = pose(
        upperarm_R=(160, 0, -10), forearm_R=(40, 0, 0), hand_R=(25, 0, 0),
        upperarm_L=(35, 0, 10), forearm_L=(30, 0, 0),
        pelvis=(4, -12, 0), spine=(6, -8, 0), chest=(8, -10, 0), head=(-4, 12, 0),
        thigh_L=(22, 0, 0), shin_L=(-14, 0, 0), foot_L=(-6, 0, 0),
        thigh_R=(-14, 0, 0), shin_R=(-6, 0, 0), foot_R=(10, 0, 0),
    )
    strike = pose(
        upperarm_R=(78, 0, -20), forearm_R=(12, 0, 0), hand_R=(-58, 0, 0),
        upperarm_L=(-25, 0, 12), forearm_L=(35, 0, 0),
        pelvis=(-8, 10, 0), spine=(-10, 8, 0), chest=(-14, 10, 0), head=(-6, -12, 0),
        thigh_L=(38, 0, 0), shin_L=(-32, 0, 0), foot_L=(-4, 0, 0),
        thigh_R=(-22, 0, 0), shin_R=(-10, 0, 0), foot_R=(18, 0, 0),
    )
    follow = pose(strike, upperarm_R=(70, 0, -20), hand_R=(-64, 0, 0), chest=(-18, 10, 0), spine=(-12, 8, 0))
    windup["root_loc"] = (0, -0.03, -0.02)
    strike["root_loc"] = (0, 0.06, -0.06)
    follow["root_loc"] = (0, 0.07, -0.07)
    return [(0, ready), (8, windup), (12, strike), (16, follow), (FRAMES, ready)]


def die_poses():
    ready = pose()
    hit = pose(
        upperarm_R=(40, 0, -25), forearm_R=(50, 0, 0), upperarm_L=(20, 0, 30), forearm_L=(40, 0, 0),
        pelvis=(4, 0, 0), spine=(8, 0, 0), chest=(14, 0, 0), neck=(10, 0, 0), head=(18, 0, 0),
        thigh_L=(-6, 0, 0), thigh_R=(8, 0, 0), shin_R=(-10, 0, 0),
    )
    hit["root_loc"] = (0, -0.06, 0)
    kneel = pose(
        upperarm_R=(20, 0, -15), forearm_R=(30, 0, 0), upperarm_L=(5, 0, 15), forearm_L=(20, 0, 0),
        pelvis=(-10, 0, 0), spine=(-10, 0, 0), chest=(-12, 0, 0), neck=(-10, 0, 0), head=(-15, 10, 0),
        thigh_L=(12, 0, 0), shin_L=(-100, 0, 0), foot_L=(-35, 0, 0),
        thigh_R=(8, 0, 0), shin_R=(-96, 0, 0), foot_R=(-35, 0, 0),
    )
    kneel["root_loc"] = (0, 0.02, -0.40)
    topple = pose(
        upperarm_R=(120, 0, -25), forearm_R=(20, 0, 0), upperarm_L=(110, 0, 25), forearm_L=(25, 0, 0),
        pelvis=(-55, 0, 0), spine=(-8, 0, 0), chest=(-8, 0, 0), neck=(15, 0, 0), head=(20, 45, 0),
        thigh_L=(55, 0, 0), shin_L=(-90, 0, 0), foot_L=(-40, 0, 0),
        thigh_R=(50, 0, 0), shin_R=(-85, 0, 0), foot_R=(-40, 0, 0),
    )
    topple["root_loc"] = (0, 0.22, -0.43)
    down = pose(
        upperarm_R=(165, 0, -30), forearm_R=(10, 0, 0), hand_R=(-60, 0, 0), upperarm_L=(150, 0, 35), forearm_L=(30, 0, 0),
        pelvis=(-92, 0, 0), spine=(-2, 0, 0), chest=(0, 0, 0), neck=(20, 0, 0), head=(20, 70, 0),
        thigh_L=(4, 0, 0), shin_L=(-10, 0, 0), foot_L=(-70, 0, 0),
        thigh_R=(12, 0, 0), shin_R=(-30, 0, 0), foot_R=(-60, 0, 0),
    )
    down["root_loc"] = (0, 0.60, -0.70)
    settle = pose(down, pelvis=(-90, 0, 0), chest=(2, 0, 0))
    settle["root_loc"] = (0, 0.62, -0.72)
    buckle = pose(
        upperarm_R=(30, 0, -20), forearm_R=(40, 0, 0), upperarm_L=(12, 0, 22), forearm_L=(30, 0, 0),
        pelvis=(-4, 0, 0), spine=(0, 0, 0), chest=(4, 0, 0), neck=(0, 0, 0), head=(4, 8, 0),
        thigh_L=(40, 0, 0), shin_L=(-75, 0, 0), foot_L=(35, 0, 0),
        thigh_R=(36, 0, 0), shin_R=(-72, 0, 0), foot_R=(36, 0, 0),
    )
    buckle["root_loc"] = (0, -0.02, -0.17)
    return [(0, ready), (4, hit), (8, buckle), (11, kneel), (17, topple), (22, down), (FRAMES - 1, settle)]


def drop_staff(rig):
    """Key the sceptre bone so it tips out of the hand and then lies still on the floor."""
    from mathutils import Matrix

    def frame(hook, axis):
        y = V(axis).normalized()
        h = (V(hook) - V(hook).dot(y) * y).normalized()
        return Matrix((h, y, h.cross(y))).transposed()

    # In the ready pose the hook points forward (+Y); keep that relation to the bone.
    scene = bpy.context.scene
    pb = rig.pose.bones["staff"]
    scene.frame_set(0)
    r0 = pb.matrix.to_3x3().normalized()
    ready = frame(V((0, 1, 0)), r0.col[1])

    def world(bottom, axis):
        y = V(axis).normalized()
        flat = y.cross(V((0, 0, 1))).normalized()  # hook lies flat, sideways
        m = (frame(flat, y) @ ready.inverted() @ r0).to_4x4()
        m.translation = V(bottom) + y * STAFF["grip"]
        return m

    lying = world((0.26, 0.15, 0.016), (0.45, 0.89, 0.0))
    # Still in hand at frame 4, then an accelerating tip-over to the floor by frame 12.
    scene.frame_set(4)
    held = pb.matrix.copy()
    targets = {}
    for f in range(5, 13):
        t = ((f - 4) / 8) ** 2
        rot = held.to_quaternion().slerp(lying.to_quaternion(), t).to_matrix()
        axis = rot.col[1].normalized()
        pos = held.translation.lerp(lying.translation, t)
        bottom = pos - axis * STAFF["grip"]
        lift = max(0.0, 0.02 - min(bottom.z, bottom.z + axis.z * 1.85))
        m = rot.to_4x4()
        m.translation = pos + V((0, 0, lift))
        targets[f] = m
    targets.update({f: lying for f in range(12, FRAMES)})
    for f in sorted(targets):
        scene.frame_set(f)
        pb.matrix = targets[f]
        bpy.context.view_layer.update()
        pb.keyframe_insert("rotation_euler", frame=f)
        pb.keyframe_insert("location", frame=f)


# ---------------------------------------------------------------- climb
# Climbing a ladder: hands and feet on IK targets (empties parented to the rig, rig space: the ladder is at +Y), the
# root at a fixed height. The engine draws it turned 180 (back to the camera) and sets the frame from the climb height,
# one cycle per CLIMB_RISE; a gripping hand or foot slides down by exactly that per cycle, so it holds still on the
# ladder while the engine lifts the body. Diagonal gait: the right hand reaches with the left foot.
# Ladder (ladder.py): rails at x = +-0.1 tile, holds every tile / 12, the lowest half a hold up. The clip is fitted
# to the Anubis guard (kind scale 19, 1 tile = 40 world units, the walk frame 0's largest extent = 1 kind scale).
GUARD_SCALE, TILE_WORLD, RUNGS = 19.0, 40.0, 12
CLIMB_RISE_RUNGS = 2  # rise per cycle in holds (even: both hands land on holds)
CLIMB_DUTY = 0.6  # share of the cycle a hand or foot grips (> 0.5: both hands hold for a moment)
CLIMB_GRIP_Y = 0.30  # fist centres close round the rungs in this plane (m in front of the root)
CLIMB_HAND = {"x": 0.30, "reach": 1.86, "lift": 0.09}  # fist height at the top of the stroke (arm nearly straight)
CLIMB_FOOT = {"x": 0.14, "y": CLIMB_GRIP_Y - 0.13, "lift": 0.08}
CLIMB_ROOT_Z = -0.04
# Limb phase at frame 0. foot.R ends its grip at frame 0 (its lowest: on the floor); the hands' phase is set by
# climb_layout so that they reach CLIMB_HAND["reach"] and grip on holds; each hand leads the opposite foot.
CLIMB_PHASE = {"foot.L": 0.1, "foot.R": 0.6}
# IK bone: (target, pole position, pole angle): elbows out and back, knees forward and out.
IK_CHAINS = {"forearm": ("hand", (0.75, -0.25, 1.0), -90), "shin": ("foot", (0.8, 0.45, 0.6), 90)}
CLIMB_LAYOUT = {}  # measured from walk frame 0 by climb_layout


def build_ik(rig, coll):
    """IK targets and poles for the climb; every action keys the IK influence (1 in the climb, 0 elsewhere)."""
    for s in (1, -1):
        for bone, (target, pole, pole_angle) in IK_CHAINS.items():
            empties = []
            for kind in ("target", "pole"):
                name = "anubis_ik_{}_{}".format(side_name(target if kind == "target" else bone, s), kind)
                e = bpy.data.objects.get(name) or bpy.data.objects.new(name, None)
                if e.name not in coll.objects:
                    coll.objects.link(e)
                e.parent = rig
                e.empty_display_size = 0.05
                empties.append(e)
            empties[1].location = (pole[0] * s, pole[1], pole[2])
            pb = rig.pose.bones[side_name(bone, s)]
            con = pb.constraints.get("IK") or pb.constraints.new("IK")
            con.target, con.pole_target = empties
            con.pole_angle = math.radians(pole_angle)
            con.chain_count = 2
            con.use_tail = True


def key_ik(rig, on):
    for pb in rig.pose.bones:
        for con in pb.constraints:
            if con.type == "IK":
                con.influence = 1.0 if on else 0.0
                con.keyframe_insert("influence", frame=0)


def evaluated_coords(obj):
    ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    return [ev.matrix_world @ v.co for v in ev.data.vertices]


def climb_layout(obj, rig, walk):
    """Engine numbers from walk frame 0 (the reference clip), the ladder in authored units, the stroke heights."""
    rig.animation_data.action = walk
    bpy.context.scene.frame_set(0)
    co = evaluated_coords(obj)
    lo = V([min(c[i] for c in co) for i in range(3)])
    hi = V([max(c[i] for c in co) for i in range(3)])
    size = max(hi - lo)
    tile = TILE_WORLD * size / GUARD_SCALE
    rung = tile / RUNGS
    rise = CLIMB_RISE_RUNGS * rung
    stroke = rise * CLIMB_DUTY
    # Holds at lo.z + (k + 0.5) * rung. A hand at phase h grips at the top of its stroke when the body has risen
    # (1 - h) * rise, so it lands on a hold when reach - lo.z - (k + 0.5) * rung = h * rise (mod rise).
    hand_high = CLIMB_HAND["reach"]
    h = ((hand_high - lo.z) / rung - 0.5) / CLIMB_RISE_RUNGS % 1.0
    CLIMB_PHASE.update({"hand.R": h, "hand.L": (h + 0.5) % 1.0})
    # foot.R ends its grip at frame 0 with the sole on the floor (walk frame 0's lowest point).
    foot_high = lo.z + stroke
    CLIMB_LAYOUT.update(size=size, height=hi.z - lo.z, centre_y=(lo.y + hi.y) / 2, floor=lo.z, tile=tile, rung=rung,
                        rise=rise, stroke=stroke, hand_high=hand_high, foot_high=foot_high)
    return CLIMB_LAYOUT


def climb_stroke(u, high):
    """Height and lift (0..1) at limb phase u: grip (slide down by CLIMB_RISE per cycle), then reach up clear of the ladder.
    The reach is a cubic that leaves and lands at the grip speed, so the limb lets go and grips without a jerk."""
    L = CLIMB_LAYOUT
    u %= 1.0
    if u < CLIMB_DUTY:
        return high - L["rise"] * u, 0.0
    t = (u - CLIMB_DUTY) / (1 - CLIMB_DUTY)
    m = -L["rise"] * (1 - CLIMB_DUTY)
    low = high - L["stroke"]
    h00, h10, h01, h11 = 2 * t**3 - 3 * t**2 + 1, t**3 - 2 * t**2 + t, -2 * t**3 + 3 * t**2, t**3 - t**2
    return h00 * low + h10 * m + h01 * high + h11 * m, math.sin(math.pi * t)


def climb_targets(p):
    """Rig-space fist centres and sole points at cycle phase p, with each limb's lift (0..1)."""
    L = CLIMB_LAYOUT
    out = {}
    for s in (1, -1):
        hand, foot = side_name("hand", s), side_name("foot", s)
        z, lift = climb_stroke(p + CLIMB_PHASE[hand], L["hand_high"])
        out[hand] = (V((CLIMB_HAND["x"] * s, CLIMB_GRIP_Y - CLIMB_HAND["lift"] * lift, z)), lift)
        z, lift = climb_stroke(p + CLIMB_PHASE[foot], L["foot_high"])
        out[foot] = (V((CLIMB_FOOT["x"] * s, CLIMB_FOOT["y"] - CLIMB_FOOT["lift"] * lift, z)), lift)
    return out


def fist(rig, s):
    pb = rig.pose.bones[side_name("hand", s)]
    return pb.head + (pb.tail - pb.head).normalized() * 0.06


def sole(rig, s):
    """Ball of the foot on the sole, in pose space (the foot is kept flat, as at rest)."""
    pb = rig.pose.bones[side_name("foot", s)]
    j = joints(s)
    rest = V((j["ankle"].x, j["ankle"].y + 0.10, 0.0))
    return pb.matrix @ pb.bone.matrix_local.inverted() @ rest


def staff_on_back(rig):
    """Sceptre bone matrix for the sceptre slung diagonally across the back, following the chest."""

    def frame(hook, axis):
        y = V(axis).normalized()
        h = (V(hook) - V(hook).dot(y) * y).normalized()
        return Matrix((h, y, h.cross(y))).transposed()

    pb = rig.pose.bones["staff"]
    rest = pb.bone.matrix_local
    axis = V((0.30, 0.0, 1.0)).normalized()  # top over the right shoulder, the hook pointing out to the right
    bottom = V((-0.27, -0.13, 0.36))
    rot = frame(V((1, 0, 0)), axis) @ frame(STAFF["hook"], rest.col[1].to_3d()).inverted()
    placed = rot.to_4x4() @ rest  # rest -> slung, in the standing body's space
    placed.translation = bottom + axis * STAFF["grip"]
    chest = rig.pose.bones["chest"]
    return chest.matrix @ chest.bone.matrix_local.inverted() @ placed


def climb_action(rig, frames):
    """FK part (sway, head looking up the ladder, slung sceptre), then the IK targets solved so the fist centres and
    the soles meet climb_targets, and the feet kept flat."""
    scene = bpy.context.scene
    for f in range(frames + 1):
        p = f / frames
        w = 2 * math.pi * p
        sway = math.sin(w)
        q = pose({}, pelvis=(-4, 0, 3 * sway), spine=(-2, 0, -1.5 * sway), chest=(-2, 0, -2 * sway),
                 neck=(-12, 0, 0), head=(-20, 8 * math.cos(w), 0), hand_L=(-25, 0, 0), hand_R=(-25, 0, 0))
        q["root_loc"] = (0.02 * sway, 0.0, CLIMB_ROOT_Z + 0.01 * math.cos(2 * w))
        key_pose(rig, q, f)
        want = climb_targets(p % 1.0)
        empties = {b: bpy.data.objects["anubis_ik_{}_target".format(b)] for b in want}
        offset = {b: V((0, 0, -0.06)) if b.startswith("hand") else V((0, -0.10, 0.085)) for b in want}
        for _ in range(8):  # the IK aims the wrist / ankle: move it until the fist centre / sole lands on the target
            for b, (pos, lift) in want.items():
                empties[b].location = pos + offset[b]
                empties[b].keyframe_insert("location", frame=f)
            scene.frame_set(f)
            for s in (1, -1):
                for b, got in ((side_name("hand", s), fist(rig, s)), (side_name("foot", s), sole(rig, s))):
                    offset[b] += want[b][0] - got
            for s in (1, -1):  # feet flat on the rungs, the toes dipping a little in the reach
                pb = rig.pose.bones[side_name("foot", s)]
                tilt = Matrix.Rotation(math.radians(-25 * want[side_name("foot", s)][1]), 3, "X")
                m = (tilt @ pb.bone.matrix_local.to_3x3()).to_4x4()
                m.translation = pb.head
                pb.matrix = m
                bpy.context.view_layer.update()
                pb.keyframe_insert("rotation_euler", frame=f)
        pb = rig.pose.bones["staff"]
        pb.matrix = staff_on_back(rig)
        bpy.context.view_layer.update()
        pb.keyframe_insert("rotation_euler", frame=f)
        pb.keyframe_insert("location", frame=f)


def make_climb(rig, obj, walk):
    from bpy_extras import anim_utils

    name, _, frames = CLIMB
    act = bpy.data.actions.get(name)
    if act:
        bpy.data.actions.remove(act)
    L = climb_layout(obj, rig, walk)
    act = bpy.data.actions.new(name)
    act.use_fake_user = True
    rig.animation_data.action = act
    key_ik(rig, True)
    climb_action(rig, frames)
    bag = anim_utils.action_get_channelbag_for_slot(act, rig.animation_data.action_slot)
    for fc in bag.fcurves:
        fc.modifiers.new("CYCLES")
        fc.update()
    print("anubis climb: frames {} R {:.4f} G {:.4f} C {:.4f} H {:.4f} S_ref {:.4f} tile {:.4f} rung {:.4f}".format(
        frames, L["rise"], CLIMB_GRIP_Y, L["centre_y"], L["height"], L["size"], L["tile"], L["rung"]))
    print("anubis climb: climbRise {:.4f} climbGrip {:.4f}".format(L["rise"] / L["height"], (CLIMB_GRIP_Y - L["centre_y"]) / L["height"]))
    return act


def add_review_ladder(coll):
    """Ladder stand-in at the grip plane (rails at x = +-0.1 tile, rungs every tile / 12), sliding down by CLIMB_RISE
    per cycle as the engine lifts the body: gripping fists and feet should stay on it. Review renders only."""
    L = CLIMB_LAYOUT
    b = Builder()
    rail_x, top = 0.1 * L["tile"], 2.6 + L["rise"]
    M = {"wood": common.make_material("anubis_ladder", ("solid", "bronze"), COL)}
    for s in (1, -1):
        b.add(tube([((rail_x * s, CLIMB_GRIP_Y + 0.02, L["floor"] - L["rise"]), 0.012, 0.012), ((rail_x * s, CLIMB_GRIP_Y + 0.02, top), 0.012, 0.012)],
                   sub=1, n=8), M["wood"], "root")
    k = 0
    while L["floor"] + (k + 0.5) * L["rung"] - L["rise"] < top:
        z = L["floor"] + (k + 0.5) * L["rung"] - L["rise"]
        b.add(tube([((-rail_x - 0.02, CLIMB_GRIP_Y, z), 0.01, 0.01), ((rail_x + 0.02, CLIMB_GRIP_Y, z), 0.01, 0.01)], sub=1, n=8), M["wood"], "root")
        k += 1
    lad = common.finish_mesh(b, coll, "anubis_review_ladder")
    fc = lad.driver_add("location", 2)
    fc.driver.type = "SCRIPTED"
    fc.driver.expression = "-{:.6f} * frame / {}".format(L["rise"], CLIMB[2])
    return lad


def make_actions(rig, obj):
    from bpy_extras import anim_utils

    rig.animation_data_create()
    acts = {}
    for name, poses, cyclic in (("walk", walk_poses(), True), ("attack", attack_poses(), True), ("die", die_poses(), False)):
        act = bpy.data.actions.get("anubis_" + name)
        if act:
            bpy.data.actions.remove(act)
        act = bpy.data.actions.new("anubis_" + name)
        act.use_fake_user = True
        rig.animation_data.action = act
        key_ik(rig, False)
        for f, p in poses:
            key_pose(rig, p, f)
        if name == "die":
            drop_staff(rig)
        if cyclic:
            bag = anim_utils.action_get_channelbag_for_slot(act, rig.animation_data.action_slot)
            for fc in bag.fcurves:
                fc.modifiers.new("CYCLES")
                fc.update()
        acts[name] = act
    acts["climb"] = make_climb(rig, obj, acts["walk"])
    return acts


# ---------------------------------------------------------------- entry points


def build(bake=True, tex_path=None):
    """Build mesh + rig + actions. bake=False skips the (slow) texture bake for quick iteration."""
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
    build_clothes(b, M)
    build_props(b, M, rig)
    obj = common.finish_mesh(b, coll, "anubis_new")
    common.uv_unwrap(obj, b.tags, {"head": ((0, 0.05, 1.75), 1.8)})
    if bake:
        tex = common.bake_texture(obj, tex_path or os.path.join(bpy.app.tempdir or "/tmp", "anubis_preview.png"), TEX_SIZE, "anubis")
        common.use_baked_material(obj, tex)
    common.rig_object(obj, rig)
    build_ik(rig, coll)
    acts = make_actions(rig, obj)
    if os.environ.get("ANUBIS_LADDER"):
        add_review_ladder(coll)
    rig.animation_data.action = acts["walk"]
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 0, FRAMES - 1
    scene.frame_set(0)
    print("anubis: {} verts, {} tris".format(len(obj.data.vertices), common.tri_count(obj)))
    return obj, rig


def export(models_dir=None, clips=CLIPS):
    """Write models/monsters/anubis{,_att,_die,_climb}.md3 from the built objects."""
    common.export_files(bpy.data.objects["anubis_new"], bpy.data.objects["anubis_rig"], "anubis", clips, "monsters", models_dir)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    tex_dir = os.path.join(REPO, "textures", "monsters")
    if "--boss-texture" in args:
        obj, _ = build(bake=False)
        materials(COL_BOSS)
        common.bake_texture(obj, os.path.join(tex_dir, "anubis_boss.png"), TEX_SIZE, "anubis_boss")
    elif "--export-climb" in args:
        build(bake=False)
        export(clips=[CLIMB])
    else:
        build(tex_path=os.path.join(tex_dir, "anubis.png") if "--export" in args else None)
    if "--export" in args:
        export()
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "anubis.blend"))
