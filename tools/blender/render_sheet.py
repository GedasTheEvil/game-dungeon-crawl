"""Render animation frames of a built model to PNGs for review (headless), plus motion checks as numbers and images.

    blender -b --python tools/blender/render_sheet.py -- <model_script> <out_dir> [options] [action:frames ...]
e.g. ... -- tools/blender/models/anubis.py /tmp/frames anubis_walk:0,6,12 anubis_die:0,25
Then tile them with ImageMagick `montage`.

Spec: action[@yaw][^elevation][#ortho_scale][~x,y,z target]:frames (frames: 0,6,12 or "all").
Options:
  --bake            bake the texture (else flat colours)
  --onion           per spec also <name>_onion.png: its frames over each other, older ones fainter
  --paths[=b1,b2]   per spec also <name>_paths.png: the bone tips' paths over the whole clip (a dot per frame,
                    so the spacing shows the speed) over its first frame, dimmed. Default bones: deforming leaf bones.
  --intersect       print the self-intersecting face pairs per frame (slow on big meshes)
Printed for every clip: MINZ (lowest point per frame), SEAM (last frame -> frame 0 jump vs the median frame step),
SLIDE (horizontal step of the vertices on the floor per frame; an in-place walk wants an even step).
"""

import math
import os
import subprocess
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

args = sys.argv[sys.argv.index("--") + 1 :]
script, out_dir, rest = os.path.abspath(args[0]), args[1], args[2:]
opts = [a for a in rest if a.startswith("--")]
specs = [a for a in rest if not a.startswith("--")]
os.makedirs(out_dir, exist_ok=True)
onion = "--onion" in opts
intersect = "--intersect" in opts
paths = next((a for a in opts if a.startswith("--paths")), None)
path_bones = paths.split("=", 1)[1].split(",") if paths and "=" in paths else None

bpy.ops.wm.read_factory_settings(use_empty=True)
g = {"__file__": script, "__name__": "model"}
exec(open(script).read(), g)
obj, rig = g["build"](bake="--bake" in opts)

scene = bpy.context.scene
scene.render.engine = "BLENDER_EEVEE"
scene.render.film_transparent = False
world = bpy.data.worlds.new("w")
world.color = (0.18, 0.18, 0.2)
scene.world = world
BACKGROUND = "#2e2e33"  # the world colour as the renders show it, for the composites

cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
scene.collection.objects.link(cam)
scene.camera = cam
cam.data.type = "ORTHO"

sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN"))
sun.data.energy = 3.5
sun.rotation_euler = (math.radians(50), 0, math.radians(30))
scene.collection.objects.link(sun)

VIEW = g.get("REVIEW_VIEW", {"target": (0, 0.3, 0.95), "ortho": 2.6})
cam.data.ortho_scale = VIEW["ortho"]
scene.render.resolution_x, scene.render.resolution_y = VIEW.get("res", (360, 420))
fill = bpy.data.objects.new("fill", bpy.data.lights.new("fill", "SUN"))
fill.data.energy = 1.5
fill.rotation_euler = (math.radians(60), 0, math.radians(200))
scene.collection.objects.link(fill)

# Exported frame count per clip (the model's CLIPS: (action, suffix, frames)); a loop's frame N is frame 0.
CLIP_FRAMES = {c[0]: c[2] for c in g.get("CLIPS", [])}
PATH_COLOURS = [(1, 0.25, 0.2), (0.3, 0.9, 0.3), (0.3, 0.6, 1), (1, 0.85, 0.2), (1, 0.4, 1), (0.3, 1, 1), (1, 0.6, 0.2), (0.7, 0.5, 1)]


def aim(yaw_deg, target=Vector(VIEW["target"]), dist=8.0, elev_deg=None):
    yaw = math.radians(yaw_deg)
    if elev_deg is None:
        cam.location = target + Vector((math.sin(yaw) * dist, -math.cos(yaw) * dist, 0.6))
    else:
        el = math.radians(min(elev_deg, 89.9))
        cam.location = target + Vector((math.sin(yaw) * math.cos(el), -math.cos(yaw) * math.cos(el), math.sin(el))) * dist
    cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()


def clip_frames(act):
    """The exported frames of a clip: 0 .. N-1."""
    return range(int(act.frame_range[0]), int(act.frame_range[0]) + CLIP_FRAMES.get(act.name, int(act.frame_range[1] - act.frame_range[0]) + 1))


def render(path, transparent=False):
    scene.render.film_transparent = transparent
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    scene.render.film_transparent = False


def magick(*cmd):
    subprocess.run(["convert", *cmd], check=True)


def faded(path, alpha, tint=None):
    """ImageMagick args: path laid over the image so far at alpha, tinted (0..1) towards blue."""
    colour = ["-fill", "#3a7bff", "-colorize", "%d%%" % (tint * 100)] if tint else []
    return ["(", path, *colour, "-channel", "A", "-evaluate", "multiply", "%.3f" % alpha, "+channel", ")", "-composite"]


def blank():
    return ["-size", "%dx%d" % (scene.render.resolution_x, scene.render.resolution_y), "xc:" + BACKGROUND]


def emission_material(name, colour):
    mat = bpy.data.materials.new(name)
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (0, 0, 0, 1)
    bsdf.inputs["Emission Color"].default_value = (*colour, 1)
    bsdf.inputs["Emission Strength"].default_value = 1.0
    return mat


def default_path_bones():
    return [b.name for b in rig.data.bones if not b.children and b.use_deform]


def add_paths(act, bones):
    """Curves through the bones' tips over the clip (the loop's frame 0 again at the end), a dot per frame."""
    made = []
    radius = cam.data.ortho_scale * 0.003
    frames = list(clip_frames(act)) + [clip_frames(act)[0]]
    for i, name in enumerate(bones):
        mat = emission_material("path_" + name, PATH_COLOURS[i % len(PATH_COLOURS)])
        pts = []
        for f in frames:
            scene.frame_set(f)
            pts.append(rig.matrix_world @ rig.pose.bones[name].tail)
        cu = bpy.data.curves.new("path_" + name, "CURVE")
        cu.dimensions = "3D"
        cu.bevel_depth = radius
        sp = cu.splines.new("POLY")
        sp.points.add(len(pts) - 1)
        for p, co in zip(sp.points, pts):
            p.co = (*co, 1)
        ob = bpy.data.objects.new("path_" + name, cu)
        ob.data.materials.append(mat)
        made.append(ob)
        dot = bpy.data.meshes.new("dot")
        bm = bmesh.new()
        bmesh.ops.create_icosphere(bm, subdivisions=1, radius=radius * 1.8)
        bm.to_mesh(dot)
        bm.free()
        dot.materials.append(mat)
        for j, co in enumerate(pts[:-1]):
            d = bpy.data.objects.new("dot", dot)
            d.location = co
            made.append(d)
            if j == 0:  # frame 0 bigger, so the direction reads
                d.scale = (2, 2, 2)
    for ob in made:
        scene.collection.objects.link(ob)
    return made


def spec_name(action, yaw, elev):
    return "{}_{:03d}_{:02d}_{:04.1f}".format(action, int(float(yaw)), int(elev or 0), cam.data.ortho_scale)


for spec in specs:
    action, frames = spec.split(":")
    target = Vector(VIEW["target"])
    if "~" in action:
        action, t = action.split("~")
        target = Vector([float(c) for c in t.split(",")])
    cam.data.ortho_scale = VIEW["ortho"]
    if "#" in action:
        action, scale = action.split("#")
        cam.data.ortho_scale = float(scale)
    elev = None
    if "^" in action:
        action, elev = action.split("^")
        elev = float(elev)
    yaw = 0
    if "@" in action:
        action, yaw = action.split("@")
    act = bpy.data.actions[action]
    rig.animation_data.action = act
    aim(float(yaw), target, elev_deg=elev)
    frame_list = list(clip_frames(act)) if frames == "all" else [int(f) for f in frames.split(",")]
    name = spec_name(action, yaw, elev)
    for f in frame_list:
        scene.frame_set(f)
        render(os.path.join(out_dir, "{}_{:02d}.png".format(name, f)))
    if onion:
        layers = []
        for f in frame_list:
            scene.frame_set(f)
            p = os.path.join(out_dir, "_onion_{:02d}.png".format(f))
            render(p, transparent=True)
            layers.append(p)
        cmd = blank()
        for i, p in enumerate(layers):  # the last frame as is, the older ones bluer and fainter
            age = (len(layers) - 1 - i) / max(len(layers) - 1, 1)
            cmd += faded(p, 1.0 - 0.5 * age, 0.2 + 0.5 * age if age else None)
        magick(*cmd, os.path.join(out_dir, name + "_onion.png"))
        for p in layers:
            os.remove(p)
    if paths:
        scene.frame_set(frame_list[0])
        body = os.path.join(out_dir, "_body.png")
        render(body, transparent=True)
        obj.hide_render = True
        made = add_paths(act, path_bones or default_path_bones())
        scene.frame_set(frame_list[0])
        lines = os.path.join(out_dir, "_paths.png")
        view = scene.view_settings.view_transform
        scene.view_settings.view_transform = "Standard"  # the paths in their own colours, not tone-mapped
        render(lines, transparent=True)
        scene.view_settings.view_transform = view
        for ob in made:
            bpy.data.objects.remove(ob)
        obj.hide_render = False
        magick(*blank(), *faded(body, 0.45), lines, "-composite", os.path.join(out_dir, name + "_paths.png"))
        os.remove(body)
        os.remove(lines)
    if paths and not path_bones:
        print("PATHS {}: {}".format(action, " ".join(default_path_bones())))


def vertex_frames(act):
    """World positions of the evaluated mesh, one (n, 3) array per exported frame."""
    rig.animation_data.action = act
    out = []
    for f in clip_frames(act):
        scene.frame_set(f)
        ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
        co = np.empty(len(ev.data.vertices) * 3)
        ev.data.vertices.foreach_get("co", co)
        co = co.reshape(-1, 3) @ np.array(ev.matrix_world)[:3, :3].T + np.array(ev.matrix_world)[:3, 3]
        out.append(co)
    return out


def self_intersections():
    ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    verts = [ev.matrix_world @ v.co for v in ev.data.vertices]
    polys = [tuple(p.vertices) for p in ev.data.polygons]
    tree = BVHTree.FromPolygons(verts, polys)
    return sum(1 for i, j in tree.overlap(tree) if i < j and not set(polys[i]) & set(polys[j]))


# Lowest point per frame (floor is z = 0 at frame 0), the loop seam, the slide of the planted vertices.
for act in bpy.data.actions:
    vf = vertex_frames(act)
    print("MINZ {}: {}".format(act.name, " ".join("{:.3f}".format(c[:, 2].min()) for c in vf)))
    if len(vf) > 2 and len(vf[0]) == len(vf[-1]):
        steps = [np.linalg.norm(b - a, axis=1).max() for a, b in zip(vf, vf[1:])]
        seam = np.linalg.norm(vf[0] - vf[-1], axis=1).max()
        print("SEAM {}: last -> 0 {:.4f}, median step {:.4f}, ratio {:.2f}".format(act.name, seam, np.median(steps), seam / max(np.median(steps), 1e-9)))
        size = max(np.ptp(vf[0], axis=0))
        slides = []
        for a, b in zip(vf, vf[1:] + vf[:1]):
            on_floor = (a[:, 2] < 0.005 * size) & (b[:, 2] < 0.005 * size)
            slides.append("{:.3f}".format(np.median(np.linalg.norm((b - a)[on_floor, :2], axis=1))) if on_floor.any() else "-")
        print("SLIDE {}: {}".format(act.name, " ".join(slides)))
    if intersect:
        counts = []
        for f in clip_frames(act):
            scene.frame_set(f)
            counts.append(str(self_intersections()))
        print("INTERSECT {}: {}".format(act.name, " ".join(counts)))
