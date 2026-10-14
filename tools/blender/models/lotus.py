"""Procedural blue lotus (Nymphaea caerulea), the man-eating flower of the flooded halls (plan water-lotus).

    CLI:  blender -b --python tools/blender/models/lotus.py -- [--export] [--review OUT_DIR]

Blender space: Z up, 1 unit = 1 tile, origin at the water surface over the centre of its cell; the flower faces -Y
(the camera; game rotA 0). The engine floats the origin at the surface (Monster::floatLift), so whatever goes below
z = 0 is under the water.

No rig: every frame is rebuilt from a handful of pose numbers (head position and pitch, jaw opening, fangs, stem
base, pad bob, wilt, sink) with the same topology, and written straight to MD3 (md3.py). The rest mesh (move frame 0)
gets the UVs and the baked texture.

Parts: three floating leaves (the big one under the flower), a stem rising from the big leaf in a swan neck, and the
head: green sepals, a collar of blue petals (the open flower), a dark red maw ringed with gold stamens, and two jaws of
upright petals (upper and lower) hinged across the mouth, with ivory fangs at their bases.

Clips: move 24 (the reference, loops: reared up, the head 0.27 over the water facing forward, swaying, the jaws
breathing), attack 14 (loops from move frame 0: draws back with the jaws wide, lunges forward and down, snaps, recoils),
die 28 (once: shudders, the jaws gape and wilt, the head flops onto the leaf and everything sinks), idle 112 (`_idle`:
the disguise, like the decor lotus `water_lotus` in decor.py: the flower open on the water, the stem straight down, fangs
drawn in among the stamens, a short stem; the leaves bob; once a loop the jaws twitch shut and open again, the only tell),
rise 10 (`_rise`, once: idle frame 0 to move frame 0, the head rears up out of the water).
"""

import importlib
import math
import os
import random
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
from common import REPO, Builder, cone, ellipsoid, smoothstep, tube  # noqa: E402

COLL = "lotus_new"
CLIPS = [("move", "", 24), ("attack", "_att", 14), ("die", "_die", 28), ("idle", "_idle", 112), ("rise", "_rise", 10)]
TEX_SIZE = 512

COL = {
    "pad": (0.10, 0.30, 0.08),
    "pad_vein": (0.20, 0.42, 0.12),
    "stem": (0.22, 0.36, 0.09),
    "sepal": (0.22, 0.36, 0.09),
    "lotus_blue": (0.30, 0.42, 0.85),
    "lotus_pale": (0.72, 0.78, 0.95),
    "lotus_gold": (0.90, 0.68, 0.10),
    "maw": (0.30, 0.02, 0.06),
    "maw_deep": (0.10, 0.0, 0.02),
    "fang": (0.88, 0.84, 0.70),
}


def stripes(axis, period, bands, interp="CONSTANT"):
    return ("stripes", axis, period, bands, interp)


def materials():
    specs = {
        "pad": stripes("u", 1 / 14, [(4, "pad"), (1, "pad_vein")], "LINEAR"),
        "stem": stripes("v", 0.05, [(3, "stem"), (1, "pad")], "LINEAR"),
        "sepal": ("solid", "sepal"),
        "lotus": stripes("v", 0.025, [(1, "lotus_pale"), (2, "lotus_blue")], "LINEAR"),
        "stamen": ("solid", "lotus_gold"),
        "maw": stripes("v", 0.5, [(1, "maw_deep"), (1, "maw")], "LINEAR"),
        "fang": ("solid", "fang"),
    }
    return {name: common.make_material("lotus_" + name, spec, COL) for name, spec in specs.items()}


# ---------------------------------------------------------------- geometry


def surface(fn, nu, nv):
    """Grid surface; fn(u, v) -> point."""
    verts = [V(fn(i / nu, j / nv)) for j in range(nv + 1) for i in range(nu + 1)]
    faces, fuv = [], []
    for j in range(nv):
        for i in range(nu):
            a = j * (nu + 1) + i
            faces.append([a, a + 1, a + nu + 2, a + nu + 1])
            fuv.append([(i / nu, j / nv), ((i + 1) / nu, j / nv), ((i + 1) / nu, (j + 1) / nv), (i / nu, (j + 1) / nv)])
    return verts, faces, fuv


def moved(geo, m):
    verts, faces, fuv = geo
    return [m @ V(v) for v in verts], faces, fuv


def pad(c, r, yaw, seed, notch=0.3, cup=0.012):
    """A floating lotus leaf: a disc with its V slit, the rim turned up a little (decor.py's)."""
    rnd = random.Random(seed)
    wob = [rnd.uniform(0, 6.28) for _ in range(2)]
    c = V(c)

    def fn(u, v):
        th = notch / 2 + (2 * math.pi - notch) * u
        rr = r * v * (1 + 0.04 * math.sin(5 * th + wob[0]))
        a = th + math.radians(yaw)
        return c + V((rr * math.cos(a), rr * math.sin(a) * 0.9, cup * v ** 3 + 0.003 * math.sin(3 * th + wob[1]) * v))

    return surface(fn, 18, 3)


def petal(base, direction, length, width, lift, cup=0.35, curl=0.0):
    """A pointed petal from base along direction (angle in the XY plane, degrees), rising by lift (degrees) towards +Z,
    cupped; curl bends the tip further up (degrees)."""
    yaw, up = math.radians(direction), math.radians(lift)
    side = V((-math.sin(yaw), math.cos(yaw), 0.0))
    base = V(base)

    def fn(u, v):
        a = up + math.radians(curl) * v * v
        along = V((math.cos(yaw) * math.cos(a), math.sin(yaw) * math.cos(a), math.sin(a)))
        normal = side.cross(along).normalized()
        w = width * math.sin(math.pi * min(1.0, v * 0.85 + 0.15)) * (1 - v) ** 0.35
        s = (u - 0.5) * 2
        # Integrate the bend: the curled part follows its own angle (enough for a short petal).
        a0 = up
        mid = V((math.cos(yaw) * math.cos((a0 + a) / 2), math.sin(yaw) * math.cos((a0 + a) / 2), math.sin((a0 + a) / 2)))
        return base + mid * (length * v) + side * (w * s) + normal * (cup * w * (1 - s * s))

    return surface(fn, 4, 6)


def rot_x(deg):
    return Matrix.Rotation(math.radians(deg), 4, "X")


def about(m, pivot):
    """m applied round pivot."""
    return Matrix.Translation(pivot) @ m @ Matrix.Translation(-V(pivot))


# Head, in its own frame: origin at the bottom of the calyx, the mouth along +Z. Sizes in tiles.
RING_Z = 0.03  # the petals' bases
MAW_R = 0.05
UPPER = (58, 82, 98, 122)  # jaw petals (directions, degrees); the upper jaw is +Y, the lower -Y
LOWER = tuple(a + 180 for a in UPPER)
FANGS_UP = (64, 84, 96, 116)
FANGS_LO = tuple(a + 180 for a in FANGS_UP)


def head_parts(p, M):
    """[(geo, material)] of the head in its frame, posed by p (jaw, fang, wilt, swell)."""
    parts = []
    s = p["swell"]
    ring = V((0, 0, RING_Z))
    parts.append((ellipsoid((0, 0, 0.004), (0.04 * s, 0.04 * s, 0.022), n=10, rings=5), M["sepal"]))  # under the maw
    for k in range(5):  # green sepals behind the petals
        parts.append((petal((0, 0, 0.018), 10 + 72 * k, 0.085 * s, 0.022, 4 - p["wilt"] * 0.5, 0.2), M["sepal"]))
    for k in range(8):  # the collar: the open flower
        parts.append((petal((0, 0, 0.026), 22.5 + 45 * k, 0.12 * s, 0.028, 18 - p["wilt"], 0.35, 10), M["lotus"]))
    for k in range(2):  # side petals of the inner ring, between the jaws, half up
        parts.append((petal(ring, 180 * k, 0.085 * s, 0.022, 48 - p["wilt"], 0.35), M["lotus"]))

    # The maw: a funnel into the throat, then the gold stamens round its rim.
    def maw(u, v):
        a = 2 * math.pi * u
        r = MAW_R * (0.15 + 0.85 * v)
        return V((r * math.cos(a), r * math.sin(a), RING_Z + 0.004 - 0.03 * (1 - v) ** 2))

    parts.append((surface(maw, 16, 3), M["maw"]))
    for k in range(14):
        a = 2 * math.pi * (k + 0.5) / 14
        foot = V((math.cos(a) * MAW_R * 0.95, math.sin(a) * MAW_R * 0.95, RING_Z))
        tip = foot + V((math.cos(a) * 0.006, math.sin(a) * 0.006, 0.02))
        tangent = V((-math.sin(a), math.cos(a), 0))
        parts.append((tube([(foot, 0.003, 0.003), (tip, 0.0035, 0.0035)], sub=1, n=4, ref=tangent, caps=(False, True)), M["stamen"]))

    # The jaws: upright petals hinged across the mouth (local X through the ring), fangs at their bases.
    for jaw, dirs, fangs, sign in ((p["jaw"], UPPER, FANGS_UP, -1), (p["jaw"], LOWER, FANGS_LO, 1)):
        m = about(rot_x(sign * jaw), ring)
        for d in dirs:
            parts.append((moved(petal(ring + V((math.cos(math.radians(d)), math.sin(math.radians(d)), 0)) * 0.03, d, 0.11 * s, 0.03, 64, 0.4, -14), m),
                          M["lotus"]))
        for d in fangs:
            a = math.radians(d)
            out = V((math.cos(a), math.sin(a), 0))
            foot = ring + out * (MAW_R * 1.05) + V((0, 0, 0.004))
            direction = (-out * 0.55 + V((0, 0, 0.85))).normalized()
            length = 0.05 * max(p["fang"], 0.05)
            parts.append((moved(cone(foot, direction, length, 0.009 * max(p["fang"], 0.3), n=5), m), M["fang"]))
    return parts


def head_axis(p):
    """The head's frame in the model: position, then pitch (0 the mouth up, 90 facing -Y) and a yaw sway."""
    r = Euler((math.radians(p["pitch"]), math.radians(p["roll"]), math.radians(p["yaw"])), "XYZ").to_matrix().to_4x4()
    return Matrix.Translation(V((p["hx"], p["hy"], p["hz"]))) @ r


def bezier(p0, p1, p2, p3, t):
    u = 1 - t
    return p0 * u ** 3 + p1 * 3 * u * u * t + p2 * 3 * u * t * t + p3 * t ** 3


STEM_BASE = V((0.0, 0.02, 0.0))
PADS = (((0.0, 0.02, 0.0), 0.13, 75, 11), ((-0.15, 0.09, 0.0), 0.065, 200, 12), ((0.15, 0.1, 0.0), 0.055, 320, 13))


def build_geo(p, M):
    """The whole lotus in pose p: a Builder (the same topology for every pose)."""
    b = Builder()
    sink = V((0, 0, -p["sink"]))
    for k, (c, r, yaw, seed) in enumerate(PADS):
        bob = p["bob"] * (1.0 if k == 0 else math.cos(1.7 * k + p["phase"]))
        m = Matrix.Translation(V(c) + V((0, 0, 0.002 + bob)) + sink) @ Matrix.Rotation(p["padtilt"] * (k + 1) * 0.7, 4, "X")
        b.add(moved(pad((0, 0, 0), r, yaw, seed), m), M["pad"], "root", "pad")

    h = head_axis(p)
    top = h @ V((0, 0, 0))
    axis = (h.to_3x3() @ V((0, 0, 1))).normalized()
    base = STEM_BASE + V((0, 0, p["base"]))
    reach = (top - base).length
    p1 = base + V((0, 0, reach * 0.55))
    p2 = top - axis * reach * 0.45
    keys = []
    for i in range(8):
        t = i / 7
        r = 0.013 - 0.003 * t
        keys.append((bezier(base, p1, p2, top, t) + sink, r, r))
    b.add(tube(keys, sub=2, n=6, ref=V((1, 0, 0)), caps=(False, False)), M["stem"], "root", "stem")

    hm = Matrix.Translation(sink) @ h
    for geo, mat in head_parts(p, M):
        b.add(moved(geo, hm), mat, "root", "head")
    return b


# ---------------------------------------------------------------- poses

REARED = {"hx": 0.0, "hy": -0.07, "hz": 0.24, "pitch": 90.0, "roll": 0.0, "yaw": 0.0, "jaw": 10.0, "fang": 1.0, "wilt": 0.0, "swell": 1.0,
          "base": 0.0, "bob": 0.0, "padtilt": 0.0, "sink": 0.0, "phase": 0.0}
# The disguise: the flower open on the water, like the decor lotus; a short stem straight down, as the decor one.
FLOATING = dict(REARED, hy=0.02, hz=0.004, pitch=0.0, jaw=34.0, fang=0.0, base=-0.09)


def mix(a, b, t):
    return {k: a[k] + (b[k] - a[k]) * t for k in a}


def pose(clip, f, frames):
    if clip == "move":
        t = f / frames
        w = 2 * math.pi * t
        return dict(REARED, hx=0.012 * math.sin(w), hz=REARED["hz"] + 0.008 * math.sin(2 * w), yaw=8 * math.sin(w), roll=4 * math.sin(w + 1.0),
                    pitch=90 + 5 * math.sin(2 * w + 0.6), jaw=10 + 6 * math.sin(2 * w), swell=1 + 0.02 * math.sin(2 * w), bob=0.003 * math.sin(w),
                    phase=w)
    if clip == "attack":
        t = f / frames  # loops from move frame 0
        p = dict(REARED)
        back = smoothstep(0.0, 0.25, t) * (1 - smoothstep(0.25, 0.4, t))
        lunge = smoothstep(0.25, 0.42, t) * (1 - smoothstep(0.55, 0.95, t))
        p["hy"] += 0.05 * back - 0.17 * lunge
        p["hz"] += 0.04 * back - 0.08 * lunge
        p["pitch"] += -22 * back + 22 * lunge
        snap = smoothstep(0.42, 0.5, t)
        p["jaw"] = 10 + 46 * smoothstep(0.0, 0.25, t) * (1 - snap) - 10 * snap * (1 - smoothstep(0.6, 1.0, t)) + 10 * snap * smoothstep(0.6, 1.0, t)
        p["swell"] = 1 + 0.06 * back
        p["yaw"] = 6 * math.sin(2 * math.pi * t) * lunge
        return p
    if clip == "die":
        t = f / (frames - 1)
        p = dict(REARED)
        shake = math.sin(t * 55) * (1 - smoothstep(0.0, 0.35, t))
        p["yaw"] = 14 * shake
        p["roll"] = 8 * shake
        flop = smoothstep(0.2, 0.6, t)
        p["hy"] = REARED["hy"] * (1 - flop) - 0.1 * flop
        p["hz"] = REARED["hz"] * (1 - flop) + 0.02 * flop
        p["pitch"] = 90 + 80 * flop
        p["jaw"] = 10 + 50 * smoothstep(0.0, 0.3, t)
        p["wilt"] = 40 * smoothstep(0.25, 0.8, t)
        p["fang"] = 1 - 0.5 * flop
        p["sink"] = 0.22 * smoothstep(0.55, 1.0, t)
        p["padtilt"] = 0.15 * smoothstep(0.55, 1.0, t)
        return p
    if clip == "idle":
        t = f / frames
        w = 2 * math.pi * t
        p = dict(FLOATING, bob=0.005 * math.sin(2 * w), hz=FLOATING["hz"] + 0.004 * math.sin(2 * w), yaw=3 * math.sin(w), phase=2 * w)
        k = (t - 0.62) / 0.06  # the twitch: snaps the jaws half shut and back, the head jerks
        if 0 <= k <= 1:
            j = math.sin(math.pi * k)
            p["jaw"] -= 26 * j
            p["fang"] = 0.5 * j
            p["hz"] += 0.012 * j
            p["roll"] = 6 * math.sin(2 * math.pi * k)
        return p
    if clip == "rise":
        t = f / (frames - 1)
        p = mix(FLOATING, REARED, smoothstep(0.0, 1.0, t))
        p["jaw"] += 20 * math.sin(math.pi * t)  # opens as it comes up
        p["hz"] += 0.03 * math.sin(math.pi * t)  # overshoots a little
        return p
    raise ValueError(clip)


# ---------------------------------------------------------------- build and export


def frame_coords(clip, f, frames, M):
    return build_geo(pose(clip, f, frames), M).verts


def build(bake=True, tex_path=None):
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    M = materials()
    b = build_geo(pose("move", 0, CLIPS[0][2]), M)
    obj = common.finish_mesh(b, coll, "lotus_new")
    common.uv_unwrap(obj, b.tags, {"head": (V((0, -0.07, 0.24)), 1.6)})
    if bake:
        tex = common.bake_texture(obj, tex_path or os.path.join(bpy.app.tempdir or "/tmp", "lotus_preview.png"), TEX_SIZE, "lotus",
                                  ao_distance=0.05)
        common.use_baked_material(obj, tex)
    report(M)
    print("lotus: {} verts, {} tris".format(len(obj.data.vertices), common.tri_count(obj)))
    return obj, M


def set_coords(obj, coords):
    obj.data.vertices.foreach_set("co", [c for v in coords for c in v])
    obj.data.update()


def report(M):
    """Per clip: lowest and highest z (z < 0 is under the water), frame 0 width; the move clip's frame 0 extent sets
    the KINDS scale."""
    for clip, _, frames in CLIPS:
        lo, hi = 1e9, -1e9
        for f in range(frames):
            zs = [v.z for v in frame_coords(clip, f, frames, M)]
            lo, hi = min(lo, min(zs)), max(hi, max(zs))
        v0 = frame_coords(clip, 0, frames, M)
        xs, ys, zs = [v.x for v in v0], [v.y for v in v0], [v.z for v in v0]
        size = (max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs))
        print("LOTUS {}: z {:.3f}..{:.3f}; frame 0 {:.3f} wide x {:.3f} deep x {:.3f} high".format(clip, lo, hi, *size))
        if clip == "move":
            print("LOTUS scale (KINDS): {:.1f} world units, frame 0 bottom z {:.4f}".format(max(size) * 40, min(zs)))


def export(obj, M, models_dir=None):
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
            set_coords(obj, frame_coords(clip, f, frames, M))
            pos_frames.append([(mesh.vertices[mesh.loops[l].vertex_index].co.x, mesh.vertices[mesh.loops[l].vertex_index].co.z,
                                -mesh.vertices[mesh.loops[l].vertex_index].co.y) for l in loops])
            nrm_frames.append([(n.x, n.z, -n.y) for n in (mesh.corner_normals[l].vector for l in loops)])
        path = os.path.join(models_dir, "lotus" + suffix + ".md3")
        stats = md3.write_md3(path, pos_frames, nrm_frames, uvs, name="lotus" + suffix)
        print("Exported {}: {verts} verts, {tris} tris, {frames} frames, {bytes} bytes".format(path, **stats))
    set_coords(obj, rest)


def review(obj, M, out_dir, shots):
    """Renders (workbench) of clip frames: shots = [(clip, frame, yaw, elevation)]."""
    os.makedirs(out_dir, exist_ok=True)
    scene = bpy.context.scene
    for o in list(bpy.data.objects):  # the startup scene's cube
        if o.type == "MESH" and o != obj:
            bpy.data.objects.remove(o)
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "TEXTURE" if obj.data.materials and "baked" in obj.data.materials[0].name else "MATERIAL"
    scene.render.resolution_x, scene.render.resolution_y = 480, 360
    cam = bpy.data.objects.get("lotus_cam") or bpy.data.objects.new("lotus_cam", bpy.data.cameras.new("lotus_cam"))
    if cam.name not in scene.collection.objects:
        scene.collection.objects.link(cam)
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = 0.7
    scene.camera = cam
    rest = [v.co.copy() for v in obj.data.vertices]
    names = {c: n for c, _, n in CLIPS}
    for clip, f, yaw, elev in shots:
        set_coords(obj, frame_coords(clip, f, names[clip], M))
        target = V((0, 0, 0.12))
        d = V((math.sin(math.radians(yaw)) * math.cos(math.radians(elev)), -math.cos(math.radians(yaw)) * math.cos(math.radians(elev)),
               math.sin(math.radians(elev))))
        cam.location = target + d * 3
        cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
        scene.render.filepath = os.path.join(out_dir, "{}_{:03d}_{}_{}.png".format(clip, f, yaw, elev))
        bpy.ops.render.render(write_still=True)
    set_coords(obj, rest)


DEFAULT_SHOTS = [("move", 0, 0, 12), ("move", 0, 90, 12), ("move", 0, 35, 35), ("idle", 0, 0, 12), ("idle", 0, 30, 45), ("idle", 70, 0, 12),
                 ("attack", 3, 90, 12), ("attack", 6, 90, 12), ("attack", 7, 0, 12), ("die", 14, 0, 12), ("die", 27, 0, 12), ("rise", 5, 90, 12)]

if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    if "--export" in args:
        obj, M = build(tex_path=os.path.join(REPO, "media", "textures", "monsters", "lotus.png"))
        export(obj, M)
    else:
        obj, M = build(bake="--bake" in args)
    if "--review" in args:
        review(obj, M, args[args.index("--review") + 1], DEFAULT_SHOTS)
