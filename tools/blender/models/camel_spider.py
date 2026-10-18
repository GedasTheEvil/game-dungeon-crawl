"""Procedural camel spider (a solifuge, Galeodes arabs): mesh, rig, baked texture and the .md3 animations (walk, attack,
die, idle).

    MCP:  p = ".../tools/blender/models/camel_spider.py"; g = {"__file__": p, "__name__": "camel_spider"}
          exec(open(p).read(), g); g["build"]()          # then g["export"]()
    CLI:  blender -b --python tools/blender/models/camel_spider.py -- [--export]

Blender space: Z up, the spider faces +Y (game uses rotA = 180), its right side is +X.
Built on scorpion.py's rig: rigid parts follow one bone each. The prosoma (the domed head plate, the eye tubercle) and
the opisthosoma (a fat segmented abdomen on a flex bone); the two huge chelicerae (each a bone that saws forward and
back, the movable lower finger on a hinge that gapes); eight thin hairy legs (femur, tibia, tarsus; two-bone IK,
alternating tetrapod gait); two long pedipalps held forward like a fifth pair of legs (femur, tibia by two-bone IK
towards a wrist target, the tarsus with its pale sucker), so it looks ten-legged. The whole pose is lifted every frame
so the lowest vertex touches the floor (numpy skinning, as in scorpion.py).
Clips: walk (16 frames, two gait cycles: a fast scuttle, the palps tapping), attack (18: rears, palps up, lunges with
the jaws open, snaps), die (26: flips onto its back, the legs curl), idle (24: waits, palps tapping, jaws sawing; shown
while it stands at the edge of the light).
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

COLL = "camel_spider_new"
CLIPS = [("camel_spider_walk", "", 16), ("camel_spider_attack", "_att", 18), ("camel_spider_die", "_die", 26),
         ("camel_spider_idle", "_idle", 24)]
TEX_SIZE = 1024
REVIEW_VIEW = {"target": (0, 0.1, 0.25), "ortho": 2.2, "res": (560, 400)}

COL = {
    "sand": (0.66, 0.46, 0.22),
    "sand_hi": (0.80, 0.62, 0.36),
    "sand_dark": (0.40, 0.25, 0.10),
    "pale": (0.86, 0.76, 0.55),
    "rust": (0.50, 0.20, 0.07),
    "rust_dark": (0.20, 0.07, 0.025),
    "black": (0.012, 0.008, 0.005),
    "eye": (0.004, 0.004, 0.004),
}

UP, FWD, X = V((0, 0, 1)), V((0, 1, 0)), V((1, 0, 0))
PIVOT = V((0, 0.15, 0.20))  # body rotations happen around this point
ABD = V((0, 0.07, 0.21))  # opisthosoma flex joint
FOOT_Z = 0.035  # ankle height above the floor
PALP_HIP = 0.07, 0.37, 0.17
PALP_A, PALP_B, TARSUS_LEN = 0.34, 0.36, 0.20
CHEL_BASE_Y, CHEL_TIP_Y = 0.30, 0.78


def leg_defs():
    """Rest layout of the eight legs (ankle = foot). s = +1 right, -1 left. Tetrapod gait: L1 R2 L3 R4 vs R1 L2 R3 L4."""
    legs = []
    spec = [  # hip y, hip x, foot (x, y), femur, tibia, thickness, curled foot (x, y)
        ("1", 0.31, 0.075, (0.34, 0.58), 0.28, 0.34, 0.65, (0.14, 0.42)),
        ("2", 0.25, 0.09, (0.47, 0.30), 0.31, 0.38, 1.0, (0.18, 0.30)),
        ("3", 0.18, 0.095, (0.51, -0.03), 0.33, 0.42, 1.0, (0.19, 0.12)),
        ("4", 0.11, 0.085, (0.45, -0.40), 0.38, 0.50, 1.05, (0.18, -0.08)),
    ]
    for s in (1, -1):
        sfx = ".R" if s > 0 else ".L"
        for n, hy, hx, (fx, fy), a, b, th, (cx, cy) in spec:
            hip = V((hx * s, hy, 0.17))
            foot = V((fx * s, fy, FOOT_Z))
            out = V((foot.x - hip.x, foot.y - hip.y, 0)).normalized()
            group_a = (n in "13") == (s < 0)  # L1, R2, L3, R4
            legs.append(dict(name=n + sfx, s=s, hip=hip, foot=foot, a=a, b=b, th=th, bend=V((0.08 * s, 0.04 * (int(n) - 2.5), 1)),
                             tdir=(out * 0.9 + V((0, 0, -0.42))).normalized(), tlen=0.13,
                             curled=V((cx * s, cy, 0.07)), phase=0.0 if group_a else 0.5))
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


def speckle(mat, scale=70.0, lo=0.82, hi=1.08):
    """Multiply the base colour by a fine granulation noise (object space): the short hairs."""
    nt = mat.node_tree
    bsdf = nt.nodes["Principled BSDF"]
    sock = bsdf.inputs["Base Color"]
    coord = nt.nodes.new("ShaderNodeTexCoord")
    tex = nt.nodes.new("ShaderNodeTexNoise")
    tex.inputs["Scale"].default_value = scale
    tex.inputs["Detail"].default_value = 6.0
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
        "head": ("solid", "sand_hi"),
        "thorax": ("solid", "sand"),
        # Abdomen tube, v = length from the front: a dark band at each segment's rear edge.
        "abdomen": ("stripes", "v", 0.075, [(6, "sand"), (2, "sand_dark")], "LINEAR"),
        "belly": ("solid", "pale"),
        "leg": ("solid", "sand_hi"),
        "leg_end": ("stripes", "v", 0.12, [(4, "sand"), (2, "sand_dark")], "LINEAR"),
        "joint": ("solid", "sand_dark"),
        "palp": ("solid", "sand"),
        "palp_end": ("solid", "sand_dark"),
        "sucker": ("solid", "pale"),
        # Chelicera tube, v from the base: pale, rust, the dark finger.
        "chelicera": ("stripes", "v", 0.42, [(10, "sand_hi"), (12, "rust"), (6, "rust_dark")], "LINEAR"),
        "finger": ("stripes", "v", 0.2, [(3, "rust"), (4, "rust_dark"), (3, "black")], "LINEAR"),
        "hair": ("solid", "sand_dark"),
        "eye": ("solid", "eye"),
        "tubercle": ("solid", "black"),
    }
    M = {name: common.make_material("camel_spider_" + name, spec, pal) for name, spec in specs.items()}
    for name in ("head", "thorax", "abdomen", "leg", "palp", "chelicera"):
        speckle(M[name])
    return M


# ---------------------------------------------------------------- parts


def flat_belly(th, a):
    sn = math.sin(th)
    return 1.0 if sn >= 0 else 1.0 - 0.3 * sn * sn


def hairs(b, M, bone, start, d, length, r, count, normal, tag, size=0.05):
    """Short bristles along a segment, alternating to both sides of the leg plane and up."""
    side = d.cross(normal).normalized()
    for k in range(count):
        t = (k + 0.5) / count
        p = start + d * length * t
        out = (side * (1 if k % 2 else -1) + normal * 0.6 + d * 0.5).normalized()
        b.add(common.cone(p + out * r * 0.6, out, size, 0.006, n=3), M["hair"], bone, tag=tag)


def build_prosoma(b, M):
    def dome(th, a):  # domed top, flat belly
        sn = math.sin(th)
        return (1.0 + 0.06 * sn) if sn >= 0 else 1.0 - 0.3 * sn * sn

    # Propeltidium: the big domed head plate, the chelicerae under its front edge.
    head = [((0, 0.24, 0.215), 0.10, 0.07), ((0, 0.28, 0.225), 0.15, 0.10), ((0, 0.34, 0.235), 0.165, 0.115),
            ((0, 0.40, 0.235), 0.155, 0.11), ((0, 0.44, 0.225), 0.12, 0.085), ((0, 0.46, 0.215), 0.07, 0.05)]
    b.add(tube(head, sub=3, n=24, ref=UP, shape=dome, bulge=(0.02, 0.012)), M["head"], "body")
    # Meso- and metapeltidium: the narrow neck to the abdomen.
    neck = [((0, 0.06, 0.205), 0.09, 0.07), ((0, 0.12, 0.205), 0.105, 0.075), ((0, 0.19, 0.208), 0.11, 0.078), ((0, 0.25, 0.212), 0.10, 0.072)]
    b.add(tube(neck, sub=2, n=20, ref=UP, shape=flat_belly, bulge=(0.01, 0.01)), M["thorax"], "body")
    # Eye tubercle at the front of the head plate, two black eyes.
    b.add(ellipsoid((0, 0.43, 0.31), (0.035, 0.03, 0.022), n=10, rings=5, axis=UP, ref=FWD), M["tubercle"], "body")
    for s in (1, -1):
        b.add(ellipsoid((0.018 * s, 0.442, 0.322), (0.014, 0.014, 0.014), n=8, rings=5), M["eye"], "body")
        for leg in LEGS:  # coxae under the plate
            if leg["s"] == s:
                h = leg["hip"]
                b.add(ellipsoid(h - V((0.015 * s, 0, 0)), (0.04, 0.03, 0.032), n=8, rings=4, axis=V((s, 0, 0)), ref=UP), M["belly"], "body")
        b.add(ellipsoid(((PALP_HIP[0] - 0.015) * s, PALP_HIP[1], PALP_HIP[2]), (0.045, 0.036, 0.036), n=8, rings=4, axis=V((s, 0, 0)), ref=UP),
              M["belly"], "body")
    b.add(ellipsoid((0, 0.22, 0.155), (0.08, 0.04, 0.16), n=12, rings=6, axis=FWD, ref=UP), M["belly"], "body")
    for k in range(10):  # bristles on the head plate
        a = 2 * math.pi * k / 10
        p = V((0.11 * math.cos(a), 0.34 + 0.06 * math.sin(a), 0.30))
        out = (p - V((0, 0.34, 0.15))).normalized()
        b.add(common.cone(p, out + V((0, 0.4, 0)), 0.05, 0.006, n=3), M["hair"], "body")


def chelicera_rest(s):
    base = V((0.062 * s, CHEL_BASE_Y, 0.225))
    tip = V((0.066 * s, CHEL_TIP_Y, 0.20))
    hinge = V((0.066 * s, 0.58, 0.15))
    return base, tip, hinge


def build_chelicerae(b, M):
    """Each a massive pale-to-rust tube with the fixed upper finger hooking down at the tip; the movable lower finger
    on a hinge under it, its teeth up."""
    for s in (1, -1):
        sfx = ".R" if s > 0 else ".L"
        base, tip, hinge = chelicera_rest(s)
        d = (tip - base).normalized()
        L = (tip - base).length
        keys = [(base - d * 0.02, 0.05, 0.065), (base + d * L * 0.25, 0.064, 0.095), (base + d * L * 0.5, 0.06, 0.088),
                (base + d * L * 0.72, 0.045, 0.06), (base + d * L * 0.88, 0.03, 0.036)]
        b.add(tube(keys, sub=3, n=14, ref=UP, bulge=(0.01, 0.01)), M["chelicera"], "chel" + sfx, tag="jaws")
        # Fixed finger: on from the tip, curving down.
        p = base + d * L * 0.86
        fk = [(p, 0.024, 0.028)]
        for k, (dz, step) in enumerate(((-0.05, 0.035), (-0.25, 0.035), (-0.6, 0.03), (-1.2, 0.025))):
            p = p + (d + V((0, 0, dz))).normalized() * step
            r = 0.022 * (1 - (k + 1) / 4.6) + 0.003
            fk.append((p, r, r * 1.1))
        b.add(tube(fk, sub=2, n=8, ref=X * s, caps=(False, True)), M["finger"], "chel" + sfx, tag="jaws")
        for k in range(3):  # teeth on the finger's underside
            q = base + d * L * (0.9 + 0.035 * k) + V((0, 0.02 * k, -0.02 - 0.008 * k))
            b.add(common.cone(q, V((0, 0.2, -1)), 0.022, 0.006, n=4), M["finger"], "chel" + sfx, tag="jaws")
        # Movable finger: from the hinge forward, curving up to meet the fixed one.
        p = hinge.copy()
        mk = [(p - d * 0.02, 0.026, 0.024)]
        for k, (dz, step) in enumerate(((0.0, 0.05), (0.08, 0.05), (0.3, 0.045), (0.7, 0.035), (1.3, 0.025))):
            p = p + (d + V((0, 0, dz))).normalized() * step
            r = 0.024 * (1 - (k + 1) / 5.6) + 0.003
            mk.append((p, r, r * 1.05))
        b.add(tube(mk, sub=2, n=8, ref=X * s, caps=(True, True)), M["finger"], "jaw" + sfx, tag="jaws")
        for k in range(3):
            q = hinge + d * (0.07 + 0.04 * k) + V((0, 0, 0.018 + 0.004 * k))
            b.add(common.cone(q, V((0, 0.2, 1)), 0.02, 0.006, n=4), M["finger"], "jaw" + sfx, tag="jaws")
        for k in range(4):  # bristles on top of the chelicera
            q = base + d * L * (0.15 + 0.15 * k) + V((0.02 * s, 0, 0.055))
            b.add(common.cone(q, V((0.4 * s, 0.6, 1)), 0.05, 0.006, n=3), M["hair"], "chel" + sfx, tag="jaws")


def build_opisthosoma(b, M):
    keys = [((0, 0.09, 0.205), 0.09, 0.075), ((0, 0.02, 0.215), 0.14, 0.11), ((0, -0.12, 0.225), 0.175, 0.14),
            ((0, -0.28, 0.225), 0.18, 0.145), ((0, -0.42, 0.215), 0.145, 0.12), ((0, -0.53, 0.20), 0.08, 0.07), ((0, -0.565, 0.195), 0.03, 0.025)]
    b.add(tube(keys, sub=3, n=26, ref=UP, shape=flat_belly, bulge=(0.0, 0.015)), M["abdomen"], "abd")
    for k in range(14):  # a few long bristles on the back and sides
        y = 0.0 - 0.04 * k
        a = math.pi * (0.2 + 0.6 * ((k * 7) % 10) / 10)
        p = V((0.15 * math.cos(a), y, 0.225 + 0.12 * math.sin(a)))
        out = (p - V((0, y, 0.2))).normalized()
        b.add(common.cone(p, out + V((0, -0.5, 0)), 0.06, 0.006, n=3), M["hair"], "abd")


def rest_leg(leg):
    return solve_leg(leg["hip"], leg["foot"], leg["a"], leg["b"], leg["bend"])


def build_legs(b, M):
    for leg in LEGS:
        n, hip, th = leg["name"], leg["hip"], leg["th"]
        knee, normal, foot = rest_leg(leg)
        fd, td = (knee - hip).normalized(), (foot - knee).normalized()
        r = 0.024 * th
        b.add(tube([(hip - fd * 0.02, r, r * 0.85), (hip + fd * leg["a"] * 0.5, r * 1.1, r * 0.9), (knee, r * 0.85, r * 0.75)], sub=2, n=8,
                   ref=normal, bulge=(0.006, 0.0)), M["leg"], "femur" + n, tag="leg")
        hairs(b, M, "femur" + n, hip, fd, leg["a"], r, 3, normal, "leg")
        b.add(ellipsoid(knee, (r * 1.0, r * 1.0, r * 1.0), n=8, rings=4), M["joint"], "tibia" + n, tag="leg")
        b.add(tube([(knee, r * 0.85, r * 0.75), (knee + td * leg["b"] * 0.5, r * 0.8, r * 0.7), (foot, r * 0.6, r * 0.55)], sub=2, n=8,
                   ref=normal, bulge=(0.0, 0.005)), M["leg_end"], "tibia" + n, tag="leg")
        hairs(b, M, "tibia" + n, knee, td, leg["b"], r, 4, normal, "leg")
        t = leg["tdir"]
        end = foot + t * leg["tlen"]
        b.add(tube([(foot - t * 0.01, r * 0.6, r * 0.55), (foot + t * leg["tlen"] * 0.5, r * 0.45, r * 0.4), (end, 0.004, 0.004)], sub=2, n=6,
                   ref=normal, bulge=(0.0, 0.003)), M["leg_end"], "tarsus" + n, tag="leg")


def palp_rest(s):
    hip = V((PALP_HIP[0] * s, PALP_HIP[1], PALP_HIP[2]))
    wrist = V((0.26 * s, 0.84, 0.30))
    bend = V((0.7 * s, -0.2, 1))
    tdir = V((0.12 * s, 0.55, -0.83)).normalized()
    return hip, wrist, bend, tdir


def build_palps(b, M):
    """Long and thick, darker towards the end, the pale adhesive sucker at the tip."""
    for s in (1, -1):
        sfx = ".R" if s > 0 else ".L"
        hip, wrist, bend, tdir = palp_rest(s)
        elbow, normal, wrist = solve_leg(hip, wrist, PALP_A, PALP_B, bend)
        fd, pd = (elbow - hip).normalized(), (wrist - elbow).normalized()
        b.add(tube([(hip - fd * 0.02, 0.03, 0.028), (hip + fd * PALP_A * 0.5, 0.036, 0.032), (elbow, 0.032, 0.03)], sub=2, n=10, ref=UP,
                   bulge=(0.008, 0.0)), M["palp"], "pfemur" + sfx, tag="palp")
        hairs(b, M, "pfemur" + sfx, hip, fd, PALP_A, 0.03, 4, normal, "palp", 0.06)
        b.add(ellipsoid(elbow, (0.034, 0.034, 0.034), n=10, rings=5), M["joint"], "ptibia" + sfx, tag="palp")
        b.add(tube([(elbow, 0.032, 0.03), (elbow + pd * PALP_B * 0.5, 0.036, 0.033), (wrist, 0.03, 0.028)], sub=2, n=10, ref=UP,
                   bulge=(0.0, 0.006)), M["palp"], "ptibia" + sfx, tag="palp")
        hairs(b, M, "ptibia" + sfx, elbow, pd, PALP_B, 0.032, 5, normal, "palp", 0.06)
        b.add(ellipsoid(wrist, (0.03, 0.03, 0.03), n=8, rings=4), M["joint"], "ptarsus" + sfx, tag="palp")
        end = wrist + tdir * TARSUS_LEN
        b.add(tube([(wrist, 0.028, 0.026), (wrist + tdir * TARSUS_LEN * 0.5, 0.03, 0.028), (end, 0.024, 0.022)], sub=2, n=10, ref=UP,
                   bulge=(0.0, 0.004)), M["palp_end"], "ptarsus" + sfx, tag="palp")
        b.add(ellipsoid(end + tdir * 0.015, (0.03, 0.03, 0.024), n=10, rings=5, axis=tdir, ref=UP), M["sucker"], "ptarsus" + sfx, tag="palp")


# ---------------------------------------------------------------- rig


def build_rig(coll):
    arm = bpy.data.armatures.new("camel_spider_rig")
    rig = bpy.data.objects.new("camel_spider_rig", arm)
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
    bone("abd", ABD, ABD + V((0, -0.3, 0)), X, "body")
    for s in (1, -1):
        sfx = ".R" if s > 0 else ".L"
        base, tip, hinge = chelicera_rest(s)
        bone("chel" + sfx, base, tip, X, "body")
        bone("jaw" + sfx, hinge, hinge + (tip - base).normalized() * 0.2, X, "chel" + sfx)
    for leg in LEGS:
        knee, normal, foot = rest_leg(leg)
        n = leg["name"]
        bone("femur" + n, leg["hip"], knee, normal, "body")
        bone("tibia" + n, knee, foot, normal, "femur" + n)
        bone("tarsus" + n, foot, foot + leg["tdir"] * leg["tlen"], normal, "tibia" + n)
    for s in (1, -1):
        sfx = ".R" if s > 0 else ".L"
        hip, wrist, bend, tdir = palp_rest(s)
        elbow, normal, wrist = solve_leg(hip, wrist, PALP_A, PALP_B, bend)
        bone("pfemur" + sfx, hip, elbow, normal, "body")
        bone("ptibia" + sfx, elbow, wrist, normal, "pfemur" + sfx)
        bone("ptarsus" + sfx, wrist, wrist + tdir * TARSUS_LEN, normal, "ptibia" + sfx)
    bpy.ops.object.mode_set(mode="OBJECT")
    for pb in rig.pose.bones:
        pb.rotation_mode = "QUATERNION"
    return rig


# ---------------------------------------------------------------- animation

REST = dict(x=0.0, y=0.0, z=0.0, pitch=0.0, roll=0.0, yaw=0.0, flex=0.0,
            pup=0.0, pfwd=0.0, pspread=0.0, ptap=0.0, pphase=0.0, plimp=0.0, gape=0.0, saw=0.0, sphase=0.0,
            plant=1.0, curl=0.0, flail=0.0, fphase=0.0)
STRIDE, STEP_H, CYCLES = 0.22, 0.09, 2  # gait cycles per walk clip


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
    p.update(z=0.008 * math.cos(2 * w), roll=1.5 * math.sin(w), yaw=2.0 * math.cos(w), pitch=1.0 * math.sin(2 * w), flex=2.0 * math.sin(w + 0.5),
             ptap=0.07, pphase=CYCLES * t, pfwd=0.03, saw=0.01, sphase=CYCLES * t)
    p["feet"] = {leg["name"]: gait(leg, CYCLES * t) for leg in LEGS}
    return p


READY = walk_params(0.0)
READY_FEET = {k: v[0].copy() for k, v in READY["feet"].items()}


def body_mats(p):
    T = Matrix.Translation(V((p["x"], p["y"], p["z"]))) @ rot_about(PIVOT, euler(p["pitch"], p["roll"], p["yaw"]))
    Ma = T @ rot_about(ABD, euler(p["flex"]))
    return T, Ma


def leg_targets(p, T):
    """Foot target (armature space) and swing amount per leg for non-walk frames."""
    out = {}
    for leg in LEGS:
        n, s = leg["name"], leg["s"]
        planted = READY_FEET[n]
        free = planted.lerp(leg["curled"], p["curl"])
        ph = 2 * math.pi * (p["fphase"] + int(n[0]) * 0.27 + (0.13 if s > 0 else 0.0))
        free += V((0.12 * s * math.sin(ph * 1.7), 0.14 * math.cos(ph * 1.3), 0.14 * math.sin(ph + 1))) * p["flail"]
        w = p["plant"]
        target = planted.lerp(T @ free, 1 - w)
        if abs(p["roll"]) < 45:
            target.z = max(target.z, FOOT_Z)
        out[n] = (target, 1 - w)
    return out


def palp_mats(rest, p, T, s, mats):
    sfx = ".R" if s > 0 else ".L"
    hip0, wrist0, bend0, tdir0 = palp_rest(s)
    elbow0, normal0, wrist0 = solve_leg(hip0, wrist0, PALP_A, PALP_B, bend0)
    R = T.to_3x3()
    # The two palps tap in turn (the right one half a cycle after the left).
    tap = p["ptap"] * math.sin(2 * math.pi * (p["pphase"] + (0.5 if s > 0 else 0.0)))
    limp = V((0.22 * s, 0.62, 0.12))
    w_body = wrist0 + V((p["pspread"] * s, p["pfwd"], p["pup"] + tap))
    w_body = w_body.lerp(limp, p["plimp"])
    target = T @ w_body
    td = R @ (euler(70 * p["pup"] - 40 * p["plimp"] + 150 * tap, 0, 0) @ tdir0)
    hip = T @ hip0
    elbow, normal, wrist = solve_leg(hip, target, PALP_A, PALP_B, R @ bend0)
    mats["pfemur" + sfx] = frame(hip, elbow - hip, normal) @ frame(hip0, elbow0 - hip0, normal0).inverted() @ rest["pfemur" + sfx]
    mats["ptibia" + sfx] = frame(elbow, wrist - elbow, normal) @ frame(elbow0, wrist0 - elbow0, normal0).inverted() @ rest["ptibia" + sfx]
    mats["ptarsus" + sfx] = frame(wrist, td, normal) @ frame(wrist0, tdir0, normal0).inverted() @ rest["ptarsus" + sfx]


def chel_mats(rest, p, T, s, mats):
    """The chelicerae saw in turn (one forward while the other pulls back), the lower finger gapes on its hinge."""
    sfx = ".R" if s > 0 else ".L"
    saw = p["saw"] * math.sin(2 * math.pi * (p["sphase"] + (0.5 if s > 0 else 0.0)))
    C = T @ Matrix.Translation(V((0, saw, 0)))
    mats["chel" + sfx] = C @ rest["chel" + sfx]
    hinge0 = rest["jaw" + sfx].translation
    mats["jaw" + sfx] = C @ rot_about(hinge0, Matrix.Rotation(math.radians(-p["gape"]), 3, X)) @ rest["jaw" + sfx]


def pose_matrices(rest, p):
    """Armature-space pose matrices of every bone."""
    T, Ma = body_mats(p)
    mats = {"root": rest["root"], "body": T @ rest["body"], "abd": Ma @ rest["abd"]}
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
        td = planted.lerp(hang, min(1.0, swing * 1.5)).normalized()
        mats["tarsus" + n] = frame(foot, td, normal) @ frame(foot0, td0, normal0).inverted() @ rest["tarsus" + n]
    for s in (1, -1):
        palp_mats(rest, p, T, s, mats)
        chel_mats(rest, p, T, s, mats)
    return mats


class Skin:
    """numpy linear blend skinning of all vertices, for the floor contact."""

    def __init__(self, b, rest):
        import numpy as np

        self.np = np
        self.bones = sorted({n for w in b.weights for n in w})
        col = {n: j for j, n in enumerate(self.bones)}
        self.P = np.array([[*v, 1.0] for v in b.verts])
        self.W = np.zeros((len(b.verts), len(self.bones)))
        for r, w in enumerate(b.weights):
            tot = sum(w.values())
            for n, val in w.items():
                self.W[r, col[n]] = val / tot
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
    rest = {bn.name: bn.matrix_local.copy() for bn in bones}
    mats = pose_matrices(rest, p)
    S = Matrix.Translation(V((0, 0, -skin.lowest(mats))))
    mats = {n: S @ m for n, m in mats.items()}
    for bn in bones:
        pose = mats[bn.name]
        if bn.parent:
            rel = bn.parent.matrix_local.inverted() @ bn.matrix_local
            basis = rel.inverted() @ mats[bn.parent.name].inverted() @ pose
        else:
            basis = bn.matrix_local.inverted() @ pose
        pb = rig.pose.bones[bn.name]
        pb.matrix_basis = basis
        pb.keyframe_insert("rotation_quaternion", frame=frame_no)
        pb.keyframe_insert("location", frame=frame_no)


def interpolate(keys, frame_no, linear=("fphase", "pphase", "sphase")):
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


# Rears with the palps up and the jaws open, lunges and snaps them shut, saws, draws back.
ATTACK = [
    (0, {}),
    (4, dict(y=-0.05, pitch=12, z=0.03, pup=0.14, pspread=0.08, ptap=0.0, gape=30, saw=0.02, sphase=0.5)),
    (7, dict(y=0.16, pitch=-8, z=0.0, pup=0.02, pspread=0.12, pfwd=0.10, ptap=0.0, gape=38, saw=0.0, sphase=0.8)),
    (9, dict(y=0.18, pitch=-9, pup=0.0, pspread=0.12, pfwd=0.10, ptap=0.0, gape=0, saw=0.04, sphase=1.2)),
    (12, dict(y=0.12, pitch=-5, pup=0.02, pspread=0.08, pfwd=0.06, ptap=0.0, gape=12, saw=0.05, sphase=2.0)),
    (15, dict(y=0.04, pitch=0, pup=0.0, pspread=0.02, ptap=0.03, gape=4, saw=0.02, sphase=2.6)),
    (18, {}),
]

DIE = [
    (0, {}),
    (2, dict(pitch=14, z=0.04, pup=0.15, pspread=0.1, gape=35, plant=0.5, flail=0.3, fphase=0.2, ptap=0.0)),
    (4, dict(pitch=-4, roll=12, gape=10, plant=0.2, flail=0.6, fphase=0.5, pspread=0.12, ptap=0.0)),
    (6, dict(pitch=6, roll=-30, gape=30, plant=0, flail=0.9, fphase=0.9, pup=0.08, ptap=0.0)),
    (9, dict(roll=-90, x=-0.05, flex=6, plant=0, flail=1.0, fphase=1.4, gape=12, plimp=0.3, ptap=0.0)),
    (12, dict(roll=-150, x=-0.10, plant=0, flail=0.9, fphase=2.0, gape=25, plimp=0.5, ptap=0.0)),
    (15, dict(roll=-180, x=-0.12, flex=-4, plant=0, flail=0.6, fphase=2.6, curl=0.3, gape=10, plimp=0.6, ptap=0.0)),
    (19, dict(roll=-180, x=-0.12, flex=3, plant=0, flail=0.35, fphase=3.2, curl=0.65, gape=20, plimp=0.8, ptap=0.0)),
    (23, dict(roll=-180, x=-0.12, plant=0, flail=0.1, fphase=3.6, curl=0.95, gape=15, plimp=1.0, ptap=0.0)),
    (25, dict(roll=-180, x=-0.12, plant=0, flail=0.0, fphase=3.8, curl=1.0, gape=15, plimp=1.0, ptap=0.0)),
]


def idle_params(t):
    """Waits on its eight legs: breathing, the palps tapping slowly in turn, the jaws sawing now and then."""
    p = dict(REST)
    w = 2 * math.pi * t
    p.update(z=0.006 * math.sin(w), pitch=1.5 * math.sin(w), flex=1.5 * math.sin(w + 1),
             ptap=0.06, pphase=t, pup=0.03 + 0.02 * math.sin(2 * w), pfwd=0.02,
             saw=0.025 * max(0.0, math.sin(w)), sphase=3 * t, gape=6 * max(0.0, math.sin(w)))
    return p


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
        last = frames if name != "camel_spider_die" else frames - 1  # loops key frame N = frame 0
        for f in range(last + 1):
            if name == "camel_spider_walk":
                p = walk_params(f / frames)
            elif name == "camel_spider_idle":
                p = idle_params(f / frames)
            else:
                p = interpolate({"camel_spider_attack": ATTACK, "camel_spider_die": DIE}[name], f)
            key_frame(rig, skin, p, f)
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
    build_prosoma(b, M)
    build_chelicerae(b, M)
    build_opisthosoma(b, M)
    build_legs(b, M)
    build_palps(b, M)
    obj = common.finish_mesh(b, coll, "camel_spider_new")
    common.uv_unwrap(obj, b.tags, {"jaws": (V((0, 0.55, 0.2)), 1.4), "palp": (V((0, 0.6, 0.25)), 1.2)})
    if bake:
        tmp = bpy.app.tempdir or "/tmp"
        tex = common.bake_texture(obj, tex_path or os.path.join(tmp, "camel_spider_preview.png"), TEX_SIZE, "camel_spider", ao_distance=0.1)
        common.use_baked_material(obj, tex)
    common.rig_object(obj, rig)
    rest = {bn.name: bn.matrix_local.copy() for bn in rig.data.bones}
    acts = make_actions(rig, Skin(b, rest))
    rig.animation_data.action = acts["camel_spider_walk"]
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 0, CLIPS[0][2] - 1
    scene.frame_set(0)
    print("camel_spider: {} verts, {} tris".format(len(obj.data.vertices), common.tri_count(obj)))
    return obj, rig


def export(models_dir=None):
    common.export_files(bpy.data.objects["camel_spider_new"], bpy.data.objects["camel_spider_rig"], "camel_spider", CLIPS, "monsters", models_dir)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    if "--export" in args:
        build(tex_path=os.path.join(REPO, "media", "textures", "monsters", "camel_spider.png"))
        export()
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "camel_spider.blend"))
    else:
        build()
