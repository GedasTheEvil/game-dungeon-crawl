"""Procedural hamadryas baboon of the Lake of Fire (plan baboon): mesh, rig, baked texture and the .md3 animations
(walk, bite, ranged throw of a burning coal, die, idle: sitting and greeting the sun, climb: up a ladder).

    MCP:  p = ".../tools/blender/models/baboon.py"; g = {"__file__": p, "__name__": "baboon"}
          exec(open(p).read(), g); g["build"]()          # then g["export"]()
    CLI:  blender -b --python tools/blender/models/baboon.py -- [--export | --export-climb | --measure]
          (--measure prints the coal per ranged frame: the release, RangedRules; BABOON_LADDER=1 adds a ladder
          stand-in for review renders of the climb)

Blender space: Z up, units metres, the baboon faces +Y (game uses rotA = 180), its right side is +X.
Built like rat.py: one loft from the rump to the head over a hips, a chest and a head bone, the four legs by
two-bone IK towards foot targets (planted on the floor, in body space or at world points: the raised hands), the tail
an FK chain (up from the rump, a kink, then hanging in an arc). The silver mane is a second, wider loft over the
shoulders, the neck and the crown, its belly side buried in the body, with cheek tufts framing the pink-red face; a long
dog-like muzzle, small close-set eyes under a heavy brow, long upper fangs (the lower jaw on a hinge), a red rump pad.
The coal is a rigid prop on its own bone in the right hand, scaled to zero where it is not held (as anubis.py drops its
sceptre bone). Every frame is lifted so the lowest vertex (tail and coal excluded) touches the floor (numpy skinning).
Clips (the engine plays ~14 fps):
  walk 16 (the reference, loops: two cycles of a quick trot on the palms, long strides, the shoulders rolling; it covers
  about 0.8 reference sizes a second at 14 fps),
  attack 18 (_att, loops: crouches, lunges with the jaw wide open on the fangs, snaps, shakes),
  ranged 14 (_att_ranged, plays once: scoops a coal, rears on the hind legs with it high over the head, screeches,
  hurls it overarm; the coal leaves the hand at frame RELEASE_FRAME and is hidden after; back on all fours at the end),
  die 28 (_die, plays once: a scream, collapses onto its left side, the limbs go limp),
  idle 32 (_idle, loops: sits on its haunches, both hands raised to the sun, palms forward, breathing and swaying),
  climb 16 (_climb, loops: up a ladder hand over hand, back to the camera; the frame set from the climb height).
Numbers the engine reads: --measure prints the ranged release (RangedRules release, releaseY, releaseAhead); the
build prints climbRise and climbGrip (shares of the reference clip's frame 0 height, computed for KIND_SCALE).
One texture, baboon.png (1024x1024). The UVs only depend on the mesh, so a boss texture (Babi) can be baked on the same
layout later from another palette.
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

COLL = "baboon_new"
CLIPS = [("baboon_walk", "", 16), ("baboon_attack", "_att", 18), ("baboon_ranged", "_att_ranged", 14),
         ("baboon_die", "_die", 28), ("baboon_idle", "_idle", 32), ("baboon_climb", "_climb", 16)]
ONCE = ("baboon_ranged", "baboon_die")  # one-shot clips: no loop frame
TEX_SIZE = 1024
REVIEW_VIEW = {"target": (0, -0.05, 0.5), "ortho": 1.9, "res": (560, 440)}

# Linear-space colours.
COL = {
    "fur_belly": (0.30, 0.27, 0.22),
    "fur_flank": (0.20, 0.18, 0.145),
    "fur_back": (0.15, 0.135, 0.11),
    "fur_dark": (0.10, 0.09, 0.075),
    "mane_lo": (0.22, 0.22, 0.21),
    "mane": (0.38, 0.38, 0.36),
    "mane_hi": (0.54, 0.54, 0.51),
    "face": (0.62, 0.17, 0.14),
    "face_dark": (0.40, 0.09, 0.08),
    "nose": (0.30, 0.06, 0.06),
    "hand": (0.05, 0.042, 0.038),
    "pad": (0.75, 0.10, 0.08),
    "pad_dark": (0.50, 0.05, 0.05),
    "eye": (0.35, 0.16, 0.03),
    "pupil": (0.004, 0.003, 0.003),
    "glint": (1.0, 1.0, 1.0),
    "brow": (0.30, 0.08, 0.07),
    "tooth": (0.80, 0.72, 0.52),
    "mouth": (0.30, 0.03, 0.04),
    "crust": (0.10, 0.025, 0.01),
    "ember": (1.0, 0.30, 0.02),
    "ember_hi": (1.0, 0.80, 0.30),
}

UP, FWD, X = V((0, 0, 1)), V((0, 1, 0)), V((1, 0, 0))
DZ = 0.04  # the body, head and mane sit this much higher than their literal coordinates (the legs' length)
PIVOT = V((0, -0.30, 0.46 + DZ))  # hips bone: body rotations happen around this point
MID = V((0, 0.02, 0.48 + DZ))  # chest bone: spine flex
NECK = V((0, 0.25, 0.56 + DZ))  # head bone
JAW_HINGE = V((0, 0.40, 0.535 + DZ))
TAIL_BASE = V((0, -0.52, 0.50 + DZ))
NT, TAIL_LEN = 10, 0.56  # tail bones, length
FOOT_Z = 0.035  # wrist / heel height above the floor
COAL_R = 0.052  # the coal, about fist-size (a little more, to read in the game)


def leg_defs():
    """Rest layout of the four legs (wrist / heel = foot). s = +1 right, -1 left. Arms as long as the legs or longer.
    phase: a quick trot, the diagonal pairs together."""
    legs = []
    for s in (1, -1):
        sfx = ".R" if s > 0 else ".L"
        legs.append(dict(name="H" + sfx, s=s, hip=V((0.10 * s, -0.33, 0.42 + DZ)), foot=V((0.115 * s, -0.36, FOOT_Z)), a=0.25, b=0.25,
                         bend=V((0, 1, 0.2)), pdir=V((0.1 * s, 1, -0.12)), plen=0.15, curled=V((0.12 * s, -0.20, 0.20)),
                         phase=0.5 if s > 0 else 0.0))
        legs.append(dict(name="F" + sfx, s=s, hip=V((0.115 * s, 0.17, 0.46 + DZ)), foot=V((0.12 * s, 0.22, FOOT_Z + 0.02)), a=0.265, b=0.265,
                         bend=V((0, -1, 0)), pdir=V((0.12 * s, 1, -0.45)), plen=0.085, curled=V((0.10 * s, 0.32, 0.25)),
                         phase=0.0 if s > 0 else 0.5))
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


def hash01(*k):
    """A fixed pseudo-random number in [0, 1) from integers (the same mesh on every build)."""
    h = 0
    for v in k:
        h = (h * 1103515245 + 12345 + v * 2654435761) & 0xFFFFFFFF
    h ^= h >> 13
    h = (h * 1274126177) & 0xFFFFFFFF
    return (h & 0xFFFF) / 65536.0


# ---------------------------------------------------------------- materials


def noise_ramp(nt, coord, scale, detail, lo, hi, stretch=(1, 1, 1), rough=0.5):
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


def fresh(name):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nt = mat.node_tree
    for node in list(nt.nodes):
        if node.type not in ("OUTPUT_MATERIAL", "BSDF_PRINCIPLED"):
            nt.nodes.remove(node)
    nt.nodes["Principled BSDF"].inputs["Roughness"].default_value = 0.8
    return mat, nt


def multiply(nt, out, factors):
    for f in factors:
        m = nt.nodes.new("ShaderNodeVectorMath")
        m.operation = "MULTIPLY"
        nt.links.new(out, m.inputs[0])
        nt.links.new(f.outputs["Color"], m.inputs[1])
        out = m.outputs[0]
    return out


def tone_by_normal(nt, stops):
    """Colour ramp over the rest-pose normal's z (belly 0 .. back 1)."""
    new, link = nt.nodes.new, nt.links.new
    geo = new("ShaderNodeNewGeometry")
    sep = new("ShaderNodeSeparateXYZ")
    link(geo.outputs["Normal"], sep.inputs[0])
    tone = new("ShaderNodeValToRGB")
    els = tone.color_ramp.elements
    (p0, c0), (p1, c1) = stops[0], stops[-1]
    els[0].position, els[0].color = p0, (*c0, 1)
    els[1].position, els[1].color = p1, (*c1, 1)
    for p, c in stops[1:-1]:
        els.new(p).color = (*c, 1)
    remap = new("ShaderNodeMath")
    remap.operation = "MULTIPLY_ADD"
    remap.inputs[1].default_value = 0.5
    remap.inputs[2].default_value = 0.5
    link(sep.outputs["Z"], remap.inputs[0])
    link(remap.outputs[0], tone.inputs["Fac"])
    return tone.outputs["Color"]


def fur_material(pal):
    """Olive-grey body fur: a dark back fading to a paler belly, fine streaks along the body, mottling."""
    mat, nt = fresh("baboon_fur")
    coord = nt.nodes.new("ShaderNodeTexCoord")
    out = tone_by_normal(nt, [(0.2, pal["fur_belly"]), (0.5, pal["fur_flank"]), (0.75, pal["fur_back"]), (0.97, pal["fur_dark"])])
    out = multiply(nt, out, [noise_ramp(nt, coord, 3.0, 6.0, (0.3, 0.7), (0.7, 1.3), stretch=(16, 3, 16), rough=0.7),
                             noise_ramp(nt, coord, 5.0, 3.0, (0.35, 0.85), (0.65, 1.12))])
    nt.links.new(out, nt.nodes["Principled BSDF"].inputs["Base Color"])
    return mat


def mane_material(pal):
    """The silver cape: long strands hanging down (noise stretched along z), darker roots under the hem, light tips."""
    mat, nt = fresh("baboon_mane")
    coord = nt.nodes.new("ShaderNodeTexCoord")
    out = tone_by_normal(nt, [(0.1, pal["mane_lo"]), (0.45, pal["mane"]), (0.85, pal["mane_hi"])])
    out = multiply(nt, out, [noise_ramp(nt, coord, 4.0, 8.0, (0.3, 0.62), (0.72, 1.25), stretch=(22, 22, 3), rough=0.75),
                             noise_ramp(nt, coord, 2.5, 3.0, (0.35, 0.85), (0.65, 1.1))])
    nt.links.new(out, nt.nodes["Principled BSDF"].inputs["Base Color"])
    return mat


def face_material(pal):
    """Pink-red bare skin, mottled, darker in the folds."""
    mat, nt = fresh("baboon_face")
    coord = nt.nodes.new("ShaderNodeTexCoord")
    mix = nt.nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    n = noise_ramp(nt, coord, 9.0, 4.0, (0.35, 0.0), (0.7, 1.0))
    nt.links.new(n.outputs["Color"], next(s for s in mix.inputs if s.identifier == "Factor_Float"))
    next(s for s in mix.inputs if s.identifier == "A_Color").default_value = (*pal["face_dark"], 1)
    next(s for s in mix.inputs if s.identifier == "B_Color").default_value = (*pal["face"], 1)
    nt.links.new(next(s for s in mix.outputs if s.identifier == "Result_Color"), nt.nodes["Principled BSDF"].inputs["Base Color"])
    return mat


def coal_material(pal):
    """A glowing ember: a black crust cracked open on orange and yellow heat (bright colours, no emission: baked)."""
    mat, nt = fresh("baboon_coal")
    new, link = nt.nodes.new, nt.links.new
    coord = new("ShaderNodeTexCoord")
    vor = new("ShaderNodeTexVoronoi")
    vor.feature = "DISTANCE_TO_EDGE"
    vor.inputs["Scale"].default_value = 55.0
    link(coord.outputs["Object"], vor.inputs["Vector"])
    ramp = new("ShaderNodeValToRGB")
    els = ramp.color_ramp.elements
    els[0].position, els[0].color = 0.0, (*pal["ember_hi"], 1)
    els[1].position, els[1].color = 0.22, (*pal["crust"], 1)
    els.new(0.07).color = (*pal["ember"], 1)
    link(vor.outputs["Distance"], ramp.inputs["Fac"])
    glow = noise_ramp(nt, coord, 30.0, 2.0, (0.4, 0.0), (0.6, 1.0))
    mix = new("ShaderNodeMix")
    mix.data_type = "RGBA"
    link(glow.outputs["Color"], next(s for s in mix.inputs if s.identifier == "Factor_Float"))
    link(ramp.outputs["Color"], next(s for s in mix.inputs if s.identifier == "A_Color"))
    next(s for s in mix.inputs if s.identifier == "B_Color").default_value = (*pal["ember"], 1)
    link(next(s for s in mix.outputs if s.identifier == "Result_Color"), nt.nodes["Principled BSDF"].inputs["Base Color"])
    return mat


def materials(pal):
    specs = {
        "nose": ("solid", "nose"),
        "hand": ("solid", "hand"),
        "pad": ("stripes", "u", 0.25, [(3, "pad"), (1, "pad_dark")], "LINEAR"),
        "eye": ("solid", "eye"),
        "pupil": ("solid", "pupil"),
        "glint": ("solid", "glint"),
        "brow": ("solid", "brow"),
        "tooth": ("solid", "tooth"),
        "mouth": ("solid", "mouth"),
    }
    M = {name: common.make_material("baboon_" + name, spec, pal) for name, spec in specs.items()}
    M["fur"] = fur_material(pal)
    M["mane"] = mane_material(pal)
    M["face"] = face_material(pal)
    M["coal"] = coal_material(pal)
    return M


# ---------------------------------------------------------------- parts


def body_shape(th, a):
    """Flatter belly, a little keel on the back."""
    sn = math.sin(th)
    return 1.0 - 0.12 * sn * sn if sn < 0 else 1.0 + 0.03 * sn ** 4


BODY_KEYS = [((0, -0.565, 0.49), 0.03, 0.03), ((0, -0.545, 0.49), 0.09, 0.10), ((0, -0.48, 0.485), 0.135, 0.14),
             ((0, -0.38, 0.48), 0.15, 0.15), ((0, -0.24, 0.475), 0.13, 0.13), ((0, -0.10, 0.475), 0.125, 0.135),
             ((0, 0.04, 0.485), 0.145, 0.165), ((0, 0.16, 0.50), 0.14, 0.155), ((0, 0.25, 0.54), 0.085, 0.085),
             ((0, 0.31, 0.575), 0.065, 0.065)]
BODY_W = chain_weights([V((0, -0.75, 0.46)), MID, NECK, V((0, 0.7, 0.6))], ["hips", "chest", "head"], blend=0.1)


def build_body(b, M):
    """One loft from the rump into the head, blended over the hips, chest and head bones."""
    first = len(b.verts)
    b.add(tube(BODY_KEYS, sub=3, n=24, ref=UP, shape=body_shape, bulge=(0.015, 0.01)), M["fur"], BODY_W)
    b.tags[first:] = ["head" if v.y > 0.28 else "body" for v in b.verts[first:]]


def mane_shape(th, a):
    """The cape's section: full on the back and the flanks, the belly side buried in the body; strand ridges."""
    sn = math.sin(th)
    k = 1.0 if sn > -0.15 else 1.0 - 0.32 * smoothstep(-0.15, -0.95, sn)
    return k * (1.0 + 0.045 * math.sin(19 * th + 7 * a) + 0.025 * math.sin(31 * th - 11 * a))


def lock(base, d, normal, length, width, thick=0.012):
    """A flat lock of hair: a tapering strip from base along d, lying on a surface with the given normal."""
    d = V(d).normalized()
    nrm = (V(normal) - V(normal).dot(d) * d).normalized()
    keys = [(base - d * 0.01, width * 0.8, thick), (base + d * length * 0.45, width, thick * 0.9),
            (base + d * length * 0.8 + nrm * thick * 0.5, width * 0.5, thick * 0.6), (base + d * length + nrm * thick, 0.003, 0.003)]
    return tube(keys, sub=1, n=6, ref=nrm)


MANE_KEYS = [((0, -0.20, 0.50), 0.10, 0.11, 0.0), ((0, -0.12, 0.505), 0.15, 0.16, 1.0), ((0, -0.01, 0.51), 0.18, 0.19, 2.0),
             ((0, 0.10, 0.52), 0.185, 0.195, 3.0), ((0, 0.20, 0.555), 0.15, 0.16, 4.0), ((0, 0.28, 0.59), 0.105, 0.11, 5.0),
             ((0, 0.34, 0.615), 0.075, 0.08, 6.0), ((0, 0.385, 0.625), 0.04, 0.04, 7.0)]


def mane_point(y, th):
    """A point on the cape's surface (rest pose) at y and section angle th (0 the right side, pi/2 the top), its normal."""
    for (c0, rx0, ry0, _), (c1, rx1, ry1, _) in zip(MANE_KEYS, MANE_KEYS[1:]):
        if c0[1] <= y <= c1[1]:
            t = (y - c0[1]) / (c1[1] - c0[1])
            z, rx, ry = c0[2] + (c1[2] - c0[2]) * t, rx0 + (rx1 - rx0) * t, ry0 + (ry1 - ry0) * t
            k = mane_shape(th, 0.0) / (1.0 + 0.045 * math.sin(19 * th) + 0.025 * math.sin(31 * th))
            pt = V((rx * k * math.cos(th), y, z + ry * k * math.sin(th)))
            nrm = V((math.cos(th) / rx, 0, math.sin(th) / ry)).normalized()
            return pt, nrm
    raise ValueError(y)


def build_mane(b, M):
    """The silver mane over the shoulders, the neck and the crown, longer at the shoulders like a cape; shaggy locks
    hang from the hem over the flanks and the upper arms and lie back over the cape's back edge."""
    w = chain_weights([V((0, -0.75, 0.46)), MID, NECK, V((0, 0.7, 0.6))], ["hips", "chest", "head"], blend=0.12)
    first = len(b.verts)
    b.add(tube(MANE_KEYS, sub=3, n=36, ref=UP, shape=mane_shape, bulge=(0.02, 0.01)), M["mane"], w)
    b.tags[first:] = ["head" if v.y > 0.30 else "mane" for v in b.verts[first:]]
    for s in (1, -1):
        for i in range(26):  # the hem: hanging down the flanks and over the upper arms, ragged
            y = -0.10 + 0.30 * (i + hash01(i, s, 1)) / 26
            th = math.radians(-12 - 26 * hash01(i, s, 2) + (8 if y > 0.05 else 0))
            th = math.pi - th if s < 0 else th
            pt, nrm = mane_point(y, th)
            d = V((0.1 * s, -0.35 + 0.3 * hash01(i, s, 3), -1))
            b.add(lock(pt - nrm * 0.01, d, nrm, 0.06 + 0.07 * hash01(i, s, 4), 0.028 + 0.018 * hash01(i, s, 5)), M["mane"], w(pt), tag="mane")
        for i in range(9):  # the back edge: lying back over the loins
            th = math.radians(8 + 82 * (i + hash01(i, s, 6)) / 9)
            th = math.pi - th if s < 0 else th
            y = -0.15 + 0.04 * hash01(i, s, 7)
            pt, nrm = mane_point(y, th)
            d = V((0.15 * s * math.cos(th), -1, -0.25 - 0.3 * math.cos(th)))
            b.add(lock(pt - nrm * 0.01, d, nrm, 0.07 + 0.05 * hash01(i, s, 8), 0.035 + 0.015 * hash01(i, s, 9)), M["mane"], w(pt), tag="mane")


def build_head(b, M):
    first = len(b.verts)
    # Cranium, sloping back into the mane.
    b.add(ellipsoid((0, 0.36, 0.60), (0.068, 0.07, 0.085), n=16, rings=8, axis=FWD, ref=UP), M["fur"], "head")
    # Heavy brow ridge over the small close-set eyes.
    b.add(ellipsoid((0, 0.432, 0.633), (0.058, 0.016, 0.028), n=14, rings=6, axis=FWD, ref=UP), M["brow"], "head")
    # The long dog-like muzzle: a boxy loft sloping down to the nose, its underside flat (the mouth line).

    def muzzle_shape(th, a):
        sn, cs = math.sin(th), math.cos(th)
        k = 1.0 - 0.18 * sn * sn if sn < 0 else 1.0
        return k * (1.0 + 0.16 * (abs(cs) * max(sn, 0.0)) ** 0.5)  # ridged sides: square shoulders on top

    keys = [((0, 0.40, 0.572), 0.056, 0.064), ((0, 0.47, 0.563), 0.052, 0.053), ((0, 0.54, 0.553), 0.046, 0.044),
            ((0, 0.60, 0.543), 0.04, 0.038), ((0, 0.638, 0.538), 0.031, 0.029)]
    b.add(tube(keys, sub=3, n=18, ref=UP, shape=muzzle_shape, bulge=(0.0, 0.012)), M["face"], "head")
    # Face skin around the eyes (bare, pink-red), set under the brow.
    b.add(ellipsoid((0, 0.435, 0.612), (0.05, 0.03, 0.03), n=14, rings=6, axis=FWD, ref=UP), M["face"], "head")
    # Nose pad and nostrils.
    b.add(ellipsoid((0, 0.648, 0.545), (0.03, 0.024, 0.012), n=12, rings=5, axis=FWD, ref=UP), M["nose"], "head")
    for s in (1, -1):
        b.add(ellipsoid((0.012 * s, 0.658, 0.541), (0.008, 0.006, 0.004), n=6, rings=4, axis=FWD, ref=UP), M["pupil"], "head")
        # Small close-set eye with a dark pupil and a glint.
        ec = V((0.029 * s, 0.452, 0.622))
        eye_dir = V((0.6 * s, 1, 0.1)).normalized()
        b.add(ellipsoid(ec, (0.014, 0.013, 0.011), n=10, rings=5, axis=eye_dir, ref=UP), M["eye"], "head")
        b.add(ellipsoid(ec + eye_dir * 0.008, (0.007, 0.007, 0.004), n=8, rings=4, axis=eye_dir, ref=UP), M["pupil"], "head")
        b.add(ellipsoid(ec + eye_dir * 0.011 + V((0, 0, 0.004)), (0.003, 0.003, 0.002), n=6, rings=3, axis=eye_dir, ref=UP), M["glint"], "head")
        # Cheek tufts: the white whiskers framing the face, swept back.
        b.add(ellipsoid((0.07 * s, 0.375, 0.565), (0.05, 0.07, 0.075), n=14, rings=7, axis=V((0.3 * s, 1, -0.2)), ref=UP), M["mane"], "head")
        for i in range(5):
            base = V((0.085 * s, 0.39 - 0.012 * i, 0.61 - 0.028 * i))
            d = V((0.45 * s, -1, -0.2 - 0.25 * i))
            b.add(lock(base, d, V((s, 0, 0.2)), 0.055 + 0.02 * hash01(i, s, 11), 0.03), M["mane"], "head")
        # Small ear hidden in the mane.
        b.add(ellipsoid((0.07 * s, 0.33, 0.635), (0.018, 0.024, 0.008), n=8, rings=4, axis=V((s, 0.3, 0)), ref=UP), M["fur"], "head")
        # Long upper fang, curved a little back.
        base = V((0.024 * s, 0.572, 0.525))
        d = V((0, 0.12, -1)).normalized()
        b.add(tube([(base + V((0, 0, 0.01)), 0.0075, 0.0085), (base + d * 0.028, 0.0062, 0.007), (base + d * 0.048 + V((0, -0.004, 0)), 0.0015, 0.0015)],
                   sub=2, n=6, ref=FWD), M["tooth"], "head")
    # Upper incisors.
    b.add(ellipsoid((0, 0.628, 0.516), (0.02, 0.007, 0.009), n=8, rings=4, axis=UP, ref=FWD), M["tooth"], "head")
    # Mouth inside, shows when the jaw opens.
    b.add(ellipsoid((0, 0.50, 0.522), (0.032, 0.016, 0.085), n=12, rings=6, axis=FWD, ref=UP), M["mouth"], "head")
    # Lower jaw: a narrower loft, a little beard of mane under it; lower fangs and incisors.
    jaw = [((0, 0.405, 0.53), 0.05, 0.032), ((0, 0.49, 0.513), 0.042, 0.024), ((0, 0.57, 0.505), 0.034, 0.018), ((0, 0.625, 0.508), 0.024, 0.013)]
    b.add(tube(jaw, sub=2, n=12, ref=UP, bulge=(0.0, 0.008)), M["face"], "jaw")
    b.add(ellipsoid((0, 0.44, 0.50), (0.04, 0.02, 0.05), n=10, rings=5, axis=FWD, ref=UP), M["mane"], "jaw")
    for s in (1, -1):
        base = V((0.02 * s, 0.585, 0.512))
        d = V((0, 0.15, 1)).normalized()
        b.add(tube([(base, 0.0055, 0.006), (base + d * 0.018, 0.0045, 0.005), (base + d * 0.032, 0.0012, 0.0012)], sub=1, n=6, ref=FWD),
              M["tooth"], "jaw")
    b.add(ellipsoid((0, 0.608, 0.515), (0.015, 0.005, 0.006), n=8, rings=4, axis=UP, ref=FWD), M["tooth"], "jaw")
    b.tags[first:] = ["head"] * (len(b.verts) - first)


def build_rump(b, M):
    """The red rump pad (the callosities) round the tail root, seen from the side and from behind."""
    for s in (1, -1):
        b.add(ellipsoid((0.045 * s, -0.525, 0.425), (0.05, 0.065, 0.03), n=14, rings=6, axis=V((0.3 * s, -1, -0.5)), ref=UP), M["pad"], "hips", tag="rump")
    b.add(ellipsoid((0, -0.54, 0.465), (0.05, 0.04, 0.025), n=14, rings=6, axis=V((0, -1, -0.2)), ref=UP), M["pad"], "hips", tag="rump")


def rest_leg(leg):
    return solve_leg(leg["hip"], leg["foot"], leg["a"], leg["b"], leg["bend"])


def build_legs(b, M):
    for leg in LEGS:
        n, s, hip = leg["name"], leg["s"], leg["hip"]
        knee, normal, foot = rest_leg(leg)
        fd, td = (knee - hip).normalized(), (foot - knee).normalized()
        if n[0] == "H":  # haunch, thigh, shin, long foot
            upper = [(hip - fd * 0.06, 0.095, 0.11), (hip + fd * leg["a"] * 0.45, 0.08, 0.085), (knee, 0.05, 0.05)]
            lower = [(knee - td * 0.01, 0.046, 0.046), (knee + td * leg["b"] * 0.5, 0.038, 0.04), (foot, 0.028, 0.03)]
            umat = M["fur"]
            pw, ph, toe = 0.03, 0.02, 0.011
        else:  # the upper arm under the mane, a long forearm
            upper = [(hip - fd * 0.06, 0.075, 0.075), (hip + fd * leg["a"] * 0.5, 0.062, 0.062), (knee, 0.04, 0.04)]
            lower = [(knee - td * 0.01, 0.042, 0.042), (knee + td * leg["b"] * 0.5, 0.036, 0.036), (foot, 0.027, 0.027)]
            umat = M["mane"]
            pw, ph, toe = 0.028, 0.016, 0.01
        b.add(tube(upper, sub=2, n=12, ref=normal, bulge=(0.0, 0.01)), umat, "upper" + n, tag="limb")
        b.add(ellipsoid(knee, (upper[-1][1],) * 3, n=10, rings=5), M["fur"], "lower" + n, tag="limb")
        b.add(tube(lower, sub=2, n=10, ref=normal, bulge=(0.0, 0.01)), M["fur"], "lower" + n, tag="limb")
        # Dark bare hand / foot, the fingers or toes at the front.
        pd = leg["pdir"].normalized()
        side = pd.cross(UP).normalized()
        tip = foot + pd * leg["plen"]
        paw = [(foot - pd * 0.02, pw * 0.85, ph), (foot + pd * leg["plen"] * 0.5, pw, ph * 0.9), (tip, pw * 0.9, ph * 0.65)]
        b.add(tube(paw, sub=2, n=10, ref=UP, bulge=(0.01, 0.005)), M["hand"], "paw" + n, tag="limb")
        for k in range(4):
            off = (k - 1.5) / 1.5
            c = tip + side * off * pw * 0.85 + pd * (0.014 - 0.008 * abs(off))
            d = (pd + side * off * 0.3).normalized()
            b.add(ellipsoid(c, (toe, toe * 0.85, toe * 1.9), n=6, rings=4, axis=d, ref=UP), M["hand"], "paw" + n, tag="limb")


def coal_centre():
    """Rest position of the coal: in the right palm (under the hand, between the fingers' roots)."""
    leg = next(lg for lg in LEGS if lg["name"] == "F.R")
    pd = leg["pdir"].normalized()
    _, _, foot = rest_leg(leg)
    palm = pd.cross(X).normalized()  # down out of the palm in the rest pose
    if palm.z > 0:
        palm = -palm
    return foot + pd * 0.05 + palm * (COAL_R + 0.008)


def coal_grip():
    """Rest position of the coal bone's head: inside the right hand."""
    leg = next(lg for lg in LEGS if lg["name"] == "F.R")
    return rest_leg(leg)[2] + leg["pdir"].normalized() * 0.04


def build_coal(b, M):
    """A lumpy ember, roughly fist-size."""
    c = coal_centre()
    verts, faces, fuv = ellipsoid(c, (COAL_R, COAL_R * 0.85, COAL_R * 1.1), n=12, rings=8, axis=V((0.3, 0.2, 1)), ref=FWD)
    out = []
    for i, v in enumerate(verts):
        d = v - c
        k = 1.0 + 0.22 * (hash01(int(d.x * 400), int(d.y * 400), int(d.z * 400)) - 0.5)
        out.append(c + d * k)
    b.add((out, faces, fuv), M["coal"], "coal", tag="coal")


def tail_radius(u):
    r = 0.034 * (1 - u) ** 0.7 + 0.012 * u
    return r + 0.012 * smoothstep(0.75, 0.95, u)  # a small tuft at the tip


def build_tail(b, M, joints):
    keys = [(j, tail_radius(k / NT), tail_radius(k / NT)) for k, j in enumerate(joints)]
    bones = ["tail%02d" % k for k in range(NT)]
    b.add(tube(keys, sub=3, n=10, ref=UP, bulge=(0.0, 0.01)), M["fur"], chain_weights(joints, bones, blend=0.03), tag="tail")


# ---------------------------------------------------------------- tail

TAIL_EL = [32, 14, -28, -52, -64, -70, -72, -70, -64, -56]  # body-space elevation per segment: up, the kink, the arc


def tail_dirs(p):
    """Body-space direction per tail segment."""
    out = []
    for k in range(NT):
        u = k / (NT - 1)
        el = TAIL_EL[k] + p["tlift"] * (1 - u) ** 1.2 + p["tbend"]
        yaw = p["tsway"] * math.sin(2 * math.pi * (p["tphase"] - 0.08 * k)) * (0.2 + 0.8 * u) + p["tcurl"] * u
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
    arm = bpy.data.armatures.new("baboon_rig")
    rig = bpy.data.objects.new("baboon_rig", arm)
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
    c = coal_grip()  # the head inside the hand: a hidden (scaled down) coal shrinks into it
    bone("coal", c, c + V((0, 0.08, 0)), X, "pawF.R")
    bpy.ops.object.mode_set(mode="OBJECT")
    for pb in rig.pose.bones:
        pb.rotation_mode = "QUATERNION"
    return rig


# ---------------------------------------------------------------- animation

# Pose parameters (all interpolated between key poses):
#   x y z pitch roll yaw: the body (around PIVOT); flex_p flex_y: the chest; hp hy hr: the head; jaw: open, degrees;
#   tsway tphase tlift tbend tcurl tflop: the tail (tlift raises its root, tbend all of it); plant / fplant: hind / fore feet on the floor (1) or carried by the body (0)
#   with hup hfwd / fup ffwd offsets, curl, flail; hgy fgy: the planted hind / fore feet shifted along y;
#   wR wL: the right / left hand at the world point aR* / aL* (x mirrored), pupR pupL: that hand's fingers along the
#   forearm (palm forward); elb: the elbows turned out; coal: the coal's scale (0 hidden); wH: the hind feet planted at
#   the world point aHx aHy (sitting).
REST = dict(x=0.0, y=0.0, z=0.0, pitch=0.0, roll=0.0, yaw=0.0, flex_p=0.0, flex_y=0.0, hp=0.0, hy=0.0, hr=0.0, jaw=0.0,
            tsway=0.0, tphase=0.0, tlift=0.0, tbend=0.0, tcurl=0.0, tflop=0.0,
            plant=1.0, fplant=1.0, fup=0.0, ffwd=0.0, hup=0.0, hfwd=0.0, curl=0.0, flail=0.0, fphase=0.0, hgy=0.0, fgy=0.0,
            wR=0.0, wL=0.0, aRx=0.12, aRy=0.2, aRz=0.5, aLx=0.12, aLy=0.2, aLz=0.5, pupR=0.0, pupL=0.0, elb=0.0, coal=0.0, wH=0.0, aHx=0.13, aHy=-0.2)
STRIDE, STEP_H, CYCLES, DUTY = 0.35, 0.08, 2, 0.5  # gait cycles per walk clip, stance share


def gait(leg, t):
    """Walk: foot position (armature space) and swing amount for gait phase t."""
    ph = (t + leg["phase"]) % 1.0
    base = leg["foot"]
    if ph < DUTY:  # stance: foot slides back under the moving body
        return base + V((0, STRIDE * (0.5 - ph / DUTY), 0)), 0.0
    k = (ph - DUTY) / (1 - DUTY)
    lift = math.sin(math.pi * k)
    return base + V((0, STRIDE * (smoothstep(0, 1, k) - 0.5), STEP_H * lift * (1.2 if leg["name"][0] == "F" else 1.0))), lift


def walk_params(t):
    p = dict(REST)
    w = 2 * math.pi * CYCLES * t
    p.update(z=0.02 * math.cos(2 * w + 0.5), roll=2.5 * math.sin(w), yaw=2.0 * math.cos(w), pitch=1.2 * math.sin(2 * w),
             flex_y=-4 * math.cos(w), flex_p=1.5 * math.sin(2 * w + 0.6), hp=-2 * math.sin(2 * w + 1.0), hy=2.5 * math.cos(w + 0.5),
             tsway=7, tphase=t * CYCLES)
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
        planted = READY_FEET[n] + V((0, p["fgy"] if front else p["hgy"], 0))
        free = READY_FEET[n].copy()
        free += V((0, p["ffwd"], p["fup"])) if front else V((0, p["hfwd"], p["hup"]))
        free = free.lerp(leg["curled"], p["curl"])
        ph = 2 * math.pi * (p["fphase"] + {"F": 0.0, "H": 0.4}[n[0]] + (0.23 if s > 0 else 0.0))
        free += V((0.10 * s * math.sin(ph * 1.7), 0.14 * math.cos(ph * 1.3), 0.12 * math.sin(ph + 1))) * p["flail"]
        w = p["fplant"] if front else p["plant"]
        target = planted.lerp(mats_by_leg[n] @ free, 1 - w)
        swing = 1 - w
        if front:
            side = "R" if s > 0 else "L"
            aw = p["w" + side]
            if aw > 0:
                pt = V((p["a%sx" % side] * s + p["x"], p["a%sy" % side] + p["y"], p["a%sz" % side]))
                target = target.lerp(pt, aw)
                swing = max(swing, aw)
        elif p["wH"] > 0:  # both feet planted at a world point (sitting)
            target = target.lerp(V((p["aHx"] * s + p["x"], p["aHy"] + p["y"], FOOT_Z)), p["wH"])
        if abs(p["roll"]) < 45:
            target.z = max(target.z, FOOT_Z)
        out[n] = (target, swing)
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
        n, s = leg["name"], leg["s"]
        B = by_leg[n]
        R = B.to_3x3()
        knee0, normal0, foot0 = rest_leg(leg)
        target, swing = feet[n]
        hip = B @ leg["hip"]
        if "bends" in p:
            bend = R @ p["bends"][n]
        elif n[0] == "F" and p["elb"]:
            bend = R @ (leg["bend"] + V((s * p["elb"], 0, 0)))
        else:
            bend = R @ leg["bend"]
        knee, normal, foot = solve_leg(hip, target, leg["a"], leg["b"], bend)
        mats["upper" + n] = frame(hip, knee - hip, normal) @ frame(leg["hip"], knee0 - leg["hip"], normal0).inverted() @ rest["upper" + n]
        mats["lower" + n] = frame(knee, foot - knee, normal) @ frame(knee0, foot0 - knee0, normal0).inverted() @ rest["lower" + n]
        pd0 = leg["pdir"].normalized()
        body_pd = R @ pd0
        fore = (foot - knee).normalized()
        hang = (body_pd + fore * 0.6).normalized()
        k = min(1.0, swing * 1.5) * (0.4 if "feet" in p and "pdirs" not in p else 1.0)  # walk: the toes trail, clear of the floor
        if "pdirs" in p:  # climb: the hand lies on the rung in the given direction, curls in the swing
            pd = p["pdirs"][n].lerp(fore, k * 0.15).normalized()
            xh = X.copy()
        else:
            pd = flat(body_pd).lerp(hang, k).normalized()
            xh = flat(R @ X).lerp(R @ X, k)
            up = p.get("pup" + ("R" if s > 0 else "L"), 0.0) if n[0] == "F" else 0.0
            if up > 0:  # fingers on along the forearm, the palm forward
                pd = pd.lerp((fore + V((0, 0.25, 0))).normalized(), up).normalized()
                xh = xh.lerp(X, up)
        mats["paw" + n] = frame(foot, pd, xh) @ frame(foot0, pd0, X).inverted() @ rest["paw" + n]
    mats["coal"] = mats["pawF.R"] @ rest["pawF.R"].inverted() @ rest["coal"]
    return mats, T


class Skin:
    """numpy linear blend skinning of the vertices that rest on the floor (no tail, no coal)."""

    def __init__(self, b, rest):
        import numpy as np

        self.np = np
        keep = [i for i, t in enumerate(b.tags) if t not in ("tail", "coal")]
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


def key_frame(rig, skin, p, frame_no, floor_fix=True):
    bones = rig.data.bones
    rest = {b.name: b.matrix_local.copy() for b in bones}
    mats, T = pose_matrices(rest, p)
    S = Matrix.Translation(V((0, 0, -skin.lowest(mats) if floor_fix else 0.0)))
    mats = {n: S @ m for n, m in mats.items()}
    pts = tail_points(S @ T, p)
    xh = (S @ T).to_3x3() @ X
    for k in range(NT):
        n = "tail%02d" % k
        h0, t0 = bones[n].head_local, bones[n].tail_local
        mats[n] = frame(pts[k], pts[k + 1] - pts[k], xh) @ frame(h0, t0 - h0, X).inverted() @ rest[n]
    for b in bones:
        pose = mats[b.name]
        if b.parent:
            rel = b.parent.matrix_local.inverted() @ b.matrix_local
            basis = rel.inverted() @ mats[b.parent.name].inverted() @ pose
        else:
            basis = b.matrix_local.inverted() @ pose
        pb = rig.pose.bones[b.name]
        pb.matrix_basis = basis
        if b.name == "coal":  # a hidden coal shrinks to a point in the palm (scale 0 breaks the bake: keep it tiny)
            pb.scale = (max(p.get("coal", 0.0), 1e-4),) * 3
            pb.keyframe_insert("scale", frame=frame_no)
        pb.keyframe_insert("rotation_quaternion", frame=frame_no)
        pb.keyframe_insert("location", frame=frame_no)


LINEAR = ("fphase", "tphase")
STEP = ("coal",)  # held from key to key, switched at the next key


def interpolate(keys, frame_no):
    full = []
    for f, over in keys:
        p = {k: v for k, v in READY.items() if k != "feet"}
        p.update(over)
        full.append((f, p))
    for (f0, p0), (f1, p1) in zip(full, full[1:]):
        if f0 <= frame_no <= f1:
            t = smoothstep(f0, f1, frame_no)
            lt = (frame_no - f0) / (f1 - f0)
            out = {k: p0[k] + (p1[k] - p0[k]) * (lt if k in LINEAR else t) for k in p0}
            for k in STEP:
                out[k] = p1[k] if frame_no == f1 else p0[k]
            return out
    return full[-1][1]


ATTACK = [
    (0, {}),
    (4, dict(y=-0.05, z=-0.03, pitch=-4, flex_p=-6, hp=6, jaw=10, fgy=-0.03, tlift=6)),
    (7, dict(y=-0.04, pitch=10, flex_p=12, hp=10, jaw=48, fplant=0.0, fup=0.12, ffwd=0.06, tlift=14, tsway=6)),
    (9, dict(y=0.16, pitch=-6, flex_p=-6, hp=-12, jaw=50, fplant=0.0, fup=0.05, ffwd=0.18, plant=0.6, hfwd=0.08, tlift=10, tsway=6)),
    (11, dict(y=0.18, pitch=-6, flex_p=-8, hp=-16, jaw=0, fplant=0.3, ffwd=0.18, tlift=6, tsway=6)),
    (13, dict(y=0.16, pitch=-5, flex_p=-6, hp=-12, hy=16, hr=14, jaw=6, fplant=0.8, fgy=0.10, tsway=6)),
    (15, dict(y=0.10, pitch=-3, flex_p=-4, hp=-8, hy=-12, hr=-12, jaw=4, fgy=0.05, tsway=7)),
    (18, {}),
]

# The throw (plays once, short: the game's view is narrow). The coal is scooped off the floor by the right hand (frame 2),
# lifted high over the head while it rears on its hind legs and screeches (4..6), wound back (7) and hurled overarm: it leaves the hand at
# RELEASE_FRAME (hidden from there), the arm follows through, the hands come down and the last frame is walk frame 0.
RELEASE_FRAME = 8
RANGED = [
    (0, {}),
    (2, dict(y=-0.02, z=-0.03, pitch=-6, flex_p=-8, hp=-10, hy=-8, wR=1.0, aRx=0.10, aRy=0.40, aRz=0.14, fgy=-0.04, coal=1.0, tlift=5)),
    (4, dict(y=-0.06, pitch=52, flex_p=12, hp=-46, jaw=20, wR=1.0, aRx=0.12, aRy=-0.10, aRz=1.24, pupR=0.8, elb=0.6,
             wL=1.0, aLx=0.18, aLy=0.06, aLz=0.50, pupL=0.3, fplant=0.0, hgy=0.04, coal=1.0, tbend=40, tsway=5)),
    (5, dict(y=-0.07, pitch=62, flex_p=14, hp=-50, hy=6, jaw=46, wR=1.0, aRx=0.12, aRy=-0.24, aRz=1.40, pupR=1.0, elb=0.8,
             wL=1.0, aLx=0.24, aLy=0.06, aLz=0.56, pupL=0.3, fplant=0.0, hgy=0.05, coal=1.0, tbend=52, tsway=5)),
    (6, dict(y=-0.07, pitch=62, flex_p=14, hp=-44, hy=-6, jaw=50, wR=1.0, aRx=0.11, aRy=-0.26, aRz=1.40, pupR=1.0, elb=0.8,
              wL=1.0, aLx=0.24, aLy=0.08, aLz=0.58, pupL=0.3, fplant=0.0, hgy=0.05, coal=1.0, tbend=52, tsway=5)),
    (7, dict(y=-0.09, pitch=66, flex_p=10, hp=-46, jaw=36, wR=1.0, aRx=0.13, aRy=-0.40, aRz=1.26, pupR=0.7, elb=0.4,
              wL=1.0, aLx=0.22, aLy=0.20, aLz=0.62, pupL=0.3, fplant=0.0, hgy=0.05, coal=1.0, tbend=54, tsway=5)),
    (RELEASE_FRAME, dict(y=-0.03, pitch=48, flex_p=2, hp=-34, jaw=40, wR=1.0, aRx=0.10, aRy=0.30, aRz=1.06, pupR=0.3, elb=0.2,
                         wL=1.0, aLx=0.22, aLy=-0.02, aLz=0.50, pupL=0.2, fplant=0.0, hgy=0.05, coal=0.0, tbend=40, tsway=5)),
    (10, dict(y=0.03, pitch=24, flex_p=-6, hp=-20, jaw=24, wR=1.0, aRx=0.10, aRy=0.42, aRz=0.45, wL=0.6, aLx=0.15, aLy=0.15,
              aLz=0.25, fplant=0.0, fup=0.1, hgy=0.03, tbend=20, tsway=5)),
    (11, dict(y=0.02, pitch=6, flex_p=-4, hp=-6, jaw=8, fplant=0.4, fup=0.04, ffwd=0.06, wR=0.3, aRx=0.12, aRy=0.32, aRz=0.06,
              tlift=6, tsway=6)),
    (13, {}),
]

DIE = [
    (0, {}),
    (3, dict(pitch=14, flex_p=12, hp=24, jaw=45, fplant=0.3, fup=0.12, tlift=12, tsway=10)),
    (6, dict(pitch=6, roll=-28, flex_p=6, flex_y=12, hp=10, hr=-15, jaw=35, plant=0.5, fplant=0, fup=0.1, flail=0.5, fphase=0.3, tsway=10, tcurl=10)),
    (9, dict(tflop=0.4, roll=-72, x=-0.10, z=-0.05, flex_y=10, hp=2, hr=-20, jaw=30, plant=0, fplant=0, flail=0.9, fphase=0.8, tsway=8, tcurl=20)),
    (12, dict(tflop=0.9, roll=-96, x=-0.20, z=-0.10, flex_p=-8, flex_y=6, hp=-6, hr=-15, jaw=28, plant=0, fplant=0, flail=1.0, fphase=1.4, tsway=6, tcurl=30)),
    (14, dict(tflop=1.0, roll=-104, x=-0.22, z=-0.12, flex_p=-10, hp=-8, hr=-10, jaw=24, plant=0, fplant=0, flail=0.8, fphase=1.8, tsway=4, tcurl=35)),
    (17, dict(tflop=1.0, roll=-102, x=-0.22, z=-0.12, flex_p=-8, hp=-12, hr=-10, jaw=22, plant=0, fplant=0, flail=0.6, fphase=2.5, tsway=2, tcurl=38, curl=0.1)),
    (21, dict(tflop=1.0, roll=-101, x=-0.22, z=-0.12, flex_p=-8, hp=-12, hr=-14, jaw=18, plant=0, fplant=0, flail=0.3, fphase=3.1, tcurl=40, curl=0.25)),
    (24, dict(tflop=1.0, roll=-100, x=-0.22, z=-0.12, flex_p=-8, hp=-14, hr=-15, jaw=15, plant=0, fplant=0, flail=0.1, fphase=3.6, tcurl=40, curl=0.35)),
    (27, dict(tflop=1.0, roll=-100, x=-0.22, z=-0.12, flex_p=-8, hp=-14, hr=-15, jaw=14, plant=0, fplant=0, flail=0.0, fphase=3.8, tcurl=40, curl=0.4)),
]

# Idle (loops): sits on its haunches facing the sun, both hands raised, palms forward; breathes and sways.
IDLE_FRAMES = 32
IDLE_SIT = dict(y=0.0, z=-0.25, pitch=60, flex_p=8, hp=-46, wH=1.0, aHx=0.15, aHy=-0.10, plant=1.0, fplant=0.0, wR=1.0, wL=1.0, tbend=45,
                aRx=0.24, aRy=0.20, aRz=1.02, aLx=0.24, aLy=0.20, aLz=1.02, pupR=1.0, pupL=1.0, elb=1.0, tcurl=30)


def idle_params(f):
    u = 2 * math.pi * f / IDLE_FRAMES
    p = {k: v for k, v in READY.items() if k != "feet"}
    p.update(IDLE_SIT)
    breath = math.sin(u)
    sway = math.sin(u + 0.7)
    p.update(flex_p=IDLE_SIT["flex_p"] + 2.5 * breath, pitch=IDLE_SIT["pitch"] + 1.2 * breath, roll=2.0 * sway, yaw=1.5 * math.sin(u - 0.4),
             hp=IDLE_SIT["hp"] - 3 * breath, hr=-2.5 * sway, hy=3 * math.sin(u + 1.4), jaw=3 + 3 * max(0.0, math.sin(2 * u)),
             aRz=IDLE_SIT["aRz"] + 0.03 * breath, aLz=IDLE_SIT["aLz"] + 0.03 * breath,
             aRx=IDLE_SIT["aRx"] + 0.02 * sway, aLx=IDLE_SIT["aLx"] - 0.02 * sway, tsway=6, tphase=f / IDLE_FRAMES)
    return p


# Ladder climb (loops), as rat.py: the engine draws it turned round (+Y at the ladder, the back to the camera) and sets
# the frame from the climb height, one cycle per CLIMB_RUNGS rungs (climbRise; the clock does not advance it). The body
# stays put; a gripping hand or foot slides down at that rate, so it holds still on its rung while the engine lifts the
# body. Gait: hand over hand, as a primate climbs: the left hand with the right foot, then the right hand with the left
# foot.
KIND_SCALE, TILE = 24, 40  # the scale the clip is authored for (KINDS; change it with the scale, then --export-climb), world units per tile
LADDER = dict(rungs=12, rung_front=0.013 + 0.7 * 0.0082, rung_r=0.0082)  # ladder.py, in tiles
CLIMB_RUNGS = 2  # rungs per cycle (R)
CLIMB_PITCH, CLIMB_Z, CLIMB_BELLY = 80, 0.36, 0.20  # body pitch, lift, rung front this far in front of PIVOT (+Y)
# Per limb: stance start, stance share, x, tip height above its hip or shoulder (stance middle), hand / foot direction.
CLIMB_LIMBS = {"F.R": (0.5, 0.55, 0.12, 0.20, V((0.15, 0.4, 1))), "F.L": (0.0, 0.55, -0.12, 0.20, V((-0.15, 0.4, 1))),
               "H.R": (0.04, 0.55, 0.15, -0.06, V((0.3, 0.35, 1))), "H.L": (0.54, 0.55, -0.15, -0.06, V((-0.3, 0.35, 1)))}
CLIMB_BENDS = {"F.R": V((0.8, -1, 0)), "F.L": V((-0.8, -1, 0)), "H.R": V((0.6, 1, 0.3)), "H.L": V((-0.6, 1, 0.3))}
CLIMB_LIFT = 0.08  # a swinging hand clears the rungs by this much (towards -Y)


def climb_body(u):
    w = 2 * math.pi * u
    p = dict(REST)
    p.update(z=CLIMB_Z + 0.012 * math.cos(2 * w), pitch=CLIMB_PITCH, roll=3 * math.sin(w), yaw=2 * math.cos(w),
             flex_p=2 * math.cos(2 * w), flex_y=-4 * math.sin(w), hp=-68 + 3 * math.cos(2 * w), hy=4 * math.sin(w + 0.8),
             hr=-3 * math.sin(w), tlift=-10, tsway=8, tphase=u, tcurl=10)
    return p


def climb_setup(ref):
    """Ladder metrics in authored units and every limb's hold height (its tip at the start of its stance)."""
    tile = TILE * ref["S"] / KIND_SCALE
    rs = tile / LADDER["rungs"]
    c = dict(tile=tile, rs=rs, R=CLIMB_RUNGS * rs, r=LADDER["rung_r"] * tile)
    c["y_rung"] = PIVOT.y + CLIMB_BELLY + c["r"]  # rung centre plane
    c["G"] = c["y_rung"] + LADDER["rung_front"] * tile  # rail centre plane
    rest = {leg["name"]: leg for leg in LEGS}
    T, C, _, _ = body_mats(climb_body(0.0))
    c["top"] = {}
    for n, (u0, share, x, dz, pd) in CLIMB_LIMBS.items():
        base = (T if n[0] == "H" else C) @ rest[n]["hip"]
        off = 0.5 * rs - u0 * c["R"]  # world = phase * R + clip must be a hold: (k + 0.5) * rs
        want = base.z + dz + share * c["R"] / 2
        c["top"][n] = off + rs * round((want - off) / rs)
    return c


def climb_params(u, c):
    p = climb_body(u)
    feet, pdirs = {}, {}
    a = math.radians(50)  # tip on the rung's front top
    for leg in LEGS:
        n = leg["name"]
        u0, share, x, dz, pd = CLIMB_LIMBS[n]
        ph = (u - u0) % 1.0
        if ph < share:  # gripping: slides down at the climb rate, still on the ladder
            z, lift = c["top"][n] - ph * c["R"], 0.0
        else:  # reaching up to the next hold, clear of the rungs
            k = (ph - share) / (1 - share)
            z, lift = c["top"][n] - share * c["R"] * (1 - smoothstep(0, 1, k)), math.sin(math.pi * k)
        tip = V((x, c["y_rung"] - (c["r"] + 0.012) * math.cos(a) - CLIMB_LIFT * lift, z + (c["r"] + 0.012) * math.sin(a) + 0.02 * lift))
        pdirs[n] = pd.normalized()
        feet[n] = (tip - pdirs[n] * leg["plen"], lift)
    p.update(feet=feet, pdirs=pdirs, bends=CLIMB_BENDS)
    return p


def ref_box(obj, rig, act):
    """Bounding box of the evaluated mesh at frame 0 of act (the engine's normalization reference)."""
    import numpy as np

    rig.animation_data.action = act
    bpy.context.scene.frame_set(0)
    ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    co = np.empty(len(ev.data.vertices) * 3)
    ev.data.vertices.foreach_get("co", co)
    co = co.reshape(-1, 3)
    lo, hi = co.min(axis=0), co.max(axis=0)
    return dict(S=float((hi - lo).max()), C=float((lo[1] + hi[1]) / 2), H=float(hi[2] - lo[2]), lo=lo, hi=hi)


def climb_report(ref, c, frames):
    rise, grip = c["R"] / ref["H"], (c["G"] - ref["C"]) / ref["H"]
    print("baboon_climb: frames {} R {:.4f} G {:.4f} C {:.4f} H {:.4f} S_ref {:.4f} tile {:.4f} rung {:.4f} (KIND_SCALE {})".format(
        frames, c["R"], c["G"], ref["C"], ref["H"], ref["S"], c["tile"], c["rs"], KIND_SCALE))
    print("baboon_climb: climbRise = {:.4f}f, climbGrip = {:.4f}f".format(rise, grip))
    reach = {}
    for f in range(frames):
        p = climb_params(f / frames, c)
        T, C, _, _ = body_mats(p)
        for leg in LEGS:
            n = leg["name"]
            hip = (T if n[0] == "H" else C) @ leg["hip"]
            reach[n] = max(reach.get(n, 0), (p["feet"][n][0] - hip).length / (leg["a"] + leg["b"]))
    print("baboon_climb: max reach share (>= 0.999 falls short) " + " ".join("{} {:.3f}".format(k, v) for k, v in reach.items()))


def ladder_standin(coll, rig, c, frames):
    """Review only (BABOON_LADDER=1): rails and rungs as in ladder.py, scrolling down R per climb cycle."""
    import bmesh

    tile = c["tile"]
    mesh = bpy.data.meshes.new("baboon_ladder")
    bm = bmesh.new()
    for s in (1, -1):
        m = Matrix.Translation(V((0.1 * tile * s, c["G"], 0.5 * tile)))
        bmesh.ops.create_cone(bm, cap_ends=True, segments=12, radius1=0.013 * tile, radius2=0.013 * tile, depth=2 * tile, matrix=m)
    for k in range(-12, 24):
        m = Matrix.Translation(V((0, c["y_rung"], (k + 0.5) * c["rs"]))) @ Matrix.Rotation(math.pi / 2, 4, "Y")
        bmesh.ops.create_cone(bm, cap_ends=True, segments=10, radius1=c["r"], radius2=c["r"], depth=0.24 * tile, matrix=m)
    bm.to_mesh(mesh)
    bm.free()
    lad = bpy.data.objects.new("baboon_ladder", mesh)
    coll.objects.link(lad)
    lad.data.materials.append(common.make_material("baboon_ladder_wood", ("solid", "wood"), {"wood": (0.35, 0.24, 0.14)}))

    def follow(scene, *_):
        act = rig.animation_data.action
        on = act is not None and act.name == "baboon_climb"
        lad.hide_render = lad.hide_viewport = not on
        lad.location.z = -(scene.frame_current % frames) / frames * c["R"] if on else 0.0

    bpy.app.handlers.frame_change_post.append(follow)


def make_actions(rig, skin, obj):
    rig.animation_data_create()
    acts = {}
    keyed = {"baboon_attack": ATTACK, "baboon_ranged": RANGED, "baboon_die": DIE}
    for name, suffix, frames in CLIPS:
        act = bpy.data.actions.get(name)
        if act:
            bpy.data.actions.remove(act)
        act = bpy.data.actions.new(name)
        act.use_fake_user = True
        rig.animation_data.action = act
        if name == "baboon_climb":  # authored in the walk's normalized space: measure the walk's frame 0 first
            ref = ref_box(obj, rig, acts["baboon_walk"])
            rig.animation_data.action = act
            c = climb_setup(ref)
            climb_report(ref, c, frames)
            if os.environ.get("BABOON_LADDER"):
                ladder_standin(obj.users_collection[0], rig, c, frames)
            for f in range(frames + 1):
                key_frame(rig, skin, climb_params(f / frames, c), f, floor_fix=False)
            acts[name] = act
            continue
        last = frames - 1 if name in ONCE else frames  # loops key frame N = frame 0
        for f in range(last + 1):
            if name == "baboon_walk":
                p = walk_params(f / frames)
            elif name == "baboon_idle":
                p = idle_params(f)
            else:
                p = interpolate(keyed[name], f)
                p["tphase"] = f / CLIPS[0][2] * CYCLES  # keeps the tail sway going from where the walk's frame 0 is
            key_frame(rig, skin, p, f)
        acts[name] = act
    return acts


# ---------------------------------------------------------------- entry points


def build(bake=True, tex_path=None):
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    M = materials(COL)
    tail = tail_points(Matrix(), dict(REST))
    rig = build_rig(coll, tail)
    b = Builder()
    build_body(b, M)
    build_mane(b, M)
    build_head(b, M)
    build_rump(b, M)
    b.verts = [v + V((0, 0, DZ)) for v in b.verts]
    build_legs(b, M)
    build_coal(b, M)
    build_tail(b, M, tail)
    obj = common.finish_mesh(b, coll, "baboon_new")
    common.uv_unwrap(obj, b.tags, {"head": (V((0, 0.48, 0.57)), 1.8), "coal": (coal_centre(), 2.0)})
    if bake:
        tex = common.bake_texture(obj, tex_path or os.path.join(bpy.app.tempdir or "/tmp", "baboon_preview.png"), TEX_SIZE, "baboon",
                                  ao_distance=0.12)
        common.use_baked_material(obj, tex)
    common.rig_object(obj, rig)
    rest = {bn.name: bn.matrix_local.copy() for bn in rig.data.bones}
    acts = make_actions(rig, Skin(b, rest), obj)
    rig.animation_data.action = acts["baboon_walk"]
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 0, CLIPS[0][2] - 1
    scene.frame_set(0)
    box = ref_box(obj, rig, acts["baboon_walk"])
    size = box["hi"] - box["lo"]
    print("baboon: {} verts, {} tris; walk frame 0: length {:.3f} (y), width {:.3f} (x), height {:.3f} (z)".format(
        len(obj.data.vertices), common.tri_count(obj), size[1], size[0], size[2]))
    speed = STRIDE / DUTY * CYCLES / CLIPS[0][2] * 14
    print("baboon: walk ground speed {:.3f} m/s at 14 fps = {:.3f} reference sizes per second".format(speed, speed / box["S"]))
    return obj, rig


def measure():
    """The ranged attack's release: the coal per ranged frame, its height as a share of the reference (walk frame 0)
    height (RangedRules::releaseY), how far ahead of the reference's centre in the engine's half widths (the larger of
    the half width and half length, ModelInfo::HalfWidth; RangedRules::releaseAhead)."""
    obj, rig = bpy.data.objects["baboon_new"], bpy.data.objects["baboon_rig"]
    scene = bpy.context.scene

    def mesh_box():
        ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
        pts = [ev.matrix_world @ v.co for v in ev.to_mesh().vertices]
        ev.to_mesh_clear()
        return (min(q.z for q in pts), max(q.z for q in pts), min(q.y for q in pts), max(q.y for q in pts),
                max(q.x for q in pts) - min(q.x for q in pts))

    rig.animation_data.action = bpy.data.actions["baboon_walk"]
    scene.frame_set(0)
    z0, z1, y0, y1, width = mesh_box()
    height, centre, half = z1 - z0, (y0 + y1) / 2, max(width, y1 - y0) / 2
    print("MEASURE reference: height {:.3f}, centre y {:.3f}, half width {:.3f} (x) / length {:.3f} (y): the engine's {:.3f}".format(
        height, centre, width / 2, (y1 - y0) / 2, half))
    frames = next(c[2] for c in CLIPS if c[0] == "baboon_ranged")
    rig.animation_data.action = bpy.data.actions["baboon_ranged"]
    pb = rig.pose.bones["coal"]
    off = pb.bone.matrix_local.to_3x3().inverted() @ (coal_centre() - coal_grip())

    def coal():  # the coal's centre as if held (its scale ignored)
        return rig.matrix_world @ (pb.head + pb.matrix.to_3x3().normalized() @ off)

    for f in range(frames):
        scene.frame_set(f)
        c = coal()
        print("COAL ranged f{:02d} ({:.2f} of the clip){}: height {:.3f} = {:.3f} of the reference, ahead {:+.3f} = {:+.2f} half widths".format(
            f, f / (frames - 1), " held" if pb.scale[0] > 0.01 else "     ", c.z - z0, (c.z - z0) / height, c.y - centre, (c.y - centre) / half))
    scene.frame_set(RELEASE_FRAME)
    c = coal()
    print("MEASURE release: frame {} of {}: release = {:.3f}f, releaseY = {:.3f}f, releaseAhead = {:.3f}f".format(
        RELEASE_FRAME, frames, RELEASE_FRAME / (frames - 1), (c.z - z0) / height, (c.y - centre) / half))
    rig.animation_data.action = bpy.data.actions["baboon_walk"]
    scene.frame_set(0)


def export(models_dir=None, clips=CLIPS):
    """Write media/models/monsters/baboon{,_att,_att_ranged,_die,_idle,_climb}.md3 from the built objects."""
    common.export_files(bpy.data.objects["baboon_new"], bpy.data.objects["baboon_rig"], "baboon", clips, "monsters", models_dir)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    tex_dir = os.path.join(REPO, "media", "textures", "monsters")
    if "--export" in args:
        build(tex_path=os.path.join(tex_dir, "baboon.png"))
        export()
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "baboon.blend"))
    elif "--export-climb" in args:  # only baboon_climb.md3 (the texture and the other clips stay as they are)
        build(bake=False)
        export(clips=[c for c in CLIPS if c[1] == "_climb"])
    elif "--measure" in args:
        build(bake=False)
        measure()
    else:
        build()
