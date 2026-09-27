"""Procedural tomb rat (plus the giant rat texture): mesh, rig, baked textures and the three .md3 animations.

    MCP:  p = ".../tools/blender/models/rat.py"; g = {"__file__": p, "__name__": "rat"}
          exec(open(p).read(), g); g["build"]()          # then g["export"]()
    CLI:  blender -b --python tools/blender/models/rat.py -- [--export]

Blender space: Z up, the rat faces +Y (game uses rotA = 180), its right side is +X.
Rigid parts follow one bone each; the body blends between a hips and a chest bone. Every frame
is posed procedurally: body and head as rigid transforms, the four legs by two-bone IK towards
foot targets (planted on the floor or in body space), the tail as an FK chain that drapes on the
floor. The whole pose is then lifted so the lowest vertex (tail excluded, it is clamped on its own)
touches the floor, computed with a numpy copy of the skinning.
Two textures share one UV layout: rat.png (brown-grey) and rat_giant.png (near-black, mangy, red eyes).
Set RAT_TEX=giant to show the giant texture on the built object (review renders with --bake).
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
from common import REPO, Builder, chain_weights, ellipsoid, smoothstep, tube  # noqa: E402

COLL = "rat_new"
CLIPS = [("rat_walk", "", 24), ("rat_attack", "_att", 24), ("rat_die", "_die", 30)]
TEX_SIZE = 1024
REVIEW_VIEW = {"target": (0, -0.3, 0.3), "ortho": 2.4, "res": (560, 340)}

COL = {
    "fur_belly": (0.42, 0.37, 0.31),
    "fur_flank": (0.22, 0.17, 0.13),
    "fur_back": (0.13, 0.10, 0.08),
    "fur_dark": (0.07, 0.055, 0.045),
    "patch": (0.05, 0.04, 0.035),
    "scar": (0.40, 0.25, 0.24),
    "skin": (0.78, 0.42, 0.40),
    "skin_dark": (0.52, 0.26, 0.25),
    "ear_in": (0.80, 0.45, 0.45),
    "nose": (0.85, 0.40, 0.42),
    "tail": (0.66, 0.43, 0.40),
    "tail_ring": (0.42, 0.25, 0.24),
    "eye": (0.005, 0.004, 0.004),
    "glint": (1.0, 1.0, 1.0),
    "tooth": (0.85, 0.62, 0.20),
    "mouth": (0.42, 0.07, 0.08),
    "whisker": (0.55, 0.52, 0.48),
}
COL_GIANT = dict(COL, **{
    "fur_belly": (0.13, 0.11, 0.10),
    "fur_flank": (0.07, 0.058, 0.05),
    "fur_back": (0.042, 0.035, 0.03),
    "fur_dark": (0.015, 0.012, 0.01),
    "patch": (0.008, 0.006, 0.005),
    "scar": (0.20, 0.11, 0.10),
    "skin": (0.55, 0.30, 0.29),
    "skin_dark": (0.34, 0.17, 0.16),
    "ear_in": (0.50, 0.24, 0.24),
    "nose": (0.50, 0.22, 0.24),
    "tail": (0.45, 0.28, 0.27),
    "tail_ring": (0.24, 0.13, 0.12),
    "eye": (0.55, 0.02, 0.01),
    "tooth": (0.70, 0.50, 0.14),
    "whisker": (0.30, 0.28, 0.26),
})

UP, FWD = V((0, 0, 1)), V((0, 1, 0))
DZ = -0.045  # body and head sit this much lower than their literal coordinates
PIVOT = V((0, -0.12, 0.30 + DZ))  # hips bone: body rotations happen around this point
MID = V((0, 0.04, 0.30 + DZ))  # chest bone: spine flex
NECK = V((0, 0.28, 0.31 + DZ))
JAW_HINGE = V((0, 0.48, 0.235 + DZ))
TAIL_BASE = V((0, -0.52, 0.26 + DZ))
NT, TAIL_LEN = 12, 0.9  # tail bones, length
FOOT_Z = 0.03  # ankle / wrist height above the floor


def leg_defs():
    """Rest layout of the four legs (ankle / wrist = foot). s = +1 right, -1 left. Trot: diagonal pairs."""
    legs = []
    for s in (1, -1):
        sfx = ".R" if s > 0 else ".L"
        legs.append(dict(name="H" + sfx, s=s, hip=V((0.12 * s, -0.30, 0.225)), foot=V((0.15 * s, -0.37, FOOT_Z)), a=0.14, b=0.15,
                         bend=V((0, 1, 0.3)), pdir=V((0.12 * s, 1, 0)), plen=0.15, curled=V((0.14 * s, -0.22, 0.08)),
                         phase=0.0 if s > 0 else 0.5))
        legs.append(dict(name="F" + sfx, s=s, hip=V((0.08 * s, 0.17, 0.19)), foot=V((0.12 * s, 0.25, FOOT_Z)), a=0.10, b=0.10,
                         bend=V((0, -1, 0)), pdir=V((0.15 * s, 1, -0.05)), plen=0.075, curled=V((0.10 * s, 0.30, 0.10)),
                         phase=0.5 if s > 0 else 0.0))
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
    """Two-bone IK: knee position, the leg plane normal and the reached foot position."""
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


def flat(v):
    return V((v.x, v.y, 0)).normalized()


# ---------------------------------------------------------------- materials


def socket(sockets, ident):
    return next(s for s in sockets if s.identifier == ident)


def fur_material(pal, mangy):
    """Dark back fading to a pale belly (rest-pose normal z), fur streaks along Y, mottling;
    mangy: dark patches and pale scar streaks on top."""
    mat = bpy.data.materials.get("rat_fur") or bpy.data.materials.new("rat_fur")
    nt = mat.node_tree
    for node in list(nt.nodes):
        if node.type not in ("OUTPUT_MATERIAL", "BSDF_PRINCIPLED"):
            nt.nodes.remove(node)
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Roughness"].default_value = 0.8
    new, link = nt.nodes.new, nt.links.new

    geo = new("ShaderNodeNewGeometry")
    sep = new("ShaderNodeSeparateXYZ")
    link(geo.outputs["Normal"], sep.inputs[0])
    tone = new("ShaderNodeValToRGB")
    els = tone.color_ramp.elements
    els[0].position, els[0].color = 0.2, (*pal["fur_belly"], 1)
    els[1].position, els[1].color = 0.95, (*pal["fur_dark"], 1)
    els.new(0.48).color = (*pal["fur_flank"], 1)
    els.new(0.72).color = (*pal["fur_back"], 1)
    remap = new("ShaderNodeMath")
    remap.operation = "MULTIPLY_ADD"
    remap.inputs[1].default_value = 0.5
    remap.inputs[2].default_value = 0.5
    link(sep.outputs["Z"], remap.inputs[0])
    link(remap.outputs[0], tone.inputs["Fac"])

    coord = new("ShaderNodeTexCoord")

    def noise(scale, detail, lo, hi, stretch=(1, 1, 1), rough=0.5):
        mp = new("ShaderNodeMapping")
        mp.inputs["Scale"].default_value = stretch
        link(coord.outputs["Object"], mp.inputs["Vector"])
        tex = new("ShaderNodeTexNoise")
        tex.inputs["Scale"].default_value = scale
        tex.inputs["Detail"].default_value = detail
        tex.inputs["Roughness"].default_value = rough
        link(mp.outputs["Vector"], tex.inputs["Vector"])
        ramp = new("ShaderNodeValToRGB")
        re = ramp.color_ramp.elements
        re[0].position, re[0].color = lo[0], (lo[1],) * 3 + (1,)
        re[1].position, re[1].color = hi[0], (hi[1],) * 3 + (1,)
        link(tex.outputs["Fac"], ramp.inputs["Fac"])
        return ramp

    factors = [noise(3.0, 6.0, (0.3, 0.6), (0.7, 1.35), stretch=(14, 2.5, 14), rough=0.7),  # streaks
               noise(5.0, 3.0, (0.35, 0.8), (0.65, 1.15))]  # mottling
    if mangy:
        factors.append(noise(3.2, 4.0, (0.5, 1.0), (0.58, 0.4)))  # dark patches
    out = tone.outputs["Color"]
    for f in factors:
        m = new("ShaderNodeVectorMath")
        m.operation = "MULTIPLY"
        link(out, m.inputs[0])
        link(f.outputs["Color"], m.inputs[1])
        out = m.outputs[0]
    if mangy:
        # Scars: thin distorted wave lines, only where a coarse mask is high.
        wave = new("ShaderNodeTexWave")
        wave.inputs["Scale"].default_value = 3.0
        wave.inputs["Distortion"].default_value = 4.0
        wave.inputs["Detail"].default_value = 2.0
        link(coord.outputs["Object"], wave.inputs["Vector"])
        line = new("ShaderNodeValToRGB")
        le = line.color_ramp.elements
        le[0].position, le[0].color = 0.93, (0, 0, 0, 1)
        le[1].position, le[1].color = 0.98, (1, 1, 1, 1)
        link(wave.outputs["Fac"], line.inputs["Fac"])
        mask = noise(1.6, 2.0, (0.55, 0.0), (0.62, 1.0))
        both = new("ShaderNodeMath")
        both.operation = "MULTIPLY"
        link(line.outputs["Color"], both.inputs[0])
        link(mask.outputs["Color"], both.inputs[1])
        mix = new("ShaderNodeMix")
        mix.data_type = "RGBA"
        link(both.outputs[0], socket(mix.inputs, "Factor_Float"))
        link(out, socket(mix.inputs, "A_Color"))
        socket(mix.inputs, "B_Color").default_value = (*pal["scar"], 1)
        out = socket(mix.outputs, "Result_Color")
    link(out, bsdf.inputs["Base Color"])
    return mat


def materials(pal, mangy=False):
    specs = {
        "skin": ("solid", "skin"),
        "paw": ("stripes", "v", 0.16, [(3, "skin_dark"), (4, "skin"), (3, "skin")], "LINEAR"),
        "ear_in": ("solid", "ear_in"),
        "nose": ("solid", "nose"),
        "tail": ("stripes", "v", 0.028, [(4, "tail"), (1, "tail_ring")], "LINEAR"),
        "eye": ("solid", "eye"),
        "glint": ("solid", "glint"),
        "tooth": ("solid", "tooth"),
        "mouth": ("solid", "mouth"),
        "whisker": ("solid", "whisker"),
    }
    M = {name: common.make_material("rat_" + name, spec, pal) for name, spec in specs.items()}
    M["fur"] = fur_material(pal, mangy)
    return M


# ---------------------------------------------------------------- parts


def lift(geo):
    return common.transform(geo, lambda v: v + V((0, 0, DZ)))


def flat_belly(th, a):
    sn = math.sin(th)
    return 1.0 if sn >= 0 else 1.0 - 0.18 * sn * sn


def build_body(b, M):
    """One loft from the rump to the nose tip, blended over the hips, chest and head bones."""
    keys = [((0, -0.585, 0.27), 0.05, 0.05), ((0, -0.53, 0.285), 0.14, 0.15), ((0, -0.42, 0.30), 0.195, 0.20),
            ((0, -0.26, 0.31), 0.215, 0.215), ((0, -0.08, 0.30), 0.20, 0.20), ((0, 0.08, 0.29), 0.165, 0.175),
            ((0, 0.20, 0.295), 0.135, 0.15), ((0, 0.32, 0.315), 0.118, 0.123), ((0, 0.43, 0.315), 0.104, 0.10),
            ((0, 0.53, 0.29), 0.076, 0.072), ((0, 0.62, 0.265), 0.047, 0.046), ((0, 0.68, 0.25), 0.024, 0.024)]
    w = chain_weights([V((0, -0.7, 0.26)), MID, NECK, V((0, 0.8, 0.22))], ["hips", "chest", "head"], blend=0.1)
    first = len(b.verts)
    b.add(lift(tube(keys, sub=3, n=24, ref=UP, shape=flat_belly, bulge=(0.02, 0.012))), M["fur"], w)
    b.tags[first:] = ["head" if v.y > 0.3 else "body" for v in b.verts[first:]]


def build_head(b, M):
    b.add(lift(ellipsoid((0, 0.688, 0.255), (0.03, 0.026, 0.026), n=10, rings=5, axis=FWD, ref=UP)), M["nose"], "head")
    # Mouth inside, shows when the jaw opens.
    b.add(lift(ellipsoid((0, 0.56, 0.252), (0.035, 0.024, 0.075), n=10, rings=6, axis=FWD, ref=UP)), M["mouth"], "head")
    for s in (1, -1):
        # Beady eye with a glint.
        ec = V((0.074 * s, 0.47, 0.345))
        b.add(lift(ellipsoid(ec, (0.03, 0.028, 0.03), n=12, rings=6, axis=V((s, 0.3, 0.2)), ref=UP)), M["eye"], "head")
        gl = ec + V((0.62 * s, 0.45, 0.62)).normalized() * 0.027
        b.add(lift(ellipsoid(gl, (0.009, 0.009, 0.006), n=6, rings=4, axis=V((s, 0.4, 0.5)), ref=UP)), M["glint"], "head")
        # Big round ear: fur back, pink cup in front.
        nrm = V((0.5 * s, 0.85, 0.1)).normalized()
        c = V((0.11 * s, 0.35, 0.455))
        b.add(lift(ellipsoid(c, (0.10, 0.095, 0.017), n=16, rings=6, axis=nrm, ref=UP)), M["fur"], "head")
        b.add(lift(ellipsoid(c + nrm * 0.011, (0.079, 0.075, 0.008), n=14, rings=4, axis=nrm, ref=UP)), M["ear_in"], "head")
        # Upper incisor.
        base = V((0.011 * s, 0.674, 0.245))
        d = V((0, -0.15, -1)).normalized()
        b.add(lift(tube([(base, 0.011, 0.007), (base + d * 0.04, 0.0105, 0.0065), (base + d * 0.075, 0.009, 0.004)], sub=1, n=6, ref=FWD)),
              M["tooth"], "head")
        # Whiskers.
        root = V((0.03 * s, 0.655, 0.255))
        for dz, fw in ((0.12, 0.45), (-0.02, 0.3), (-0.16, 0.5)):
            d = V((s, fw, dz)).normalized()
            end = root + d * 0.2 + V((0, 0, -0.02))
            b.add(lift(tube([(root, 0.004, 0.004), (root + d * 0.1, 0.003, 0.003), (end, 0.0012, 0.0012)], sub=2, n=4, ref=UP, caps=(False, False))),
                  M["whisker"], "head")
    # Lower jaw with the lower incisors.
    jaw = [((0, 0.45, 0.262), 0.06, 0.04), ((0, 0.555, 0.242), 0.043, 0.027), ((0, 0.64, 0.23), 0.022, 0.015)]
    b.add(lift(tube(jaw, sub=2, n=12, ref=UP, bulge=(0.01, 0.008))), M["fur"], "jaw")
    for s in (1, -1):
        base = V((0.009 * s, 0.645, 0.218))
        d = V((0, 0.4, 1)).normalized()
        b.add(lift(tube([(base, 0.009, 0.006), (base + d * 0.025, 0.0085, 0.005), (base + d * 0.045, 0.007, 0.003)], sub=1, n=6, ref=FWD)),
              M["tooth"], "jaw")


def rest_leg(leg):
    return solve_leg(leg["hip"], leg["foot"], leg["a"], leg["b"], leg["bend"])


def build_legs(b, M):
    for leg in LEGS:
        n, s, hip = leg["name"], leg["s"], leg["hip"]
        knee, normal, foot = rest_leg(leg)
        fd, td = (knee - hip).normalized(), (foot - knee).normalized()
        if n[0] == "H":
            upper = [(hip - fd * 0.03, 0.09, 0.095), (hip + fd * leg["a"] * 0.45, 0.075, 0.075), (knee, 0.048, 0.048)]
            lower = [(knee - td * 0.01, 0.042, 0.042), (knee + td * leg["b"] * 0.5, 0.032, 0.034), (foot, 0.022, 0.024)]
            pw, ph, toe = 0.028, 0.017, 0.011
        else:
            upper = [(hip - fd * 0.04, 0.05, 0.05), (hip + fd * leg["a"] * 0.5, 0.045, 0.045), (knee, 0.036, 0.036)]
            lower = [(knee - td * 0.01, 0.034, 0.034), (knee + td * leg["b"] * 0.5, 0.026, 0.026), (foot, 0.018, 0.018)]
            pw, ph, toe = 0.02, 0.014, 0.009
        b.add(tube(upper, sub=2, n=12, ref=normal, bulge=(0.0, 0.01)), M["fur"], "upper" + n)
        b.add(ellipsoid(knee, (upper[-1][1],) * 3, n=10, rings=5), M["fur"], "lower" + n)
        b.add(tube(lower, sub=2, n=10, ref=normal, bulge=(0.0, 0.01)), M["fur"], "lower" + n)
        # Pink paw lying on the floor, toes spread at the front.
        pd = leg["pdir"].normalized()
        side = pd.cross(UP).normalized()  # to the right when looking along pd
        tip = foot + pd * leg["plen"]
        paw = [(foot - pd * 0.02, pw * 0.8, ph), (foot + pd * leg["plen"] * 0.5, pw, ph * 0.85), (tip, pw * 0.9, ph * 0.6)]
        b.add(tube(paw, sub=2, n=10, ref=UP, bulge=(0.01, 0.005)), M["paw"], "paw" + n)
        for k in range(4):
            off = (k - 1.5) / 1.5
            c = tip + side * off * pw * 0.9 + pd * (0.012 - 0.008 * abs(off)) + V((0, 0, -ph * 0.25))
            d = (pd + side * off * 0.5).normalized()
            b.add(ellipsoid(c, (toe, toe * 0.8, toe * 1.5), n=6, rings=4, axis=d, ref=UP), M["skin"], "paw" + n)


def tail_radius(u):
    return 0.042 * (1 - u) ** 0.9 + 0.007 * u


def build_tail(b, M, joints):
    keys = [(j, tail_radius(k / NT), tail_radius(k / NT)) for k, j in enumerate(joints)]
    bones = ["tail%02d" % k for k in range(NT)]
    b.add(tube(keys, sub=3, n=8, ref=UP, bulge=(0.0, 0.008)), M["tail"], chain_weights(joints, bones, blend=0.03), tag="tail")


# ---------------------------------------------------------------- tail


def tail_dirs(p):
    """Body-space direction per tail segment."""
    out = []
    for k in range(NT):
        u = k / (NT - 1)
        el = -40 + 48 * u ** 0.7 + p["tlift"] * (1 - u) ** 1.5
        yaw = p["tsway"] * math.sin(2 * math.pi * (p["tphase"] - 0.08 * k)) * (0.25 + 0.75 * u) + p["tcurl"] * u
        el, yaw = math.radians(el), math.radians(yaw)
        out.append(V((math.sin(yaw) * math.cos(el), -math.cos(yaw) * math.cos(el), math.sin(el))))
    return out


def tail_points(T, p):
    """Armature-space joints of the tail for hips matrix T; segments below the floor are laid onto it."""
    R = T.to_3x3()
    pts = [T @ TAIL_BASE]
    seg = TAIL_LEN / NT
    for k, d in enumerate(tail_dirs(p)):
        d = R @ d
        if p["tflop"] > 0:  # limp tail: sags towards the floor instead of following the body
            h = V((d.x, d.y, 0))
            h = h.normalized() if h.length > 1e-4 else V((0, -1, 0))
            d = d.lerp((h + V((0, 0, -0.4))).normalized(), p["tflop"]).normalized()
        nxt = pts[-1] + d * seg
        floor = tail_radius((k + 1) / NT) + 0.004
        if nxt.z < floor:
            dz = floor - pts[-1].z
            if abs(dz) < seg:
                h = V((d.x, d.y, 0))
                h = h.normalized() if h.length > 1e-4 else V((0, -1, 0))
                nxt = pts[-1] + h * math.sqrt(seg * seg - dz * dz) + V((0, 0, dz))
            else:
                nxt.z = floor
        pts.append(nxt)
    return pts


# ---------------------------------------------------------------- rig


def build_rig(coll, tail):
    arm = bpy.data.armatures.new("rat_rig")
    rig = bpy.data.objects.new("rat_rig", arm)
    coll.objects.link(rig)
    arm.display_type = "STICK"
    rig.show_in_front = True
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    eb = arm.edit_bones

    def bone(name, head, tail_, x_axis, parent):
        e = eb.new(name)
        e.head, e.tail = V(head), V(tail_)
        e.align_roll(V(x_axis).cross((e.tail - e.head).normalized()))
        if parent:
            e.parent = eb[parent]

    X = (1, 0, 0)
    bone("root", (0, 0, 0), (0, 0.3, 0), X, None)
    bone("hips", PIVOT, PIVOT + V((0, 0.2, 0)), X, "root")
    bone("chest", MID, MID + V((0, 0.2, 0)), X, "hips")
    bone("head", NECK, NECK + V((0, 0.3, 0)), X, "chest")
    bone("jaw", JAW_HINGE, JAW_HINGE + V((0, 0.15, 0)), X, "head")
    for k in range(NT):
        bone("tail%02d" % k, tail[k], tail[k + 1], X, "hips" if k == 0 else "tail%02d" % (k - 1))
    for leg in LEGS:
        knee, normal, foot = rest_leg(leg)
        n = leg["name"]
        bone("upper" + n, leg["hip"], knee, normal, "hips" if n[0] == "H" else "chest")
        bone("lower" + n, knee, foot, normal, "upper" + n)
        bone("paw" + n, foot, foot + leg["pdir"].normalized() * leg["plen"], X, "lower" + n)
    bpy.ops.object.mode_set(mode="OBJECT")
    for pb in rig.pose.bones:
        pb.rotation_mode = "QUATERNION"
    return rig


# ---------------------------------------------------------------- animation

REST = dict(x=0.0, y=0.0, z=0.0, pitch=0.0, roll=0.0, yaw=0.0, flex_p=0.0, flex_y=0.0, hp=0.0, hy=0.0, hr=0.0, jaw=0.0,
            tsway=0.0, tphase=0.0, tlift=0.0, tcurl=0.0, tflop=0.0,
            plant=1.0, fplant=1.0, fup=0.0, ffwd=0.0, curl=0.0, flail=0.0, fphase=0.0)
STRIDE, STEP_H, CYCLES = 0.2, 0.06, 2  # gait cycles per walk clip


def gait(leg, t):
    """Walk: foot position (armature space) and swing amount for gait phase t."""
    ph = (t + leg["phase"]) % 1.0
    base = leg["foot"]
    if ph < 0.5:  # stance: foot slides back under the moving body
        return base + V((0, STRIDE * (0.5 - ph / 0.5), 0)), 0.0
    k = (ph - 0.5) / 0.5
    lift = math.sin(math.pi * k)
    return base + V((0, STRIDE * (smoothstep(0, 1, k) - 0.5), STEP_H * lift)), lift


def walk_params(t):
    p = dict(REST)
    w = 2 * math.pi * CYCLES * t
    p.update(z=0.01 * math.cos(2 * w), roll=2.0 * math.sin(w), yaw=2.0 * math.cos(w), pitch=1.0 * math.sin(2 * w),
             flex_y=-4 * math.cos(w), flex_p=1.5 * math.sin(2 * w + 0.6), hp=3 * math.sin(2 * w + 1.2), hy=3 * math.cos(w + 0.5),
             tsway=14, tphase=t)
    p["feet"] = {leg["name"]: gait(leg, CYCLES * t) for leg in LEGS}
    return p


READY = walk_params(0.0)
READY_FEET = {k: v[0].copy() for k, v in READY["feet"].items()}


def leg_targets(p, mats_by_leg):
    """Foot target (armature space) and swing amount per leg for non-walk frames."""
    out = {}
    for leg in LEGS:
        n, s = leg["name"], leg["s"]
        front = n[0] == "F"
        planted = READY_FEET[n]
        free = planted.copy()
        if front:
            free += V((0, p["ffwd"], p["fup"]))
        free = free.lerp(leg["curled"], p["curl"])
        ph = 2 * math.pi * (p["fphase"] + {"F": 0.0, "H": 0.4}[n[0]] + (0.23 if s > 0 else 0.0))
        free += V((0.10 * s * math.sin(ph * 1.7), 0.14 * math.cos(ph * 1.3), 0.12 * math.sin(ph + 1))) * p["flail"]
        w = p["fplant"] if front else p["plant"]
        target = planted.lerp(mats_by_leg[n] @ free, 1 - w)
        if abs(p["roll"]) < 45:
            target.z = max(target.z, FOOT_Z)
        out[n] = (target, 1 - w)
    return out


def body_mats(p):
    T = Matrix.Translation(V((p["x"], p["y"], p["z"]))) @ rot_about(PIVOT, euler(p["pitch"], p["roll"], p["yaw"]))
    C = T @ rot_about(MID, euler(p["flex_p"], 0, p["flex_y"]))
    H = C @ rot_about(NECK, euler(p["hp"], p["hr"], p["hy"]))
    J = H @ rot_about(JAW_HINGE, euler(-p["jaw"]))
    return T, C, H, J


def pose_matrices(rest, p):
    """Armature-space pose matrices of every bone except the tail."""
    T, C, H, J = body_mats(p)
    mats = {"root": rest["root"], "hips": T @ rest["hips"], "chest": C @ rest["chest"], "head": H @ rest["head"], "jaw": J @ rest["jaw"]}
    by_leg = {leg["name"]: (T if leg["name"][0] == "H" else C) for leg in LEGS}
    feet = p["feet"] if "feet" in p else leg_targets(p, by_leg)
    for leg in LEGS:
        n = leg["name"]
        B = by_leg[n]
        R = B.to_3x3()
        knee0, normal0, foot0 = rest_leg(leg)
        target, swing = feet[n]
        hip = B @ leg["hip"]
        knee, normal, foot = solve_leg(hip, target, leg["a"], leg["b"], R @ leg["bend"])
        mats["upper" + n] = frame(hip, knee - hip, normal) @ frame(leg["hip"], knee0 - leg["hip"], normal0).inverted() @ rest["upper" + n]
        mats["lower" + n] = frame(knee, foot - knee, normal) @ frame(knee0, foot0 - knee0, normal0).inverted() @ rest["lower" + n]
        pd0 = leg["pdir"].normalized()
        body_pd = R @ pd0
        hang = (body_pd + (foot - knee).normalized() * 0.6).normalized()
        k = min(1.0, swing * 1.5)
        pd = flat(body_pd).lerp(hang, k).normalized()
        xh = flat(R @ V((1, 0, 0))).lerp(R @ V((1, 0, 0)), k)
        mats["paw" + n] = frame(foot, pd, xh) @ frame(foot0, pd0, V((1, 0, 0))).inverted() @ rest["paw" + n]
    return mats, T


class Skin:
    """numpy linear blend skinning of the non-tail vertices (for the floor contact)."""

    def __init__(self, b, rest):
        import numpy as np

        self.np = np
        keep = [i for i, t in enumerate(b.tags) if t != "tail"]
        self.bones = sorted({n for i in keep for n in b.weights[i]})
        col = {n: j for j, n in enumerate(self.bones)}
        self.P = np.array([[*b.verts[i], 1.0] for i in keep])
        self.W = np.zeros((len(keep), len(self.bones)))
        for r, i in enumerate(keep):
            tot = sum(b.weights[i].values())
            for n, w in b.weights[i].items():
                self.W[r, col[n]] = w / tot
        self.inv = {n: rest[n].inverted() for n in self.bones}

    def lowest(self, mats):
        np = self.np
        z = np.zeros(len(self.P))
        for j, n in enumerate(self.bones):
            row = np.array((mats[n] @ self.inv[n])[2])
            z += self.W[:, j] * (self.P @ row)
        return float(z.min())


def key_frame(rig, skin, p, frame_no):
    bones = rig.data.bones
    rest = {b.name: b.matrix_local.copy() for b in bones}
    mats, T = pose_matrices(rest, p)
    S = Matrix.Translation(V((0, 0, -skin.lowest(mats))))
    mats = {n: S @ m for n, m in mats.items()}
    pts = tail_points(S @ T, p)
    xh = (S @ T).to_3x3() @ V((1, 0, 0))
    for k in range(NT):
        n = "tail%02d" % k
        h0, t0 = bones[n].head_local, bones[n].tail_local
        mats[n] = frame(pts[k], pts[k + 1] - pts[k], xh) @ frame(h0, t0 - h0, V((1, 0, 0))).inverted() @ rest[n]
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
    (4, dict(y=-0.04, pitch=-3, flex_p=-5, hp=-4, tsway=10)),
    (8, dict(y=-0.05, pitch=7, flex_p=20, hp=-6, jaw=18, fplant=0, fup=0.10, ffwd=0.02, tlift=12, tsway=8)),
    (11, dict(y=0.13, pitch=-3, flex_p=-4, hp=-10, jaw=34, fplant=0, fup=0.05, ffwd=0.12, tlift=6, tsway=8)),
    (13, dict(y=0.14, pitch=-4, flex_p=-6, hp=-14, jaw=0, fplant=0.2, fup=0.0, ffwd=0.12, tsway=8)),
    (15, dict(y=0.13, pitch=-3, flex_p=-5, hp=-12, hy=14, hr=10, jaw=4, fplant=0.6, ffwd=0.1, tsway=8)),
    (17, dict(y=0.12, pitch=-3, flex_p=-4, hp=-10, hy=-12, hr=-10, jaw=4, fplant=0.8, tsway=10)),
    (20, dict(y=0.05, hp=-4, hy=3, fplant=1, tsway=12)),
    (24, {}),
]

DIE = [
    (0, {}),
    (3, dict(flex_p=16, pitch=5, hp=18, jaw=35, fplant=0.3, fup=0.1, tlift=18, tsway=18)),
    (6, dict(roll=-25, flex_p=6, flex_y=14, hp=8, hr=-15, jaw=25, plant=0.5, fplant=0, fup=0.1, flail=0.5, fphase=0.3, tsway=16, tcurl=10)),
    (9, dict(tflop=0.4, roll=-75, x=-0.08, flex_y=10, hp=2, hr=-20, jaw=30, plant=0, fplant=0, flail=0.9, fphase=0.8, tsway=14, tcurl=20)),
    (12, dict(tflop=0.9, roll=-100, x=-0.18, flex_p=-8, flex_y=6, hp=-6, hr=-15, jaw=25, plant=0, fplant=0, flail=1.0, fphase=1.4, tsway=10, tcurl=30)),
    (14, dict(tflop=1.0, roll=-112, x=-0.2, flex_p=-10, hp=-8, hr=-10, jaw=22, plant=0, fplant=0, flail=0.9, fphase=1.8, tsway=8, tcurl=35)),
    (17, dict(tflop=1.0, roll=-111, x=-0.2, flex_p=-8, hp=-10, hr=-10, jaw=20, plant=0, fplant=0, flail=0.8, fphase=2.5, tsway=6, tcurl=38, curl=0.1)),
    (20, dict(tflop=1.0, roll=-110, x=-0.2, flex_p=-8, hp=-10, hr=-10, jaw=18, plant=0, fplant=0, flail=0.5, fphase=3.1, tsway=3, tcurl=40, curl=0.2)),
    (23, dict(tflop=1.0, roll=-110, x=-0.2, flex_p=-8, hp=-12, hr=-10, jaw=16, plant=0, fplant=0, flail=0.25, fphase=3.6, tsway=1, tcurl=40, curl=0.35)),
    (26, dict(tflop=1.0, roll=-110, x=-0.2, flex_p=-8, hp=-12, hr=-10, jaw=15, plant=0, fplant=0, flail=0.08, fphase=4.0, tsway=0, tcurl=40, curl=0.45)),
    (29, dict(tflop=1.0, roll=-110, x=-0.2, flex_p=-8, hp=-12, hr=-10, jaw=15, plant=0, fplant=0, flail=0.0, fphase=4.2, tsway=0, tcurl=40, curl=0.5)),
]


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
        last = frames if name != "rat_die" else frames - 1  # loops key frame N = frame 0
        for f in range(last + 1):
            if name == "rat_walk":
                p = walk_params(f / frames)
            else:
                p = interpolate(ATTACK if name == "rat_attack" else DIE, f)
                p["tphase"] = f / CLIPS[0][2]  # keeps the tail sway going from where the walk's frame 0 is
            key_frame(rig, skin, p, f)
        acts[name] = act
    return acts


# ---------------------------------------------------------------- entry points


def build(bake=True, tex_path=None, giant_path=None):
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    M = materials(COL)
    tail = tail_points(Matrix(), dict(REST))
    rig = build_rig(coll, tail)
    b = Builder()
    build_body(b, M)
    first = len(b.verts)
    build_head(b, M)
    b.tags[first:] = ["head"] * (len(b.verts) - first)
    build_legs(b, M)
    build_tail(b, M, tail)
    obj = common.finish_mesh(b, coll, "rat_new")
    common.uv_unwrap(obj, b.tags, {"head": (V((0, 0.45, 0.3)), 1.6)})
    if bake:
        tmp = bpy.app.tempdir or "/tmp"
        tex = common.bake_texture(obj, tex_path or os.path.join(tmp, "rat_preview.png"), TEX_SIZE, "rat", ao_distance=0.12)
        materials(COL_GIANT, mangy=True)
        giant = common.bake_texture(obj, giant_path or os.path.join(tmp, "rat_giant_preview.png"), TEX_SIZE, "rat_giant", ao_distance=0.12)
        common.use_baked_material(obj, giant if os.environ.get("RAT_TEX") == "giant" else tex)
    common.rig_object(obj, rig)
    rest = {bn.name: bn.matrix_local.copy() for bn in rig.data.bones}
    acts = make_actions(rig, Skin(b, rest))
    rig.animation_data.action = acts["rat_walk"]
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 0, CLIPS[0][2] - 1
    scene.frame_set(0)
    print("rat: {} verts, {} tris".format(len(obj.data.vertices), common.tri_count(obj)))
    return obj, rig


def export(models_dir=None):
    common.export_files(bpy.data.objects["rat_new"], bpy.data.objects["rat_rig"], "rat", CLIPS, "monsters", models_dir)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    tex_dir = os.path.join(REPO, "Textures", "monsters")
    if "--export" in args:
        build(tex_path=os.path.join(tex_dir, "rat.png"), giant_path=os.path.join(tex_dir, "rat_giant.png"))
        export()
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "rat.blend"))
    else:
        build()
