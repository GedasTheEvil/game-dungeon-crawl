"""Procedural Egyptian deathstalker scorpion (Leiurus quinquestriatus): mesh, rig, baked texture and the .md3
animations (walk, attack, die, climb).

    MCP:  p = ".../tools/blender/models/scorpion.py"; g = {"__file__": p, "__name__": "scorpion"}
          exec(open(p).read(), g); g["build"]()          # then g["export"]()
    CLI:  blender -b --python tools/blender/models/scorpion.py -- [--export | --export-climb]

Two textures share one UV layout: scorpion.png (straw, the deathstalker) and scorpion_giant.png (black-brown, a
carnelian sheen on the claws and the tail). Set SCORPION_TEX=giant to show the giant texture on the built object.
scorpion_queen.png (Serket, the boss: pale gold, lapis joints and claw tips, a gold sting): bake it alone with
--boss-texture; SCORPION_TEX=queen shows it.

Blender space: Z up, the scorpion faces +Y (game uses rotA = 180), its right side is +X.
Rigid parts follow one bone each: prosoma (carapace, chelicerae, eyes), mesosoma (seven overlapping tergites, a flex
bone behind the carapace), eight walking legs (femur, tibia, tarsus; two-bone IK, alternating tetrapod gait with
planted feet), two pedipalps (femur, patella by two-bone IK towards a wrist target, the chela hand with the fixed
finger, the movable finger on a hinge) and the metasoma (five segments and the telson) as an FK chain whose segment
directions are absolute angles in the body's mid plane (e: 0 = straight back, 90 = up, 180 = forward), blended
between the curled rest pose, the strike and the limp pose. The whole pose is lifted every frame so the lowest
vertex (tail excluded; its joints are laid onto the floor on their own) touches the floor, from a numpy copy of the
skinning (as in rat.py).
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

COLL = "scorpion_new"
CLIMB = ("scorpion_climb", "_climb", 24)
CLIPS = [("scorpion_walk", "", 24), ("scorpion_attack", "_att", 24), ("scorpion_die", "_die", 30), CLIMB]
TEX_SIZE = 1024
REVIEW_VIEW = {"target": (0, 0.25, 0.4), "ortho": 2.5, "res": (560, 380)}

COL = {
    "straw": (0.72, 0.50, 0.16),
    "straw_hi": (0.85, 0.66, 0.26),
    "straw_pale": (0.80, 0.64, 0.30),
    "amber": (0.50, 0.28, 0.06),
    "amber_dark": (0.24, 0.11, 0.025),
    "rim": (0.42, 0.25, 0.06),
    "black": (0.012, 0.008, 0.005),
    "eye": (0.004, 0.004, 0.004),
    # Claws, tail and telson: the straw / amber of the body here; their own colours in the giant's palette.
    "claw": (0.72, 0.50, 0.16),
    "tail": (0.72, 0.50, 0.16),
    "tail_end": (0.50, 0.28, 0.06),
    "telson": (0.72, 0.50, 0.16),
}
# The giant scorpion (scorpion_giant.png, same UVs): black-brown, a carnelian sheen on the claws and the tail, the
# telson darker red.
COL_GIANT = {
    "straw": (0.06, 0.035, 0.022),
    "straw_hi": (0.10, 0.06, 0.035),
    "straw_pale": (0.08, 0.05, 0.03),
    "amber": (0.08, 0.035, 0.02),
    "amber_dark": (0.10, 0.02, 0.012),
    "rim": (0.025, 0.014, 0.01),
    "black": (0.01, 0.006, 0.004),
    "eye": (0.004, 0.004, 0.004),
    "claw": (0.30, 0.07, 0.035),
    "tail": (0.26, 0.06, 0.03),
    "tail_end": (0.20, 0.035, 0.02),
    "telson": (0.16, 0.025, 0.015),
}
# The scorpion queen (scorpion_queen.png, same UVs, a boss; Serket): pale gold carapace, lapis-blue joints, rims and claw
# tips, a gold sting.
COL_QUEEN = {
    "straw": (0.78, 0.58, 0.20),
    "straw_hi": (0.92, 0.76, 0.36),
    "straw_pale": (0.84, 0.68, 0.30),
    "amber": (0.62, 0.40, 0.08),
    "amber_dark": (0.03, 0.07, 0.36),
    "rim": (0.04, 0.10, 0.42),
    "black": (0.55, 0.36, 0.05),
    "eye": (0.004, 0.004, 0.02),
    "claw": (0.80, 0.60, 0.22),
    "tail": (0.80, 0.60, 0.22),
    "tail_end": (0.70, 0.48, 0.12),
    "telson": (0.86, 0.66, 0.22),
}

UP, FWD, X = V((0, 0, 1)), V((0, 1, 0)), V((1, 0, 0))
PIVOT = V((0, 0.25, 0.17))  # body rotations happen around this point
MESO = V((0, 0.13, 0.18))  # mesosoma flex joint
TAIL_BASE = V((0, -0.52, 0.19))
TAIL_LEN = [0.26, 0.28, 0.30, 0.32, 0.38, 0.20]  # metasoma I-V, telson vesicle
TAIL_R = [0.070, 0.068, 0.066, 0.066, 0.070, 0.08]
NT = len(TAIL_LEN)
# Segment directions (degrees in the body's mid plane: 0 = back, 90 = up, 180 = forward, 270 = down).
TAIL_REST = [58, 104, 146, 178, 208, 238]
TAIL_STRIKE = [105, 140, 172, 196, 214, 238]
TAIL_COCK = [-14, -10, -6, 0, 6, 12]  # added per unit of tcock: base leans back, tip curls in
TAIL_LIMP = [12, 6, 10, 18, 30, 50]
TAIL_CLIMB = [40, 85, 125, 160, 195, 225]  # climbing (body pitched up): hangs from the body, curls up on the camera side
FOOT_Z = 0.045  # ankle height above the floor
PALP_HIP = 0.125, 0.50, 0.165
PALP_A, PALP_B, HAND_LEN, FINGER_LEN = 0.28, 0.29, 0.19, 0.21


def leg_defs():
    """Rest layout of the eight legs (ankle = foot). s = +1 right, -1 left. Tetrapod gait: L1 R2 L3 R4 vs R1 L2 R3 L4."""
    legs = []
    spec = [  # hip y, hip x, foot (x, y), femur, tibia, curled foot (x, y)
        ("1", 0.43, 0.10, (0.36, 0.68), 0.26, 0.30, (0.16, 0.55)),
        ("2", 0.35, 0.13, (0.48, 0.45), 0.28, 0.33, (0.20, 0.40)),
        ("3", 0.26, 0.14, (0.52, 0.10), 0.30, 0.37, (0.22, 0.22)),
        ("4", 0.17, 0.14, (0.50, -0.25), 0.34, 0.43, (0.22, 0.02)),
    ]
    for s in (1, -1):
        sfx = ".R" if s > 0 else ".L"
        for n, hy, hx, (fx, fy), a, b, (cx, cy) in spec:
            hip = V((hx * s, hy, 0.13))
            foot = V((fx * s, fy, FOOT_Z))
            out = V((foot.x - hip.x, foot.y - hip.y, 0)).normalized()
            group_a = (n in "13") == (s < 0)  # L1, R2, L3, R4
            legs.append(dict(name=n + sfx, s=s, hip=hip, foot=foot, a=a, b=b, bend=V((0.25 * s, 0.05 * (int(n) - 2.5), 1)),
                             tdir=(out * 0.92 + V((0, 0, -0.40))).normalized(), tlen=0.11,
                             curled=V((cx * s, cy, 0.06)), phase=0.0 if group_a else 0.5))
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


def tail_dir(e, yaw=0.0):
    """Body-space direction for mid-plane angle e (degrees), turned by yaw (degrees) about Z."""
    e, yaw = math.radians(e), math.radians(yaw)
    return V((math.sin(yaw) * math.cos(e), -math.cos(yaw) * math.cos(e), math.sin(e)))


# ---------------------------------------------------------------- materials


def speckle(mat, scale=60.0, lo=0.86, hi=1.06):
    """Multiply the base colour by a fine granulation noise (object space)."""
    nt = mat.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    sock = bsdf.inputs["Base Color"]
    coord = nt.nodes.new("ShaderNodeTexCoord")
    tex = nt.nodes.new("ShaderNodeTexNoise")
    tex.inputs["Scale"].default_value = scale
    tex.inputs["Detail"].default_value = 4.0
    nt.links.new(coord.outputs["Object"], tex.inputs["Vector"])
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    els = ramp.color_ramp.elements
    els[0].position, els[0].color = 0.3, (lo,) * 3 + (1,)
    els[1].position, els[1].color = 0.7, (hi,) * 3 + (1,)
    nt.links.new(tex.outputs["Fac"], ramp.inputs["Fac"])
    mul = nt.nodes.new("ShaderNodeVectorMath")
    mul.operation = "MULTIPLY"
    if sock.is_linked:
        nt.links.new(sock.links[0].from_socket, mul.inputs[0])
    else:
        mul.inputs[0].default_value = sock.default_value[:3]
    nt.links.new(ramp.outputs["Color"], mul.inputs[1])
    nt.links.new(mul.outputs[0], sock)
    return mat


def materials(pal=COL):
    specs = {
        "carapace": ("solid", "straw"),
        # Tergite tube, v = length from the front: pale where it tucks under the plate in front, dark rear rim.
        "tergite": ("stripes", "v", 0.13, [(2, "straw_hi"), (7, "straw"), (2, "rim"), (2, "amber")], "LINEAR"),
        "last_tergite": ("stripes", "v", 0.20, [(2, "straw_hi"), (12, "straw"), (2, "rim")], "LINEAR"),
        "leg": ("solid", "straw_pale"),
        "joint": ("solid", "rim"),
        "tarsus": ("stripes", "v", 0.12, [(3, "straw_pale"), (2, "amber")], "LINEAR"),
        "palp": ("solid", "claw"),
        "finger": ("stripes", "v", 0.25, [(3, "claw"), (3, "amber"), (2, "amber_dark")], "LINEAR"),
        "telson": ("solid", "telson"),
        "aculeus": ("stripes", "v", 0.16, [(1, "amber"), (2, "amber_dark"), (3, "black")], "LINEAR"),
        "chelicera": ("stripes", "v", 0.08, [(3, "straw_hi"), (2, "amber_dark")], "LINEAR"),
        "eye": ("solid", "eye"),
        "tubercle": ("solid", "amber"),
    }
    for k, length in enumerate(TAIL_LEN[:5]):
        # Each metasoma segment darkens towards its rear joint; segment V is amber overall.
        body = "tail_end" if k == 4 else "tail"
        specs["tail%d" % k] = ("stripes", "v", length + 0.06, [(3, body), (5, body), (2, "rim" if k < 4 else "amber_dark")], "LINEAR")
    M = {name: common.make_material("scorpion_" + name, spec, pal) for name, spec in specs.items()}
    for name in ("carapace", "tergite", "last_tergite", "leg", "palp", "telson") + tuple("tail%d" % k for k in range(5)):
        speckle(M[name])
    return M


# ---------------------------------------------------------------- parts


def flat_belly(th, a):
    sn = math.sin(th)
    return 1.0 if sn >= 0 else 1.0 - 0.35 * sn * sn


def build_prosoma(b, M):
    def plate(th, a):  # flat top with sloping sides, flat belly
        sn = math.sin(th)
        return (1.0 - 0.12 * sn * sn) if sn >= 0 else 1.0 - 0.35 * sn * sn

    keys = [((0, 0.10, 0.18), 0.17, 0.07), ((0, 0.16, 0.18), 0.19, 0.08), ((0, 0.30, 0.178), 0.18, 0.08),
            ((0, 0.44, 0.172), 0.145, 0.072), ((0, 0.53, 0.165), 0.115, 0.06), ((0, 0.565, 0.16), 0.09, 0.04)]
    b.add(tube(keys, sub=3, n=24, ref=UP, shape=plate, bulge=(0.02, 0.012)), M["carapace"], "body")
    # Ocular tubercle with the two median eyes; three lateral eyes at each front corner.
    b.add(ellipsoid((0, 0.43, 0.245), (0.04, 0.05, 0.022), n=10, rings=5, axis=UP, ref=FWD), M["tubercle"], "body")
    for s in (1, -1):
        b.add(ellipsoid((0.02 * s, 0.435, 0.258), (0.015, 0.015, 0.015), n=8, rings=5), M["eye"], "body")
        for k in range(3):
            b.add(ellipsoid((0.10 * s - 0.008 * k * s, 0.515 - 0.018 * k, 0.205), (0.009, 0.009, 0.009), n=6, rings=4), M["eye"], "body")
        # Chelicerae: small pincers under the front edge.
        base = V((0.035 * s, 0.53, 0.15))
        d = V((0.05 * s, 1, -0.15)).normalized()
        b.add(tube([(base, 0.03, 0.026), (base + d * 0.05, 0.03, 0.026), (base + d * 0.085, 0.016, 0.012), (base + d * 0.105, 0.005, 0.004)],
                   sub=2, n=8, ref=UP, bulge=(0.01, 0.0)), M["chelicera"], "body")
        # Coxae under the carapace.
        for leg in LEGS:
            if leg["s"] == s:
                h = leg["hip"]
                b.add(ellipsoid(h - V((0.02 * s, 0, 0.0)), (0.045, 0.035, 0.035), n=8, rings=4, axis=V((s, 0, 0)), ref=UP), M["leg"], "body")
        b.add(ellipsoid(((PALP_HIP[0] - 0.02) * s, PALP_HIP[1], PALP_HIP[2]), (0.05, 0.04, 0.04), n=8, rings=4, axis=V((s, 0, 0)), ref=UP),
              M["palp"], "body")
    # Sternum / leg bases underneath.
    b.add(ellipsoid((0, 0.30, 0.125), (0.12, 0.04, 0.2), n=12, rings=6, axis=FWD, ref=UP), M["leg"], "body")


def build_mesosoma(b, M):
    """Seven tergite rings, each tucked under the rear rim of the one in front."""
    widths = [0.19, 0.21, 0.225, 0.232, 0.225, 0.20, 0.15]
    heights = [0.088, 0.096, 0.102, 0.104, 0.102, 0.095, 0.08]
    step, length = 0.098, 0.125
    for i in range(7):
        y0 = 0.15 - step * i
        y1 = y0 - length
        rx, ry, z = widths[i], heights[i], 0.185 - 0.004 * i
        if i < 6:
            keys = [((0, y0, z - 0.004), rx * 0.93, ry * 0.9), ((0, y0 - 0.05, z), rx * 0.99, ry), ((0, y1, z + 0.004), rx, ry * 1.02)]
            mat = M["tergite"]
        else:  # the last one tapers to the tail base
            keys = [((0, y0, z - 0.004), rx * 0.93, ry * 0.9), ((0, y0 - 0.06, z), rx * 0.9, ry), ((0, y0 - 0.12, z + 0.002), rx * 0.6, ry * 0.85),
                    ((0, y0 - 0.17, z + 0.005), rx * 0.42, ry * 0.8)]
            mat = M["last_tergite"]
        b.add(tube(keys, sub=3, n=24, ref=UP, shape=flat_belly, bulge=(0.0, 0.015)), mat, "meso")


def tail_points_rest():
    pts = [TAIL_BASE.copy()]
    for k in range(NT):
        pts.append(pts[-1] + tail_dir(TAIL_REST[k]) * TAIL_LEN[k])
    return pts


def build_tail(b, M, joints):
    def keel(th, a):  # rounded square section with low keels on the edges
        c, s = abs(math.cos(th)), abs(math.sin(th))
        sq = (c ** 3 + s ** 3) ** (-1 / 3)
        return 0.82 * sq + 0.06 * max(0.0, math.cos(4 * th)) ** 6

    for k in range(5):
        a, e = joints[k], joints[k + 1]
        d = (e - a).normalized()
        r, L = TAIL_R[k], TAIL_LEN[k]

        def P(t):
            return a + d * L * t

        keys = [(P(-0.06), r * 0.5, r * 0.5), (P(0.04), r * 0.88, r * 0.85), (P(0.5), r * 0.98, r * 0.95), (P(0.9), r * 1.06, r * 1.0),
                (P(1.0), r * 0.92, r * 0.88)]
        b.add(tube(keys, sub=3, n=12, ref=X, shape=keel, bulge=(0.01, 0.01)), M["tail%d" % k], "tail%d" % k, tag="tail")
    # Telson: bulb (vesicle) and the curved sting (aculeus).
    a = joints[5]
    d = (joints[6] - a).normalized()
    side = X
    up = d.cross(side).normalized()  # towards the outside of the curl
    c = a + d * 0.10
    b.add(ellipsoid(c, (0.062, 0.068, 0.105), n=12, rings=8, axis=d, ref=up), M["telson"], "tail5", tag="tail")
    e0 = math.degrees(math.atan2(d.z, -d.y))
    p = a + d * 0.19
    keys = [(p - d * 0.02, 0.034, 0.034)]
    for k, (de, step) in enumerate(((10, 0.04), (25, 0.04), (45, 0.035), (68, 0.03))):
        p = p + tail_dir(e0 + de) * step
        r = 0.028 * (1 - (k + 1) / 4.6) + 0.002
        keys.append((p, r, r))
    b.add(tube(keys, sub=2, n=8, ref=X, caps=(False, True)), M["aculeus"], "tail5", tag="tail")


def rest_leg(leg):
    return solve_leg(leg["hip"], leg["foot"], leg["a"], leg["b"], leg["bend"])


def build_legs(b, M):
    for leg in LEGS:
        n, hip = leg["name"], leg["hip"]
        knee, normal, foot = rest_leg(leg)
        fd, td = (knee - hip).normalized(), (foot - knee).normalized()
        b.add(tube([(hip - fd * 0.02, 0.034, 0.027), (hip + fd * leg["a"] * 0.5, 0.037, 0.029), (knee, 0.031, 0.025)], sub=2, n=8, ref=normal,
                   bulge=(0.01, 0.0)), M["leg"], "femur" + n, tag="leg")
        b.add(ellipsoid(knee, (0.033, 0.033, 0.033), n=8, rings=4), M["joint"], "tibia" + n, tag="leg")
        b.add(tube([(knee, 0.029, 0.025), (knee + td * leg["b"] * 0.42, 0.027, 0.024), (knee + td * leg["b"] * 0.46, 0.031, 0.027),
                    (knee + td * leg["b"] * 0.5, 0.025, 0.022), (foot, 0.02, 0.018)], sub=2, n=8, ref=normal, bulge=(0.0, 0.01)), M["leg"],
              "tibia" + n, tag="leg")
        t = leg["tdir"]
        end = foot + t * leg["tlen"]
        b.add(tube([(foot - t * 0.01, 0.02, 0.018), (foot + t * leg["tlen"] * 0.5, 0.014, 0.012), (end, 0.006, 0.006)], sub=2, n=6, ref=normal,
                   bulge=(0.0, 0.004)), M["tarsus"], "tarsus" + n, tag="leg")


def palp_rest(s):
    hip = V((PALP_HIP[0] * s, PALP_HIP[1], PALP_HIP[2]))
    wrist = V((0.33 * s, 0.86, 0.21))
    bend = V((s, -0.1, 0.45))
    hdir = V((-0.38 * s, 1, -0.06)).normalized()
    return hip, wrist, bend, hdir


def hinge_point(s, wrist, hdir, up):
    lat = hdir.cross(up).normalized() * -s  # towards the outside
    return wrist + hdir * HAND_LEN + lat * 0.026


def finger_geo(base, d, lat, sign, length, r0):
    """Slim finger from base along d, bowing towards lat * sign, tapering to a point."""
    keys = []
    for k in range(5):
        t = k / 4
        p = base + d * length * t + lat * sign * 0.012 * math.sin(math.pi * t) - lat * sign * 0.018 * t
        r = r0 * (1 - 0.8 * t) + 0.002
        keys.append((p, r, r * 0.85))
    return tube(keys, sub=2, n=8, ref=UP, caps=(True, True), bulge=(0.0, 0.004))


def build_palps(b, M):
    for s in (1, -1):
        sfx = ".R" if s > 0 else ".L"
        hip, wrist, bend, hdir = palp_rest(s)
        elbow, normal, wrist = solve_leg(hip, wrist, PALP_A, PALP_B, bend)
        fd, pd = (elbow - hip).normalized(), (wrist - elbow).normalized()
        b.add(tube([(hip - fd * 0.02, 0.036, 0.033), (hip + fd * PALP_A * 0.5, 0.04, 0.036), (elbow, 0.036, 0.033)], sub=2, n=10, ref=UP, bulge=(0.01, 0.0)),
              M["palp"], "pfemur" + sfx, tag="palp")
        b.add(ellipsoid(elbow, (0.042, 0.042, 0.042), n=10, rings=5), M["joint"], "ppatella" + sfx, tag="palp")
        b.add(tube([(elbow, 0.038, 0.035), (elbow + pd * PALP_B * 0.55, 0.048, 0.042), (wrist, 0.038, 0.035)], sub=2, n=10, ref=UP, bulge=(0.0, 0.01)),
              M["palp"], "ppatella" + sfx, tag="palp")
        # Chela: slim hand and the fixed finger (inner side), movable finger on the outer side.
        lat = hdir.cross(UP).normalized() * -s  # outwards
        b.add(ellipsoid(wrist + hdir * 0.1, (0.062, 0.052, 0.115), n=12, rings=7, axis=hdir, ref=UP), M["palp"], "phand" + sfx, tag="palp")
        fbase = wrist + hdir * (HAND_LEN - 0.015) - lat * 0.02
        b.add(finger_geo(fbase, hdir, lat, 1, FINGER_LEN, 0.026), M["finger"], "phand" + sfx, tag="palp")
        hinge = hinge_point(s, wrist, hdir, UP)
        b.add(finger_geo(hinge - hdir * 0.015, hdir, lat, -1, FINGER_LEN - 0.01, 0.024), M["finger"], "pfinger" + sfx, tag="palp")


# ---------------------------------------------------------------- rig


def build_rig(coll, tail):
    arm = bpy.data.armatures.new("scorpion_rig")
    rig = bpy.data.objects.new("scorpion_rig", arm)
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
    bone("body", PIVOT, PIVOT + V((0, 0.3, 0)), X, "root")
    bone("meso", MESO, MESO + V((0, -0.3, 0)), X, "body")
    for k in range(NT):
        bone("tail%d" % k, tail[k], tail[k + 1], X, "meso" if k == 0 else "tail%d" % (k - 1))
    for leg in LEGS:
        knee, normal, foot = rest_leg(leg)
        n = leg["name"]
        bone("femur" + n, leg["hip"], knee, normal, "body")
        bone("tibia" + n, knee, foot, normal, "femur" + n)
        bone("tarsus" + n, foot, foot + leg["tdir"] * leg["tlen"], normal, "tibia" + n)
    for s in (1, -1):
        sfx = ".R" if s > 0 else ".L"
        hip, wrist, bend, hdir = palp_rest(s)
        elbow, normal, wrist = solve_leg(hip, wrist, PALP_A, PALP_B, bend)
        bone("pfemur" + sfx, hip, elbow, normal, "body")
        bone("ppatella" + sfx, elbow, wrist, normal, "pfemur" + sfx)
        bone("phand" + sfx, wrist, wrist + hdir * HAND_LEN, UP, "ppatella" + sfx)
        h = hinge_point(s, wrist, hdir, UP)
        bone("pfinger" + sfx, h, h + hdir * FINGER_LEN, UP, "phand" + sfx)
    bpy.ops.object.mode_set(mode="OBJECT")
    for pb in rig.pose.bones:
        pb.rotation_mode = "QUATERNION"
    return rig


# ---------------------------------------------------------------- animation

REST = dict(x=0.0, y=0.0, z=0.0, pitch=0.0, roll=0.0, yaw=0.0, flex=0.0,
            preach=0.0, pspread=0.0, pup=0.0, popen=0.0, pyaw=0.0, plimp=0.0,
            strike=0.0, tcock=0.0, tlimp=0.0, tflop=0.0, tsway=0.0, tphase=0.0, tbob=0.0, tclimb=0.0,
            plant=1.0, curl=0.0, flail=0.0, fphase=0.0)
STRIDE, STEP_H, CYCLES = 0.13, 0.07, 2  # gait cycles per walk clip


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
    p.update(z=0.006 * math.cos(2 * w), roll=1.2 * math.sin(w), yaw=1.5 * math.cos(w), pitch=0.6 * math.sin(2 * w), flex=1.0 * math.sin(w + 0.5),
             preach=0.015 * math.sin(w), pspread=0.012 * math.cos(w), popen=14 + 5 * math.sin(2 * math.pi * t * 2 + 1),
             tsway=5, tphase=t, tbob=2.5 * math.sin(2 * math.pi * t * 2))
    p["feet"] = {leg["name"]: gait(leg, CYCLES * t) for leg in LEGS}
    return p


READY = walk_params(0.0)
READY_FEET = {k: v[0].copy() for k, v in READY["feet"].items()}


def body_mats(p):
    T = Matrix.Translation(V((p["x"], p["y"], p["z"]))) @ rot_about(PIVOT, euler(p["pitch"], p["roll"], p["yaw"]))
    Ms = T @ rot_about(MESO, euler(p["flex"]))
    return T, Ms


def leg_targets(p, T):
    """Foot target (armature space) and swing amount per leg for non-walk frames."""
    out = {}
    for leg in LEGS:
        n, s = leg["name"], leg["s"]
        planted = READY_FEET[n]
        free = planted.lerp(leg["curled"], p["curl"])
        ph = 2 * math.pi * (p["fphase"] + int(n[0]) * 0.27 + (0.13 if s > 0 else 0.0))
        free += V((0.10 * s * math.sin(ph * 1.7), 0.12 * math.cos(ph * 1.3), 0.12 * math.sin(ph + 1))) * p["flail"]
        w = p["plant"]
        target = planted.lerp(T @ free, 1 - w)
        if abs(p["roll"]) < 45:
            target.z = max(target.z, FOOT_Z)
        out[n] = (target, 1 - w)
    return out


def tail_angles(p):
    out = []
    for k in range(NT):
        e = TAIL_REST[k] + (TAIL_STRIKE[k] - TAIL_REST[k]) * p["strike"]
        e += TAIL_COCK[k] * p["tcock"] + p["tbob"] * (0.4 + 0.6 * k / (NT - 1))
        e += (TAIL_LIMP[k] - e) * p["tlimp"]
        e += (TAIL_CLIMB[k] - e) * p["tclimb"]
        yaw = p["tsway"] * math.sin(2 * math.pi * (p["tphase"] - 0.07 * k)) * (0.3 + 0.7 * k / (NT - 1))
        out.append((e, yaw))
    return out


def tail_points(Ms, p):
    """Armature-space tail joints for mesosoma matrix Ms; limp segments sag and are laid onto the floor."""
    R = Ms.to_3x3()
    pts = [Ms @ TAIL_BASE]
    for k, (e, yaw) in enumerate(tail_angles(p)):
        d = R @ tail_dir(e, yaw)
        if p["tflop"] > 0:
            h = V((d.x, d.y, 0))
            h = h.normalized() if h.length > 1e-4 else V((0, -1, 0))
            d = d.lerp((h + V((0, 0, -0.4))).normalized(), p["tflop"]).normalized()
        seg = TAIL_LEN[k]
        nxt = pts[-1] + d * seg
        floor = TAIL_R[k] * 1.05 + 0.004
        if nxt.z < floor and not p["tclimb"]:  # climbing, the body is held above the floor by the ladder
            dz = floor - pts[-1].z
            h = V((d.x, d.y, 0))
            h = h.normalized() if h.length > 1e-4 else V((0, -1, 0))
            if abs(dz) < seg:
                nxt = pts[-1] + h * math.sqrt(seg * seg - dz * dz) + V((0, 0, dz))
            else:
                nxt.z = floor
        pts.append(nxt)
    return pts, R @ X


def palp_mats(rest, p, T, s, mats):
    sfx = ".R" if s > 0 else ".L"
    hip0, wrist0, bend0, hdir0 = palp_rest(s)
    elbow0, normal0, wrist0 = solve_leg(hip0, wrist0, PALP_A, PALP_B, bend0)
    R = T.to_3x3()
    if "palps" in p:  # climbing: wrist target and hand direction in armature space, the claw's fingers above and below a rung
        target, hd_arm, x_hint = p["palps"][s]
    else:
        limp = V((0.2 * s, 0.70, 0.08))
        w_body = wrist0 + V((p["pspread"] * s, p["preach"], p["pup"]))
        w_body = w_body.lerp(limp, p["plimp"])
        target = T @ w_body
        hd_arm, x_hint = R @ (euler(45 * p["pup"] - 20 * p["plimp"], 0, p["pyaw"] * s) @ hdir0), R @ UP
    hip = T @ hip0
    elbow, normal, wrist = solve_leg(hip, target, PALP_A, PALP_B, R @ bend0)
    mats["pfemur" + sfx] = frame(hip, elbow - hip, normal) @ frame(hip0, elbow0 - hip0, normal0).inverted() @ rest["pfemur" + sfx]
    mats["ppatella" + sfx] = frame(elbow, wrist - elbow, normal) @ frame(elbow0, wrist0 - elbow0, normal0).inverted() @ rest["ppatella" + sfx]
    H = frame(wrist, hd_arm, x_hint) @ frame(wrist0, hdir0, UP).inverted()
    mats["phand" + sfx] = H @ rest["phand" + sfx]
    hinge0 = rest["pfinger" + sfx].translation
    mats["pfinger" + sfx] = H @ rot_about(hinge0, Matrix.Rotation(math.radians(-s * p["popen"]), 3, UP)) @ rest["pfinger" + sfx]


def pose_matrices(rest, p):
    """Armature-space pose matrices of every bone except the tail."""
    T, Ms = body_mats(p)
    mats = {"root": rest["root"], "body": T @ rest["body"], "meso": Ms @ rest["meso"]}
    R = T.to_3x3()
    feet = p["feet"] if "feet" in p else leg_targets(p, T)
    for leg in LEGS:
        n = leg["name"]
        knee0, normal0, foot0 = rest_leg(leg)
        target, swing = feet[n]
        hip = T @ leg["hip"]
        knee, normal, foot = solve_leg(hip, target, leg["a"], leg["b"], R @ leg["bend"])
        mats["femur" + n] = frame(hip, knee - hip, normal) @ frame(leg["hip"], knee0 - leg["hip"], normal0).inverted() @ rest["femur" + n]
        mats["tibia" + n] = frame(knee, foot - knee, normal) @ frame(knee0, foot0 - knee0, normal0).inverted() @ rest["tibia" + n]
        td0 = leg["tdir"]
        body_td = R @ td0
        hang = (body_td + (foot - knee).normalized() * 0.8).normalized()
        planted = flat(body_td) * math.sqrt(1 - td0.z * td0.z) + V((0, 0, td0.z))
        if "tdirs" in p:  # climbing: the tarsus hooks onto the rung
            planted = p["tdirs"][n]
        td = planted.lerp(hang, min(1.0, swing * 1.5)).normalized()
        mats["tarsus" + n] = frame(foot, td, normal) @ frame(foot0, td0, normal0).inverted() @ rest["tarsus" + n]
    for s in (1, -1):
        palp_mats(rest, p, T, s, mats)
    return mats, Ms


class Skin:
    """numpy linear blend skinning of the non-tail vertices (or all of them) for the floor contact."""

    def __init__(self, b, rest, tail=False):
        import numpy as np

        self.np = np
        keep = [i for i, t in enumerate(b.tags) if tail or t != "tail"]
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

    def points(self, mats, rows=(0, 1, 2)):
        """Skinned vertex coordinates (the given axes) for pose matrices mats."""
        np = self.np
        out = np.zeros((len(self.P), len(rows)))
        for j, n in enumerate(self.bones):
            m = np.array(mats[n] @ self.inv[n])[list(rows)]
            out += self.W[:, j : j + 1] * (self.P @ m.T)
        return out


STING = {}  # frame -> sting tip (armature space), filled while keying the attack


def frame_mats(rig, skins, p, lift=None):
    """Armature-space matrices of every bone: floor-fixed, or raised by a fixed lift (the climb holds its height)."""
    skin, full = skins
    bones = rig.data.bones
    rest = {b.name: b.matrix_local.copy() for b in bones}
    mats, Ms = pose_matrices(rest, p)
    S = Matrix.Translation(V((0, 0, -skin.lowest(mats) if lift is None else lift)))
    mats = {n: S @ m for n, m in mats.items()}
    pts, xh = tail_points(S @ Ms, p)
    for k in range(NT):
        n = "tail%d" % k
        h0, t0 = bones[n].head_local, bones[n].tail_local
        mats[n] = frame(pts[k], pts[k + 1] - pts[k], xh) @ frame(h0, t0 - h0, X).inverted() @ rest[n]
    low = full.lowest(mats)
    if low < 0 and lift is None:  # the sting (beyond the last tail joint) would dip below the floor: lift everything
        S = Matrix.Translation(V((0, 0, -low)))
        mats = {n: S @ m for n, m in mats.items()}
    return mats, rest


def key_frame(rig, skins, p, frame_no, record=None, lift=None):
    bones = rig.data.bones
    mats, rest = frame_mats(rig, skins, p, lift)
    if record is not None:
        record[frame_no] = (mats["tail5"] @ rest["tail5"].inverted() @ STING_REST, mats["body"] @ rest["body"].inverted() @ V((0, 0.565, 0.16)))
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


def interpolate(keys, frame_no, linear=("fphase", "tphase")):
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


# Grab with both claws and pinch, cock the tail, strike over the head (the sting stabs down in front of the claws), recoil.
ATTACK = [
    (0, {}),
    (3, dict(y=-0.03, pitch=4, preach=0.10, pspread=0.10, pup=0.05, popen=42, pyaw=-8, tcock=0.6)),
    (6, dict(y=0.04, pitch=-2, preach=0.17, pspread=0.03, pup=0.0, popen=0, pyaw=6, tcock=1.0)),
    (8, dict(y=0.0, pitch=4, preach=0.08, pspread=0.05, popen=0, pyaw=4, tcock=1.3)),
    (11, dict(y=0.12, pitch=-6, preach=-0.04, pspread=0.14, popen=0, pyaw=14, strike=1.0)),
    (13, dict(y=0.14, pitch=-7, preach=-0.04, pspread=0.15, popen=0, pyaw=14, strike=0.97)),
    (16, dict(y=0.07, pitch=-2, preach=0.05, pspread=0.08, popen=6, pyaw=6, strike=0.35, tcock=0.5)),
    (20, dict(y=0.02, preach=0.02, popen=12, strike=0.0, tcock=0.15)),
    (24, {}),
]

DIE = [
    (0, {}),
    (2, dict(pitch=8, flex=-10, z=0.03, pup=0.08, popen=40, tcock=1.0, plant=0.5, flail=0.3, fphase=0.2, tsway=10)),
    (4, dict(pitch=-4, flex=8, roll=10, popen=5, tcock=-0.6, plant=0.2, flail=0.6, fphase=0.5, pspread=0.1, tsway=12)),
    (6, dict(pitch=6, flex=-10, roll=-25, popen=35, tcock=1.2, plant=0, flail=0.8, fphase=0.9, pup=0.05, tsway=10)),
    (9, dict(roll=-70, x=-0.05, flex=6, tlimp=0.3, tflop=0.3, plant=0, flail=1.0, fphase=1.4, popen=10, plimp=0.3, tsway=8)),
    (12, dict(roll=-100, x=-0.12, tlimp=0.6, tflop=0.7, plant=0, flail=0.9, fphase=2.0, popen=30, plimp=0.5, tsway=6)),
    (15, dict(roll=-106, x=-0.14, flex=-4, tlimp=0.85, tflop=1.0, plant=0, flail=0.6, fphase=2.6, curl=0.3, popen=10, plimp=0.6, tsway=4)),
    (19, dict(roll=-102, x=-0.14, flex=3, tlimp=1.0, tflop=1.0, plant=0, flail=0.35, fphase=3.2, curl=0.6, popen=25, plimp=0.8, tsway=2)),
    (24, dict(roll=-104, x=-0.14, tlimp=1.0, tflop=1.0, plant=0, flail=0.1, fphase=3.6, curl=0.9, popen=15, plimp=1.0, tsway=0)),
    (29, dict(roll=-104, x=-0.14, tlimp=1.0, tflop=1.0, plant=0, flail=0.0, fphase=3.8, curl=1.0, popen=20, plimp=1.0, tsway=0)),
]


# Climbing a ladder (scorpion_climb.md3). The engine draws it at rotA + 180, so +Y points at the ladder and the back
# faces the camera. It sets the frame from the climb height (one cycle per CLIMB_RUNGS rungs; the clock does not advance
# it) and lifts the body, so here the body holds its height and a gripping tarsus slides down at the climb rate.
# Alternating tetrapods as in the walk: one tetrapod grips and pulls (half a cycle, one rung) while the other swings
# up one rung, off the ladder. The claws take turns pinching a rung above. Ladder sizes (rails at x = +-0.1 tile, a hold
# every tile / 12) come from the walk's frame 0, which the engine scales to the kind's size (scorpion 15 units = 0.375 tile).
CLIMB_SCALE = 15  # the scorpion's kind scale (world units, 40 = 1 tile); the giant and the queen share the clip
CLIMB_RUNGS = 2  # rungs climbed per cycle (R)
CLIMB_PITCH = 78  # body pitched head-up, the belly towards the ladder
CLIMB_GRIP_Y = 0.50  # G: the ladder plane, where the tarsi and the claws grip
CLIMB_OFF = 0.13  # a swinging tarsus comes this far off the ladder
# Per leg: x of the grip, the wanted top of its stroke in the body's height (before the lift); snapped to a rung.
CLIMB_LEGS = {"1": (0.40, 0.80), "2": (0.50, 0.36), "3": (0.58, 0.30), "4": (0.46, -0.10)}
CLIMB_PALP = (0.20, 1.15)  # x of the claw's grip, its wanted top
CLIMB_HAND = V((0, 0.45, 0.9)).normalized()  # claw direction while gripping: up, towards the ladder
CLIMB_PINCH = HAND_LEN + 0.08  # wrist -> the rung between the fingers
CLIMB_REF = {}  # walk frame 0 sizes, the ladder in authored units, the lift; filled by climb_setup


def climb_stroke(u):
    """Stroke position (1 = top, 0 = bottom) and lift at phase u: grip and pull down at the climb rate, swing up."""
    u %= 1.0
    if u < 0.5:
        return 1 - u / 0.5, 0.0  # linear: the ladder rises at a constant rate against the body
    k = (u - 0.5) / 0.5
    return smoothstep(0, 1, k), math.sin(math.pi * k)


def climb_params(t, ref):
    """Pose at climb phase t; foot and wrist targets in armature space before the lift (ref["lift"])."""
    p = dict(REST)
    w = 2 * math.pi * t
    p.update(pitch=CLIMB_PITCH + 0.8 * math.sin(2 * w), roll=2.5 * math.sin(w), yaw=2.0 * math.cos(w), flex=-3 + 2 * math.sin(2 * w),
             tclimb=1.0, tsway=6, tphase=t, tbob=2.5 * math.sin(2 * w))
    lift, rung, tops = ref["lift"], ref["rung"], ref["tops"]
    feet, tdirs = {}, {}
    for leg in LEGS:
        n, s = leg["name"], leg["s"]
        x, _ = CLIMB_LEGS[n[0]]
        k, up = climb_stroke(t + leg["phase"])
        tip = V((x * s, CLIMB_GRIP_Y - CLIMB_OFF * up, tops[n[0]] - rung + rung * k - lift))
        td = V((0.25 * s, 1, -0.45)).normalized()
        feet[n] = (tip - td * leg["tlen"], up)
        tdirs[n] = td
    p["feet"], p["tdirs"] = feet, tdirs
    palps, popen = {}, []
    for s, phase in ((1, 0.0), (-1, 0.5)):  # the right claw grips with L1 R2 L3 R4
        k, up = climb_stroke(t + phase)
        pinch = V((CLIMB_PALP[0] * s, CLIMB_GRIP_Y - 0.12 * up, tops["p"] - rung + rung * k - lift))
        hd = (CLIMB_HAND + V((0.15 * s, -0.3, 0)) * up).normalized()
        palps[s] = (pinch - hd * CLIMB_PINCH, hd, X)
        popen.append(40 * up)
    p["palps"], p["popen"] = palps, max(popen)  # one movable-finger angle for both claws: the swinging claw's
    return p


def climb_setup(rig, skins):
    """Walk frame 0 sizes, the ladder in authored units, the lift (frame 0's lowest point on the floor) and the rungs."""
    full = skins[1]
    pts = full.points(frame_mats(rig, skins, walk_params(0.0))[0])
    lo, hi = pts.min(axis=0), pts.max(axis=0)
    size = float(max(hi - lo))
    tile = 40 * size / CLIMB_SCALE
    ref = dict(S=size, H=float(hi[2] - lo[2]), C=float(lo[1] + hi[1]) / 2, tile=tile, rung=tile / 12, R=CLIMB_RUNGS * tile / 12, lift=0.75)
    rung = ref["rung"]
    def lowest(f):
        return full.lowest(frame_mats(rig, skins, climb_params(f / CLIMB[2], ref), ref["lift"])[0])

    for _ in range(5):  # the tops snap to holds ((k + 0.5) rungs above the floor), the lift puts the clip's lowest point on it
        tops = {n: top for n, (_, top) in CLIMB_LEGS.items()}
        tops["p"] = CLIMB_PALP[1]
        ref["tops"] = {n: (round((top + ref["lift"]) / rung - 0.5) + 0.5) * rung for n, top in tops.items()}
        ref["lift"] -= min(lowest(f) for f in range(0, CLIMB[2], 2))
    ref["low"] = lowest(0)
    CLIMB_REF.update(ref)
    return ref


def make_actions(rig, skin):
    rig.animation_data_create()
    acts = {}
    # The climb is keyed first: keyed last it shifted the other clips' export by ~5e-5 (they stay byte-identical this way).
    for name, suffix, frames in sorted(CLIPS, key=lambda c: c != CLIMB):
        act = bpy.data.actions.get(name)
        if act:
            bpy.data.actions.remove(act)
        act = bpy.data.actions.new(name)
        act.use_fake_user = True
        rig.animation_data.action = act
        last = frames if name != "scorpion_die" else frames - 1  # loops key frame N = frame 0
        if name == CLIMB[0]:
            ref = climb_setup(rig, skin)
            for f in range(last + 1):
                key_frame(rig, skin, climb_params(f / frames, ref), f, lift=ref["lift"])
            acts[name] = act
            continue
        for f in range(last + 1):
            if name == "scorpion_walk":
                p = walk_params(f / frames)
            else:
                p = interpolate({"scorpion_attack": ATTACK, "scorpion_die": DIE}[name], f)
                p["tphase"] = f / CLIPS[0][2]  # keeps the tail sway going from where the walk's frame 0 is
                if name == "scorpion_attack":
                    w = 2 * math.pi * f / frames
                    p["tbob"] = 2.5 * math.sin(2 * w)  # same bob as the walk, so frame 0 matches
                    p["tsway"] = 5
            key_frame(rig, skin, p, f, STING if name == "scorpion_attack" else None)
        acts[name] = act
    return acts


STING_REST = V()


# ---------------------------------------------------------------- entry points


def build(bake=True, tex_path=None, giant_path=None):
    global STING_REST
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    M = materials()
    tail = tail_points_rest()
    rig = build_rig(coll, tail)
    b = Builder()
    build_prosoma(b, M)
    build_mesosoma(b, M)
    build_tail(b, M, tail)
    build_legs(b, M)
    build_palps(b, M)
    tail_verts = [v for v, t, w in zip(b.verts, b.tags, b.weights) if "tail5" in w]
    STING_REST = min(tail_verts, key=lambda v: v.z - 0.3 * v.y)  # aculeus tip
    obj = common.finish_mesh(b, coll, "scorpion_new")
    common.uv_unwrap(obj, b.tags, {"palp": (V((0, 0.9, 0.17)), 1.3), "tail": (V((0, -0.3, 0.6)), 1.2)})
    if bake:
        tmp = bpy.app.tempdir or "/tmp"
        tex = common.bake_texture(obj, tex_path or os.path.join(tmp, "scorpion_preview.png"), TEX_SIZE, "scorpion", ao_distance=0.12)
        materials(COL_GIANT)
        giant = common.bake_texture(obj, giant_path or os.path.join(tmp, "scorpion_giant_preview.png"), TEX_SIZE, "scorpion_giant", ao_distance=0.12)
        if os.environ.get("SCORPION_TEX") == "queen":
            materials(COL_QUEEN)
            tex = common.bake_texture(obj, os.path.join(tmp, "scorpion_queen_preview.png"), TEX_SIZE, "scorpion_queen", ao_distance=0.12)
        common.use_baked_material(obj, giant if os.environ.get("SCORPION_TEX") == "giant" else tex)
    common.rig_object(obj, rig)
    rest = {bn.name: bn.matrix_local.copy() for bn in rig.data.bones}
    acts = make_actions(rig, (Skin(b, rest), Skin(b, rest, tail=True)))
    rig.animation_data.action = acts["scorpion_walk"]
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 0, CLIPS[0][2] - 1
    scene.frame_set(0)
    print("scorpion: {} verts, {} tris".format(len(obj.data.vertices), common.tri_count(obj)))
    for f in sorted(STING):
        tip, front = STING[f]
        print("sting f{:02d}: tip y {:.3f} z {:.3f} (carapace front y {:.3f})".format(f, tip.y, tip.z, front.y))
    print_climb(rig)
    if os.environ.get("SCORPION_LADDER"):
        ladder_standin(coll)
    return obj, rig


def print_climb(rig):
    r = CLIMB_REF
    print("climb: walk frame 0 S_ref {S:.4f} H {H:.4f} C {C:.4f}; tile {tile:.4f}, rung {rung:.4f}, R {R:.4f} ({n} rungs), G {G:.3f}, "
          "lift {lift:.4f} (frame 0 lowest {low:+.4f}), {frames} frames".format(n=CLIMB_RUNGS, G=CLIMB_GRIP_Y, frames=CLIMB[2], **r))
    print("climb: climbRise = R / H = {:.4f}, climbGrip = (G - C) / H = {:.4f}".format(r["R"] / r["H"], (CLIMB_GRIP_Y - r["C"]) / r["H"]))
    print("climb: grip tops " + " ".join("{} {:.3f}".format(n, z) for n, z in sorted(r["tops"].items())))
    # How far each IK chain falls short of its target over the clip.
    short = {}
    for f in range(CLIMB[2]):
        p = climb_params(f / CLIMB[2], r)
        for leg in LEGS:
            target = p["feet"][leg["name"]][0]
            T = body_mats(p)[0]
            d = (target - T @ leg["hip"]).length - (leg["a"] + leg["b"]) * 0.999
            short[leg["name"]] = max(short.get(leg["name"], 0.0), d)
        for s in (1, -1):
            d = (p["palps"][s][0] - body_mats(p)[0] @ V((PALP_HIP[0] * s, PALP_HIP[1], PALP_HIP[2]))).length - (PALP_A + PALP_B) * 0.999
            short["palp%+d" % s] = max(short.get("palp%+d" % s, 0.0), d)
    print("climb: IK short of target (max, <= 0 reaches) " + " ".join("{} {:+.3f}".format(n, d) for n, d in sorted(short.items())))


def ladder_standin(coll):
    """Review only (SCORPION_LADDER=1): rails at x = +-0.1 tile and a rung every tile / 12 in the plane y = G."""
    r = CLIMB_REF
    b = Builder()
    mat = common.make_material("scorpion_ladder", ("solid", "rim"), COL)
    for s in (1, -1):
        b.add(tube([((0.1 * r["tile"] * s, CLIMB_GRIP_Y + 0.03, -0.1), 0.025, 0.025), ((0.1 * r["tile"] * s, CLIMB_GRIP_Y + 0.03, 2.6), 0.025, 0.025)], n=8, ref=FWD), mat, "body")
    k = 0
    while (k + 0.5) * r["rung"] < 2.6:
        z = (k + 0.5) * r["rung"]
        b.add(tube([((-0.1 * r["tile"], CLIMB_GRIP_Y + 0.01, z), 0.018, 0.018), ((0.1 * r["tile"], CLIMB_GRIP_Y + 0.01, z), 0.018, 0.018)], n=8, ref=UP), mat, "body")
        k += 1
    common.finish_mesh(b, coll, "scorpion_ladder")


def export(models_dir=None):
    common.export_files(bpy.data.objects["scorpion_new"], bpy.data.objects["scorpion_rig"], "scorpion", CLIPS, "monsters", models_dir)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    if "--boss-texture" in args:
        obj, _ = build(bake=False)
        materials(COL_QUEEN)
        common.bake_texture(obj, os.path.join(REPO, "textures", "monsters", "scorpion_queen.png"), TEX_SIZE, "scorpion_queen", ao_distance=0.12)
    elif "--export-climb" in args:  # the climb clip alone (no texture bake)
        build(bake=False)
        common.export_files(bpy.data.objects["scorpion_new"], bpy.data.objects["scorpion_rig"], "scorpion", [CLIMB], "monsters")
    elif "--export" in args:
        tex_dir = os.path.join(REPO, "textures", "monsters")
        build(tex_path=os.path.join(tex_dir, "scorpion.png"), giant_path=os.path.join(tex_dir, "scorpion_giant.png"))
        export()
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "scorpion.blend"))
    else:
        build()
