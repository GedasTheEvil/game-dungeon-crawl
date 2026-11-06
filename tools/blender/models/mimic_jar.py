"""Procedural mimic jar (plan tomb-curse-containers): a monster that passes for the sealed pottery jar container until
the player opens it or hits it, smaller and weaker than the Chest of Seth.

    MCP:  p = ".../tools/blender/models/mimic_jar.py"; g = {"__file__": p, "__name__": "mimic_jar"}
          exec(open(p).read(), g); g["build"](bake=False)      # then g["export"](obj)
    CLI:  blender -b --python tools/blender/models/mimic_jar.py -- [--export] [--bake]

Blender space: Z up, facing -Y (game rotA 0), like the container. The shell is the sealed jar of containers.py
(build_jar: the same shards and mud stopper), so the closed jar is vertex for vertex the container's frame 0 and its
smashed jar the container's last frame. Inside hides the mouth: a ring of fangs in the neck (they slide up out of it),
fangs under the stopper (they grow out of its underside), a fleshy throat and a tongue. The stopper is the lid: it
hinges up at the back of the mouth.

No rig: every frame is built whole (one shape key each, as the containers' clips) and written as vertex frames.
Clips (engine ~14 fps): the awake loop (`mimic_jar.md3`, the move clip of an ambusher: the jar rocks, the stopper
chatters over the fangs), attack (`_att`, from the awake pose: the stopper flips open, the jar leans and lunges at -Y,
the tongue lashes, the stopper snaps down, back), die (`_die`, plays once: a shudder, then the jar smashes as the
container does, the mouth shrivels; the last frame is the container's smashed jar, the one it leaves), idle (`_idle`,
loops: the closed jar, still, the stopper lifts a hair once; frame 0 is the container's frame 0, the engine's
normalization reference, AMBUSH_CLIPS in src/sim/model_info.h).

Texture: media/textures/monsters/mimic_jar.png, 512, albedo x ambient occlusion, baked on the awake pose (the fangs
out of the dark).
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
sys.path.insert(0, os.path.join(HERE, ".."))
import common  # noqa: E402
import containers  # noqa: E402

importlib.reload(common)
importlib.reload(containers)
from common import REPO, Builder, cone, ellipsoid, smoothstep, tube  # noqa: E402
from containers import about, merge_moved, part, rot  # noqa: E402

COLL = "mimic_jar_new"
CLIPS = [("walk", "", 16), ("attack", "_att", 14), ("die", "_die", 18), ("idle", "_idle", 24)]
TEX_SIZE = 512
TOP = containers.jar_rows()[-1]  # the mouth's rim, the stopper's seat
HINGE = V((0, 0.11, TOP))  # the back of the mouth
FOOT_R = containers.JAR_PROFILE[0][0]  # the foot's radius on the floor

COL = {
    "tooth": (0.84, 0.78, 0.60),
    "gum": (0.34, 0.03, 0.045),
    "throat": (0.06, 0.008, 0.01),
    "tongue": (0.58, 0.13, 0.16),
}


def materials():
    M = dict(containers.materials_cache())
    for name in COL:
        M["mj_" + name] = common.make_material("mimic_jar_" + name, ("solid", name), COL)
    return M


# ---------------------------------------------------------------- the pose
# A pose: body (yaw, roll, pitch in degrees, a lunge towards -Y in metres), stopper (degrees open on its hinge, a lift),
# fangs (0 hidden .. 1 out), tongue (0 rolled up .. 1 lashed out), mouth (1 .. 0 shrivelled, the die clip), smash
# (the container's opening, the die clip).
REST = dict(roll=0.0, pitch=0.0, lunge=0.0, lid=0.0, lift=0.0, fangs=0.0, tongue=0.0, mouth=1.0, smash=0.0)
AWAKE = dict(REST, lid=38.0, fangs=1.0)


def body_matrix(p):
    """Rocks round the edge of the foot it leans on: the foot stays on the floor."""
    roll, pitch = math.radians(p["roll"]), math.radians(p["pitch"])
    edge = V((FOOT_R * math.copysign(1, roll) if roll else 0.0, -FOOT_R if pitch > 0 else (FOOT_R if pitch < 0 else 0.0), 0))
    tilt = rot(x=p["pitch"], y=p["roll"])
    return Matrix.Translation(V((0, -p["lunge"], 0))) @ about(edge, tilt)


def stopper_matrix(p):
    """The stopper's seal centre on the rim, hinged up at the back of the mouth."""
    seat = Matrix.Translation(V((0, 0, TOP + p["lift"])))
    return about(HINGE, rot(x=-p["lid"])) @ seat


def fang_ring(b, M, count, radius, z, length, up, tilt, scale, base_mat, seed_phase=0.0):
    """count fangs round the mouth, their bases at radius and z, pointing up (up = 1) or down (-1), leaning inwards."""
    for k in range(count):
        a = 2 * math.pi * (k + seed_phase) / count
        out = V((math.cos(a), math.sin(a), 0))
        base = V((radius * out.x, radius * out.y, z))
        direction = (V((0, 0, up)) - out * math.tan(math.radians(tilt))).normalized()
        n = length * (0.8 + 0.3 * ((k * 7) % 5) / 4) * max(scale, 0.02)
        b.add(cone(base, direction, n, 0.013 * max(scale, 0.02), n=5), M["mj_tooth"], "root")
    b.add(ellipsoid((0, 0, z - 0.004), (radius + 0.008, radius + 0.008, 0.012 * max(scale, 0.02)), n=count * 2, rings=4),
          M[base_mat], "root")


def tongue_points(t):
    """The tongue's centre line: rolled up in the throat (0), lashed out over the front rim and down (1)."""
    rolled = [V((0, 0.02, TOP - 0.07)), V((0, -0.02, TOP - 0.06)), V((0, -0.04, TOP - 0.05)), V((0, -0.03, TOP - 0.04))]
    out = [V((0, 0.02, TOP - 0.07)), V((0, -0.06, TOP + 0.02)), V((0, -0.2, TOP + 0.03)), V((0, -0.32, TOP - 0.06))]
    return [a.lerp(c, t) for a, c in zip(rolled, out)]


def mouth(M, p):
    """The parts that move with the body: the throat, the lower fangs in the neck, the tongue. In the jar's space."""
    b = part()
    s = p["mouth"]
    b.add(ellipsoid((0, 0, TOP - 0.075), (0.085 * s + 0.001, 0.085 * s + 0.001, 0.02 * s + 0.001), n=14, rings=4), M["mj_throat"], "root")
    fang_ring(b, M, 9, 0.072 * s, TOP - 0.065 + 0.07 * p["fangs"], 0.05, 1, 22, p["fangs"] * s, "mj_gum")
    keys = tongue_points(p["tongue"])
    b.add(tube([(k, 0.026 * s + 0.001, 0.01 * s + 0.001) for k in keys], sub=3, n=8, ref=V((0, 0, 1))), M["mj_tongue"], "root")
    return b


def upper(M, p):
    """The fangs under the stopper, in the stopper's space (its seal's centre at the origin)."""
    b = part()
    fang_ring(b, M, 8, 0.06 * p["mouth"], -0.028, 0.045, -1, 18, p["fangs"] * p["mouth"], "mj_gum", seed_phase=0.5)
    return b


def build_pose(M, p):
    b = Builder()
    body = body_matrix(p)
    shell = part()
    smash = p["smash"]
    lid = stopper_matrix(p) if smash <= 0.0 else None
    containers.build_jar(shell, M, opening=smash, stopper=lid)
    merge_moved(b, shell, body)
    # Smashing, the fangs under the stopper go with it, shrivelling to nothing.
    merge_moved(b, upper(M, p), body @ (lid if lid is not None else containers.jar_stopper(smash)))
    # Shrivelling, the mouth sinks into the foot as it goes.
    shrink = Matrix.Translation(V((0, 0, 0.03))) @ Matrix.Scale(max(p["mouth"], 0.001), 4) @ Matrix.Translation(V((0, 0, -0.03)))
    merge_moved(b, mouth(M, p), body @ shrink)
    return b


# ---------------------------------------------------------------- clips


def blend(keys, f):
    """keys: [(frame, pose dict)], smoothstep between them."""
    for (f0, a), (f1, c) in zip(keys, keys[1:]):
        if f <= f1:
            u = smoothstep(0.0, 1.0, (f - f0) / (f1 - f0)) if f1 > f0 else 1.0
            return {k: a[k] + (c[k] - a[k]) * u for k in a}
    return dict(keys[-1][1])


def walk_params(f, n):
    """The awake loop: the jar rocks side to side and back, the stopper chatters over the fangs."""
    ph = 2 * math.pi * f / n
    p = dict(AWAKE)
    p["roll"] = 5.0 * math.sin(ph)
    p["pitch"] = 2.5 * math.sin(2 * ph)
    p["lid"] = 38.0 + 14.0 * math.sin(4 * ph)
    p["tongue"] = 0.12 + 0.08 * math.sin(2 * ph + 1.0)
    return p


ATTACK = [
    (0, AWAKE),
    (4, dict(AWAKE, pitch=-6.0, lid=95.0, tongue=0.2)),  # rears back, gapes
    (7, dict(AWAKE, pitch=16.0, lunge=0.12, lid=100.0, tongue=1.0)),  # the lunge, the tongue out
    (9, dict(AWAKE, pitch=14.0, lunge=0.1, lid=8.0, tongue=0.3)),  # the snap
    (11, dict(AWAKE, pitch=4.0, lunge=0.03, lid=30.0, tongue=0.15)),
    (13, dict(AWAKE, tongue=0.12)),
]

DIE = [
    (0, dict(AWAKE, tongue=0.12)),
    (2, dict(AWAKE, roll=6.0, lid=70.0, tongue=0.8)),  # a shudder, a cry
    (4, dict(AWAKE, roll=-6.0, lid=60.0, tongue=0.6)),
    (5, dict(AWAKE, roll=0.0, lid=30.0, tongue=0.3)),
]


def die_params(f, n):
    if f <= 5:
        return blend(DIE, f)
    u = (f - 5) / (n - 1 - 5)
    p = dict(REST, fangs=1.0)
    p["smash"] = u
    p["mouth"] = 1.0 - smoothstep(0.0, 0.35, u)
    return p


def idle_params(f, n):
    """Still; once a loop the stopper lifts a hair, as if the jar breathed."""
    p = dict(REST)
    p["lift"] = 0.006 * math.sin(math.pi * smoothstep(0.55, 0.85, f / n)) ** 2
    return p


def clip_params(name, f, n):
    if name == "walk":
        return walk_params(f, n)
    if name == "attack":
        return blend(ATTACK, f)
    if name == "die":
        return die_params(f, n)
    return idle_params(f, n)


# ---------------------------------------------------------------- build and export


def build(bake=True, tex_path=None):
    """The mimic jar on its awake pose (the texture's), one object; obj["texture"] = the image name when baked."""
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    containers._MATS.clear()
    M = materials()
    b = build_pose(M, AWAKE)
    obj = common.finish_mesh(b, coll, "mimic_jar")
    obj.data.set_sharp_from_angle(angle=math.radians(50))
    common.uv_unwrap(obj, b.tags)
    if bake:
        path = tex_path or os.path.join(bpy.app.tempdir or "/tmp", "mimic_jar.png")
        tex = common.bake_texture(obj, path, TEX_SIZE, "monster_mimic_jar", ao_distance=0.12)
        common.use_baked_material(obj, tex)
        obj["texture"] = tex.name
    closed = build_pose(M, REST)
    xs, ys, zs = ([v[a] for v in closed.verts] for a in range(3))
    size = max(max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs))
    print("mimic_jar: {} verts, {} tris; closed x {:.3f}..{:.3f} y {:.3f}..{:.3f} z {:.3f}..{:.3f}: scale {:.2f}".format(
        len(obj.data.vertices), common.tri_count(obj), min(xs), max(xs), min(ys), max(ys), min(zs), max(zs),
        size * containers.UNITS_PER_M))
    return obj


def write_clip(obj, path, g, name, count):
    """One clip, count frames, as shape keys on the baked object, written to path."""
    M = materials()
    obj.shape_key_add(name="Basis")
    keys = []
    for f in range(count):
        b = build_pose(M, clip_params(name, f, count))
        key = obj.shape_key_add(name="%s%d" % (name, f))
        if len(b.verts) != len(key.data):
            raise ValueError("{}: frame {} has {} verts, the mesh {}".format(name, f, len(b.verts), len(key.data)))
        for i, v in enumerate(b.verts):
            key.data[i].co = V(v)
        keys.append(key)
    frames, normals, uvs = [], [], None
    try:
        for key in keys:
            for other in keys:
                other.value = 1.0 if other is key else 0.0
            bpy.context.view_layer.update()
            pos, nrm, uv = g["_sample"](obj, bpy.context.evaluated_depsgraph_get())
            frames.append(pos)
            normals.append(nrm)
            uvs = uvs or uv
    finally:
        obj.shape_key_clear()
    stats = g["md3"].write_md3(path, frames, normals, uvs, name=os.path.splitext(os.path.basename(path))[0])
    print("Exported {} -> {}: {verts} verts, {tris} tris, {frames} frames, {bytes} bytes".format(obj.name, path, **stats))


def export(obj, models_dir=None):
    models_dir = models_dir or os.path.join(REPO, "media", "models", "monsters")
    p = os.path.join(REPO, "tools", "blender", "md3_export.py")
    g = {"__file__": p, "__name__": "md3_export"}
    exec(open(p).read(), g)
    for name, suffix, count in CLIPS:
        write_clip(obj, os.path.join(models_dir, "mimic_jar%s.md3" % suffix), g, name, count)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    exporting = "--export" in args
    tex = os.path.join(REPO, "media", "textures", "monsters", "mimic_jar.png")
    built = build(bake=exporting or "--bake" in args, tex_path=tex if exporting else None)
    if exporting:
        export(built)
