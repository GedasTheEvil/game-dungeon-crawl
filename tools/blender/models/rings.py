"""Procedural rings (plan ring-accessories): the band with its raised setting (ring_band) and the cut gem that sits in
it (ring_gem), two models so the engine can draw the gem see-through over the opaque band (Item::DrawPart). All 18
rings share them: the band baked once per metal (ring_band_copper, ring_band_silver, ring_band_gold), the gem baked
once in pale greys the engine tints per gem (ringGemColour in src/world/items.cpp).

    MCP:  p = ".../tools/blender/models/rings.py"; g = {"__file__": p, "__name__": "rings"}
          exec(open(p).read(), g); g["build"](bake=False)      # then g["export"](objs)
    CLI:  blender -b --python tools/blender/models/rings.py -- [--export] [--bake] [--review out.png]

Blender space: Z up, in metres three times a real ring's size (S: the shared helpers' tubes are no thinner than a
millimetre), stylised: the gem is far bigger than a real one so it reads in the inventory slot and on a chest (the
engine centres and scales the ring like the amulets, RING_SCALE). The band stands upright
in the x-z plane, its hole facing the camera (-Y), the setting on top (+Z). Both files are written from the same
origin (the band's lowest point at z = 0): the engine centres the band (Centrify) and places the gem with the band's
transform (Normalize), so the gem sits in its setting.

Textures: 256 PNGs (media/textures/items/<name>.png), albedo x ambient occlusion like the other items. The gem's is
near white with lighter and darker facets: the engine's tint gives its colour, the facets its sparkle.
"""

import importlib
import math
import os
import sys

import bpy
from mathutils import Vector as V

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import common  # noqa: E402
import decor  # noqa: E402

importlib.reload(common)
importlib.reload(decor)
from common import REPO, Builder, lathe, tube  # noqa: E402
from decor import arc_len, place, stripes  # noqa: E402

COLL = "rings_new"
TEX_SIZE = 256
S = 3.0  # the model's size over a real ring's
BAND_R = 0.011 * S  # the band's centre line
SETTING_Z = BAND_R + 0.0012 * S  # the setting's floor, on top of the band
GEM_R = 0.0058 * S  # the gem's girdle
# The band's metals, in RingMetal order: texture stem and the palette its texture uses.
METALS = [
    ("ring_band_copper", {"metal": (0.70, 0.34, 0.16), "metal_dark": (0.38, 0.15, 0.06)}),
    ("ring_band_silver", {"metal": (0.80, 0.82, 0.86), "metal_dark": (0.46, 0.48, 0.53)}),
    ("ring_band_gold", {"metal": (0.85, 0.60, 0.16), "metal_dark": (0.48, 0.31, 0.08)}),
]
COL = {
    "metal": (0.85, 0.60, 0.16),
    "metal_dark": (0.48, 0.31, 0.08),
    "gem_table": (1.0, 1.0, 1.0),
    "gem_light": (0.92, 0.92, 0.94),
    "gem_mid": (0.74, 0.74, 0.78),
    "gem_dark": (0.52, 0.52, 0.58),
}


def materials(palette=None):
    pal = {**COL, **(palette or {})}
    specs = {
        "metal": stripes("u", 1 / 4, [(5, "metal"), (1, "metal_dark")], "LINEAR"),
        "metal_plain": ("solid", "metal"),
        "gem_table": ("solid", "gem_table"),
        "gem_crown": stripes("u", 1 / 16, [(1, "gem_light"), (1, "gem_mid")]),
        "gem_pavilion": stripes("u", 1 / 16, [(1, "gem_mid"), (1, "gem_dark")]),
    }
    return {name: common.make_material("ring_" + name, spec, pal) for name, spec in specs.items()}


def build_band(b, M):
    """The hoop, thicker at the shoulders under the setting, then the setting: a cup with four claws."""
    keys = []
    steps = 32
    for k in range(steps):
        a = 2 * math.pi * k / steps  # 0 at the top
        shoulder = 0.5 + 0.5 * math.cos(a)  # 1 at the top, 0 at the bottom
        p = V((BAND_R * math.sin(a), 0.0, BAND_R * math.cos(a)))
        keys.append((p, S * (0.0014 + 0.0010 * shoulder ** 2), S * (0.0026 + 0.0012 * shoulder ** 2)))
    b.add(tube(keys, sub=1, n=10, ref=V((0, 1, 0)), closed=True), M["metal"], "root")
    # The cup: a flared collar round the gem's lower half.
    cup = [(0.0, SETTING_Z - 0.0012 * S), (0.0030 * S, SETTING_Z - 0.0010 * S), (0.0050 * S, SETTING_Z + 0.0006 * S),
           (GEM_R + 0.0004 * S, SETTING_Z + 0.0022 * S), (GEM_R - 0.0002 * S, SETTING_Z + 0.0026 * S)]
    for mat, geo in lathe(cup, arc_len(cup), 16, ["metal_plain"] * (len(cup) - 1)).items():
        b.add(geo, M[mat], "root")
    # The claws: from the cup's rim up over the girdle, bent in onto the crown.
    for k in range(4):
        a = 2 * math.pi * (k + 0.5) / 4
        out = V((math.cos(a), math.sin(a), 0.0))
        keys = [(out * (GEM_R + 0.0003 * S) + V((0, 0, SETTING_Z + 0.0018 * S)), 0.0004 * S, 0.0004 * S),
                (out * (GEM_R + 0.0003 * S) + V((0, 0, SETTING_Z + 0.0034 * S)), 0.00036 * S, 0.00036 * S),
                (out * (GEM_R - 0.0010 * S) + V((0, 0, SETTING_Z + 0.0046 * S)), 0.00034 * S, 0.00034 * S)]
        b.add(tube(keys, sub=2, n=6, ref=V((0, 0, 1))), M["metal_plain"], "root")


def build_gem(b, M):
    """A round cut with eight facets a ring: the pavilion's point down in the cup, the girdle at the cup's rim, the
    crown up to a flat table."""
    girdle = SETTING_Z + 0.0024 * S
    profile = [(0.0, SETTING_Z - 0.0008 * S), (GEM_R, girdle), (GEM_R, girdle + 0.0006 * S), (GEM_R * 0.62, girdle + 0.0034 * S),
               (0.0, girdle + 0.0036 * S)]
    mats = ["gem_pavilion", "gem_crown", "gem_crown", "gem_table"]
    for mat, geo in lathe(profile, arc_len(profile), 8, mats).items():
        b.add(geo, M[mat], "root", "gem")


BUILDERS = {"ring_band": build_band, "ring_gem": build_gem}


def build(bake=True, tex_dir=None):
    """Returns {name: obj}, both from the band's origin; each obj gets obj["texture"] when baked."""
    if bpy.context.object and bpy.context.object.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")
    coll = common.clear_collection(COLL)
    M = materials()
    builders = {}
    for name, fn in BUILDERS.items():
        b = Builder()
        fn(b, M)
        builders[name] = b
    low = min(v.z for v in builders["ring_band"].verts)
    objs = {}
    out = tex_dir or bpy.app.tempdir or "/tmp"
    for name, b in builders.items():
        b.verts = [V(v) - V((0, 0, low)) for v in b.verts]
        obj = common.finish_mesh(b, coll, "item_" + name)
        obj.data.set_sharp_from_angle(angle=math.radians(25 if name == "ring_gem" else 50))
        common.uv_unwrap(obj, b.tags)
        if bake:
            if name == "ring_band":  # a texture per metal; it shows the first
                texs = []
                for stem, palette in METALS:
                    materials(palette)
                    texs.append(common.bake_texture(obj, os.path.join(out, stem + ".png"), TEX_SIZE, "item_" + stem, ao_distance=0.012))
                materials()
                tex = texs[0]
            else:
                tex = common.bake_texture(obj, os.path.join(out, name + ".png"), TEX_SIZE, "item_" + name, ao_distance=0.006)
            common.use_baked_material(obj, tex)
            obj["texture"] = tex.name
        objs[name] = obj
        vs = obj.data.vertices
        print("{}: {} verts, {} tris, z {:.4f}..{:.4f}".format(name, len(vs), common.tri_count(obj), min(v.co.z for v in vs), max(v.co.z for v in vs)))
    return objs


def export(objs, models_dir=None):
    models_dir = models_dir or os.path.join(REPO, "media", "models", "items")
    p = os.path.join(REPO, "tools", "blender", "md3_export.py")
    g = {"__file__": p, "__name__": "md3_export"}
    exec(open(p).read(), g)
    for name, obj in objs.items():
        g["export_md3"](obj, os.path.join(models_dir, name + ".md3"), 0, 0)


def review(path, objs):
    """Front and three-quarter views of the ring, the gem tinted ruby and see-through as the engine draws it."""
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    if scene.world is None:
        scene.world = bpy.data.worlds.new("World")
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.25, 0.22, 0.18, 1)
    scene.view_settings.view_transform = "Standard"
    sun = bpy.data.objects.new("review_sun", bpy.data.lights.new("review_sun", "SUN"))
    sun.data.energy = 3.0
    sun.rotation_euler = (math.radians(50), 0, math.radians(-30))
    scene.collection.objects.link(sun)
    cam = bpy.data.objects.new("review_cam", bpy.data.cameras.new("review_cam"))
    scene.collection.objects.link(cam)
    scene.camera = cam
    cam.data.type = "ORTHO"
    cam.data.ortho_scale = 0.034 * S
    scene.render.resolution_x, scene.render.resolution_y = 400, 400
    stem = os.path.splitext(path)[0]
    for suffix, rz in (("", 0), ("_34", 40)):
        for o in objs.values():
            o.rotation_euler = (0, 0, math.radians(-rz))
        cam.rotation_euler = (math.radians(80), 0, 0)
        d = V((0, 0, -1))
        d.rotate(cam.rotation_euler)
        cam.location = V((0, 0, 0.014 * S)) - d * 0.6
        scene.render.filepath = stem + suffix + ".png"
        bpy.ops.render.render(write_still=True)
        print("review ->", scene.render.filepath)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    exporting = "--export" in args
    tex_dir = os.path.join(REPO, "media", "textures", "items")
    objs = build(bake=exporting or "--bake" in args, tex_dir=tex_dir if exporting else None)
    if "--review" in args:
        review(args[args.index("--review") + 1], objs)
    if exporting:
        export(objs)
