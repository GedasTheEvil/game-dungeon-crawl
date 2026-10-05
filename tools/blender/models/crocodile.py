"""Procedural Nile crocodile (the tomb/temple croc): mesh, rig, baked texture and the .md3 animations
(walk, attack, die and idle: lying flat on its belly, the pose the engine lowers into water).

    MCP:  p = ".../tools/blender/models/crocodile.py"; g = {"__file__": p, "__name__": "crocodile"}
          exec(open(p).read(), g); g["build"]()          # then g["export"]()
    CLI:  blender -b --python tools/blender/models/crocodile.py -- [--export] [--measure]

Blender space: Z up, the crocodile faces +Y (game uses rotA = 180, as the rat), its right side is +X.
Built like rat.py: one loft for the body blended over hips / chest / head bones (plus a leaf 'breath' bone that
swells the flanks in the idle clip), rigid head and hinged lower jaw, the four sprawling legs by two-bone IK
towards foot targets (planted, in body space, or tucked along the body), the tail as an FK chain laid onto the
floor where it would sink. Every frame the whole pose is lifted so the lowest non-tail vertex touches the floor.
The top of the head is flat with the eyes and the nostril bump raised on it, so in the idle pose eyes, nostrils
and the back ridge (scutes) are the highest points (the engine draws the idle crocodile lowered into water).
--measure prints the idle clip's height and the eye / nostril / back ridge heights as fractions of it.
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

COLL = "crocodile_new"
CLIPS = [("croc_walk", "", 24), ("croc_attack", "_att", 22), ("croc_die", "_die", 30), ("croc_idle", "_idle", 32)]
LOOPS = ("croc_walk", "croc_attack", "croc_idle")
TAGS = []  # per-vertex part tag of the last build (measure())
TEX_SIZE = 1024
REVIEW_VIEW = {"target": (0, -0.4, 0.2), "ortho": 3.3, "res": (640, 340)}

COL = {
    "belly": (0.50, 0.44, 0.24),
    "flank": (0.20, 0.21, 0.10),
    "back": (0.10, 0.11, 0.055),
    "dark": (0.045, 0.05, 0.03),
    "band": (0.035, 0.035, 0.022),
    "scute": (0.06, 0.065, 0.04),
    "claw": (0.10, 0.085, 0.06),
    "eye": (0.55, 0.58, 0.12),
    "pupil": (0.01, 0.01, 0.008),
    "nostril": (0.015, 0.012, 0.01),
    "tooth": (0.80, 0.76, 0.62),
    "mouth": (0.72, 0.40, 0.38),
    "throat": (0.45, 0.16, 0.15),
}

UP, FWD = V((0, 0, 1)), V((0, 1, 0))
PIVOT = V((0, -0.30, 0.255))  # hips bone: body rotations happen around this point
MID = V((0, 0.02, 0.255))  # chest bone: spine flex
NECK = V((0, 0.40, 0.26))
ZM = 0.25  # mouth line height
JAW_HINGE = V((0, 0.47, ZM))
TAIL_BASE = V((0, -0.50, 0.262))
NT, TAIL_LEN = 14, 1.45  # tail bones, length
FOOT_Z = 0.03  # ankle / wrist height above the floor
BREATH_C = V((0, -0.05, 0.16))  # the breath bone scales the flanks about this point (belly bottom)

# Head profiles: (y, top or bottom, half width). Upper jaw: skull top; lower jaw: chin bottom. Both meet at ZM.
UPPER = [(0.42, 0.335, 0.12), (0.50, 0.338, 0.132), (0.60, 0.332, 0.118), (0.72, 0.320, 0.090), (0.84, 0.312, 0.070),
         (0.94, 0.314, 0.068), (1.00, 0.308, 0.058), (1.045, 0.290, 0.030)]
LOWER = [(0.44, 0.165, 0.11), (0.52, 0.168, 0.112), (0.64, 0.188, 0.096), (0.78, 0.207, 0.072), (0.90, 0.217, 0.056),
         (0.98, 0.226, 0.050), (1.03, 0.234, 0.026)]


def leg_defs():
    """Rest layout of the four legs (ankle / wrist = foot). s = +1 right, -1 left. Lateral-sequence walk."""
    legs = []
    for s in (1, -1):
        sfx = ".R" if s > 0 else ".L"
        legs.append(dict(name="H" + sfx, s=s, hip=V((0.15 * s, -0.36, 0.21)), foot=V((0.38 * s, -0.30, FOOT_Z)), a=0.17, b=0.16,
                         bend=V((s, 0.5, 0.9)), pdir=V((0.55 * s, 1, 0)), plen=0.07, toes=4,
                         curled=V((0.36 * s, -0.36, 0.30)), tuck=V((0.30 * s, -0.62, 0.17)), tuck_pd=V((0.2 * s, -1, 0)),
                         phase=0.0 if s < 0 else 0.5))
        legs.append(dict(name="F" + sfx, s=s, hip=V((0.13 * s, 0.24, 0.20)), foot=V((0.33 * s, 0.32, FOOT_Z)), a=0.15, b=0.14,
                         bend=V((s, -0.5, 0.9)), pdir=V((0.45 * s, 1, 0)), plen=0.05, toes=5,
                         curled=V((0.33 * s, 0.30, 0.30)), tuck=V((0.24 * s, 0.00, 0.17)), tuck_pd=V((0.3 * s, -1, 0)),
                         phase=0.25 if s < 0 else 0.75))
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


# ---------------------------------------------------------------- materials


def socket(sockets, ident):
    return next(s for s in sockets if s.identifier == ident)


def hide_material(pal):
    """Dark olive back fading to a pale yellowish belly (rest-pose normal z), scale cells (voronoi edges),
    mottling, dark cross bands on the tail and flanks."""
    mat = bpy.data.materials.get("croc_hide") or bpy.data.materials.new("croc_hide")
    nt = mat.node_tree
    for node in list(nt.nodes):
        if node.type not in ("OUTPUT_MATERIAL", "BSDF_PRINCIPLED"):
            nt.nodes.remove(node)
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Roughness"].default_value = 0.7
    new, link = nt.nodes.new, nt.links.new

    geo = new("ShaderNodeNewGeometry")
    sep = new("ShaderNodeSeparateXYZ")
    link(geo.outputs["Normal"], sep.inputs[0])
    tone = new("ShaderNodeValToRGB")
    els = tone.color_ramp.elements
    els[0].position, els[0].color = 0.22, (*pal["belly"], 1)
    els[1].position, els[1].color = 0.92, (*pal["dark"], 1)
    els.new(0.42).color = (*pal["flank"], 1)
    els.new(0.66).color = (*pal["back"], 1)
    remap = new("ShaderNodeMath")
    remap.operation = "MULTIPLY_ADD"
    remap.inputs[1].default_value = 0.5
    remap.inputs[2].default_value = 0.5
    link(sep.outputs["Z"], remap.inputs[0])
    link(remap.outputs[0], tone.inputs["Fac"])

    coord = new("ShaderNodeTexCoord")

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

    vor = new("ShaderNodeTexVoronoi")
    vor.feature = "DISTANCE_TO_EDGE"
    vor.inputs["Scale"].default_value = 1.0
    link(mapped((26, 20, 34)).outputs["Vector"], vor.inputs["Vector"])
    cells = ramp(vor.outputs["Distance"], (0.0, 0.45), (0.09, 1.0))

    # Cross bands: a sine of y, only behind the shoulders, broken up by noise.
    wave = new("ShaderNodeTexWave")
    wave.bands_direction = "Y"
    wave.inputs["Scale"].default_value = 2.2
    wave.inputs["Distortion"].default_value = 3.0
    wave.inputs["Detail"].default_value = 1.5
    link(coord.outputs["Object"], wave.inputs["Vector"])
    bands = ramp(wave.outputs["Fac"], (0.55, 1.0), (0.8, 0.5))
    # Mask: flanks and back (not the belly), from the shoulders back, strongest on the tail.
    osep = new("ShaderNodeSeparateXYZ")
    link(coord.outputs["Object"], osep.inputs[0])

    def clamp_lin(src, mul, add):
        m = new("ShaderNodeMath")
        m.operation = "MULTIPLY_ADD"
        m.use_clamp = True
        m.inputs[1].default_value, m.inputs[2].default_value = mul, add
        link(src, m.inputs[0])
        return m.outputs[0]

    side = clamp_lin(sep.outputs["Z"], 2.5, 0.6)  # 0 under the belly, 1 from the lower flank up
    along = clamp_lin(osep.outputs["Y"], -2.0, 0.55)  # 0 at the shoulders, 1 on the tail
    mask = new("ShaderNodeMath")
    mask.operation = "MULTIPLY"
    link(side, mask.inputs[0])
    link(along, mask.inputs[1])
    bmix = new("ShaderNodeMix")
    bmix.data_type = "RGBA"
    link(mask.outputs[0], socket(bmix.inputs, "Factor_Float"))
    socket(bmix.inputs, "A_Color").default_value = (1, 1, 1, 1)
    link(bands.outputs["Color"], socket(bmix.inputs, "B_Color"))

    factors = [cells.outputs["Color"], noise(4.0, 4.0, (0.3, 0.75), (0.7, 1.2)).outputs["Color"], socket(bmix.outputs, "Result_Color")]
    out = tone.outputs["Color"]
    for f in factors:
        m = new("ShaderNodeVectorMath")
        m.operation = "MULTIPLY"
        link(out, m.inputs[0])
        link(f, m.inputs[1])
        out = m.outputs[0]
    link(out, bsdf.inputs["Base Color"])
    return mat


def materials(pal):
    specs = {name: ("solid", name) for name in ("scute", "claw", "eye", "pupil", "nostril", "tooth", "mouth", "throat")}
    M = {name: common.make_material("croc_" + name, spec, pal) for name, spec in specs.items()}
    M["hide"] = hide_material(pal)
    return M


# ---------------------------------------------------------------- parts

BODY_KEYS = [(-0.66, 0.24, 0.08, 0.07), (-0.56, 0.25, 0.15, 0.10), (-0.42, 0.255, 0.185, 0.105), (-0.22, 0.257, 0.205, 0.107),
             (0.00, 0.257, 0.21, 0.107), (0.18, 0.255, 0.19, 0.103), (0.32, 0.256, 0.155, 0.092), (0.44, 0.262, 0.13, 0.08),
             (0.54, 0.27, 0.11, 0.06)]


def body_top(y, x=0.0):
    """Height of the back at (x, y) in the rest pose (for the scutes)."""
    for (y0, z0, rx0, ry0), (y1, z1, rx1, ry1) in zip(BODY_KEYS, BODY_KEYS[1:]):
        if y0 <= y <= y1:
            t = (y - y0) / (y1 - y0)
            z, rx, ry = z0 + (z1 - z0) * t, rx0 + (rx1 - rx0) * t, ry0 + (ry1 - ry0) * t
            u = min(abs(x) / rx, 1.0)
            return z + ry * (1 - u ** 2.4) ** (1 / 2.4)
    return BODY_KEYS[0][1]


def body_shape(th, a):
    k = superellipse(2.4)(th)
    sn = math.sin(th)
    return k * (1.0 if sn >= 0 else 1.0 - 0.12 * sn * sn)


_body_w = chain_weights([V((0, -0.8, 0.25)), MID, NECK, V((0, 1.1, 0.26))], ["hips", "chest", "head"], blend=0.12)


def body_weight(co):
    """Spine blend plus a share of the breath bone over the flanks and back between hips and shoulders."""
    w = _body_w(co)
    k = 0.7 * smoothstep(-0.42, -0.2, co.y) * (1 - smoothstep(0.12, 0.32, co.y))
    if k <= 0:
        return w
    out = {n: v * (1 - k) for n, v in w.items()}
    out["breath"] = out.get("breath", 0.0) + k
    return out


def build_body(b, M):
    keys = [((0, y, z), rx, ry) for y, z, rx, ry in BODY_KEYS]
    b.add(tube(keys, sub=2, n=20, ref=UP, shape=body_shape, bulge=(0.02, 0.0)), M["hide"], body_weight)


def scute(c, size, d=FWD):
    """Keeled osteoderm: a small ridge elongated along d."""
    return ellipsoid(c, (size * 0.42, size, size * 0.75), n=4, rings=3, axis=UP, ref=d)


def build_back_scutes(b, M):
    first = len(b.verts)
    for i in range(12):  # dorsal shield, four rows from the shoulders to the hips
        y = 0.30 - i * 0.07
        for x, sz in ((0.035, 0.036), (0.098, 0.03)):
            for s in (1, -1):
                xx = x * s * (0.85 if y > 0.2 else 1.0)
                c = V((xx, y, body_top(y, xx) - 0.004))
                b.add(scute(c, sz), M["scute"], body_weight)
    for x, y in ((0.04, 0.40), (0.04, 0.46), (0.10, 0.43)):  # nuchal scutes on the neck
        for s in (1, -1):
            c = V((x * s, y, body_top(y, x) - 0.004))
            b.add(scute(c, 0.032), M["scute"], body_weight)
    b.tags[first:] = ["scute"] * (len(b.verts) - first)


def head_shape(th, a):
    return superellipse(2.6)(th)


def build_head(b, M):
    first = len(b.verts)
    # Upper jaw and skull: section centred on the mouth line, its lower half squashed to a flat palate.
    keys = [((0, y, ZM), r, top - ZM) for y, top, r in UPPER]
    up = common.transform(tube(keys, sub=2, n=18, ref=UP, shape=head_shape, bulge=(0.0, 0.01)),
                          lambda v: V((v.x, v.y, ZM - (ZM - v.z) * 0.12)) if v.z < ZM else v)
    b.add(up, M["hide"], "head")
    # Lower jaw, its upper half squashed flat.
    keys = [((0, y, ZM), r, ZM - bot) for y, bot, r in LOWER]
    lo = common.transform(tube(keys, sub=2, n=16, ref=UP, shape=head_shape, bulge=(0.0, 0.008)),
                          lambda v: V((v.x, v.y, ZM + (v.z - ZM) * 0.12)) if v.z > ZM else v)
    b.add(lo, M["hide"], "jaw")
    # Palate (pink under the upper jaw), tongue / mouth floor on the lower jaw, throat at the back.
    pal = [((0, y, ZM - 0.009), r * 0.82, 0.006) for y, top, r in UPPER[1:-1]]
    b.add(tube(pal, sub=1, n=10, ref=UP), M["mouth"], "head")
    ton = [((0, y, ZM + 0.009), r * 0.82, 0.006) for y, bot, r in LOWER[:-1]]
    b.add(tube(ton, sub=1, n=10, ref=UP), M["mouth"], "jaw")
    b.add(ellipsoid((0, 0.47, ZM), (0.085, 0.05, 0.06), n=10, rings=5, axis=FWD, ref=UP), M["throat"], "head")
    # Upper teeth along the lip, pointing down, outside the narrower lower jaw.
    for i in range(13):
        y = 0.55 + i * 0.039
        _, r = profile(UPPER, y)
        big = 1.0 + 0.35 * math.exp(-((y - 0.62) / 0.03) ** 2) + 0.3 * math.exp(-((y - 0.93) / 0.03) ** 2)
        for s in (1, -1):
            base = V((s * r * 0.9, y, ZM + 0.004))
            d = V((0.18 * s, 0.05, -1)).normalized()
            b.add(common.cone(base, d, 0.026 * big, 0.0075 * big, n=4), M["tooth"], "head")
    # Lower teeth: from the lower jaw's edge up past the upper lip, between the upper ones.
    for i in range(12):
        y = 0.57 + i * 0.039
        _, ru = profile(UPPER, y)
        _, rl = profile(LOWER, y)
        big = 1.0 + 0.45 * math.exp(-((y - 0.96) / 0.025) ** 2)
        for s in (1, -1):
            base = V((s * rl * 0.96, y, ZM - 0.006))
            tip = V((s * (ru * 1.02 + 0.004), y + 0.004, ZM + 0.022 * big))
            d = tip - base
            b.add(common.cone(base, d, d.length, 0.0068 * big, n=4), M["tooth"], "jaw")
    b.tags[first:] = ["head"] * (len(b.verts) - first)
    first = len(b.verts)
    # Raised eyes on top of the skull: bony orbit, yellow-green eyeball, vertical slit pupil.
    for s in (1, -1):
        oc = V((0.068 * s, 0.60, 0.330))
        b.add(ellipsoid(oc, (0.034, 0.046, 0.024), n=10, rings=5, axis=UP, ref=FWD), M["hide"], "head")
        ec = V((0.070 * s, 0.608, 0.345))
        look = V((0.55 * s, 0.45, 0.70)).normalized()
        b.add(ellipsoid(ec, (0.022, 0.022, 0.022), n=10, rings=6, axis=look, ref=UP), M["eye"], "head")
        b.add(ellipsoid(ec + look * 0.021, (0.0035, 0.014, 0.003), n=6, rings=3, axis=look, ref=UP), M["pupil"], "head")
        # Brow lid over the back of the eye.
        b.add(ellipsoid(V((0.066 * s, 0.592, 0.352)), (0.026, 0.02, 0.012), n=8, rings=4, axis=UP, ref=FWD), M["hide"], "head")
    b.tags[first:] = ["eye"] * (len(b.verts) - first)
    first = len(b.verts)
    # Nostril bump at the snout tip with two dark slits.
    b.add(ellipsoid((0, 1.005, 0.320), (0.036, 0.032, 0.032), n=10, rings=5, axis=UP, ref=FWD), M["hide"], "head")
    for s in (1, -1):
        b.add(ellipsoid((0.012 * s, 1.012, 0.348), (0.007, 0.011, 0.005), n=6, rings=3, axis=V((0.3 * s, 0.2, 1)), ref=FWD),
              M["nostril"], "head")
    b.tags[first:] = ["nostril"] * (len(b.verts) - first)


def rest_leg(leg):
    return solve_leg(leg["hip"], leg["foot"], leg["a"], leg["b"], leg["bend"])


def build_legs(b, M):
    for leg in LEGS:
        n, s, hip = leg["name"], leg["s"], leg["hip"]
        knee, normal, foot = rest_leg(leg)
        fd, td = (knee - hip).normalized(), (foot - knee).normalized()
        hind = n[0] == "H"
        r0, r1, r2, r3 = (0.085, 0.064, 0.048, 0.032) if hind else (0.064, 0.05, 0.04, 0.027)
        upper = [(hip - fd * 0.04, r0, r0 * 0.9), (hip + fd * leg["a"] * 0.5, r1, r1 * 0.9), (knee, r2, r2)]
        lower = [(knee - td * 0.01, r2 * 0.95, r2 * 0.95), (knee + td * leg["b"] * 0.5, r3 * 1.15, r3 * 1.15), (foot, r3, r3)]
        b.add(tube(upper, sub=2, n=10, ref=normal, bulge=(0.0, 0.01)), M["hide"], "upper" + n)
        b.add(ellipsoid(knee, (r2,) * 3, n=8, rings=4), M["hide"], "lower" + n)
        b.add(tube(lower, sub=2, n=8, ref=normal, bulge=(0.0, 0.01)), M["hide"], "lower" + n)
        # Flat foot on the floor, toes spread from its front edge, dark claws.
        pd = leg["pdir"].normalized()
        side = pd.cross(UP).normalized()
        pw, ph = (0.034, 0.016) if hind else (0.028, 0.014)
        tip = foot + pd * leg["plen"]
        paw = [(foot - pd * 0.02, pw * 0.8, ph), (foot + pd * leg["plen"] * 0.5, pw, ph * 0.9), (tip, pw * 1.05, ph * 0.7)]
        b.add(tube(paw, sub=1, n=8, ref=UP, bulge=(0.01, 0.005)), M["hide"], "paw" + n)
        nt_ = leg["toes"]
        spread, tl = (1.1, 0.06) if hind else (1.5, 0.045)
        for k in range(nt_):
            off = (k - (nt_ - 1) / 2) / ((nt_ - 1) / 2)
            ang = -off * spread * 0.5
            d = (pd * math.cos(ang) + side * math.sin(ang)).normalized()
            base = tip + side * off * pw * 0.8 - pd * 0.008 + V((0, 0, -ph * 0.3))
            L = tl * (1 - 0.25 * abs(off))
            end = base + d * L + V((0, 0, -0.004))
            b.add(tube([(base, 0.010, 0.008), (base + d * L * 0.5, 0.008, 0.0065), (end, 0.0055, 0.005)], sub=1, n=5, ref=UP),
                  M["hide"], "paw" + n)
            b.add(common.cone(end - d * 0.003, d + V((0, 0, -0.35)), 0.016, 0.005, n=4), M["claw"], "paw" + n)


def tail_rx(u):
    return 0.125 * (1 - u) ** 0.95 + 0.010


def tail_ry(u):
    return 0.068 * (1 - u) ** 0.8 + 0.022


def crest_h(u):
    return 0.024 * (1 - u) + 0.010


def build_tail(b, M, joints):
    keys = [(j, tail_rx(k / NT), tail_ry(k / NT)) for k, j in enumerate(joints)]
    bones = ["tail%02d" % k for k in range(NT)]
    w = chain_weights(joints, bones, blend=0.04)
    b.add(tube(keys, sub=2, n=12, ref=UP, shape=superellipse(2.2), bulge=(0.0, 0.01)), M["hide"], w, tag="tail")
    # Double crest of keeled scutes merging into a single row halfway down.
    first = len(b.verts)
    for k in range(1, NT * 2):
        u = k / (NT * 2)
        i = int(u * NT)
        t = u * NT - i
        c = joints[i].lerp(joints[i + 1], t)
        d = (joints[i + 1] - joints[i]).normalized()
        up = (UP - UP.dot(d) * d).normalized()
        sz = 0.034 * (1 - u) + 0.012
        top = c + up * (tail_ry(u) - 0.006)
        rows = (0.42, -0.42) if u < 0.5 else (0.0,)
        for f in rows:
            side = d.cross(up).normalized()
            p = top + side * (f * tail_rx(u)) - up * (0.012 * abs(f) * (1 - u))
            b.add(ellipsoid(p, (sz * 0.35, sz * 0.75, crest_h(u)), n=4, rings=3, axis=up, ref=d), M["scute"], w, tag="tail")
    b.tags[first:] = ["tailscute"] * (len(b.verts) - first)


# ---------------------------------------------------------------- tail


def tail_dirs(p):
    """Body-space direction per tail segment."""
    out = []
    for k in range(NT):
        u = k / (NT - 1)
        el = -12 + 12 * u ** 0.8 + p["tlift"] * (1 - u) ** 1.5
        yaw = p["tsway"] * math.sin(2 * math.pi * (p["tphase"] - 0.07 * k)) * (0.3 + 0.7 * u) + p["tcurl"] * u
        el, yaw = math.radians(el), math.radians(yaw)
        out.append(V((math.sin(yaw) * math.cos(el), -math.cos(yaw) * math.cos(el), math.sin(el))))
    return out


def tail_points(T, p):
    """Armature-space joints of the tail for hips matrix T; segments below the floor are laid onto it.
    The clearance follows the body roll (the tail's side half width when on its side, the crest when belly up)."""
    R = T.to_3x3()
    up = R @ UP
    pts = [T @ TAIL_BASE]
    seg = TAIL_LEN / NT
    for k, d in enumerate(tail_dirs(p)):
        d = R @ d
        if p["tflop"] > 0:  # limp tail: sags towards the floor instead of following the body
            h = V((d.x, d.y, 0))
            h = h.normalized() if h.length > 1e-4 else V((0, -1, 0))
            d = d.lerp((h + V((0, 0, -0.4))).normalized(), p["tflop"]).normalized()
        nxt = pts[-1] + d * seg
        u = (k + 1) / NT
        uz = abs(up.z)
        floor = math.hypot(tail_ry(u) * uz, tail_rx(u) * math.sqrt(max(0.0, 1 - uz * uz))) + 0.004
        if up.z < 0:
            floor += crest_h(u) * uz
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
    arm = bpy.data.armatures.new("crocodile_rig")
    rig = bpy.data.objects.new("crocodile_rig", arm)
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
            tsway=0.0, tphase=0.0, tlift=0.0, tcurl=0.0, tflop=0.0, breath=0.0, tuck=0.0,
            plant=1.0, fplant=1.0, fup=0.0, ffwd=0.0, hup=0.0, hfwd=0.0, curl=0.0, flail=0.0, fphase=0.0)
STRIDE, STEP_H, DUTY = 0.26, 0.07, 0.72  # one stride per walk clip, stance share of the cycle


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
    p = dict(REST)
    w = 2 * math.pi * t
    p.update(roll=1.5 * math.sin(2 * w), yaw=-3.0 * math.cos(w), pitch=0.6 * math.sin(2 * w),
             flex_y=7 * math.cos(w), hp=2 + 1.5 * math.sin(2 * w + 1.0), hy=-5 * math.cos(w + 0.3),
             tsway=16, tphase=t)
    p["feet"] = {leg["name"]: gait(leg, t) for leg in LEGS}
    return p


READY = walk_params(0.0)
READY_FEET = {k: v[0].copy() for k, v in READY["feet"].items()}


def idle_params(t):
    """Lying flat on the belly, legs tucked along the body, head low; slow breath and a slight tail sway."""
    p = dict(REST)
    w = 2 * math.pi * t
    p.update(tuck=1.0, plant=0.0, fplant=0.0, hp=1.5 + 0.6 * math.sin(w - 0.6), flex_p=0.4 * math.sin(w),
             breath=0.5 - 0.5 * math.cos(w), tsway=5, tphase=t, tcurl=6, flex_y=2.0, hy=-2.0)
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
        free += V((0.08 * s * math.sin(ph * 1.7), 0.08 * math.cos(ph * 1.3), 0.09 * math.sin(ph + 1))) * p["flail"]
        w = p["fplant"] if front else p["plant"]
        target = planted.lerp(mats_by_leg[n] @ free, 1 - w)
        if abs(p["roll"]) < 45 and p["tuck"] < 0.5:
            target.z = max(target.z, FOOT_Z)
        out[n] = (target, 1 - w)
    return out


def body_mats(p):
    T = Matrix.Translation(V((p["x"], p["y"], p["z"]))) @ rot_about(PIVOT, euler(p["pitch"], p["roll"], p["yaw"]))
    C = T @ rot_about(MID, euler(p["flex_p"], 0, p["flex_y"]))
    s = 1 + 0.035 * p["breath"]
    Bm = C @ Matrix.Translation(BREATH_C) @ Matrix.Diagonal((s, s, s, 1)) @ Matrix.Translation(-BREATH_C)
    H = C @ rot_about(NECK, euler(p["hp"], p["hr"], p["hy"]))
    J = H @ rot_about(JAW_HINGE, euler(-p["jaw"]))
    return T, C, Bm, H, J


def pose_matrices(rest, p):
    """Armature-space pose matrices of every bone except the tail."""
    T, C, Bm, H, J = body_mats(p)
    mats = {"root": rest["root"], "hips": T @ rest["hips"], "chest": C @ rest["chest"], "breath": Bm @ rest["breath"],
            "head": H @ rest["head"], "jaw": J @ rest["jaw"]}
    by_leg = {leg["name"]: (T if leg["name"][0] == "H" else C) for leg in LEGS}
    feet = p["feet"] if "feet" in p else leg_targets(p, by_leg)
    for leg in LEGS:
        n, s = leg["name"], leg["s"]
        B = by_leg[n]
        R = B.to_3x3()
        knee0, normal0, foot0 = rest_leg(leg)
        target, swing = feet[n]
        hip = B @ leg["hip"]
        bend = leg["bend"].lerp(V((s, 0.0, 0.25)), p["tuck"])
        knee, normal, foot = solve_leg(hip, target, leg["a"], leg["b"], R @ bend)
        mats["upper" + n] = frame(hip, knee - hip, normal) @ frame(leg["hip"], knee0 - leg["hip"], normal0).inverted() @ rest["upper" + n]
        mats["lower" + n] = frame(knee, foot - knee, normal) @ frame(knee0, foot0 - knee0, normal0).inverted() @ rest["lower" + n]
        pd0 = leg["pdir"].normalized()
        body_pd = R @ pd0.lerp(leg["tuck_pd"].normalized(), p["tuck"]).normalized()
        hang = (body_pd + (foot - knee).normalized() * 0.6).normalized()
        k = min(1.0, swing * 1.5) * (1 - p["tuck"])
        pd = flat(body_pd).lerp(hang, k).normalized()
        xh = flat(R @ V((1, 0, 0))).lerp(R @ V((1, 0, 0)), k)
        if p["tuck"] > 0 or abs(p["roll"]) > 45:  # lying: the paw follows the body
            pd = body_pd.lerp(pd, 1 - max(p["tuck"], min(1.0, (abs(p["roll"]) - 45) / 45))).normalized()
            xh = R @ V((1, 0, 0))
        mats["paw" + n] = frame(foot, pd, xh) @ frame(foot0, pd0, V((1, 0, 0))).inverted() @ rest["paw" + n]
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


# Loops on walk frame 0: gape with the head thrown up, lunge, snap shut, shake the head side to side.
ATTACK = [
    (0, {}),
    (4, dict(y=-0.05, pitch=2, flex_p=3, hp=12, jaw=16, tsway=12, tlift=4)),
    (8, dict(y=-0.07, pitch=4, flex_p=5, hp=22, jaw=46, fplant=0.4, fup=0.03, tsway=10, tlift=8)),
    (11, dict(y=0.16, pitch=-1, flex_p=-2, hp=6, jaw=44, fplant=0, fup=0.05, ffwd=0.16, tsway=10, tlift=4)),
    (13, dict(y=0.18, pitch=-2, flex_p=-3, hp=-5, jaw=0, fplant=0.2, fup=0.0, ffwd=0.15, tsway=10)),
    (15, dict(y=0.17, pitch=-2, flex_p=-2, hp=-3, hy=18, hr=16, flex_y=8, jaw=3, fplant=0.6, ffwd=0.12, tsway=14, tcurl=-12)),
    (17, dict(y=0.16, pitch=-2, flex_p=-2, hp=-3, hy=-16, hr=-14, flex_y=-8, jaw=3, fplant=0.8, tsway=14, tcurl=12)),
    (19, dict(y=0.08, hp=0, hy=7, hr=6, flex_y=3, fplant=1, tsway=14)),
    (22, {}),
]

# Plays once: a convulsion with the jaws snapping, rolls over onto its back, legs go limp, the tail stops.
DIE = [
    (0, {}),
    (3, dict(flex_p=6, pitch=3, hp=16, jaw=38, fplant=0.3, fup=0.06, tlift=10, tsway=18, flex_y=-8)),
    (6, dict(roll=-30, flex_y=10, hp=6, hr=-15, jaw=20, plant=0.5, fplant=0, fup=0.06, flail=0.5, fphase=0.3, tsway=16, tcurl=12)),
    (9, dict(tflop=0.4, roll=-85, x=-0.08, flex_y=8, hp=2, hr=-25, jaw=26, plant=0, fplant=0, flail=0.9, fphase=0.8, tsway=12, tcurl=18)),
    (12, dict(tflop=0.9, roll=-140, x=-0.14, flex_y=5, hp=-4, hr=-25, jaw=22, plant=0, fplant=0, flail=1.0, fphase=1.4, tsway=8, tcurl=24)),
    (14, dict(tflop=1.0, roll=-162, x=-0.16, flex_y=4, hp=-6, hr=-20, jaw=18, plant=0, fplant=0, flail=0.9, fphase=1.8, tsway=6, tcurl=26, curl=0.1)),
    (17, dict(tflop=1.0, roll=-158, x=-0.16, flex_y=4, hp=-6, hr=-20, jaw=16, plant=0, fplant=0, flail=0.7, fphase=2.5, tsway=4, tcurl=28, curl=0.3)),
    (20, dict(tflop=1.0, roll=-160, x=-0.16, flex_y=4, hp=-7, hr=-20, jaw=15, plant=0, fplant=0, flail=0.4, fphase=3.1, tsway=2, tcurl=30, curl=0.5)),
    (23, dict(tflop=1.0, roll=-160, x=-0.16, flex_y=4, hp=-8, hr=-20, jaw=14, plant=0, fplant=0, flail=0.18, fphase=3.6, tsway=1, tcurl=30, curl=0.7)),
    (26, dict(tflop=1.0, roll=-160, x=-0.16, flex_y=4, hp=-8, hr=-20, jaw=14, plant=0, fplant=0, flail=0.05, fphase=4.0, tsway=0, tcurl=30, curl=0.8)),
    (29, dict(tflop=1.0, roll=-160, x=-0.16, flex_y=4, hp=-8, hr=-20, jaw=14, plant=0, fplant=0, flail=0.0, fphase=4.2, tsway=0, tcurl=30, curl=0.8)),
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
            if name == "croc_walk":
                p = walk_params(f / frames)
            elif name == "croc_idle":
                p = idle_params(f / frames)
            else:
                p = interpolate({"croc_attack": ATTACK, "croc_die": DIE}[name], f)
                p["tphase"] = f / CLIPS[0][2]  # keeps the tail sway going from where the walk's frame 0 is
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
    build_back_scutes(b, M)
    build_head(b, M)
    build_legs(b, M)
    build_tail(b, M, tail)
    obj = common.finish_mesh(b, coll, "crocodile_new")
    TAGS[:] = b.tags
    common.uv_unwrap(obj, b.tags, {"head": (V((0, 0.75, 0.27)), 1.5), "eye": (V((0, 0.6, 0.34)), 2.2),
                                   "nostril": (V((0, 1.0, 0.31)), 2.0)})
    if bake:
        tmp = bpy.app.tempdir or "/tmp"
        tex = common.bake_texture(obj, tex_path or os.path.join(tmp, "crocodile_preview.png"), TEX_SIZE, "crocodile", ao_distance=0.12)
        common.use_baked_material(obj, tex)
    common.rig_object(obj, rig)
    rest = {bn.name: bn.matrix_local.copy() for bn in rig.data.bones}
    acts = make_actions(rig, Skin(b, rest))
    rig.animation_data.action = acts["croc_walk"]
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 0, CLIPS[0][2] - 1
    scene.frame_set(0)
    print("crocodile: {} verts, {} tris".format(len(obj.data.vertices), common.tri_count(obj)))
    return obj, rig


def measure():
    """Idle clip height and the tops of eyes, nostrils and back ridge as fractions of it (all frames)."""
    obj, rig = bpy.data.objects["crocodile_new"], bpy.data.objects["crocodile_rig"]
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
    common.export_files(bpy.data.objects["crocodile_new"], bpy.data.objects["crocodile_rig"], "crocodile", CLIPS, "monsters", models_dir)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    if "--export" in args:
        build(tex_path=os.path.join(REPO, "textures", "monsters", "crocodile.png"))
        export()
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "crocodile.blend"))
    else:
        build(bake="--bake" in args)
    if "--measure" in args:
        measure()
