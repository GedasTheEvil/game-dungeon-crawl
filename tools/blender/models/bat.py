"""Procedural tomb bat (plus the giant bat texture): mesh, rig, baked textures and the four .md3 animations.

    MCP:  p = ".../tools/blender/models/bat.py"; g = {"__file__": p, "__name__": "bat"}
          exec(open(p).read(), g); g["build"]()          # then g["export"]()
    CLI:  blender -b --python tools/blender/models/bat.py -- [--export] [--boss-texture]

Blender space: Z up, the bat faces +Y (game uses rotA = 180), its right wing is +X. Rest pose = wings spread flat.
Every bone is posed by a deformation matrix D (armature space, D = pose @ rest^-1) built by FK about the rest
joint positions: body -> head -> jaw / ears, body -> upper arm -> forearm -> thumb and fingers 2..5, body -> leg -> foot,
body -> tail. The wing membranes are double-sided grids between the finger bones, the arm and the leg, skinned
with blended weights so they stretch and crumple when the wing folds.
Clips: bat_fly (move, the normalization reference), bat_attack, bat_die (lowest vertex every frame on the level of
fly frame 0's lowest point = the engine's floor, computed with a numpy copy of the skinning) and bat_idle (hanging upside down by the feet, feet fixed).
Three textures share one UV layout: bat.png (brown-grey), bat_giant.png (near-black, red-brown, mangy, red eyes)
and bat_vampire.png (the boss: blue-black, blood-red veins, crimson eyes; bake it alone with --boss-texture).
Set BAT_TEX=giant to show the giant texture on the built object (review renders with --bake).
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

COLL = "bat_new"
CLIPS = [("bat_fly", "", 12), ("bat_attack", "_att", 12), ("bat_die", "_die", 24), ("bat_idle", "_idle", 24)]
TEX_SIZE = 1024
REVIEW_VIEW = {"target": (0, 0, 0), "ortho": 2.3, "res": (560, 340)}

COL = {
    "fur_belly": (0.36, 0.29, 0.22),
    "fur_flank": (0.21, 0.16, 0.12),
    "fur_back": (0.13, 0.10, 0.075),
    "fur_dark": (0.07, 0.055, 0.045),
    "membrane": (0.10, 0.075, 0.065),
    "membrane_vein": (0.05, 0.035, 0.03),
    "bone": (0.14, 0.11, 0.095),
    "skin": (0.30, 0.20, 0.18),
    "ear_in": (0.46, 0.28, 0.26),
    "nose": (0.28, 0.16, 0.15),
    "eye": (0.005, 0.004, 0.004),
    "glint": (1.0, 1.0, 1.0),
    "tooth": (0.88, 0.84, 0.72),
    "mouth": (0.42, 0.07, 0.08),
    "claw": (0.04, 0.035, 0.03),
    "scar": (0.40, 0.25, 0.24),
}
COL_GIANT = dict(COL, **{
    "fur_belly": (0.14, 0.075, 0.06),
    "fur_flank": (0.075, 0.045, 0.035),
    "fur_back": (0.045, 0.03, 0.025),
    "fur_dark": (0.015, 0.011, 0.009),
    "membrane": (0.06, 0.028, 0.024),
    "membrane_vein": (0.025, 0.01, 0.01),
    "bone": (0.08, 0.045, 0.04),
    "skin": (0.20, 0.10, 0.09),
    "ear_in": (0.30, 0.12, 0.11),
    "nose": (0.18, 0.08, 0.07),
    "eye": (0.60, 0.03, 0.015),
    "tooth": (0.78, 0.70, 0.52),
    "scar": (0.26, 0.12, 0.10),
})
# The vampire bat (a boss): sleek blue-black fur over a wine-dark belly, wings of dark leather with blood-red
# veins, crimson eyes, ivory fangs in a red maw.
COL_VAMPIRE = dict(COL, **{
    "fur_belly": (0.16, 0.02, 0.035),
    "fur_flank": (0.05, 0.02, 0.05),
    "fur_back": (0.025, 0.015, 0.035),
    "fur_dark": (0.008, 0.006, 0.012),
    "membrane": (0.07, 0.012, 0.02),
    "membrane_vein": (0.45, 0.02, 0.03),
    "bone": (0.06, 0.025, 0.04),
    "skin": (0.24, 0.05, 0.07),
    "ear_in": (0.40, 0.06, 0.09),
    "nose": (0.22, 0.04, 0.06),
    "eye": (0.95, 0.04, 0.02),
    "tooth": (0.93, 0.89, 0.76),
    "mouth": (0.62, 0.03, 0.04),
    "claw": (0.02, 0.015, 0.02),
})

UP, FWD = V((0, 0, 1)), V((0, 1, 0))
PIVOT = V((0, -0.02, 0.012))  # body bone: body rotations happen around this point
NECK = V((0, 0.11, 0.015))
JAW_HINGE = V((0, 0.19, -0.008))
TAIL_BASE = V((0, -0.19, 0.0))
TAIL_TIP = V((0, -0.34, 0.0))
WING_Z = 0.02  # the spread wings lie in this plane in the rest pose


def side_points(s):
    """Rest joints of one side (s = +1 right, -1 left)."""
    def m(x, y, z=WING_Z):
        return V((x * s, y, z))
    return dict(
        SH=m(0.05, 0.06), EL=m(0.22, 0.02), WR=m(0.48, 0.10), TH=m(0.53, 0.165, 0.035),
        D2=m(0.72, 0.17), D3=m(0.95, 0.03), D4=m(0.82, -0.22), D5=m(0.55, -0.30),
        HIP=m(0.045, -0.13, -0.01), ANK=m(0.105, -0.27, -0.02), TOE=m(0.115, -0.315, -0.025),
        EAR=m(0.04, 0.18, 0.055),
    )


SIDES = {"R": side_points(1), "L": side_points(-1)}
FINGERS = ("d2", "d3", "d4", "d5")
# Fold angle (deg) that lays each finger back along the forearm, and its joint key.
FOLD = {"d2": 172, "d3": 154, "d4": 118, "d5": 82}
TIP = {"d2": "D2", "d3": "D3", "d4": "D4", "d5": "D5"}


# ---------------------------------------------------------------- math helpers


def frame(origin, y, x_hint):
    """4x4 matrix: Y along y, X = x_hint made perpendicular, origin as translation."""
    y = V(y).normalized()
    x = (V(x_hint) - V(x_hint).dot(y) * y).normalized()
    m = Matrix((x, y, x.cross(y))).transposed().to_4x4()
    m.translation = origin
    return m


def rot_about(pivot, rot3):
    return Matrix.Translation(pivot) @ rot3.to_4x4() @ Matrix.Translation(-pivot)


def euler(pitch=0.0, roll=0.0, yaw=0.0):
    """Degrees. pitch: nose up (+), roll: right side down (+), yaw: turn left (+)."""
    return Matrix.Rotation(math.radians(yaw), 3, "Z") @ Matrix.Rotation(math.radians(roll), 3, "Y") @ Matrix.Rotation(math.radians(pitch), 3, "X")


def rx(d):
    return Matrix.Rotation(math.radians(d), 3, "X")


def ry(d):
    return Matrix.Rotation(math.radians(d), 3, "Y")


def rz(d):
    return Matrix.Rotation(math.radians(d), 3, "Z")


# ---------------------------------------------------------------- materials


def socket(sockets, ident):
    return next(s for s in sockets if s.identifier == ident)


def noise_factor(nt, coord, scale, detail, lo, hi, stretch=(1, 1, 1), rough=0.5):
    new, link = nt.nodes.new, nt.links.new
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


def multiply(nt, out, factors):
    for f in factors:
        m = nt.nodes.new("ShaderNodeVectorMath")
        m.operation = "MULTIPLY"
        nt.links.new(out, m.inputs[0])
        nt.links.new(f.outputs["Color"], m.inputs[1])
        out = m.outputs[0]
    return out


def clean(name):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nt = mat.node_tree
    for node in list(nt.nodes):
        if node.type not in ("OUTPUT_MATERIAL", "BSDF_PRINCIPLED"):
            nt.nodes.remove(node)
    return mat, nt


def fur_material(pal, mangy):
    """Dark back fading to a paler belly (rest-pose normal z), streaks, mottling; mangy: dark patches, scars."""
    mat, nt = clean("bat_fur")
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Roughness"].default_value = 0.85
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
    factors = [noise_factor(nt, coord, 7.0, 6.0, (0.3, 0.6), (0.7, 1.35), stretch=(14, 3, 14), rough=0.7),
               noise_factor(nt, coord, 11.0, 3.0, (0.35, 0.8), (0.65, 1.15))]
    if mangy:
        factors.append(noise_factor(nt, coord, 7.0, 4.0, (0.5, 1.0), (0.58, 0.45)))
    out = multiply(nt, tone.outputs["Color"], factors)
    if mangy:
        wave = new("ShaderNodeTexWave")
        wave.inputs["Scale"].default_value = 7.0
        wave.inputs["Distortion"].default_value = 4.0
        wave.inputs["Detail"].default_value = 2.0
        link(coord.outputs["Object"], wave.inputs["Vector"])
        line = new("ShaderNodeValToRGB")
        le = line.color_ramp.elements
        le[0].position, le[0].color = 0.93, (0, 0, 0, 1)
        le[1].position, le[1].color = 0.98, (1, 1, 1, 1)
        link(wave.outputs["Fac"], line.inputs["Fac"])
        mask = noise_factor(nt, coord, 3.5, 2.0, (0.55, 0.0), (0.62, 1.0))
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


def membrane_material(pal, mangy):
    """Leathery wing skin: mottled, with fine dark veins (wave lines) and faint wrinkles."""
    mat, nt = clean("bat_membrane")
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Roughness"].default_value = 0.55
    new, link = nt.nodes.new, nt.links.new
    coord = new("ShaderNodeTexCoord")
    base = new("ShaderNodeRGB")
    base.outputs[0].default_value = (*pal["membrane"], 1)
    factors = [noise_factor(nt, coord, 6.0, 4.0, (0.3, 0.75), (0.7, 1.3)),
               noise_factor(nt, coord, 30.0, 2.0, (0.4, 0.9), (0.6, 1.1), stretch=(1, 4, 1))]
    if mangy:
        factors.append(noise_factor(nt, coord, 5.0, 3.0, (0.52, 1.0), (0.6, 0.55)))
    out = multiply(nt, base.outputs[0], factors)
    wave = new("ShaderNodeTexWave")
    wave.wave_type = "BANDS"
    wave.inputs["Scale"].default_value = 9.0
    wave.inputs["Distortion"].default_value = 6.0
    wave.inputs["Detail"].default_value = 3.0
    link(coord.outputs["Object"], wave.inputs["Vector"])
    line = new("ShaderNodeValToRGB")
    le = line.color_ramp.elements
    le[0].position, le[0].color = 0.9, (0, 0, 0, 1)
    le[1].position, le[1].color = 0.97, (1, 1, 1, 1)
    link(wave.outputs["Fac"], line.inputs["Fac"])
    mix = new("ShaderNodeMix")
    mix.data_type = "RGBA"
    link(line.outputs["Color"], socket(mix.inputs, "Factor_Float"))
    link(out, socket(mix.inputs, "A_Color"))
    socket(mix.inputs, "B_Color").default_value = (*pal["membrane_vein"], 1)
    link(socket(mix.outputs, "Result_Color"), bsdf.inputs["Base Color"])
    return mat


def materials(pal, mangy=False):
    specs = {
        "bone": ("stripes", "v", 0.12, [(4, "bone"), (1, "membrane_vein")], "LINEAR"),
        "skin": ("solid", "skin"),
        "ear_in": ("solid", "ear_in"),
        "nose": ("solid", "nose"),
        "eye": ("solid", "eye"),
        "glint": ("solid", "glint"),
        "tooth": ("solid", "tooth"),
        "mouth": ("solid", "mouth"),
        "claw": ("solid", "claw"),
    }
    M = {name: common.make_material("bat_" + name, spec, pal) for name, spec in specs.items()}
    M["fur"] = fur_material(pal, mangy)
    M["membrane"] = membrane_material(pal, mangy)
    return M


# ---------------------------------------------------------------- parts


def build_body(b, M):
    """One loft from the rump to the snout, blended over the body and head bones."""
    keys = [((0, -0.215, 0.0), 0.02, 0.02), ((0, -0.19, 0.004), 0.05, 0.045), ((0, -0.13, 0.01), 0.078, 0.068),
            ((0, -0.05, 0.014), 0.09, 0.078), ((0, 0.03, 0.016), 0.084, 0.074), ((0, 0.09, 0.016), 0.066, 0.06),
            ((0, 0.13, 0.02), 0.06, 0.058), ((0, 0.18, 0.026), 0.066, 0.062), ((0, 0.23, 0.02), 0.056, 0.052),
            ((0, 0.275, 0.006), 0.036, 0.032), ((0, 0.305, -0.004), 0.018, 0.016)]
    w = chain_weights([V((0, -0.3, 0.0)), NECK, V((0, 0.4, 0.0))], ["body", "head"], blend=0.04)
    first = len(b.verts)
    b.add(tube(keys, sub=3, n=20, ref=UP, bulge=(0.01, 0.008)), M["fur"], w)
    b.tags[first:] = ["head" if v.y > 0.12 else "body" for v in b.verts[first:]]


def build_head(b, M):
    b.add(ellipsoid((0, 0.308, 0.006), (0.022, 0.014, 0.018), n=10, rings=5, axis=FWD, ref=UP), M["nose"], "head", "head")
    b.add(ellipsoid((0, 0.25, -0.012), (0.026, 0.02, 0.05), n=10, rings=6, axis=FWD, ref=UP), M["mouth"], "head", "head")
    for s in (1, -1):
        ec = V((0.034 * s, 0.245, 0.04))
        b.add(ellipsoid(ec, (0.014, 0.013, 0.014), n=10, rings=6, axis=V((s, 0.4, 0.3)), ref=UP), M["eye"], "head", "head")
        gl = ec + V((0.5 * s, 0.6, 0.6)).normalized() * 0.013
        b.add(ellipsoid(gl, (0.004, 0.004, 0.003), n=6, rings=4, axis=V((s, 0.4, 0.5)), ref=UP), M["glint"], "head", "head")
        # Big pointed ear: fur back, pinkish cup in front. Ear bone twitches it.
        ear = "ear." + ("R" if s > 0 else "L")
        axis = V((0.35 * s, -0.12, 1)).normalized()
        nrm = V((0.35 * s, 1, -0.05)).normalized()
        c = SIDES["R" if s > 0 else "L"]["EAR"] + axis * 0.07
        b.add(ellipsoid(c, (0.042, 0.009, 0.08), n=14, rings=7, axis=axis, ref=nrm), M["fur"], ear, "head")
        b.add(ellipsoid(c + nrm * 0.006 + axis * 0.005, (0.03, 0.005, 0.064), n=12, rings=6, axis=axis, ref=nrm), M["ear_in"], ear, "head")
        # Upper fang.
        base = V((0.013 * s, 0.284, -0.01))
        d = V((0, 0.1, -1)).normalized()
        b.add(tube([(base, 0.006, 0.005), (base + d * 0.018, 0.005, 0.004), (base + d * 0.034, 0.0015, 0.0015)], sub=1, n=6, ref=FWD),
              M["tooth"], "head", "head")
    jaw = [((0, 0.19, -0.02), 0.042, 0.022), ((0, 0.25, -0.028), 0.03, 0.015), ((0, 0.292, -0.026), 0.015, 0.009)]
    b.add(tube(jaw, sub=2, n=12, ref=UP, bulge=(0.008, 0.006)), M["fur"], "jaw", "head")
    for s in (1, -1):
        base = V((0.01 * s, 0.284, -0.028))
        d = V((0, 0.2, 1)).normalized()
        b.add(tube([(base, 0.004, 0.0035), (base + d * 0.012, 0.0035, 0.003), (base + d * 0.02, 0.001, 0.001)], sub=1, n=5, ref=FWD),
              M["tooth"], "jaw", "head")


def claw(base, d, length, r):
    d = V(d).normalized()
    bend = V((0, 0, -1)) if abs(d.z) < 0.8 else V((0, -1, 0))
    mid = base + d * length * 0.55 + bend * length * 0.12
    return tube([(base, r, r), (mid, r * 0.6, r * 0.6), (base + d * length + bend * length * 0.35, 0.0008, 0.0008)], sub=2, n=5, ref=UP)


def build_wing(b, M, n):
    s = 1 if n == "R" else -1
    P = SIDES[n]
    # Arm bones as tubes, joints as knobs.
    b.add(tube([(P["SH"], 0.03, 0.026), ((P["SH"] + P["EL"]) * 0.5, 0.019, 0.017), (P["EL"], 0.014, 0.013)], sub=2, n=10, ref=UP),
          M["fur"], "upper." + n, "arm")
    b.add(ellipsoid(P["EL"], (0.015, 0.015, 0.014), n=8, rings=4), M["bone"], "fore." + n, "arm")
    b.add(tube([(P["EL"], 0.012, 0.011), ((P["EL"] + P["WR"]) * 0.5, 0.009, 0.009), (P["WR"], 0.011, 0.01)], sub=2, n=8, ref=UP),
          M["bone"], "fore." + n, "arm")
    b.add(ellipsoid(P["WR"], (0.014, 0.014, 0.012), n=8, rings=4), M["bone"], "fore." + n, "arm")
    # Thumb with a hooked claw.
    td = (P["TH"] - P["WR"]).normalized()
    b.add(tube([(P["WR"], 0.008, 0.008), (P["TH"], 0.006, 0.006)], sub=1, n=6, ref=UP), M["bone"], "fore." + n, "arm")
    b.add(claw(P["TH"], td + V((0, 0.3, 0)), 0.03, 0.005), M["claw"], "fore." + n, "arm")
    for f in FINGERS:
        tip = P[TIP[f]]
        mid = P["WR"] + (tip - P["WR"]) * 0.45 + V((0, 0, 0.002))
        b.add(tube([(P["WR"], 0.008, 0.007), (mid, 0.006, 0.0055), (tip, 0.0025, 0.0025)], sub=2, n=6, ref=UP),
              M["bone"], f + "." + n, "arm")
    # Membranes.
    front = [P["SH"], P["EL"], P["WR"]]
    L1, L2 = (P["EL"] - P["SH"]).length, (P["WR"] - P["EL"]).length

    def arm_at(u):
        a = u * (L1 + L2)
        return P["SH"].lerp(P["EL"], a / L1) if a < L1 else P["EL"].lerp(P["WR"], (a - L1) / L2)

    def arm_bone(u):
        a = u * (L1 + L2)
        k = smoothstep(L1 - 0.04, L1 + 0.04, a)
        return {"upper." + n: 1 - k, "fore." + n: k}

    # Plagiopatagium: arm in front, from the body side at the ankle to finger 5 behind.
    NS, NT_ = 10, 7

    def plagio(si, ti):
        u, t = si / NS, ti / NT_
        F = arm_at(u)
        B = P["ANK"].lerp(P["D5"], u)
        B = B + (F - B) * 0.16 * math.sin(math.pi * u)
        pos = F.lerp(B, t)
        pos.z = WING_Z
        wf = arm_bone(u)
        kb = smoothstep(0.25, 0.75, u)
        wb = {"leg." + n: 1 - kb, "d5." + n: kb}
        w = {}
        for bone, v in wf.items():
            w[bone] = w.get(bone, 0) + v * (1 - t)
        for bone, v in wb.items():
            w[bone] = w.get(bone, 0) + v * t
        kbody = 1 - smoothstep(0.0, 0.18, u)
        if kbody > 0:
            w = {k: v * (1 - kbody) for k, v in w.items()}
            w["body"] = w.get("body", 0) + kbody
        return pos, w

    membrane(b, M, NS, NT_, plagio, s)
    # Panels between the fingers, radiating from the wrist.
    for fa, fb in (("d2", "d3"), ("d3", "d4"), ("d4", "d5")):
        A, B = P[TIP[fa]], P[TIP[fb]]
        NU, NV = 8, 5

        def panel(ui, vi, A=A, B=B, fa=fa, fb=fb):
            u, v = ui / NU, vi / NV
            pos = P["WR"].lerp(A, u).lerp(P["WR"].lerp(B, u), v)
            pos = pos - (pos - P["WR"]) * 0.2 * math.sin(math.pi * v) * u * u
            pos.z = WING_Z
            w = {fa + "." + n: 1 - v, fb + "." + n: v}
            kw = 1 - smoothstep(0.0, 0.12, u)
            if kw > 0:
                w = {k: x * (1 - kw) for k, x in w.items()}
                w["fore." + n] = kw
            return pos, w

        membrane(b, M, NU, NV, panel, s)
    # Uropatagium half: between the leg and the tail.
    NU, NV = 5, 4

    def uro(ui, vi):
        u, v = ui / NU, vi / NV
        inner = V((0, -0.18, 0.0)).lerp(TAIL_TIP, u)
        outer = P["HIP"].lerp(P["ANK"], u)
        pos = inner.lerp(outer, v)
        pos.z = 0.0
        w = {"tail": (1 - v) * u, "body": (1 - v) * (1 - u), "leg." + n: v}
        return pos, {k: x for k, x in w.items() if x > 0}

    membrane(b, M, NU, NV, uro, s, thick=0.002)
    # Leg and foot with five claws.
    b.add(tube([(P["HIP"], 0.02, 0.02), ((P["HIP"] + P["ANK"]) * 0.5, 0.009, 0.009), (P["ANK"], 0.008, 0.008)], sub=2, n=8, ref=UP),
          M["fur"], "leg." + n, "leg")
    fdir = (P["TOE"] - P["ANK"]).normalized()
    b.add(tube([(P["ANK"], 0.01, 0.007), (P["TOE"], 0.012, 0.005)], sub=1, n=8, ref=UP), M["skin"], "foot." + n, "leg")
    side = fdir.cross(UP).normalized()
    for k in range(5):
        off = (k - 2) / 2
        base = P["TOE"] + side * off * 0.012
        b.add(claw(base, fdir + side * off * 0.15, 0.03, 0.0035), M["claw"], "foot." + n, "leg")


def membrane(b, M, nu, nv, fn, s, thick=0.0025):
    """Double-sided grid; fn(ui, vi) -> (pos, weights). Top sheet faces +Z."""
    pts, ws = [], []
    for ui in range(nu + 1):
        for vi in range(nv + 1):
            p, w = fn(ui, vi)
            pts.append(p)
            ws.append(w)
    for sgn in (1, -1):
        verts = [p + V((0, 0, thick * sgn)) for p in pts]
        faces, fuv = [], []
        for ui in range(nu):
            for vi in range(nv):
                q = [ui * (nv + 1) + vi, (ui + 1) * (nv + 1) + vi, (ui + 1) * (nv + 1) + vi + 1, ui * (nv + 1) + vi + 1]
                nrm = (verts[q[1]] - verts[q[0]]).cross(verts[q[3]] - verts[q[0]])
                if nrm.z * sgn < 0:
                    q = q[::-1]
                faces.append(q)
                fuv.append([(verts[i].x, verts[i].y) for i in q])
        base = len(b.verts)
        if M["membrane"] not in b.mats:
            b.mats.append(M["membrane"])
        mi = b.mats.index(M["membrane"])
        b.verts.extend(verts)
        for f, uv in zip(faces, fuv):
            b.faces.append([base + i for i in f])
            b.fuv.append(uv)
            b.fmat.append(mi)
        b.weights.extend(dict(w) for w in ws)
        b.tags.extend(["membrane"] * len(verts))


def build_tail(b, M):
    b.add(tube([(TAIL_BASE, 0.022, 0.02), ((TAIL_BASE + TAIL_TIP) * 0.5, 0.008, 0.008), (TAIL_TIP, 0.003, 0.003)], sub=2, n=8, ref=UP),
          M["fur"], chain_weights([TAIL_BASE, TAIL_TIP], ["tail"]), "body")


# ---------------------------------------------------------------- rig


def build_rig(coll):
    arm = bpy.data.armatures.new("bat_rig")
    rig = bpy.data.objects.new("bat_rig", arm)
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
    bone("root", (0, 0, 0), (0, 0.2, 0), X, None)
    bone("body", PIVOT, PIVOT + V((0, 0.1, 0)), X, "root")
    bone("head", NECK, NECK + V((0, 0.15, 0)), X, "body")
    bone("jaw", JAW_HINGE, JAW_HINGE + V((0, 0.1, 0)), X, "head")
    bone("tail", TAIL_BASE, TAIL_TIP, X, "body")
    for n, P in SIDES.items():
        bone("ear." + n, P["EAR"], P["EAR"] + V((0, 0, 0.1)), X, "head")
        bone("upper." + n, P["SH"], P["EL"], UP, "body")
        bone("fore." + n, P["EL"], P["WR"], UP, "upper." + n)
        for f in FINGERS:
            bone(f + "." + n, P["WR"], P[TIP[f]], UP, "fore." + n)
        bone("leg." + n, P["HIP"], P["ANK"], UP, "body")
        bone("foot." + n, P["ANK"], P["TOE"], UP, "leg." + n)
    bpy.ops.object.mode_set(mode="OBJECT")
    for pb in rig.pose.bones:
        pb.rotation_mode = "QUATERNION"
    return rig


# ---------------------------------------------------------------- animation

WING_KEYS = ("flap", "sweep", "twist", "efold", "wfold", "cup", "legp", "footp", "ear")
REST = dict(x=0.0, y=0.0, z=0.0, pitch=0.0, roll=0.0, yaw=0.0, hp=0.0, hy=0.0, hr=0.0, hfwd=0.0, jaw=0.0, tail=0.0)
for _k in WING_KEYS:
    REST[_k] = 0.0
    REST[_k + "R"] = 0.0  # per-side offsets, added to the symmetric value
    REST[_k + "L"] = 0.0


def deformations(p):
    """Deformation matrix (armature space) of every bone for pose parameters p."""
    Db = Matrix.Translation(V((p["x"], p["y"], p["z"]))) @ rot_about(PIVOT, euler(p["pitch"], p["roll"], p["yaw"]))
    Dh = Db @ Matrix.Translation(V((0, p["hfwd"], 0))) @ rot_about(NECK, euler(p["hp"], p["hr"], p["hy"]))
    D = {"root": Matrix(), "body": Db, "head": Dh, "jaw": Dh @ rot_about(JAW_HINGE, rx(-p["jaw"])),
         "tail": Db @ rot_about(TAIL_BASE, rx(p["tail"]))}
    for n, P in SIDES.items():
        s = 1 if n == "R" else -1

        def v(k):
            return p[k] + p[k + n]

        D["ear." + n] = Dh @ rot_about(P["EAR"], ry(s * v("ear")))
        Du = Db @ rot_about(P["SH"], rz(s * v("sweep")) @ ry(-s * v("flap")) @ rx(v("twist")))
        Df = Du @ rot_about(P["EL"], rz(s * v("efold")))
        D["upper." + n], D["fore." + n] = Du, Df
        for f in FINGERS:
            D[f + "." + n] = Df @ rot_about(P["WR"], rz(-s * FOLD[f] * v("wfold")) @ ry(s * v("cup") * (0.6 + 0.2 * FINGERS.index(f))))
        Dl = Db @ rot_about(P["HIP"], rx(v("legp")))
        D["leg." + n] = Dl
        D["foot." + n] = Dl @ rot_about(P["ANK"], rx(v("footp")))
    return D


class Skin:
    """numpy linear blend skinning of every vertex (floor contact and measurements)."""

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

    def coords(self, D):
        np = self.np
        out = np.zeros((len(self.P), 3))
        for j, n in enumerate(self.bones):
            M = np.array(D[n])[:3]
            out += self.W[:, j:j + 1] * (self.P @ M.T)
        return out


def key_frame(rig, skin, p, frame_no, floor=None):
    """floor: z the lowest vertex is moved to (None: the pose stays where the parameters put it)."""
    bones = rig.data.bones
    D = deformations(p)
    if floor is not None:
        D = {n: Matrix.Translation(V((0, 0, floor - float(skin.coords(D)[:, 2].min())))) @ m for n, m in D.items()}
    for bn in bones:
        pose = D[bn.name] @ bn.matrix_local
        if bn.parent:
            rel = bn.parent.matrix_local.inverted() @ bn.matrix_local
            basis = rel.inverted() @ (D[bn.parent.name] @ bn.parent.matrix_local).inverted() @ pose
        else:
            basis = bn.matrix_local.inverted() @ pose
        pb = rig.pose.bones[bn.name]
        pb.matrix_basis = basis
        pb.keyframe_insert("rotation_quaternion", frame=frame_no)
        pb.keyframe_insert("location", frame=frame_no)
    return D


def fly_params(t, beats=2):
    """Flapping flight at clip phase t (0..1). Frame 0: wings spread level, just into the downstroke."""
    p = dict(REST)
    ph = beats * t
    w = 2 * math.pi * ph
    k = math.sin(math.pi * ((ph - 0.1) % 1.0)) ** 2  # upstroke: wing half folded
    p.update(flap=8 - 46 * math.sin(w), efold=8 + 55 * k, wfold=0.04 + 0.4 * k, sweep=-4 - 16 * k, twist=-8 * math.cos(w),
             cup=12 * (1 - k), z=0.025 * math.cos(w), pitch=4 * math.sin(w + 0.5), hp=-6 + 3 * math.sin(w + 1.0),
             legp=8, footp=-20, tail=8 + 4 * math.sin(w), ear=4 * math.sin(w + 1.5))
    return p


def bump(f, f0, f1, f2):
    """0 before f0, rises to 1 at f1, back to 0 at f2."""
    if f <= f0 or f >= f2:
        return 0.0
    return smoothstep(f0, f1, f) if f < f1 else 1 - smoothstep(f1, f2, f)


def attack_params(f, frames):
    p = fly_params(f / frames)
    lunge = bump(f, 0, 4, 10)
    p["pitch"] += -18 * lunge
    p["hfwd"] += 0.05 * lunge
    p["hp"] += -22 * lunge
    p["jaw"] = 48 * bump(f, 0.5, 3.2, 5.2)
    p["legp"] += 75 * bump(f, 1, 4.5, 9)
    p["footp"] += 40 * bump(f, 1, 4.5, 9)
    p["ear"] += -18 * lunge
    p["y"] += 0.04 * lunge
    return p


def interpolate(keys, frame_no, base):
    full = []
    for f, over in keys:
        p = dict(base)
        for k, val in over.items():
            p[k] = val
        full.append((f, p))
    for (f0, p0), (f1, p1) in zip(full, full[1:]):
        if f0 <= frame_no <= f1:
            t = smoothstep(f0, f1, frame_no)
            return {k: p0[k] + (p1[k] - p0[k]) * t for k in p0}
    return full[-1][1]


FLY0 = fly_params(0.0)

DIE = [
    (0, {}),
    (3, dict(flap=45, efold=55, wfold=0.45, jaw=40, hp=25, roll=25, pitch=12, legp=30, ear=-15, twist=0, cup=0)),
    (6, dict(flap=75, efold=105, wfold=0.75, sweep=-30, jaw=35, hp=10, roll=95, pitch=-25, legp=50, tail=30)),
    (9, dict(roll=160, pitch=-10, flap=40, flapL=30, flapR=-30, efold=110, wfold=0.8, sweep=-40, jaw=30, hp=-5, legp=40)),
    (12, dict(roll=176, pitch=-4, flap=10, flapR=-10, flapL=15, efold=95, efoldR=-55, efoldL=40, wfold=0.7, wfoldR=-0.35, wfoldL=0.25,
              sweep=-35, sweepR=20, jaw=26, hp=-14, hy=18, legp=30, footp=-40, tail=10)),
    (15, dict(roll=172, pitch=-6, flap=4, flapR=-10, flapL=12, efold=92, efoldR=-50, efoldL=45, wfold=0.7, wfoldR=-0.32, wfoldL=0.25,
              sweep=-35, sweepR=22, jaw=22, hp=-16, hy=22, legp=45, footp=-55, tail=6)),
    (19, dict(roll=174, pitch=-5, flap=0, flapR=-12, flapL=10, efold=90, efoldR=-48, efoldL=45, wfold=0.72, wfoldR=-0.34, wfoldL=0.25,
              sweep=-35, sweepR=22, jaw=20, hp=-18, hy=24, legp=38, footp=-60, tail=4)),
    (23, dict(roll=174, pitch=-5, flap=0, flapR=-12, flapL=10, efold=90, efoldR=-48, efoldL=45, wfold=0.72, wfoldR=-0.34, wfoldL=0.25,
              sweep=-35, sweepR=22, jaw=18, hp=-18, hy=24, legp=40, footp=-62, tail=4)),
]


def idle_params(f, frames):
    """Hanging upside down, head down, belly to the camera (+Y), wings wrapped around the body. Feet stay put."""
    t = f / frames
    w = 2 * math.pi * t
    p = dict(REST)
    breath = math.sin(w)
    look = math.sin(w + 0.7) * smoothstep(0.2, 0.5, (t + 0.1) % 1.0) * (1 - smoothstep(0.7, 0.95, (t + 0.1) % 1.0))
    p.update(pitch=-90, yaw=180, flap=-75 + 3 * breath, sweep=-70, twist=-90, efold=150, wfold=0.97, cup=6,
             legp=-12, footp=55, tail=-15, hp=-55 + 3 * math.sin(w + 1.3), hy=22 * look, jaw=2 + 2 * max(0, breath),
             ear=6 * math.sin(2 * w) + 10 * max(0, math.sin(3 * w + 2)) ** 8)
    p["earR"] = 6 * max(0, math.sin(2 * w + 1.0)) ** 6
    return p


def make_actions(rig, skin):
    rig.animation_data_create()
    acts = {}
    # The engine puts fly frame 0's lowest point on the monster's origin; the dead bat lies on that same level.
    floor = float(skin.coords(deformations(FLY0))[:, 2].min())
    for name, suffix, frames in CLIPS:
        act = bpy.data.actions.get(name)
        if act:
            bpy.data.actions.remove(act)
        act = bpy.data.actions.new(name)
        act.use_fake_user = True
        rig.animation_data.action = act
        last = frames if name != "bat_die" else frames - 1  # loops key frame N = frame 0
        for f in range(last + 1):
            if name == "bat_fly":
                p = fly_params(f / frames)
            elif name == "bat_attack":
                p = attack_params(f, frames)
            elif name == "bat_die":
                p = interpolate(DIE, f, FLY0)
            else:
                p = idle_params(f, frames)
            key_frame(rig, skin, p, f, floor if name == "bat_die" else None)
        acts[name] = act
    return acts


def report(skin):
    """Measurements the engine is calibrated with (Blender units)."""
    c = skin.coords(deformations(FLY0))
    lo = c.min(axis=0)
    hi = c.max(axis=0)
    idle = [skin.coords(deformations(idle_params(f, CLIPS[3][2]))) for f in range(CLIPS[3][2])]
    tops = [float(i[:, 2].max()) for i in idle]
    print("bat fly0: x {:.3f}..{:.3f} y {:.3f}..{:.3f} z {:.3f}..{:.3f} (wingspan {:.3f})".format(lo[0], hi[0], lo[1], hi[1], lo[2], hi[2], hi[0] - lo[0]))
    print("bat idle: top z {:.4f}..{:.4f}, bottom z {:.3f}, x {:.3f}..{:.3f}, y {:.3f}..{:.3f}".format(
        min(tops), max(tops), float(idle[0][:, 2].min()), float(idle[0][:, 0].min()), float(idle[0][:, 0].max()),
        float(idle[0][:, 1].min()), float(idle[0][:, 1].max())))


# ---------------------------------------------------------------- entry points


def build(bake=True, tex_path=None, giant_path=None):
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    M = materials(COL)
    rig = build_rig(coll)
    b = Builder()
    build_body(b, M)
    build_head(b, M)
    build_tail(b, M)
    for n in SIDES:
        build_wing(b, M, n)
    obj = common.finish_mesh(b, coll, "bat_new")
    common.uv_unwrap(obj, b.tags, {"head": (V((0, 0.2, 0.02)), 2.0), "leg": (V((0, -0.25, 0)), 1.5)})
    if bake:
        tmp = bpy.app.tempdir or "/tmp"
        tex = common.bake_texture(obj, tex_path or os.path.join(tmp, "bat_preview.png"), TEX_SIZE, "bat", ao_distance=0.06)
        materials(COL_GIANT, mangy=True)
        giant = common.bake_texture(obj, giant_path or os.path.join(tmp, "bat_giant_preview.png"), TEX_SIZE, "bat_giant", ao_distance=0.06)
        common.use_baked_material(obj, giant if os.environ.get("BAT_TEX") == "giant" else tex)
    common.rig_object(obj, rig)
    skin = Skin(b)
    acts = make_actions(rig, skin)
    report(skin)
    rig.animation_data.action = acts["bat_fly"]
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 0, CLIPS[0][2] - 1
    scene.frame_set(0)
    print("bat: {} verts, {} tris".format(len(obj.data.vertices), common.tri_count(obj)))
    return obj, rig


def export(models_dir=None):
    common.export_files(bpy.data.objects["bat_new"], bpy.data.objects["bat_rig"], "bat", CLIPS, "monsters", models_dir)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    tex_dir = os.path.join(REPO, "textures", "monsters")
    if "--boss-texture" in args:
        obj, _ = build(bake=False)
        materials(COL_VAMPIRE)
        common.bake_texture(obj, os.path.join(tex_dir, "bat_vampire.png"), TEX_SIZE, "bat_vampire", ao_distance=0.06)
    elif "--export" in args:
        build(tex_path=os.path.join(tex_dir, "bat.png"), giant_path=os.path.join(tex_dir, "bat_giant.png"))
        export()
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "bat.blend"))
    else:
        build()
