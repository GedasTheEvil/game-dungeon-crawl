"""Procedural living ushabti (plan living-ushabti): the overseer among the faience servant figures of the tomb.

    CLI:  blender -b --python tools/blender/models/ushabti.py -- [--export] [--review OUT_DIR]

Blender space: Z up, 1 unit = 1 tile, origin on the floor under its centre; it faces -Y (the camera; game rotA 0).

The decor `ushabti` (decor.py: five small figures, two standing, two fallen, one broken) is its disguise: the same
mummiform figure in the same blue-green faience, black wig and ink text bands, only taller (the overseer of the
workers), with its hoe and its seed bag. It stands on its spawn cell among them (scatterProps puts the decor there).

No rig: rigid parts posed from a handful of numbers (rock, yaw, lift, knee, waist, head, arms), every frame rebuilt with
the same topology and written straight to MD3 (md3.py), as lotus.py does. Parts: the base (feet to knees), the hips,
the torso with the bag on its back, the head (wig, face, eyes, beard), two folded arms pivoting at the shoulders, the
hoe in the right fist, and eight shards hidden (collapsed) inside the torso until the die clip throws them out.

Clips: move 20 (the reference, loops: a waddle, the legs being one piece, rocking from one base edge onto the other,
the hips twisting forward), attack 16 (loops from move frame 0: the hoe raised over the head, chopped down forward),
die 30 (once: it cracks, the shards burst out, the head drops and rolls, the arms, the hoe and the torso fall, the
hips topple, the base stands), idle 120 (`_idle`: still as the decor, arms crossed on the chest, the hoe over the
shoulder; once a loop its head turns a little to the player and back, the only tell), rise 14 (`_rise`, once: idle
frame 0 to move frame 0, a shiver, the arms come free, it steps off its place).
"""

import importlib
import math
import os
import sys

import bpy
from mathutils import Euler, Matrix
from mathutils import Vector as V

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, ".."))
import common  # noqa: E402
import md3  # noqa: E402

importlib.reload(common)
from common import REPO, Builder, ellipsoid, smoothstep, tube  # noqa: E402

COLL = "ushabti_new"
CLIPS = [("move", "", 20), ("attack", "_att", 16), ("die", "_die", 30), ("idle", "_idle", 120), ("rise", "_rise", 14)]
TEX_SIZE = 512

# decor.py's colours, so the overseer matches the figures it stands among.
COL = {
    "faience": (0.05, 0.40, 0.38),
    "faience_pale": (0.12, 0.52, 0.48),
    "black": (0.015, 0.01, 0.008),
    "ink": (0.05, 0.04, 0.03),
}


def stripes(axis, period, bands, interp="CONSTANT"):
    return ("stripes", axis, period, bands, interp)


def materials():
    specs = {
        "text": stripes("v", 0.04, [(5, "faience"), (1, "ink")]),  # decor.py's bands, at the overseer's size
        "faience": stripes("v", 0.08, [(1, "faience"), (1, "faience_pale")], "LINEAR"),
        "wig": stripes("u", 1 / 24, [(2, "black"), (1, "ink")]),
        "black": ("solid", "black"),
    }
    return {name: common.make_material("ushabti_" + name, spec, COL) for name, spec in specs.items()}


# ---------------------------------------------------------------- geometry

K = 6.4  # decor.py's figure (0.066 tall before its PROP_SCALE) to the overseer's ~0.5 tile
# The mummiform body (decor.py's ushabti() keys times K): z, half width, half depth.
BODY = [(0.0, 0.070, 0.051), (0.077, 0.077, 0.058), (0.192, 0.083, 0.064), (0.294, 0.096, 0.070), (0.346, 0.077, 0.058),
        (0.371, 0.051, 0.045)]
KNEE = V((0.0, 0.0, 0.15))
WAIST = V((0.0, 0.0, 0.25))
NECK = V((0.0, 0.0, 0.37))
SHOULDER = 0.085  # x of the shoulder pivots
SHOULDER_Z = 0.325
BASE_RX = 0.070  # the base's edge it rocks on


def body_at(z):
    """Half width and depth of the body at height z (linear between the keys)."""
    for (z0, x0, y0), (z1, x1, y1) in zip(BODY, BODY[1:]):
        if z <= z1:
            t = (z - z0) / (z1 - z0)
            return x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
    return BODY[-1][1], BODY[-1][2]


def body_piece(z0, z1, steps):
    keys = []
    for i in range(steps + 1):
        z = z0 + (z1 - z0) * i / steps
        rx, ry = body_at(z)
        keys.append((V((0, 0, z)), rx, ry))
    return tube(keys, sub=1, n=14, ref=V((0, 1, 0)))


def rod(a, b, r0, r1=None, n=8):
    a, b = V(a), V(b)
    r1 = r0 if r1 is None else r1
    d = (b - a).normalized()
    ref = V((0, 0, 1)) if abs(d.z) < 0.9 else V((1, 0, 0))
    return tube([(a, r0, r0), (b, r1, r1)], sub=1, n=n, ref=ref)


def box(c, half):
    """An axis-aligned box as a 4-ring tube along Z (keeps the pattern uvs)."""
    c, (hx, hy, hz) = V(c), half
    sq = lambda th: 1.0 / max(abs(math.cos(th)), abs(math.sin(th)))  # noqa: E731
    return tube([(c + V((0, 0, -hz)), hx, hy), (c + V((0, 0, hz)), hx, hy)], sub=1, n=8, shape=lambda th, a: sq(th))


def arm(s):
    """A folded arm at the shoulder (s = +1 the figure's left, +x): the upper arm down, the forearm forward, a fist."""
    sh = V((s * SHOULDER, -0.01, SHOULDER_Z))
    elbow = sh + V((s * 0.012, 0.0, -0.085))
    hand = elbow + V((0, -0.085, 0.0))
    return [(tube([(sh, 0.024, 0.024), (elbow, 0.020, 0.020), (hand, 0.017, 0.017)], sub=2, n=8), "faience"),
            (ellipsoid(hand + V((0, -0.008, 0)), (0.022, 0.022, 0.024), n=8, rings=4), "faience")]


def hand_of(s):
    return V((s * (SHOULDER + 0.012), -0.103, SHOULDER_Z - 0.085))


def hoe():
    """In the right fist (-x): the handle up through the fist, the blade at the top bent forward (painted black)."""
    h = hand_of(-1)
    top = h + V((0, 0.0, 0.17))
    blade_end = top + V((0, -0.07, -0.05))
    return [(rod(h + V((0, 0, -0.04)), top, 0.008, 0.007, n=6), "black"),
            (tube([(top, 0.018, 0.006), (top + (blade_end - top) * 0.5, 0.022, 0.006), (blade_end, 0.012, 0.004)], sub=1, n=6,
                  ref=V((0, 1, 0))), "black")]


def head():
    """The tripartite wig (a cap, two lappets down in front of the shoulders), the face, ink eyes, the beard."""
    lappets = [(tube([(V((s * 0.05, -0.035, 0.44)), 0.022, 0.012), (V((s * 0.058, -0.05, 0.38)), 0.026, 0.012), (V((s * 0.06, -0.055, 0.335)), 0.024, 0.01)],
                     sub=2, n=8), "wig") for s in (-1, 1)]
    return lappets + [(ellipsoid((0, 0.012, 0.43), (0.072, 0.07, 0.085), n=14, rings=7), "wig"),
            (ellipsoid((0, -0.036, 0.415), (0.046, 0.038, 0.056), n=12, rings=6), "faience"),
            (ellipsoid((-0.018, -0.07, 0.43), (0.009, 0.005, 0.006), n=6, rings=3), "black"),
            (ellipsoid((0.018, -0.07, 0.43), (0.009, 0.005, 0.006), n=6, rings=3), "black"),
            (rod((0, -0.05, 0.37), (0, -0.056, 0.335), 0.011, 0.008, n=6), "black")]


SHARDS = [((0.04, -0.03, 0.30), (-0.25, -0.9), 0.016), ((-0.05, -0.02, 0.28), (0.3, -0.8), 0.014), ((0.03, 0.03, 0.22), (-0.2, 0.7), 0.015),
          ((-0.03, 0.02, 0.18), (0.45, 0.4), 0.013), ((0.0, -0.04, 0.24), (0.0, -1.0), 0.018), ((0.05, 0.0, 0.33), (-0.7, -0.2), 0.012),
          ((-0.04, 0.0, 0.34), (0.75, -0.3), 0.012), ((0.0, 0.03, 0.31), (0.1, 0.9), 0.014)]
SHARD_CENTRE = V((0.0, 0.0, 0.27))


def shard(r):
    return ellipsoid((0, 0, 0), (r, r * 0.7, r * 0.5), n=4, rings=2)


def pieces():
    """{piece: [(geo, mat)]} in the rest pose (standing, arms folded forward)."""
    return {
        "base": [(body_piece(0.0, KNEE.z + 0.008, 3), "text")],
        "hips": [(body_piece(KNEE.z, WAIST.z + 0.008, 2), "text")],
        "torso": [(body_piece(WAIST.z, NECK.z + 0.01, 3), "faience"), (box((0, 0.075, 0.28), (0.045, 0.014, 0.04)), "black")],
        "head": head(),
        "armL": arm(1),
        "armR": arm(-1),
        "hoe": hoe(),
    }


# ---------------------------------------------------------------- poses


def rot(x=0.0, y=0.0, z=0.0):
    return Euler((math.radians(x), math.radians(y), math.radians(z)), "XYZ").to_matrix().to_4x4()


def about(m, pivot):
    return Matrix.Translation(V(pivot)) @ m @ Matrix.Translation(-V(pivot))


# Numbers: rock (deg about Y on the base edge, + tips to +x), yaw, lean (+ forward), lift; knee, waist (pitch, roll),
# head (pitch + down, yaw, roll), arms (x, y, z eulers at the shoulder; - x raises forward), the hoe's tilt in the fist
# (- to the right shoulder), drift (dx, dy).
STAND = {"rock": 0.0, "yaw": 0.0, "lean": 0.0, "lift": 0.0, "knee": 0.0, "waistP": 0.0, "waistR": 0.0, "headP": 0.0, "headY": 0.0, "headR": 0.0,
         "aLx": -10.0, "aLy": 0.0, "aLz": 0.0, "aRx": -10.0, "aRy": 0.0, "aRz": 0.0, "hoe": 0.0, "dx": 0.0, "dy": 0.0}
# The disguise: the arms crossed on the chest (the forearms turned in), the hoe over the right shoulder, as in decor.py.
FOLDED = dict(STAND, aLx=-28.0, aLz=-62.0, aRx=-28.0, aRz=62.0, hoe=-40.0)


def matrices(p):
    sign = 1.0 if p["rock"] >= 0 else -1.0
    root = (Matrix.Translation(V((p["dx"], p["dy"], p["lift"]))) @ rot(z=p["yaw"]) @ about(rot(y=-p["rock"]), (sign * BASE_RX, 0, 0)) @
            about(rot(x=p["lean"]), (0, -BODY[0][2], 0)))
    hips = root @ about(rot(x=p["knee"]), KNEE)
    torso = hips @ about(rot(x=p["waistP"], y=p["waistR"]), WAIST)
    m = {"base": root, "hips": hips, "torso": torso, "head": torso @ about(rot(x=p["headP"], y=p["headR"], z=p["headY"]), NECK),
         "armL": torso @ about(rot(p["aLx"], p["aLy"], p["aLz"]), (SHOULDER, -0.01, SHOULDER_Z)),
         "armR": torso @ about(rot(p["aRx"], p["aRy"], p["aRz"]), (-SHOULDER, -0.01, SHOULDER_Z))}
    # The hoe tilts in the fist across the body (the torso's frame), wherever the arm has put the fist.
    turn = torso.to_quaternion().to_matrix().to_4x4()
    m["hoe"] = about(turn @ rot(y=p["hoe"]) @ turn.inverted(), m["armR"] @ hand_of(-1)) @ m["armR"]
    return m


def mix(a, b, t):
    return {k: a[k] + (b[k] - a[k]) * t for k in a}


def walk(t):
    w = 2 * math.pi * t
    return dict(STAND, rock=6.5 * math.sin(w), yaw=10 * math.sin(w), lift=0.004 * abs(math.sin(w)), waistR=-3 * math.sin(w), headR=-4 * math.sin(w),
                lean=3.0, aLx=-10 + 8 * math.sin(w), aRx=-10 - 8 * math.sin(w))


def pose(clip, f, frames):
    if clip == "move":
        return walk(f / frames)
    if clip == "attack":
        t = f / frames  # loops from move frame 0
        p = walk(0.0)
        up = smoothstep(0.0, 0.4, t) * (1 - smoothstep(0.45, 0.58, t))  # the hoe raised over the head
        chop = smoothstep(0.45, 0.58, t) * (1 - smoothstep(0.7, 1.0, t))  # chopped down in front
        p["aRx"] += -150 * up + -55 * chop
        p["aRz"] += 10 * up
        p["aLx"] += -30 * up - 20 * chop
        p["waistP"] = -10 * up + 18 * chop
        p["lean"] += -4 * up + 6 * chop
        p["headP"] = -10 * up + 12 * chop
        return p
    if clip == "idle":
        t = f / frames
        p = dict(FOLDED)
        k = (t - 0.55) / 0.25  # the tell: the head turns a little towards the camera side and back
        if 0 <= k <= 1:
            p["headY"] = 14 * math.sin(math.pi * k) ** 2
        return p
    if clip == "rise":
        t = f / (frames - 1)
        p = mix(FOLDED, walk(0.0), smoothstep(0.35, 0.9, t))
        shiver = math.sin(t * 40) * (1 - smoothstep(0.0, 0.45, t)) * smoothstep(0.0, 0.08, t)
        p["rock"] += 2.5 * shiver
        p["headP"] += -12 * math.sin(math.pi * smoothstep(0.2, 0.8, t))  # looks up
        p["lift"] += 0.03 * math.sin(math.pi * smoothstep(0.55, 0.95, t))  # hops off its place
        return p
    if clip == "die":
        return walk(0.0)  # the pieces' fall comes on top (die_matrices)
    raise ValueError(clip)


# The fall of each piece in the die clip: start, end (of the clip, 0..1), floor drift (x, y), turn axis and angle, rest on
# the floor. The base stays.
FALLS = {
    "head": (0.12, 0.55, (0.16, -0.08), (1, 0.3, 0), 100),
    "armL": (0.18, 0.5, (0.12, -0.04), (0, 1, 0.2), 75),
    "armR": (0.1, 0.45, (-0.15, -0.05), (0, -1, 0.3), 80),
    "hoe": (0.12, 0.5, (-0.24, -0.06), (0.2, -1, 0), 85),
    "torso": (0.25, 0.75, (0.0, 0.17), (-1, 0, 0.1), 88),
    "hips": (0.45, 0.9, (-0.05, -0.12), (1, 0.15, 0), 85),
}


def frame_geo(clip, f, frames, M, P):
    p = pose(clip, f, frames)
    mats = matrices(p)
    b = Builder()
    die = clip == "die"
    t = f / (frames - 1) if die else 0.0
    for name, parts in P.items():
        m = mats[name]
        geos = [([m @ V(v) for v in geo[0]], geo[1], geo[2]) for geo, _ in parts]
        if die and name in FALLS:
            start, end, drift, axis, angle = FALLS[name]
            u = smoothstep(start, end, t)
            pts = [v for g in geos for v in g[0]]
            c = sum(pts, V()) / len(pts)
            r = Matrix.Rotation(math.radians(angle) * u, 4, V(axis).normalized())
            moved = [[r @ (v - c) + c for v in g[0]] for g in geos]
            # Its rest: the lowest point on the floor; down like a falling body (u squared), never below the floor.
            final_rot = Matrix.Rotation(math.radians(angle), 4, V(axis).normalized())
            rest_low = min((final_rot @ (v - c) + c).z for v in pts)
            drop = -rest_low * (u * u)
            off = V((drift[0] * u, drift[1] * u, drop))
            moved = [[v + off for v in g] for g in moved]
            under = min(v.z for g in moved for v in g)
            if under < 0:
                moved = [[v + V((0, 0, -under)) for v in g] for g in moved]
            bounce = 0.02 * math.sin(math.pi * smoothstep(end, min(1.0, end + 0.12), t)) if t > end else 0.0
            geos = [([v + V((0, 0, bounce)) for v in mv], g[1], g[2]) for mv, g in zip(moved, geos)]
        for (geo, matname), g in zip(parts, geos):
            b.add(g, M[matname], "root", name)
    # The shards: collapsed inside the torso, thrown out in the die clip, ballistic down to the floor.
    torso = mats["torso"]
    burst = smoothstep(0.08, 0.16, t) if die else 0.0
    for i, (at, (vx, vy), r) in enumerate(SHARDS):
        geo = shard(r)
        u = smoothstep(0.08, 0.6, t) if die else 0.0
        start = torso @ V(at)
        land = V((start.x + vx * 0.25, start.y + vy * 0.25, r * 0.5))
        pos = start.lerp(land, u) + V((0, 0, 0.12 * math.sin(math.pi * u) * (1 - u)))
        spin = Matrix.Rotation(u * 6.0 + i, 4, V((1, 0.5, 0.2 * i)).normalized())
        size = max(burst, 0.002)
        verts = [pos + (spin @ (V(v) * size)) for v in geo[0]]
        if not die:
            verts = [torso @ SHARD_CENTRE + V(v) * 0.002 for v in geo[0]]
        b.add((verts, geo[1], geo[2]), M["faience"], "root", "shard")
    # Rocking on a base edge tips the wider body above it under the floor: lifted back onto it.
    under = min(v.z for v in b.verts)
    if not die and under < 0:
        b.verts = [v + V((0, 0, -under)) for v in b.verts]
    return b


# ---------------------------------------------------------------- build and export


def frame_coords(clip, f, frames, M, P):
    return frame_geo(clip, f, frames, M, P).verts


def build(bake=True, tex_path=None):
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    M = materials()
    P = pieces()
    b = frame_geo("move", 0, CLIPS[0][2], M, P)
    obj = common.finish_mesh(b, coll, "ushabti_new")
    common.uv_unwrap(obj, b.tags, {"head": (V((0, 0, 0.42)), 1.6)})
    if bake:
        tex = common.bake_texture(obj, tex_path or os.path.join(bpy.app.tempdir or "/tmp", "ushabti_preview.png"), TEX_SIZE, "ushabti",
                                  ao_distance=0.05)
        common.use_baked_material(obj, tex)
    report(M, P)
    print("ushabti: {} verts, {} tris".format(len(obj.data.vertices), common.tri_count(obj)))
    return obj, M, P


def set_coords(obj, coords):
    obj.data.vertices.foreach_set("co", [c for v in coords for c in v])
    obj.data.update()


def report(M, P):
    """Per clip: lowest and highest z, frame 0 size; the move clip's frame 0 extent sets the KINDS scale."""
    for clip, _, frames in CLIPS:
        lo, hi = 1e9, -1e9
        for f in range(frames):
            zs = [v.z for v in frame_coords(clip, f, frames, M, P)]
            lo, hi = min(lo, min(zs)), max(hi, max(zs))
        v0 = frame_coords(clip, 0, frames, M, P)
        xs, ys, zs = [v.x for v in v0], [v.y for v in v0], [v.z for v in v0]
        size = (max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs))
        print("USHABTI {}: z {:.3f}..{:.3f}; frame 0 {:.3f} wide x {:.3f} deep x {:.3f} high".format(clip, lo, hi, *size))
        if clip == "move":
            print("USHABTI scale (KINDS): {:.1f} world units".format(max(size) * 40))


def export(obj, M, P, models_dir=None):
    models_dir = models_dir or os.path.join(REPO, "media", "models", "monsters")
    mesh = obj.data
    rest = [v.co.copy() for v in mesh.vertices]
    mesh.calc_loop_triangles()
    loops = [l for tri in mesh.loop_triangles for l in tri.loops]
    uv_layer = mesh.uv_layers["UVMap"]
    uvs = [tuple(uv_layer.data[l].uv) for l in loops]
    for clip, suffix, frames in CLIPS:
        pos_frames, nrm_frames = [], []
        for f in range(frames):
            set_coords(obj, frame_coords(clip, f, frames, M, P))
            pos_frames.append([(mesh.vertices[mesh.loops[l].vertex_index].co.x, mesh.vertices[mesh.loops[l].vertex_index].co.z,
                                -mesh.vertices[mesh.loops[l].vertex_index].co.y) for l in loops])
            nrm_frames.append([(n.x, n.z, -n.y) for n in (mesh.corner_normals[l].vector for l in loops)])
        path = os.path.join(models_dir, "ushabti" + suffix + ".md3")
        stats = md3.write_md3(path, pos_frames, nrm_frames, uvs, name="ushabti" + suffix)
        print("Exported {}: {verts} verts, {tris} tris, {frames} frames, {bytes} bytes".format(path, **stats))
    set_coords(obj, rest)


def review(obj, M, P, out_dir, shots):
    """Renders (workbench) of clip frames: shots = [(clip, frame, yaw, elevation)]."""
    os.makedirs(out_dir, exist_ok=True)
    scene = bpy.context.scene
    for o in list(bpy.data.objects):  # the startup scene's cube
        if o.type == "MESH" and o != obj:
            bpy.data.objects.remove(o)
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "TEXTURE" if obj.data.materials and "baked" in obj.data.materials[0].name else "MATERIAL"
    scene.render.resolution_x, scene.render.resolution_y = 360, 360
    cam = bpy.data.objects.get("ushabti_cam") or bpy.data.objects.new("ushabti_cam", bpy.data.cameras.new("ushabti_cam"))
    if cam.name not in scene.collection.objects:
        scene.collection.objects.link(cam)
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = 0.75
    scene.camera = cam
    rest = [v.co.copy() for v in obj.data.vertices]
    names = {c: n for c, _, n in CLIPS}
    for clip, f, yaw, elev in shots:
        set_coords(obj, frame_coords(clip, f, names[clip], M, P))
        target = V((0, 0, 0.25))
        d = V((math.sin(math.radians(yaw)) * math.cos(math.radians(elev)), -math.cos(math.radians(yaw)) * math.cos(math.radians(elev)),
               math.sin(math.radians(elev))))
        cam.location = target + d * 3
        cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
        scene.render.filepath = os.path.join(out_dir, "{}_{:03d}_{}_{}.png".format(clip, f, yaw, elev))
        bpy.ops.render.render(write_still=True)
    set_coords(obj, rest)


DEFAULT_SHOTS = [("idle", 0, 0, 10), ("idle", 0, 90, 10), ("idle", 0, 35, 30), ("idle", 80, 0, 10), ("move", 0, 0, 10), ("move", 0, 90, 10),
                 ("move", 5, 0, 10), ("move", 15, 0, 10), ("attack", 6, 90, 10), ("attack", 9, 90, 10), ("attack", 6, 0, 10), ("rise", 5, 0, 10),
                 ("rise", 10, 90, 10), ("die", 8, 30, 20), ("die", 16, 30, 20), ("die", 29, 30, 20), ("die", 29, 0, 60)]

if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    if "--export" in args:
        obj, M, P = build(tex_path=os.path.join(REPO, "media", "textures", "monsters", "ushabti.png"))
        export(obj, M, P)
    else:
        obj, M, P = build(bake="--bake" in args)
    if "--review" in args:
        review(obj, M, P, args[args.index("--review") + 1], DEFAULT_SHOTS)
