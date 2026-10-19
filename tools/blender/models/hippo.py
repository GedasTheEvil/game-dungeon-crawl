"""Procedural hippopotamus (the deadliest animal of the Nile): mesh, rig, baked texture and the .md3 animations
(walk, attack, die and idle: lying low on its belly, the pose the engine lowers into water).

    MCP:  p = ".../tools/blender/models/hippo.py"; g = {"__file__": p, "__name__": "hippo"}
          exec(open(p).read(), g); g["build"]()          # then g["export"]()
    CLI:  blender -b --python tools/blender/models/hippo.py -- [--export] [--measure] [--bake]

Blender space: Z up, the hippo faces +Y (game uses rotA = 180, as the crocodile and the rat), its right side is +X.
Built like crocodile.py: one barrel loft for the body blended over hips / chest / head bones (plus a leaf 'breath' bone
that swells the flanks in the idle clip), a massive rigid head with a broad muzzle and a hinged lower jaw that gapes
to the hippo's yawn (about 125 degrees), big lower canines (tusks) and forward-pointing lower incisors, small upper
canines, the four short thick legs by two-bone IK towards foot targets (planted, in body space, or tucked under the
belly), round feet with four nailed toes, two ear bones (they flick in the idle clip) and a short flat FK tail.
Every frame the whole pose is lifted so the lowest non-tail vertex touches the floor.
Eyes, ears and nostrils sit raised on top of the head; the idle pose lifts and tilts the head so that they are the
highest points (the engine draws the idle hippo lowered into water, only the top SUBMERGED_SHOW showing).
--measure prints every clip's frame 0 size and the eye / ear / nostril / body tops as fractions of the clip's height.

Walk frame 0 (the reference): 1.63 long x 0.69 high x 0.62 wide, so KINDS scale 52 makes it 1.3 tiles nose to tail.
Idle: 0.60 high (all frames); ears 1.00, eyes 0.99, nostrils 0.99 of it, the skull 0.92 and the body 0.90: with KINDS
scale 52 the idle is about 19 units high, the shown 0.5 units (2.6 %) are just the ears, the eyes and the nostrils.
Clips: walk 24 (the reference: one heavy plodding stride, in place; also shown during the charge rush), attack 24
(loops from walk frame 0; also the charge wind-up: the threat yawn with the head thrown up and the jaws gaping, then
the lunge, a crushing snap and a head shake), die 30 (sags, rolls onto its side, legs limp), idle 32 (`hippo_idle.md3`:
lying low on the belly, legs tucked, head raised, slow breath, ears flicking now and then). No rise clip.
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

COLL = "hippo_new"
CLIPS = [("hippo_walk", "", 24), ("hippo_attack", "_att", 24), ("hippo_die", "_die", 30), ("hippo_idle", "_idle", 32)]
LOOPS = ("hippo_walk", "hippo_attack", "hippo_idle")
TAGS = []  # per-vertex part tag of the last build (measure())
TEX_SIZE = 1024
REVIEW_VIEW = {"target": (0, 0.1, 0.38), "ortho": 2.0, "res": (560, 440)}

COL = {
    "belly": (0.40, 0.22, 0.20),
    "flank": (0.17, 0.115, 0.115),
    "back": (0.10, 0.072, 0.080),
    "dark": (0.06, 0.044, 0.052),
    "pink": (0.52, 0.24, 0.23),
    "fold": (0.30, 0.13, 0.13),
    "nail": (0.30, 0.25, 0.20),
    "eye": (0.16, 0.07, 0.03),
    "pupil": (0.01, 0.008, 0.008),
    "nostril": (0.02, 0.012, 0.012),
    "tooth": (0.80, 0.72, 0.52),
    "mouth": (0.70, 0.30, 0.32),
    "throat": (0.38, 0.10, 0.12),
    "tuft": (0.05, 0.04, 0.04),
}

UP, FWD = V((0, 0, 1)), V((0, 1, 0))
PIVOT = V((0, -0.30, 0.40))  # hips bone: body rotations happen around this point
MID = V((0, 0.02, 0.41))  # chest bone: spine flex
NECK = V((0, 0.36, 0.44))
ZM = 0.29  # mouth line height
JAW_HINGE = V((0, 0.47, ZM + 0.01))
TAIL_BASE = V((0, -0.585, 0.47))
NT, TAIL_LEN = 3, 0.15  # tail bones, length
FOOT_Z = 0.035  # ankle / wrist height above the floor
BREATH_C = V((0, -0.05, 0.16))  # the breath bone scales the flanks about this point (belly bottom)
EARS = {s: V((0.105 * s, 0.415, 0.545)) for s in (1, -1)}  # ear bases (ear bones pivot here)

# Head profiles: (y, top or bottom, half width). Upper jaw: skull top; lower jaw: chin bottom. Both meet at ZM.
UPPER = [(0.39, 0.55, 0.19), (0.46, 0.565, 0.20), (0.54, 0.56, 0.185), (0.62, 0.53, 0.155), (0.69, 0.52, 0.16),
         (0.76, 0.525, 0.178), (0.82, 0.52, 0.185), (0.865, 0.495, 0.172), (0.895, 0.44, 0.135), (0.905, 0.38, 0.09)]
LOWER = [(0.42, 0.17, 0.17), (0.52, 0.14, 0.165), (0.64, 0.13, 0.15), (0.75, 0.135, 0.158), (0.83, 0.15, 0.165),
         (0.875, 0.19, 0.152), (0.90, 0.245, 0.11)]


def leg_defs():
    """Rest layout of the four legs (ankle / wrist = foot). s = +1 right, -1 left. Lateral-sequence walk."""
    legs = []
    for s in (1, -1):
        sfx = ".R" if s > 0 else ".L"
        legs.append(dict(name="H" + sfx, s=s, hip=V((0.175 * s, -0.34, 0.33)), foot=V((0.205 * s, -0.33, FOOT_Z)), a=0.17, b=0.155,
                         bend=V((0.1 * s, 1, 0)), pdir=V((0.12 * s, 1, 0)), r=(0.115, 0.098, 0.082, 0.07),
                         tuck=V((0.25 * s, -0.27, 0.25)), tuck_bend=V((0.3 * s, 1, 0.05)), tuck_pd=V((0.3 * s, 1, 0)),
                         curled=V((0.30 * s, -0.25, 0.24)), phase=0.0 if s < 0 else 0.5))
        legs.append(dict(name="F" + sfx, s=s, hip=V((0.165 * s, 0.20, 0.33)), foot=V((0.195 * s, 0.23, FOOT_Z)), a=0.165, b=0.15,
                         bend=V((0.1 * s, -1, 0)), pdir=V((0.15 * s, 1, 0)), r=(0.105, 0.09, 0.078, 0.067),
                         tuck=V((0.24 * s, 0.14, 0.25)), tuck_bend=V((0.3 * s, -1, 0.05)), tuck_pd=V((0.3 * s, -1, 0)),
                         curled=V((0.28 * s, 0.30, 0.24)), phase=0.25 if s < 0 else 0.75))
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


def profile(table, y):
    """Linear interpolation in a head profile table: (z, half width) at y."""
    if y <= table[0][0]:
        return table[0][1:]
    for (y0, z0, r0), (y1, z1, r1) in zip(table, table[1:]):
        if y <= y1:
            t = (y - y0) / (y1 - y0)
            return z0 + (z1 - z0) * t, r0 + (r1 - r0) * t
    return table[-1][1:]


def superellipse(p):
    def fn(th, a=0.0):
        return (abs(math.cos(th)) ** p + abs(math.sin(th)) ** p) ** (-1.0 / p)

    return fn


def head_top(x, y):
    """Height of the upper head surface at (x, y) in the rest pose (for the eyes, ears and nostrils)."""
    top, r = profile(UPPER, y)
    u = min(abs(x) / r, 1.0)
    return ZM + (top - ZM) * (1 - u ** 2.6) ** (1 / 2.6)


# ---------------------------------------------------------------- materials


def socket(sockets, ident):
    return next(s for s in sockets if s.identifier == ident)


def hide_material(pal):
    """Dark grey-purple-brown back fading to a pinkish belly (rest-pose normal z), fine skin cracks (voronoi edges),
    mottling, pink lips round the mouth line and pink creases on the neck (object coordinates)."""
    mat = bpy.data.materials.get("hippo_hide") or bpy.data.materials.new("hippo_hide")
    nt = mat.node_tree
    for node in list(nt.nodes):
        if node.type not in ("OUTPUT_MATERIAL", "BSDF_PRINCIPLED"):
            nt.nodes.remove(node)
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Roughness"].default_value = 0.5
    new, link = nt.nodes.new, nt.links.new

    geo = new("ShaderNodeNewGeometry")
    sep = new("ShaderNodeSeparateXYZ")
    link(geo.outputs["Normal"], sep.inputs[0])
    tone = new("ShaderNodeValToRGB")
    els = tone.color_ramp.elements
    els[0].position, els[0].color = 0.12, (*pal["belly"], 1)
    els[1].position, els[1].color = 0.95, (*pal["dark"], 1)
    els.new(0.40).color = (*pal["flank"], 1)
    els.new(0.70).color = (*pal["back"], 1)
    remap = new("ShaderNodeMath")
    remap.operation = "MULTIPLY_ADD"
    remap.inputs[1].default_value = 0.5
    remap.inputs[2].default_value = 0.5
    link(sep.outputs["Z"], remap.inputs[0])
    link(remap.outputs[0], tone.inputs["Fac"])

    coord = new("ShaderNodeTexCoord")
    osep = new("ShaderNodeSeparateXYZ")
    link(coord.outputs["Object"], osep.inputs[0])

    def mapped(stretch):
        mp = new("ShaderNodeMapping")
        mp.inputs["Scale"].default_value = stretch
        link(coord.outputs["Object"], mp.inputs["Vector"])
        return mp

    def ramp(src, lo, hi):
        r = new("ShaderNodeValToRGB")
        re = r.color_ramp.elements
        re[0].position, re[0].color = lo[0], (lo[1],) * 3 + (1,)
        re[1].position, re[1].color = hi[0], (hi[1],) * 3 + (1,)
        link(src, r.inputs["Fac"])
        return r

    def noise(scale, detail, lo, hi, stretch=(1, 1, 1)):
        tex = new("ShaderNodeTexNoise")
        tex.inputs["Scale"].default_value = scale
        tex.inputs["Detail"].default_value = detail
        link(mapped(stretch).outputs["Vector"], tex.inputs["Vector"])
        return ramp(tex.outputs["Fac"], lo, hi)

    def clamp_lin(src, mul, add):
        m = new("ShaderNodeMath")
        m.operation = "MULTIPLY_ADD"
        m.use_clamp = True
        m.inputs[1].default_value, m.inputs[2].default_value = mul, add
        link(src, m.inputs[0])
        return m.outputs[0]

    def mul(a, b):
        m = new("ShaderNodeMath")
        m.operation = "MULTIPLY"
        link(a, m.inputs[0])
        link(b, m.inputs[1])
        return m.outputs[0]

    def mix(fac, a, colour):
        m = new("ShaderNodeMix")
        m.data_type = "RGBA"
        link(fac, socket(m.inputs, "Factor_Float"))
        link(a, socket(m.inputs, "A_Color"))
        socket(m.inputs, "B_Color").default_value = (*colour, 1)
        return socket(m.outputs, "Result_Color")

    vor = new("ShaderNodeTexVoronoi")
    vor.feature = "DISTANCE_TO_EDGE"
    vor.inputs["Scale"].default_value = 1.0
    link(mapped((30, 30, 30)).outputs["Vector"], vor.inputs["Vector"])
    cracks = ramp(vor.outputs["Distance"], (0.0, 0.72), (0.06, 1.0))

    out = tone.outputs["Color"]
    for f in (cracks.outputs["Color"], noise(3.5, 4.0, (0.3, 0.78), (0.7, 1.15)).outputs["Color"]):
        m = new("ShaderNodeVectorMath")
        m.operation = "MULTIPLY"
        link(out, m.inputs[0])
        link(f, m.inputs[1])
        out = m.outputs[0]
    # Pink lips: a band along the mouth line of the head (forward of the jaw hinge).
    zd = new("ShaderNodeMath")
    zd.operation = "SUBTRACT"
    link(osep.outputs["Z"], zd.inputs[0])
    zd.inputs[1].default_value = ZM
    za = new("ShaderNodeMath")
    za.operation = "ABSOLUTE"
    link(zd.outputs[0], za.inputs[0])
    lip = mul(clamp_lin(za.outputs[0], -1 / 0.075, 1.4), clamp_lin(osep.outputs["Y"], 1 / 0.08, -0.46 / 0.08))
    out = mix(lip, out, pal["pink"])
    # Neck creases: pink-dark bands round the neck on the flanks and the throat.
    wave = new("ShaderNodeTexWave")
    wave.bands_direction = "Y"
    wave.inputs["Scale"].default_value = 4.0
    wave.inputs["Distortion"].default_value = 2.0
    wave.inputs["Detail"].default_value = 1.0
    link(coord.outputs["Object"], wave.inputs["Vector"])
    creases = ramp(wave.outputs["Fac"], (0.80, 0.0), (0.95, 1.0))
    neck = mul(clamp_lin(osep.outputs["Y"], 12.0, -0.24 * 12.0), clamp_lin(osep.outputs["Y"], -12.0, 0.46 * 12.0))
    out = mix(mul(mul(neck, creases.outputs["Color"]), clamp_lin(sep.outputs["Z"], -0.8, 0.9)), out, pal["fold"])
    link(out, bsdf.inputs["Base Color"])
    return mat


def materials(pal):
    specs = {name: ("solid", name) for name in ("pink", "nail", "eye", "pupil", "nostril", "tooth", "mouth", "throat", "tuft")}
    M = {name: common.make_material("hippo_" + name, spec, pal) for name, spec in specs.items()}
    M["hide"] = hide_material(pal)
    return M


# ---------------------------------------------------------------- parts

BODY_KEYS = [(-0.62, 0.41, 0.06, 0.06), (-0.585, 0.415, 0.17, 0.17), (-0.50, 0.42, 0.25, 0.23), (-0.34, 0.42, 0.29, 0.255),
             (-0.12, 0.415, 0.305, 0.26), (0.08, 0.415, 0.30, 0.255), (0.22, 0.425, 0.275, 0.24), (0.34, 0.44, 0.225, 0.20),
             (0.43, 0.405, 0.20, 0.145), (0.52, 0.375, 0.165, 0.12)]


def body_shape(th, a):
    k = superellipse(2.3)(th)
    sn = math.sin(th)
    return k * (1.0 if sn >= 0 else 1.0 - 0.06 * sn * sn)


_body_w = chain_weights([V((0, -0.8, 0.41)), MID, NECK, V((0, 1.0, 0.44))], ["hips", "chest", "head"], blend=0.12)


def body_weight(co):
    """Spine blend plus a share of the breath bone over the flanks and back between hips and shoulders."""
    w = _body_w(co)
    k = 0.7 * smoothstep(-0.45, -0.2, co.y) * (1 - smoothstep(0.08, 0.28, co.y))
    if k <= 0:
        return w
    out = {n: v * (1 - k) for n, v in w.items()}
    out["breath"] = out.get("breath", 0.0) + k
    return out


def build_body(b, M):
    keys = [((0, y, z), rx, ry) for y, z, rx, ry in BODY_KEYS]
    b.add(tube(keys, sub=3, n=24, ref=UP, shape=body_shape, bulge=(0.02, 0.0)), M["hide"], body_weight)


def head_shape(th, a):
    return superellipse(2.6)(th)


def build_head(b, M):
    first = len(b.verts)
    # Upper jaw and skull: section centred on the mouth line, its lower half squashed to a flat palate.
    keys = [((0, y, ZM), r, top - ZM) for y, top, r in UPPER]
    up = common.transform(tube(keys, sub=3, n=22, ref=UP, shape=head_shape, bulge=(0.0, 0.01)),
                          lambda v: V((v.x, v.y, ZM - (ZM - v.z) * 0.06)) if v.z < ZM else v)
    b.add(up, M["hide"], "head")
    # Lower jaw (deep, rounded chin), its upper half squashed flat.
    keys = [((0, y, ZM), r, ZM - bot) for y, bot, r in LOWER]
    lo = common.transform(tube(keys, sub=3, n=20, ref=UP, shape=head_shape, bulge=(0.0, 0.01)),
                          lambda v: V((v.x, v.y, ZM + (v.z - ZM) * 0.08)) if v.z > ZM else v)
    b.add(lo, M["hide"], "jaw")
    # Palate (pink under the upper jaw), tongue on the lower jaw, throat at the back filling the gape's corners.
    pal = [((0, y, ZM - 0.016), r * 0.93, 0.008) for y, top, r in UPPER[1:-1]]
    b.add(tube(pal, sub=1, n=12, ref=UP), M["mouth"], "head")
    ton = [((0, y, ZM + 0.014), r * 0.78, 0.016) for y, bot, r in LOWER[:-1]]
    b.add(tube(ton, sub=1, n=12, ref=UP), M["mouth"], "jaw")
    b.add(ellipsoid((0, 0.47, ZM + 0.01), (0.13, 0.07, 0.10), n=14, rings=7, axis=FWD, ref=UP), M["throat"], "head")
    for s in (1, -1):  # mouth corners: pink cheek pouches that stretch between the jaws
        b.add(ellipsoid((0.13 * s, 0.50, ZM), (0.035, 0.07, 0.05), n=10, rings=5, axis=FWD, ref=UP), M["mouth"],
              {"head": 0.5, "jaw": 0.5})
    # Tusks: the lower canines, from the lower jaw's front corners curving up, out and back.
    for s in (1, -1):
        base = V((0.112 * s, 0.825, ZM - 0.02))
        pts = [base, base + V((0.012 * s, 0.006, 0.05)), base + V((0.026 * s, -0.006, 0.10)), base + V((0.034 * s, -0.03, 0.135))]
        keys = [(p, r, r) for p, r in zip(pts, (0.026, 0.022, 0.015, 0.003))]
        b.add(tube(keys, sub=2, n=8, ref=FWD, caps=(True, True)), M["tooth"], "jaw")
        # Lower incisors: pegs pointing forward out of the chin.
        for x, ln in ((0.032, 0.07), (0.075, 0.06)):
            c = V((x * s, 0.865, ZM - 0.02))
            b.add(common.cone(c, V((0.1 * s, 1, 0.55)), ln, 0.014, n=6), M["tooth"], "jaw")
        # Upper canines: short and pointing down behind the tusks; small upper incisors.
        b.add(common.cone(V((0.125 * s, 0.775, ZM + 0.02)), V((0.05 * s, 0.15, -1)), 0.065, 0.017, n=6), M["tooth"], "head")
        b.add(common.cone(V((0.045 * s, 0.875, ZM + 0.015)), V((0.05 * s, 0.4, -1)), 0.035, 0.010, n=5), M["tooth"], "head")
        # Molars: a short row of blunt bumps on each side.
        for i in range(4):
            y = 0.56 + i * 0.045
            _, rl = profile(LOWER, y)
            b.add(ellipsoid((s * rl * 0.62, y, ZM + 0.012), (0.016, 0.02, 0.014), n=6, rings=3), M["tooth"], "jaw")
            _, ru = profile(UPPER, y)
            b.add(ellipsoid((s * ru * 0.62, y, ZM - 0.010), (0.016, 0.02, 0.012), n=6, rings=3), M["tooth"], "head")
    # Whisker stubble on the muzzle: a few dark bumps; pink folds over the lips at the mouth corners.
    for s in (1, -1):
        for x, y in ((0.12, 0.80), (0.15, 0.76), (0.10, 0.85), (0.14, 0.84)):
            z = head_top(x, y) - 0.035
            b.add(ellipsoid((x * s, y, z), (0.006, 0.006, 0.006), n=4, rings=3), M["nostril"], "head")
        b.add(ellipsoid((0.16 * s, 0.56, ZM + 0.035), (0.03, 0.06, 0.02), n=8, rings=4, axis=V((s, 0, 0.3)), ref=FWD), M["pink"], "head")
    b.tags[first:] = ["head"] * (len(b.verts) - first)
    first = len(b.verts)
    # Raised eye turrets on top of the skull: a hide dome with a pink rim, a brown eye looking out and forward.
    for s in (1, -1):
        x, y = 0.12, 0.505
        base = head_top(x, y)
        b.add(ellipsoid(V((x * s, y, base + 0.026)), (0.055, 0.062, 0.052), n=12, rings=6, axis=UP, ref=FWD), M["hide"], "head")
        look = V((0.78 * s, 0.50, 0.35)).normalized()
        ec = V(((x + 0.022) * s, y + 0.012, base + 0.056))
        b.add(ellipsoid(ec, (0.024, 0.024, 0.024), n=10, rings=6, axis=look, ref=UP), M["eye"], "head")
        b.add(ellipsoid(ec + look * 0.022, (0.012, 0.008, 0.004), n=8, rings=3, axis=look, ref=UP), M["pupil"], "head")
        # Pink lids round the eye, the upper one a heavy brow.
        b.add(ellipsoid(ec + V((0.0, -0.004, 0.016)), (0.032, 0.034, 0.014), n=10, rings=5, axis=UP, ref=FWD), M["pink"], "head")
        b.add(ellipsoid(ec + V((0.006 * s, 0.002, -0.014)), (0.03, 0.03, 0.011), n=10, rings=5, axis=UP, ref=FWD), M["pink"], "head")
    b.tags[first:] = ["eye"] * (len(b.verts) - first)
    first = len(b.verts)
    # Small round ears on the back of the head, pink inside (on the ear bones).
    for s in (1, -1):
        e = EARS[s]
        d = V((0.35 * s, -0.25, 1)).normalized()
        b.add(ellipsoid(e + d * 0.045, (0.026, 0.016, 0.05), n=10, rings=5, axis=d, ref=FWD), M["hide"], "ear" + (".R" if s > 0 else ".L"))
        b.add(ellipsoid(e + d * 0.05 + V((0.004 * s, 0.009, 0)), (0.016, 0.008, 0.034), n=8, rings=4, axis=d, ref=FWD), M["pink"],
              "ear" + (".R" if s > 0 else ".L"))
        b.add(ellipsoid(e + V((0, 0, -0.012)), (0.035, 0.03, 0.03), n=8, rings=4), M["hide"], "head")
    b.tags[first:] = ["ear"] * (len(b.verts) - first)
    first = len(b.verts)
    # Raised nostril bumps on top of the muzzle with two dark slits.
    for s in (1, -1):
        x, y = 0.058, 0.845
        base = head_top(x, y)
        b.add(ellipsoid(V((x * s, y, base + 0.016)), (0.042, 0.042, 0.036), n=10, rings=5, axis=UP, ref=FWD), M["hide"], "head")
        b.add(ellipsoid(V((x * s, y + 0.004, base + 0.048)), (0.010, 0.020, 0.006), n=6, rings=3, axis=V((0.2 * s, 0.25, 1)), ref=FWD),
              M["nostril"], "head")
    b.tags[first:] = ["nostril"] * (len(b.verts) - first)


def rest_leg(leg):
    return solve_leg(leg["hip"], leg["foot"], leg["a"], leg["b"], leg["bend"])


def build_legs(b, M):
    for leg in LEGS:
        n, s, hip = leg["name"], leg["s"], leg["hip"]
        knee, normal, foot = rest_leg(leg)
        fd, td = (knee - hip).normalized(), (foot - knee).normalized()
        r0, r1, r2, r3 = leg["r"]
        upper = [(hip + V((0, 0, 0.10)) - fd * 0.03, r0 * 0.9, r0 * 0.9), (hip, r0, r0), (hip + fd * leg["a"] * 0.5, r1 * 1.05, r1),
                 (knee, r2, r2)]
        lower = [(knee - td * 0.01, r2 * 0.98, r2 * 0.98), (knee + td * leg["b"] * 0.5, r3 * 1.05, r3 * 1.05), (foot, r3, r3)]
        b.add(tube(upper, sub=2, n=12, ref=normal, bulge=(0.04, 0.01)), M["hide"], "upper" + n, tag="leg")
        b.add(ellipsoid(knee, (r2,) * 3, n=10, rings=5), M["hide"], "lower" + n, tag="leg")
        b.add(tube(lower, sub=2, n=12, ref=normal, bulge=(0.0, 0.01)), M["hide"], "lower" + n, tag="leg")
        # Round foot pad down to the floor, four short toes round its front with pale nails.
        pd = leg["pdir"].normalized()
        side = pd.cross(UP).normalized()
        c = foot + pd * 0.012
        pad = [(c + V((0, 0, 0.02)), r3 * 1.0, r3 * 1.0), (c + V((0, 0, -0.01)), r3 * 1.12, r3 * 1.08),
               (c + V((0, 0, -FOOT_Z + 0.004)), r3 * 1.1, r3 * 1.05)]
        b.add(tube(pad, sub=1, n=12, ref=pd, bulge=(0.0, 0.01)), M["hide"], "paw" + n, tag="leg")
        for k in range(4):
            off = (k - 1.5) / 1.5
            ang = off * 0.75
            d = (pd * math.cos(ang) + side * math.sin(ang)).normalized()
            tc = c + d * r3 * 0.95 + V((0, 0, -FOOT_Z + 0.022))
            b.add(ellipsoid(tc, (0.024, 0.026, 0.022), n=8, rings=4, axis=UP, ref=d), M["hide"], "paw" + n, tag="leg")
            nail = ellipsoid(tc + d * 0.018 + V((0, 0, 0.002)), (0.014, 0.010, 0.012), n=6, rings=3, axis=UP, ref=d)
            b.add(nail, M["nail"], "paw" + n, tag="leg")


def build_tail(b, M, joints):
    """Short tail flattened side to side, a dark tuft of bristles at the tip."""
    keys = [(j, rx, ry) for j, rx, ry in zip(joints, (0.035, 0.025, 0.018, 0.012), (0.055, 0.05, 0.042, 0.03))]
    bones = ["tail%02d" % k for k in range(NT)]
    w = chain_weights(joints, bones, blend=0.02)
    b.add(tube(keys, sub=2, n=10, ref=FWD, bulge=(0.0, 0.01)), M["hide"], w, tag="tail")
    d = (joints[-1] - joints[-2]).normalized()
    for k in range(6):
        a = 2 * math.pi * k / 6
        off = V((0.012 * math.cos(a), 0, 0.02 * math.sin(a)))
        b.add(common.cone(joints[-1] + off, d + off * 8, 0.04, 0.006, n=4), M["tuft"], "tail%02d" % (NT - 1), tag="tail")


# ---------------------------------------------------------------- tail


def tail_dirs(p):
    """Body-space direction per tail segment: hanging down and back, swaying side to side."""
    out = []
    for k in range(NT):
        u = k / max(NT - 1, 1)
        el = -40 - 25 * u + p["tlift"] * (1 - 0.4 * u)
        yaw = p["tsway"] * math.sin(2 * math.pi * (p["tphase"] - 0.1 * k)) * (0.4 + 0.6 * u)
        el, yaw = math.radians(el), math.radians(yaw)
        out.append(V((math.sin(yaw) * math.cos(el), -math.cos(yaw) * math.cos(el), math.sin(el))))
    return out


def tail_points(T, p):
    """Armature-space joints of the tail for hips matrix T (limp: hangs towards the floor)."""
    R = T.to_3x3()
    pts = [T @ TAIL_BASE]
    seg = TAIL_LEN / NT
    for d in tail_dirs(p):
        d = R @ d
        if p["tflop"] > 0:
            d = d.lerp(V((0, 0, -1)), p["tflop"] * 0.6).normalized()
        pts.append(pts[-1] + d * seg)
    return pts


# ---------------------------------------------------------------- rig


def build_rig(coll, tail):
    arm = bpy.data.armatures.new("hippo_rig")
    rig = bpy.data.objects.new("hippo_rig", arm)
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
    bone("breath", BREATH_C, BREATH_C + V((0, 0.2, 0)), X, "chest")
    bone("head", NECK, NECK + V((0, 0.3, 0)), X, "chest")
    bone("jaw", JAW_HINGE, JAW_HINGE + V((0, 0.3, 0)), X, "head")
    for s, e in EARS.items():
        bone("ear" + (".R" if s > 0 else ".L"), e, e + V((0, 0, 0.07)), X, "head")
    for k in range(NT):
        bone("tail%02d" % k, tail[k], tail[k + 1], X, "hips" if k == 0 else "tail%02d" % (k - 1))
    for leg in LEGS:
        knee, normal, foot = rest_leg(leg)
        n = leg["name"]
        bone("upper" + n, leg["hip"], knee, normal, "hips" if n[0] == "H" else "chest")
        bone("lower" + n, knee, foot, normal, "upper" + n)
        bone("paw" + n, foot, foot + leg["pdir"].normalized() * 0.06, X, "lower" + n)
    bpy.ops.object.mode_set(mode="OBJECT")
    for pb in rig.pose.bones:
        pb.rotation_mode = "QUATERNION"
    return rig


# ---------------------------------------------------------------- animation

REST = dict(x=0.0, y=0.0, z=0.0, pitch=0.0, roll=0.0, yaw=0.0, flex_p=0.0, flex_y=0.0, hp=0.0, hy=0.0, hr=0.0, hz=0.0, hfw=0.0,
            jaw=0.0, earR=0.0, earL=0.0, tsway=0.0, tphase=0.0, tlift=0.0, tflop=0.0, breath=0.0, tuck=0.0,
            plant=1.0, fplant=1.0, fup=0.0, ffwd=0.0, hup=0.0, hfwd=0.0, curl=0.0, flail=0.0, fphase=0.0, sag=0.0)
STRIDE, STEP_H, DUTY = 0.22, 0.055, 0.70  # one stride per walk clip, stance share of the cycle


def gait(leg, t):
    """Walk: foot position (armature space) and swing amount for gait phase t."""
    ph = (t + leg["phase"]) % 1.0
    base = leg["foot"]
    if ph < DUTY:  # stance: foot slides back under the moving body
        return base + V((0, STRIDE * (0.5 - ph / DUTY), 0)), 0.0
    k = (ph - DUTY) / (1 - DUTY)
    lift = math.sin(math.pi * k)
    return base + V((0, STRIDE * (smoothstep(0, 1, k) - 0.5), STEP_H * lift)), lift


def walk_params(t):
    """Heavy plod: the barrel rolls onto the supporting side, the head nods with every step, ears still."""
    p = dict(REST)
    w = 2 * math.pi * t
    p.update(roll=2.2 * math.sin(w + 0.4), yaw=-1.5 * math.cos(w), pitch=0.8 * math.sin(2 * w),
             flex_y=3 * math.cos(w), hp=-2 + 2.5 * math.sin(2 * w + 1.2), hy=-3 * math.cos(w + 0.3), hr=-1.5 * math.sin(w + 0.4),
             jaw=1.5, tsway=14, tphase=t * 2, breath=0.3 + 0.2 * math.sin(2 * w))
    p["feet"] = {leg["name"]: gait(leg, t) for leg in LEGS}
    return p


READY = walk_params(0.0)
READY_FEET = {k: v[0].copy() for k, v in READY["feet"].items()}


def flick(t, at, width=0.035):
    """A quick ear twitch: out and back around phase 'at' (wraps for the loop)."""
    d = (t - at + 0.5) % 1.0 - 0.5
    return math.exp(-((d / width) ** 2)) * math.sin(2 * math.pi * d / (2.2 * width) + 0.5)


def idle_params(t):
    """Lying low on the belly, legs tucked, the head lifted and tilted up so the eyes, ears and nostrils are the top;
    slow breath, the ears flick now and then."""
    p = dict(REST)
    w = 2 * math.pi * t
    p.update(tuck=1.0, plant=0.0, fplant=0.0, pitch=-2.5, hz=0.135, hfw=0.02, hp=11.5 + 0.6 * math.sin(w - 0.6),
             flex_p=0.3 * math.sin(w), breath=0.5 - 0.5 * math.cos(w), tsway=4, tphase=t, flex_y=1.5, hy=-2.0,
             earR=60 * (flick(t, 0.22) + 0.7 * flick(t, 0.33)), earL=-60 * (flick(t, 0.66) + 0.6 * flick(t, 0.77)))
    return p


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
        else:
            free += V((0, p["hfwd"], p["hup"]))
        free = free.lerp(leg["curled"], p["curl"]).lerp(leg["tuck"], p["tuck"])
        ph = 2 * math.pi * (p["fphase"] + {"F": 0.0, "H": 0.4}[n[0]] + (0.23 if s > 0 else 0.0))
        free += V((0.06 * s * math.sin(ph * 1.7), 0.06 * math.cos(ph * 1.3), 0.06 * math.sin(ph + 1))) * p["flail"]
        w = p["fplant"] if front else p["plant"]
        target = planted.lerp(mats_by_leg[n] @ free, 1 - w)
        if abs(p["roll"]) < 45 and p["tuck"] < 0.5:
            target.z = max(target.z, FOOT_Z)
        if p["sag"] > 0:  # limp: the free feet drop towards the floor
            target.z = max(target.z - 0.2 * p["sag"], min(target.z, 0.08))
        out[n] = (target, 1 - w)
    return out


def body_mats(p):
    T = Matrix.Translation(V((p["x"], p["y"], p["z"]))) @ rot_about(PIVOT, euler(p["pitch"], p["roll"], p["yaw"]))
    C = T @ rot_about(MID, euler(p["flex_p"], 0, p["flex_y"]))
    s = 1 + 0.035 * p["breath"]
    Bm = C @ Matrix.Translation(BREATH_C) @ Matrix.Diagonal((s, s, s, 1)) @ Matrix.Translation(-BREATH_C)
    H = C @ Matrix.Translation(V((0, p["hfw"], p["hz"]))) @ rot_about(NECK, euler(p["hp"], p["hr"], p["hy"]))
    J = H @ rot_about(JAW_HINGE, euler(-p["jaw"]))
    return T, C, Bm, H, J


def pose_matrices(rest, p):
    """Armature-space pose matrices of every bone except the tail."""
    T, C, Bm, H, J = body_mats(p)
    mats = {"root": rest["root"], "hips": T @ rest["hips"], "chest": C @ rest["chest"], "breath": Bm @ rest["breath"],
            "head": H @ rest["head"], "jaw": J @ rest["jaw"]}
    for s, e in EARS.items():
        n = "ear" + (".R" if s > 0 else ".L")
        ang = p["earR"] if s > 0 else p["earL"]
        mats[n] = H @ rot_about(e, euler(-0.3 * abs(ang), ang)) @ rest[n]
    by_leg = {leg["name"]: (T if leg["name"][0] == "H" else C) for leg in LEGS}
    feet = p["feet"] if "feet" in p else leg_targets(p, by_leg)
    for leg in LEGS:
        n = leg["name"]
        B = by_leg[n]
        R = B.to_3x3()
        knee0, normal0, foot0 = rest_leg(leg)
        target, swing = feet[n]
        hip = B @ leg["hip"]
        bend = leg["bend"].lerp(leg["tuck_bend"], p["tuck"])
        knee, normal, foot = solve_leg(hip, target, leg["a"], leg["b"], R @ bend)
        mats["upper" + n] = frame(hip, knee - hip, normal) @ frame(leg["hip"], knee0 - leg["hip"], normal0).inverted() @ rest["upper" + n]
        mats["lower" + n] = frame(knee, foot - knee, normal) @ frame(knee0, foot0 - knee0, normal0).inverted() @ rest["lower" + n]
        # The foot stays level on the floor while planted; swinging, tucked or lying it follows the shin.
        pd0 = leg["pdir"].normalized()
        body_pd = R @ pd0.lerp(leg["tuck_pd"].normalized(), p["tuck"]).normalized()
        shin = (foot - knee).normalized()
        k = min(1.0, swing * 1.2)
        if p["tuck"] > 0 or abs(p["roll"]) > 45:
            k = max(k, p["tuck"], min(1.0, (abs(p["roll"]) - 45) / 45))
        up = UP.lerp(-shin, k).normalized()
        fwd = flat(body_pd).lerp(body_pd, k)
        fwd = (fwd - fwd.dot(up) * up).normalized()
        mats["paw" + n] = frame(foot, fwd, fwd.cross(up)) @ frame(foot0, pd0, pd0.cross(UP)).inverted() @ rest["paw" + n]
    return mats, T


class Skin:
    """numpy linear blend skinning of the non-tail vertices (for the floor contact)."""

    def __init__(self, b, rest):
        import numpy as np

        self.np = np
        keep = [i for i, t in enumerate(b.tags) if not t.startswith("tail")]
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
        pb.keyframe_insert("scale", frame=frame_no)


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


# Loops on walk frame 0 (also the charge wind-up, 0.7 s = frames 0-10): the threat yawn, the head thrown up and the
# jaws gaping to about 125 degrees, held; then the lunge, the crushing snap, a head shake and back.
ATTACK = [
    (0, {}),
    (3, dict(y=-0.03, pitch=2, hp=14, jaw=30, tlift=6)),
    (7, dict(y=-0.06, pitch=6, flex_p=3, hp=42, hz=0.03, jaw=118, fplant=0.6, fup=0.02, tlift=10, earR=-12, earL=12)),
    (10, dict(y=-0.06, pitch=7, flex_p=3, hp=46, hz=0.03, jaw=126, fplant=0.6, fup=0.02, tlift=10, hy=4, earR=-15, earL=15)),
    (13, dict(y=0.13, pitch=-2, flex_p=-2, hp=16, jaw=58, fplant=0.0, fup=0.05, ffwd=0.13, tlift=4)),
    (15, dict(y=0.16, pitch=-3, flex_p=-3, hp=-8, jaw=0, fplant=0.3, fup=0.0, ffwd=0.12, tlift=2)),
    (17, dict(y=0.15, pitch=-2, flex_p=-2, hp=-6, hy=16, hr=12, flex_y=6, jaw=4, fplant=0.6, ffwd=0.10, tsway=18)),
    (19, dict(y=0.14, pitch=-2, flex_p=-2, hp=-6, hy=-14, hr=-11, flex_y=-6, jaw=4, fplant=0.8, ffwd=0.06, tsway=18)),
    (21, dict(y=0.07, hp=-2, hy=5, hr=4, flex_y=2, fplant=1, tsway=14)),
    (24, {}),
]

# Plays once: a bellow with the jaws open, the legs buckle, it sags and rolls onto its left side, legs limp.
DIE = [
    (0, {}),
    (3, dict(pitch=4, hp=24, jaw=60, fplant=0.5, fup=0.03, tlift=8, earR=-10, earL=10)),
    (6, dict(pitch=1, hp=14, jaw=45, roll=-10, plant=0.6, fplant=0.3, curl=0.3, tsway=8)),
    (9, dict(pitch=-3, hp=-3, hr=-3, jaw=25, roll=-35, x=-0.03, plant=0.3, fplant=0, curl=0.6, flail=0.3, fphase=0.4, tflop=0.3)),
    (13, dict(pitch=-3, hp=-3, hr=-12, jaw=22, roll=-82, x=-0.10, plant=0, fplant=0, curl=0.3, flail=0.6, fphase=1.2, tflop=0.7, sag=0.5)),
    (16, dict(pitch=-2, hp=-3, hr=-15, jaw=20, roll=-96, x=-0.12, plant=0, fplant=0, curl=0.15, flail=0.5, fphase=1.8, tflop=1.0, sag=1.0)),
    (19, dict(pitch=-2, hp=-3, hr=-14, jaw=18, roll=-92, x=-0.12, plant=0, fplant=0, curl=0.25, flail=0.3, fphase=2.4, tflop=1.0, sag=1.0)),
    (23, dict(pitch=-2, hp=-3, hr=-15, jaw=17, roll=-94, x=-0.12, plant=0, fplant=0, curl=0.35, flail=0.12, fphase=3.0, tflop=1.0, sag=1.0)),
    (26, dict(pitch=-2, hp=-3, hr=-15, jaw=16, roll=-94, x=-0.12, plant=0, fplant=0, curl=0.4, flail=0.03, fphase=3.3, tflop=1.0, sag=1.0)),
    (29, dict(pitch=-2, hp=-3, hr=-15, jaw=16, roll=-94, x=-0.12, plant=0, fplant=0, curl=0.4, flail=0.0, fphase=3.4, tflop=1.0, sag=1.0)),
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
        last = frames if name in LOOPS else frames - 1  # loops key frame N = frame 0
        for f in range(last + 1):
            if name == "hippo_walk":
                p = walk_params(f / frames)
            elif name == "hippo_idle":
                p = idle_params(f / frames)
            else:
                p = interpolate({"hippo_attack": ATTACK, "hippo_die": DIE}[name], f)
                p["tphase"] = f / CLIPS[0][2] * 2  # keeps the tail sway going from where the walk's frame 0 is
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
    build_head(b, M)
    build_legs(b, M)
    build_tail(b, M, tail)
    obj = common.finish_mesh(b, coll, "hippo_new")
    TAGS[:] = b.tags
    common.uv_unwrap(obj, b.tags, {"head": (V((0, 0.68, 0.36)), 1.4), "eye": (V((0, 0.5, 0.56)), 2.2),
                                   "ear": (V((0, 0.42, 0.57)), 2.0), "nostril": (V((0, 0.85, 0.52)), 2.0)})
    if bake:
        tmp = bpy.app.tempdir or "/tmp"
        tex = common.bake_texture(obj, tex_path or os.path.join(tmp, "hippo_preview.png"), TEX_SIZE, "hippo", ao_distance=0.08)
        common.use_baked_material(obj, tex)
    common.rig_object(obj, rig)
    rest = {bn.name: bn.matrix_local.copy() for bn in rig.data.bones}
    acts = make_actions(rig, Skin(b, rest))
    rig.animation_data.action = acts["hippo_walk"]
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 0, CLIPS[0][2] - 1
    scene.frame_set(0)
    print("hippo: {} verts, {} tris".format(len(obj.data.vertices), common.tri_count(obj)))
    return obj, rig


def measure():
    """Every clip's frame 0 size, its height over all frames and the tops of eyes, ears, nostrils and the other parts
    as fractions of that height."""
    obj, rig = bpy.data.objects["hippo_new"], bpy.data.objects["hippo_rig"]
    tags = TAGS
    scene = bpy.context.scene
    for name, _, frames in CLIPS:
        rig.animation_data.action = bpy.data.actions[name]
        lo, hi, tops = 1e9, -1e9, {}
        mn, mx = V((1e9,) * 3), V((-1e9,) * 3)
        for f in range(frames):
            scene.frame_set(f)
            ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
            for v, t in zip(ev.data.vertices, tags):
                z = v.co.z
                lo, hi = min(lo, z), max(hi, z)
                tops[t] = max(tops.get(t, -1e9), z)
                if f == 0:
                    for c in range(3):
                        mn[c], mx[c] = min(mn[c], v.co[c]), max(mx[c], v.co[c])
        size = mx - mn
        print("MEASURE {}: frame0 size x {:.3f} y {:.3f} z {:.3f}; all frames z {:.3f}..{:.3f}".format(name, *size, lo, hi))
        print("MEASURE {} tops / height: ".format(name) + ", ".join("{} {:.3f}".format(t, (z - lo) / (hi - lo)) for t, z in sorted(tops.items())))
    rig.animation_data.action = bpy.data.actions[CLIPS[0][0]]


def export(models_dir=None):
    common.export_files(bpy.data.objects["hippo_new"], bpy.data.objects["hippo_rig"], "hippo", CLIPS, "monsters", models_dir)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    if "--export" in args:
        build(tex_path=os.path.join(REPO, "media", "textures", "monsters", "hippo.png"))
        export()
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "hippo.blend"))
    else:
        build(bake="--bake" in args)
    if "--measure" in args:
        measure()
