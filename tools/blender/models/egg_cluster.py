"""Procedural scorpion egg cluster (the scorpion queen's hatchery): a mound of leathery eggs glued with dried resin on
a sand base. A rooted "monster": it never moves, the move clip is an idle loop.

    MCP:  p = ".../tools/blender/models/egg_cluster.py"; g = {"__file__": p, "__name__": "egg_cluster"}
          exec(open(p).read(), g); g["build"]()          # then g["export"]()
    CLI:  blender -b --python tools/blender/models/egg_cluster.py -- [--export]

Blender space: Z up, the cluster faces -Y (game uses rotA = 0); it is round, about 0.9 wide x 0.5 high.
Every egg is a rigid ellipsoid on its own bone (pointing up from the egg's centre, parented to the root): the clips
only scale and move the bones. Clips: move 24 (the reference, loops: the eggs swell and shrink slowly out of step,
two eggs twitch), attack 16 (loops: a stronger heave, the eggs bulge in a wave from the top, played while a scorpion
hatches), die 20 (once: the eggs swell and burst one after another, collapsing into flat husks on the base).
Every frame the lowest vertex stays at or above the floor (printed per clip).
"""

import importlib
import math
import os
import random
import sys

import bpy
from mathutils import Vector as V

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import common  # noqa: E402

importlib.reload(common)
from common import REPO, Builder, ellipsoid, smoothstep  # noqa: E402

COLL = "egg_cluster_new"
CLIPS = [("egg_cluster_move", "", 24), ("egg_cluster_attack", "_att", 16), ("egg_cluster_die", "_die", 20)]
TEX_SIZE = 1024
REVIEW_VIEW = {"target": (0, 0, 0.24), "ortho": 1.3, "res": (480, 360)}

COL = {
    "egg": (0.62, 0.44, 0.20),
    "egg_pale": (0.82, 0.68, 0.40),
    "vein": (0.42, 0.12, 0.06),
    "resin": (0.30, 0.15, 0.04),
    "resin_hi": (0.55, 0.32, 0.08),
    "sand": (0.62, 0.48, 0.28),
    "sand_dark": (0.40, 0.29, 0.15),
}

BASE_R, BASE_H = 0.44, 0.07  # sand base: radius, height


def egg_layout():
    """(centre, radii (side, side, along), axis) per egg: three rings and a top pair, tilted outwards."""
    rnd = random.Random(7)
    eggs = []
    rings = [(8, 0.27, 0.15, 0.085, 0.0), (4, 0.37, 0.12, 0.065, 0.39), (5, 0.14, 0.29, 0.08, 0.6), (2, 0.05, 0.40, 0.075, 1.1)]
    for count, r, z, size, phase in rings:
        for k in range(count):
            a = 2 * math.pi * k / count + phase + rnd.uniform(-0.12, 0.12)
            out = V((math.cos(a), math.sin(a), 0))
            c = V((0, 0, z)) + out * r
            tilt = 0.35 + 0.5 * (r / 0.37)
            axis = (V((0, 0, 1)) + out * tilt).normalized()
            s = size * rnd.uniform(0.9, 1.1)
            eggs.append((c, (s, s, s * 1.35), axis))
    return eggs


def resin_layout(eggs):
    """Resin blobs between neighbouring eggs, low on the mound; each moves with its two eggs."""
    blobs = []
    for i, (a, ra, _) in enumerate(eggs):
        for j, (b, rb, _) in enumerate(eggs[i + 1 :], i + 1):
            d = (a - b).length
            if d < (ra[0] + rb[0]) * 1.25:
                m = (a + b) / 2
                blobs.append((V((m.x, m.y, m.z - 0.02)), 0.035, {"egg%02d" % i: 0.5, "egg%02d" % j: 0.5}))
    return blobs


# ---------------------------------------------------------------- materials


def egg_material(name):
    """Cream to amber leather, paler at the top of each egg, a net of thin dark red veins."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nt = mat.node_tree
    for node in list(nt.nodes):
        if node.type not in ("OUTPUT_MATERIAL", "BSDF_PRINCIPLED"):
            nt.nodes.remove(node)
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Roughness"].default_value = 0.35
    new, link = nt.nodes.new, nt.links.new
    geo = new("ShaderNodeNewGeometry")
    sep = new("ShaderNodeSeparateXYZ")
    link(geo.outputs["Normal"], sep.inputs[0])
    tone = new("ShaderNodeValToRGB")
    els = tone.color_ramp.elements
    els[0].position, els[0].color = 0.2, (*COL["egg"], 1)
    els[1].position, els[1].color = 0.95, (*COL["egg_pale"], 1)
    remap = new("ShaderNodeMath")
    remap.operation = "MULTIPLY_ADD"
    remap.inputs[1].default_value, remap.inputs[2].default_value = 0.5, 0.5
    link(sep.outputs["Z"], remap.inputs[0])
    link(remap.outputs[0], tone.inputs["Fac"])
    coord = new("ShaderNodeTexCoord")
    noise = new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 14.0
    noise.inputs["Detail"].default_value = 6.0
    noise.inputs["Distortion"].default_value = 1.5
    link(coord.outputs["Object"], noise.inputs["Vector"])
    # Veins: where the noise crosses 0.5 (a thin band of it).
    band = new("ShaderNodeMath")
    band.operation = "SUBTRACT"
    link(noise.outputs["Fac"], band.inputs[0])
    band.inputs[1].default_value = 0.5
    absn = new("ShaderNodeMath")
    absn.operation = "ABSOLUTE"
    link(band.outputs[0], absn.inputs[0])
    vr = new("ShaderNodeMapRange")
    vr.inputs["From Min"].default_value, vr.inputs["From Max"].default_value = 0.0, 0.04
    vr.inputs["To Min"].default_value, vr.inputs["To Max"].default_value = 0.75, 0.0
    link(absn.outputs[0], vr.inputs["Value"])
    mix = new("ShaderNodeMix")
    mix.data_type = "RGBA"
    link(vr.outputs["Result"], mix.inputs["Factor"])
    link(tone.outputs["Color"], mix.inputs[6])
    mix.inputs[7].default_value = (*COL["vein"], 1)
    link(mix.outputs[2], bsdf.inputs["Base Color"])
    return mat


def noisy_material(name, lo, hi, scale, roughness):
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nt = mat.node_tree
    for node in list(nt.nodes):
        if node.type not in ("OUTPUT_MATERIAL", "BSDF_PRINCIPLED"):
            nt.nodes.remove(node)
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Roughness"].default_value = roughness
    coord = nt.nodes.new("ShaderNodeTexCoord")
    noise = nt.nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = scale
    noise.inputs["Detail"].default_value = 8.0
    nt.links.new(coord.outputs["Object"], noise.inputs["Vector"])
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    els = ramp.color_ramp.elements
    els[0].position, els[0].color = 0.35, (*COL[lo], 1)
    els[1].position, els[1].color = 0.65, (*COL[hi], 1)
    nt.links.new(noise.outputs["Fac"], ramp.inputs["Fac"])
    nt.links.new(ramp.outputs["Color"], bsdf.inputs["Base Color"])
    return mat


def materials():
    return {
        "egg": egg_material("egg_cluster_egg"),
        "resin": noisy_material("egg_cluster_resin", "resin", "resin_hi", 30.0, 0.25),
        "sand": noisy_material("egg_cluster_sand", "sand_dark", "sand", 22.0, 0.9),
    }


# ---------------------------------------------------------------- mesh and rig


def build_base(b, M):
    """A low sand mound: a flattened ellipsoid cut at the floor (its lower half pressed flat)."""
    verts, faces, fuv = ellipsoid((0, 0, 0), (BASE_R, BASE_R * 0.92, BASE_H), n=24, rings=8)
    verts = [V((v.x, v.y, max(v.z, 0.0))) for v in verts]
    b.add((verts, faces, fuv), M["sand"], "root", "base")


def build_eggs(b, M, eggs):
    for i, (c, r, axis) in enumerate(eggs):
        b.add(ellipsoid(c, r, n=12, rings=8, axis=axis, ref=V((1, 0, 0)) if abs(axis.x) < 0.9 else V((0, 1, 0))),
              M["egg"], "egg%02d" % i, "egg")


def build_resin(b, M, blobs):
    for c, s, weights in blobs:
        b.add(ellipsoid(c, (s, s, s * 0.6), n=8, rings=5), M["resin"], weights, "resin")


def build_rig(coll, eggs):
    arm = bpy.data.armatures.new("egg_cluster_rig")
    rig = bpy.data.objects.new("egg_cluster_rig", arm)
    coll.objects.link(rig)
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    eb = arm.edit_bones
    root = eb.new("root")
    root.head, root.tail = V((0, 0, 0)), V((0, 0, 0.1))
    root.roll = 0
    for i, (c, _, _) in enumerate(eggs):
        e = eb.new("egg%02d" % i)
        e.head, e.tail = c, c + V((0, 0, 0.1))  # local Y = world Z (up), local X = world X
        e.roll = 0
        e.parent = root
    bpy.ops.object.mode_set(mode="OBJECT")
    rig.animation_data_create()
    return rig


# ---------------------------------------------------------------- animation


def egg_pose(clip, i, n, t, egg):
    """(xy scale, vertical scale, drop) of egg i at clip time t (0..1)."""
    c, r, _ = egg
    phase = i * 2.399  # golden angle: out of step
    if clip == "egg_cluster_move":
        s = 1 + 0.035 * math.sin(2 * math.pi * t + phase)
        sz = 1 + 0.05 * math.sin(2 * math.pi * t + phase)
        if i in (3, 13):  # twitch: a quick jerk twice per loop
            k = (t * 2 + i * 0.13) % 1.0
            sz += 0.12 * math.exp(-((k - 0.5) / 0.04) ** 2)
        return s, sz, 0.0
    if clip == "egg_cluster_attack":
        wave = 2 * math.pi * t - c.z * 6.0
        s = 1 + 0.10 * max(0.0, math.sin(wave))
        sz = 1 + 0.18 * max(0.0, math.sin(wave))
        return s, sz, 0.0
    # Die: swell, burst, collapse to a flat husk on the base. Top eggs go first.
    start = 0.05 + 0.4 * (1.0 - min(c.z / 0.4, 1.0)) + 0.05 * (i % 3)
    k = (t - start) / 0.35
    if k <= 0:
        return 1.0, 1.0, 0.0
    swell = math.sin(math.pi * min(k / 0.3, 1.0)) * 0.25 if k < 0.3 else 0.0
    flat = smoothstep(0.25, 1.0, k)
    s = 1 + swell + 0.12 * flat
    sz = (1 + swell) * (1 - 0.72 * flat)
    # Sink towards the base top: the husk lies on the mound.
    drop = flat * max(0.0, c.z - (BASE_H + r[2] * 0.2))
    return s, sz, drop


def make_actions(rig, eggs, obj):
    acts = {}
    n = len(eggs)
    for name, _, frames in CLIPS:
        act = bpy.data.actions.get(name)
        if act:
            bpy.data.actions.remove(act)
        act = bpy.data.actions.new(name)
        act.use_fake_user = True
        rig.animation_data.action = act
        once = name == "egg_cluster_die"
        for f in range(frames):
            t = f / (frames - 1) if once else f / frames
            for i, egg in enumerate(eggs):
                pb = rig.pose.bones["egg%02d" % i]
                s, sz, drop = egg_pose(name, i, n, t, egg)
                pb.scale = (s, sz, s)
                pb.location = (0, -drop, 0)
                pb.keyframe_insert("scale", frame=f)
                pb.keyframe_insert("location", frame=f)
            root = rig.pose.bones["root"]
            heave = 1 + (0.04 * math.sin(2 * math.pi * t) if name == "egg_cluster_attack" else 0.0)
            root.scale = (1, heave, 1)
            root.keyframe_insert("scale", frame=f)
        acts[name] = act
    return acts


def floor_report(obj, rig):
    scene = bpy.context.scene
    for name, _, frames in CLIPS:
        rig.animation_data.action = bpy.data.actions[name]
        lo, hi, wide = 1e9, -1e9, 0.0
        for f in range(frames):
            scene.frame_set(f)
            ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
            zs = [v.co.z for v in ev.data.vertices]
            xs = [v.co.x for v in ev.data.vertices]
            lo, hi = min(lo, min(zs)), max(hi, max(zs))
            if f == 0:
                wide = max(xs) - min(xs)
        print("EGGS {}: z {:.3f}..{:.3f}, frame 0 width {:.3f}".format(name, lo, hi, wide))
    rig.animation_data.action = bpy.data.actions[CLIPS[0][0]]


# ---------------------------------------------------------------- entry points


def build(bake=True, tex_path=None):
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    M = materials()
    eggs = egg_layout()
    rig = build_rig(coll, eggs)
    b = Builder()
    build_base(b, M)
    build_resin(b, M, resin_layout(eggs))
    build_eggs(b, M, eggs)
    obj = common.finish_mesh(b, coll, "egg_cluster_new")
    common.uv_unwrap(obj, b.tags, {"egg": (V((0, 0, 0.25)), 1.4)})
    if bake:
        tex = common.bake_texture(obj, tex_path or os.path.join(bpy.app.tempdir or "/tmp", "egg_cluster_preview.png"), TEX_SIZE,
                                  "egg_cluster", ao_distance=0.08)
        common.use_baked_material(obj, tex)
    common.rig_object(obj, rig)
    acts = make_actions(rig, eggs, obj)
    rig.animation_data.action = acts[CLIPS[0][0]]
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 0, CLIPS[0][2] - 1
    scene.frame_set(0)
    floor_report(obj, rig)
    print("egg_cluster: {} eggs, {} verts, {} tris".format(len(eggs), len(obj.data.vertices), common.tri_count(obj)))
    return obj, rig


def export(models_dir=None):
    common.export_files(bpy.data.objects["egg_cluster_new"], bpy.data.objects["egg_cluster_rig"], "egg_cluster", CLIPS, "monsters",
                        models_dir)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    if "--export" in args:
        build(tex_path=os.path.join(REPO, "textures", "monsters", "egg_cluster.png"))
        export()
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "egg_cluster.blend"))
    else:
        build(bake="--bake" in args)
