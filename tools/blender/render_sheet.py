"""Render animation frames of a built model to PNGs for review (headless).

    blender -b --python tools/blender/render_sheet.py -- <model_script> <out_dir> [action:frames ...]
e.g. ... -- tools/blender/models/anubis.py /tmp/frames anubis_walk:0,6,12 anubis_die:0,25
Then tile them with ImageMagick `montage`.
"""

import math
import os
import sys

import bpy
from mathutils import Vector

args = sys.argv[sys.argv.index("--") + 1 :]
script, out_dir, specs = os.path.abspath(args[0]), args[1], args[2:]
os.makedirs(out_dir, exist_ok=True)

bpy.ops.wm.read_factory_settings(use_empty=True)
g = {"__file__": script, "__name__": "model"}
exec(open(script).read(), g)
obj, rig = g["build"](bake="--bake" in specs)
specs = [s for s in specs if s != "--bake"]

scene = bpy.context.scene
scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x, scene.render.resolution_y = 360, 420
scene.render.film_transparent = False
world = bpy.data.worlds.new("w")
world.color = (0.18, 0.18, 0.2)
scene.world = world

cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
scene.collection.objects.link(cam)
scene.camera = cam
cam.data.type = "ORTHO"
cam.data.ortho_scale = 2.6

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


def aim(yaw_deg, target=Vector(VIEW["target"]), dist=8.0, elev_deg=None):
    yaw = math.radians(yaw_deg)
    if elev_deg is None:
        cam.location = target + Vector((math.sin(yaw) * dist, -math.cos(yaw) * dist, 0.6))
    else:
        el = math.radians(min(elev_deg, 89.9))
        cam.location = target + Vector((math.sin(yaw) * math.cos(el), -math.cos(yaw) * math.cos(el), math.sin(el))) * dist
    cam.rotation_euler = (target - cam.location).to_track_quat("-Z", "Y").to_euler()


for spec in specs:
    # action[@yaw][^elevation][#ortho_scale][~x,y,z target]:frames
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
    rig.animation_data.action = bpy.data.actions[action]
    aim(float(yaw), target, elev_deg=elev)
    for f in frames.split(","):
        scene.frame_set(int(f))
        scene.render.filepath = os.path.join(out_dir, "{}_{:03d}_{:02d}_{:04.1f}_{:02d}.png".format(action, int(float(yaw)), int(elev or 0), cam.data.ortho_scale, int(f)))
        bpy.ops.render.render(write_still=True)

# Lowest point per frame (floor is z = 0 at frame 0).
for act in bpy.data.actions:
    rig.animation_data.action = act
    lows = []
    for f in range(int(act.frame_range[0]), int(act.frame_range[1]) + 1):
        scene.frame_set(f)
        ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
        lows.append(min((ev.matrix_world @ v.co).z for v in ev.data.vertices))
    print("MINZ {}: {}".format(act.name, " ".join("{:.3f}".format(z) for z in lows)))
