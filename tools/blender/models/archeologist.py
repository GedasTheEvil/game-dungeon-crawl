"""Procedural player: an archaeologist in a fedora, leather jacket, satchel and whip.

    MCP:  p = ".../tools/blender/models/archeologist.py"; g = {"__file__": p, "__name__": "archeologist"}
          exec(open(p).read(), g); g["build"]()          # then g["export"]()
    CLI:  blender -b --python tools/blender/models/archeologist.py -- [--export]

Built like anubis.py: modelled facing +Y (right side +X), then the rig object is turned 180 degrees
so the exported model faces -Y (the player uses rotA = +-70 / -110 from the camera, forward = game +Z).

The player's files are used differently from monsters:
  archeologist.md3       idle (shown when standing still),
  archeologist_walk.md3  walk cycle (shown while moving; the weapon swing is drawn separately),
  archeologist_die.md3   death (plays once, holds the last frame),
  archeologist_jump.md3  forward jump (plays once from take-off, holds the landing crouch; the game moves the body),
  archeologist_climb.md3 climbing a ladder, back to the camera (rotA = 180). The game sets the frame from the height,
                         one cycle per tile, so it runs backwards going down and holds when the player stops (see
                         CLIMB_GRIP_Y).
The weapon is drawn floating in front of the chest at ~3/4 of the height, so the fists stay raised
there in every clip. Every frame the root is moved so the lowest point (the hat excluded) touches the floor.
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
from common import REPO, Builder, chain_weights, ellipsoid, loft, smoothstep, tube  # noqa: E402

COLL = "archeologist_new"
CLIPS = [("archeologist_idle", "", 32), ("archeologist_walk", "_walk", 20), ("archeologist_die", "_die", 30), ("archeologist_jump", "_jump", 10)]
CLIMB = ("archeologist_climb", "_climb", 24)
TEX_SIZE = 1024
REVIEW_VIEW = {"target": (0, 0, 0.98), "ortho": 2.4, "res": (360, 440)}

COL = {
    "skin": (0.50, 0.30, 0.19),
    "shirt": (0.55, 0.47, 0.30),
    "jacket": (0.19, 0.085, 0.035),
    "jacket_edge": (0.08, 0.035, 0.015),
    "pants": (0.38, 0.30, 0.18),
    "pants_dark": (0.30, 0.23, 0.13),
    "boot": (0.09, 0.045, 0.02),
    "boot_hi": (0.16, 0.08, 0.035),
    "leather": (0.12, 0.06, 0.025),
    "brass": (0.60, 0.45, 0.15),
    "felt": (0.24, 0.15, 0.08),
    "band": (0.05, 0.03, 0.02),
    "hair": (0.09, 0.05, 0.025),
    "canvas": (0.42, 0.36, 0.22),
    "canvas_dark": (0.30, 0.25, 0.15),
    "white": (0.85, 0.83, 0.78),
    "black": (0.01, 0.008, 0.006),
}

# Torso sections (z, rx, ry, y); also used to lay the satchel strap on the jacket.
TORSO = [(0.86, 0.13, 0.095, 0.0), (0.93, 0.162, 0.115, 0.0), (1.03, 0.158, 0.112, 0.0), (1.13, 0.162, 0.116, 0.005), (1.23, 0.176, 0.122, 0.01),
         (1.33, 0.19, 0.126, 0.01), (1.41, 0.196, 0.116, 0.0), (1.46, 0.15, 0.09, -0.005), (1.49, 0.07, 0.06, 0.0)]


def torso_at(z):
    for (z0, *a), (z1, *b) in zip(TORSO, TORSO[1:]):
        if z <= z1:
            t = smoothstep(z0, z1, z) if z > z0 else 0.0
            return [x + (y - x) * t for x, y in zip(a, b)]
    return TORSO[-1][1:]


# ---------------------------------------------------------------- materials


def jacket_material():
    """Leather jacket, open down the front (u = 0.25) over the khaki shirt; the V widens towards the collar."""
    mat = bpy.data.materials.get("archeologist_jacket") or bpy.data.materials.new("archeologist_jacket")
    nt = mat.node_tree
    for node in list(nt.nodes):
        if node.type not in ("OUTPUT_MATERIAL", "BSDF_PRINCIPLED"):
            nt.nodes.remove(node)
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Roughness"].default_value = 0.55
    new = nt.nodes.new
    uv = new("ShaderNodeUVMap")
    uv.uv_map = "pattern"
    sep = new("ShaderNodeSeparateXYZ")
    nt.links.new(uv.outputs["UV"], sep.inputs[0])
    off = new("ShaderNodeMath")
    off.operation = "SUBTRACT"
    off.inputs[1].default_value = 0.25
    ab = new("ShaderNodeMath")
    ab.operation = "ABSOLUTE"
    nt.links.new(sep.outputs["X"], off.inputs[0])
    nt.links.new(off.outputs[0], ab.inputs[0])
    width = new("ShaderNodeMapRange")  # arc length up the torso -> half width of the opening
    width.clamp = True
    width.inputs["From Min"].default_value = 0.30
    width.inputs["From Max"].default_value = 0.58
    width.inputs["To Min"].default_value = 0.022
    width.inputs["To Max"].default_value = 0.075
    nt.links.new(sep.outputs["Y"], width.inputs["Value"])
    edge_w = new("ShaderNodeMath")
    edge_w.operation = "ADD"
    edge_w.inputs[1].default_value = 0.012
    nt.links.new(width.outputs["Result"], edge_w.inputs[0])
    is_open = new("ShaderNodeMath")
    is_open.operation = "LESS_THAN"
    nt.links.new(ab.outputs[0], is_open.inputs[0])
    nt.links.new(width.outputs["Result"], is_open.inputs[1])
    is_edge = new("ShaderNodeMath")
    is_edge.operation = "LESS_THAN"
    nt.links.new(ab.outputs[0], is_edge.inputs[0])
    nt.links.new(edge_w.outputs[0], is_edge.inputs[1])
    mix1 = new("ShaderNodeMix")
    mix1.data_type = "RGBA"
    mix1.inputs["A"].default_value = (*COL["jacket"], 1)
    mix1.inputs["B"].default_value = (*COL["jacket_edge"], 1)
    nt.links.new(is_edge.outputs[0], mix1.inputs["Factor"])
    mix2 = new("ShaderNodeMix")
    mix2.data_type = "RGBA"
    mix2.inputs["B"].default_value = (*COL["shirt"], 1)
    nt.links.new(mix1.outputs["Result"], mix2.inputs["A"])
    nt.links.new(is_open.outputs[0], mix2.inputs["Factor"])
    nt.links.new(mix2.outputs["Result"], bsdf.inputs["Base Color"])
    return mat


def materials():
    stripes = lambda axis, period, bands: ("stripes", axis, period, bands)  # noqa: E731
    specs = {
        "skin": ("solid", "skin"),
        "shirt": ("solid", "shirt"),
        "sleeve": stripes("v", 1.0, [(1, "jacket")]),
        "cuff": ("solid", "jacket_edge"),
        "pants": stripes("u", 1.0, [(46, "pants"), (4, "pants_dark"), (46, "pants"), (4, "pants_dark")]),
        "boot": stripes("v", 0.025, [(3, "boot"), (1, "boot_hi")]),
        "sole": ("solid", "leather"),
        "leather": ("solid", "leather"),
        "brass": ("solid", "brass"),
        "felt": ("solid", "felt"),
        "band": ("solid", "band"),
        "hair": ("solid", "hair"),
        "canvas": ("solid", "canvas"),
        "canvas_dark": ("solid", "canvas_dark"),
        "white": ("solid", "white"),
        "black": ("solid", "black"),
    }
    M = {name: common.make_material("archeologist_" + name, spec, COL) for name, spec in specs.items()}
    M["jacket"] = jacket_material()
    return M


# ---------------------------------------------------------------- skeleton


def side_name(bone, s):
    return bone + (".R" if s > 0 else ".L")


def joints(s):
    return {
        "hip": V((0.095 * s, 0.0, 0.93)),
        "knee": V((0.097 * s, 0.01, 0.51)),
        "ankle": V((0.097 * s, -0.01, 0.09)),
        "toe": V((0.10 * s, 0.15, 0.03)),
        "shoulder": V((0.19 * s, -0.01, 1.43)),
        "elbow": V((0.235 * s, -0.03, 1.15)),
        "wrist": V((0.26 * s, -0.01, 0.905)),
        "hand_end": V((0.266 * s, -0.004, 0.80)),
    }


HEAD_DZ = -0.07  # build_head models the head higher up; it is moved down by this much
HAT_PIVOT = V((0, -0.005, 1.815 + HEAD_DZ))


def build_rig(coll):
    arm = bpy.data.armatures.new("archeologist_rig")
    rig = bpy.data.objects.new("archeologist_rig", arm)
    coll.objects.link(rig)
    arm.display_type = "STICK"
    rig.show_in_front = True
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    bpy.context.view_layer.objects.active = rig
    rig.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    eb = arm.edit_bones

    def bone(name, head, tail, parent=None, connect=False):
        b = eb.new(name)
        b.head, b.tail = V(head), V(tail)
        b.align_roll(V((1, 0, 0)).cross((b.tail - b.head).normalized()))
        if parent:
            b.parent = eb[parent]
            b.use_connect = connect

    bone("root", (0, 0, 0), (0, 0.3, 0))
    bone("pelvis", (0, 0, 0.95), (0, 0, 1.05), "root")
    bone("spine", (0, 0, 1.05), (0, 0, 1.25), "pelvis", True)
    bone("chest", (0, 0, 1.25), (0, 0, 1.47), "spine", True)
    bone("neck", (0, 0, 1.47), (0, 0.01, 1.56), "chest", True)
    bone("head", (0, 0.01, 1.56), (0, 0.0, 1.79), "neck", True)
    bone("hat", HAT_PIVOT, HAT_PIVOT + V((0, 0, 0.12)), "head")
    for s in (1, -1):
        j = joints(s)
        n = lambda b: side_name(b, s)  # noqa: E731
        bone(n("thigh"), j["hip"], j["knee"], "pelvis")
        bone(n("shin"), j["knee"], j["ankle"], n("thigh"), True)
        bone(n("foot"), j["ankle"], j["toe"], n("shin"), True)
        bone(n("upperarm"), j["shoulder"], j["elbow"], "chest")
        bone(n("forearm"), j["elbow"], j["wrist"], n("upperarm"), True)
        bone(n("hand"), j["wrist"], j["hand_end"], n("forearm"), True)
    bpy.ops.object.mode_set(mode="OBJECT")
    for pb in rig.pose.bones:
        pb.rotation_mode = "XYZ"
    rig.rotation_euler = (0, 0, math.pi)  # face -Y in the exported files
    return rig


TORSO_CHAIN = ([(0, 0, 0.80), (0, 0, 1.05), (0, 0, 1.25), (0, 0, 1.47), (0, 0.01, 1.56), (0, 0, 1.83)], ["pelvis", "spine", "chest", "neck", "head"])


# ---------------------------------------------------------------- body


def build_body(b, M):
    fwd = V((0, 1, 0))
    torso_w = chain_weights(*TORSO_CHAIN, 0.06)

    def torso_shape(th, a):
        front = max(0.0, math.sin(th))
        return 1.0 + 0.05 * a * front ** 2 - 0.02 * a * max(0.0, -math.sin(th))

    keys = [((0, y, z), rx, ry, 1.0 if 1.2 < z < 1.42 else 0.0) for z, rx, ry, y in TORSO]
    b.add(tube(keys, sub=2, n=20, ref=fwd, shape=torso_shape, caps=(True, True), bulge=(0.02, 0.02)), M["jacket"], torso_w)
    # Jacket skirt over the hips, open at the front like the torso.
    def hem_w(co):
        t = smoothstep(0.93, 0.78, co.z)
        side = min(max(co.x / 0.12, -1.0), 1.0)
        k = 0.6 * t * abs(side)
        return {"pelvis": 1 - k, ("thigh.R" if side > 0 else "thigh.L"): k}

    hem = [((0, 0.0, 1.0), 0.166, 0.12), ((0, 0.0, 0.92), 0.172, 0.128), ((0, 0.005, 0.84), 0.182, 0.138), ((0, 0.01, 0.78), 0.188, 0.144)]
    b.add(tube(hem, sub=2, n=20, ref=fwd, caps=(False, False)), M["jacket"], hem_w)
    # Collar: turned-up leather collar around the neck, open at the front.
    path, secs = [], []
    for i in range(26):
        a = math.radians(-60 - 300 * i / 25) + math.pi / 2  # skips the front
        path.append(V((0.085 * math.cos(a), 0.0 + 0.075 * math.sin(a), 1.475)))
        secs.append((0.03, 0.008, None, -0.9))
    b.add(loft(path, secs, n=8, ref=V((0, 0, 1))), M["cuff"], "chest")
    # Neck and shirt collar.
    neck = [((0, 0.0, 1.44), 0.064, 0.06), ((0, 0.005, 1.51), 0.058, 0.057), ((0, 0.015, 1.575), 0.054, 0.055)]
    b.add(tube(neck, sub=2, n=12, ref=fwd, caps=(False, False)),
          M["skin"], chain_weights([(0, 0, 1.4), (0, 0, 1.47), (0, 0.01, 1.56), (0, 0, 1.83)], ["chest", "neck", "head"], 0.05))
    b.add(tube([((0, 0.0, 1.46), 0.07, 0.065), ((0, 0.005, 1.5), 0.066, 0.063)], sub=1, n=12, ref=fwd, caps=(False, False)), M["shirt"], "neck")

    for s in (1, -1):
        j = joints(s)
        n = lambda bn: side_name(bn, s)  # noqa: E731
        x = lambda v: V((v[0] * s, v[1], v[2]))  # noqa: E731
        leg = [(x((0.09, 0.0, 0.98)), 0.075, 0.078), (x((0.098, 0.004, 0.88)), 0.082, 0.084), (x((0.098, 0.01, 0.72)), 0.07, 0.072),
               (x((0.097, 0.012, 0.56)), 0.058, 0.06), (x((0.097, 0.008, 0.48)), 0.056, 0.058), (x((0.097, -0.002, 0.40)), 0.055, 0.056)]
        b.add(tube(leg, sub=2, n=12, ref=fwd, caps=(False, False)), M["pants"],
              chain_weights([x((0.10, 0, 1.08)), j["hip"], j["knee"], j["ankle"]], ["pelvis", n("thigh"), n("shin")], 0.06))
        # Lace-up boot to mid calf.
        boot = [(x((0.097, -0.004, 0.36)), 0.054, 0.056), (x((0.097, -0.008, 0.25)), 0.047, 0.05), (x((0.097, -0.012, 0.13)), 0.043, 0.047),
                (x((0.097, -0.012, 0.08)), 0.046, 0.05)]
        b.add(tube(boot, sub=2, n=12, ref=fwd, caps=(False, False)), M["boot"], n("shin"))
        b.add(tube([(x((0.097, -0.004, 0.37)), 0.057, 0.059), (x((0.097, -0.004, 0.345)), 0.058, 0.06)], sub=1, n=12, caps=(False, False)),
              M["sole"], n("shin"))

        def sole(th, a):
            return 0.8 if math.sin(th) < 0 else 1.0

        foot = [(x((0.097, -0.075, 0.05)), 0.036, 0.04), (x((0.097, -0.045, 0.065)), 0.046, 0.058), (x((0.098, 0.03, 0.05)), 0.05, 0.045),
                (x((0.1, 0.11, 0.036)), 0.05, 0.032), (x((0.102, 0.17, 0.03)), 0.042, 0.026)]
        b.add(tube(foot, sub=2, n=10, ref=V((0, 0, 1)), shape=sole, bulge=(0.012, 0.014)), M["boot"],
              chain_weights([x((0.10, -0.012, 0.2)), j["ankle"], j["toe"]], [n("shin"), n("foot")], 0.03))
        b.add(tube([(x((0.097, -0.085, 0.012)), 0.04, 0.012), (x((0.098, 0.04, 0.012)), 0.053, 0.012), (x((0.102, 0.185, 0.014)), 0.043, 0.012)],
                   sub=2, n=10, ref=V((0, 0, 1)), bulge=(0.01, 0.01)), M["sole"],
              chain_weights([x((0.10, -0.012, 0.2)), j["ankle"], j["toe"]], [n("shin"), n("foot")], 0.03))

        arm = [(x((0.13, 0.0, 1.45)), 0.062, 0.064), (x((0.19, -0.01, 1.43)), 0.07, 0.07), (x((0.215, -0.02, 1.32)), 0.058, 0.06),
               (x((0.232, -0.028, 1.22)), 0.052, 0.054), (x((0.237, -0.03, 1.15)), 0.048, 0.05), (x((0.247, -0.022, 1.06)), 0.048, 0.048),
               (x((0.256, -0.014, 0.96)), 0.04, 0.04), (x((0.26, -0.01, 0.925)), 0.038, 0.038)]
        b.add(tube(arm, sub=2, n=12, ref=fwd, caps=(True, False)), M["sleeve"],
              chain_weights([x((0.12, 0, 1.44)), j["shoulder"], j["elbow"], j["wrist"], j["hand_end"]], ["chest", n("upperarm"), n("forearm"), n("hand")], 0.05))
        b.add(tube([(x((0.256, -0.013, 0.965)), 0.043, 0.043), (x((0.259, -0.011, 0.935)), 0.044, 0.044)], sub=1, n=12, caps=(False, False)),
              M["cuff"], n("forearm"))
        w, e = j["wrist"], j["hand_end"]
        d = (e - w).normalized()
        fist = [(w - d * 0.02, 0.026, 0.03), (w + d * 0.025, 0.032, 0.043), (w + d * 0.06, 0.036, 0.05), (w + d * 0.09, 0.034, 0.046), (w + d * 0.108, 0.022, 0.03)]
        b.add(tube(fist, sub=2, n=10, ref=fwd, bulge=(0, 0.006)), M["skin"], n("hand"))
        thumb = [(w + d * 0.03 + V((0, 0.032, 0)), 0.014, 0.014), (w + d * 0.06 + V((-0.004 * s, 0.047, 0)), 0.012, 0.012), (w + d * 0.08 + V((-0.008 * s, 0.049, 0)), 0.01, 0.01)]
        b.add(tube(thumb, sub=1, n=8, ref=V((1, 0, 0)), bulge=(0.004, 0.006)), M["skin"], n("hand"))


def build_gear(b, M):
    torso_w = chain_weights(*TORSO_CHAIN, 0.06)
    # Satchel strap: a tilted loop around the torso, over the right shoulder down to the left hip.
    path = []
    for i in range(36):
        phi = 2 * math.pi * i / 36
        z = 1.19 + 0.27 * math.cos(phi)
        rx, ry, y = torso_at(z)
        path.append(V(((rx + 0.014) * math.cos(phi), y + (ry + 0.016) * math.sin(phi), z)))

    def radial(i):
        p = path[i]
        return V((p.x, p.y, 0)).normalized()

    b.add(loft(path, [(0.024, 0.004)] * 36, n=6, ref=radial, closed=True), M["leather"], torso_w)
    # Satchel on the left hip: flat, rounded box with a flap and a brass buckle.
    def boxy(th):
        c, s = abs(math.cos(th)), abs(math.sin(th))
        return 1.0 / (c ** 4 + s ** 4) ** 0.25

    bag_c = V((-0.215, -0.02, 0.84))
    bag = []
    for y in (-0.11, -0.1, 0.1, 0.11):
        k = 0.8 if abs(y) > 0.105 else 1.0
        bag.append(V((bag_c.x, bag_c.y + y, bag_c.z)))
    secs = [(0.035 * k, 0.1 * k, boxy) for k in (0.75, 1.0, 1.0, 0.75)]
    b.add(loft(bag, secs, n=16, ref=V((0, 0, 1)), bulge=(0.004, 0.004)), M["canvas"], "pelvis")
    flap = [V((bag_c.x - 0.038, bag_c.y + y, bag_c.z + 0.05)) for y in (-0.105, 0.0, 0.105)]
    b.add(loft(flap, [(0.006, 0.055)] * 3, n=8, ref=V((0, 0, 1))), M["canvas_dark"], "pelvis")
    b.add(ellipsoid(V((bag_c.x - 0.046, bag_c.y, bag_c.z + 0.01)), (0.006, 0.018, 0.014), n=8, rings=4, axis=V((1, 0, 0)), ref=V((0, 0, 1))), M["brass"], "pelvis")
    # Coiled bullwhip hanging on the right hip.
    for k in range(3):
        c = V((0.205 + 0.007 * k, 0.03, 0.83 - 0.006 * k))
        loop = [c + V((0, 0.07 * math.cos(2 * math.pi * i / 16) * (1 - 0.06 * k), 0.08 * math.sin(2 * math.pi * i / 16))) for i in range(16)]
        b.add(loft(loop, [(0.011, 0.011)] * 16, n=6, ref=V((1, 0, 0)), closed=True), M["leather"], "pelvis")
    handle = [(V((0.21, 0.09, 0.9)), 0.014, 0.014), (V((0.21, 0.1, 0.78)), 0.016, 0.016)]
    b.add(tube(handle, sub=1, n=8, ref=V((1, 0, 0)), bulge=(0.004, 0.004)), M["band"], "pelvis")
    # Belt buckle showing in the open jacket.
    b.add(ellipsoid(V((0, 0.123, 0.955)), (0.028, 0.006, 0.02), n=10, rings=4, axis=V((0, 1, 0)), ref=V((0, 0, 1))), M["brass"], "pelvis")
    b.add(tube([((0, 0.117, 0.935), 0.05, 0.004), ((0, 0.117, 0.975), 0.05, 0.004)], sub=1, n=8, ref=V((0, 1, 0))), M["leather"], "pelvis")


def build_head(b, M):
    fwd = V((0, 1, 0))
    head = [((0, 0.04, 1.615), 0.022, 0.022), ((0, 0.035, 1.628), 0.05, 0.052), ((0, 0.02, 1.655), 0.07, 0.082), ((0, 0.01, 1.695), 0.08, 0.097),
            ((0, 0.0, 1.745), 0.086, 0.104), ((0, -0.005, 1.795), 0.087, 0.104), ((0, -0.01, 1.84), 0.075, 0.09), ((0, -0.01, 1.872), 0.045, 0.056),
            ((0, -0.01, 1.884), 0.012, 0.012)]
    b.add(tube(head, sub=2, n=16, ref=fwd, bulge=(0.004, 0.003)), M["skin"], "head")
    # Short hair: covers the crown (visible once the hat is gone), hairline above the forehead.
    b.add(ellipsoid((0, -0.02, 1.795), (0.093, 0.1, 0.097), n=16, rings=8), M["hair"], "head")
    # Nose, eyes, brows, ears, mouth.
    b.add(tube([((0, 0.098, 1.77), 0.012, 0.012), ((0, 0.116, 1.74), 0.016, 0.016), ((0, 0.12, 1.722), 0.018, 0.014)], sub=2, n=8, ref=V((0, 0, 1)),
               caps=(False, True), bulge=(0, 0.006)), M["skin"], "head")
    b.add(ellipsoid((0, 0.104, 1.68), (0.022, 0.004, 0.004), n=8, rings=3, axis=V((1, 0, 0))), M["black"], "head")
    for s in (1, -1):
        x = lambda v: V((v[0] * s, v[1], v[2]))  # noqa: E731
        b.add(ellipsoid(x((0.036, 0.096, 1.765)), (0.014, 0.008, 0.009), n=8, rings=4), M["white"], "head")
        b.add(ellipsoid(x((0.036, 0.103, 1.765)), (0.006, 0.004, 0.006), n=6, rings=3), M["black"], "head")
        b.add(tube([(x((0.018, 0.104, 1.787)), 0.006, 0.006), (x((0.04, 0.101, 1.792)), 0.007, 0.007), (x((0.058, 0.09, 1.787)), 0.005, 0.005)],
                   sub=2, n=6, ref=V((0, 1, 0))), M["hair"], "head")
        b.add(ellipsoid(x((0.088, -0.005, 1.748)), (0.012, 0.024, 0.032), n=8, rings=4), M["skin"], "head")


def build_hat(b, M):
    """Fedora: pinched teardrop crown with a dented top, dark band, brim up at the sides."""
    c = HAT_PIVOT

    def crown_shape(th, a):
        return 1.0 - 0.22 * a * max(0.0, math.sin(th)) ** 3

    crown = [(c + V((0, 0, -0.005)), 0.1, 0.117, 0.0), (c + V((0, 0, 0.045)), 0.102, 0.12, 0.3), (c + V((0, -0.002, 0.1)), 0.097, 0.115, 0.8),
             (c + V((0, -0.004, 0.13)), 0.085, 0.102, 1.0)]
    b.add(tube(crown, sub=2, n=20, ref=V((0, 1, 0)), shape=crown_shape, caps=(False, True), bulge=(0, -0.035)), M["felt"], "hat", "head")
    band = [(c + V((0, 0, 0.0)), 0.104, 0.121), (c + V((0, 0, 0.03)), 0.105, 0.122)]
    b.add(tube(band, sub=1, n=20, ref=V((0, 1, 0)), caps=(False, False)), M["band"], "hat", "head")
    # Brim: annulus between the crown base and an outer oval.
    n, rings = 36, [0.0, 0.5, 1.0]
    verts, faces, fuv, idx = [], [], [], {}

    def brim_point(th, p, top):
        rx = 0.1 + (0.19 - 0.1) * p
        ry = 0.117 + (0.215 - 0.117) * p
        z = 0.028 * math.cos(th) ** 2 * p * p - 0.016 * math.sin(th) ** 2 * p * p + (0.004 if top else -0.004)
        return c + V((rx * math.cos(th), ry * math.sin(th), z - 0.004))

    for side in ("top", "bot"):
        for i, p in enumerate(rings):
            for j in range(n):
                idx[side, i, j] = len(verts)
                verts.append(brim_point(2 * math.pi * j / n, p, side == "top"))

    def add(face, expected):
        a, bb, cc = (verts[k] for k in face[:3])
        faces.append(face if (bb - a).cross(cc - a).dot(expected) >= 0 else face[::-1])
        fuv.append([(0.1, 0.1)] * len(face))

    for j in range(n):
        j2 = (j + 1) % n
        for i in range(len(rings) - 1):
            add([idx["top", i, j], idx["top", i, j2], idx["top", i + 1, j2], idx["top", i + 1, j]], V((0, 0, 1)))
            add([idx["bot", i, j], idx["bot", i, j2], idx["bot", i + 1, j2], idx["bot", i + 1, j]], V((0, 0, -1)))
        th = 2 * math.pi * (j + 0.5) / n
        last = len(rings) - 1
        add([idx["top", last, j], idx["top", last, j2], idx["bot", last, j2], idx["bot", last, j]], V((math.cos(th), math.sin(th), 0)))
    b.add((verts, faces, fuv), M["felt"], "hat", "head")


def hat_indices(b):
    return [i for i, t in enumerate(b.tags) if t == "head" and b.weights[i].get("hat")]


# ---------------------------------------------------------------- animation
# Pose = {bone: (rx, ry, rz) degrees}, plus "root_loc": (x, y, z) (z is replaced by the floor fix).
# Down-pointing limbs: +rx swings forward, +rz swings towards -X. Up-pointing bones: +rx leans back.

READY = {
    "upperarm.R": (38, 0, 26), "forearm.R": (96, 0, 0), "hand.R": (-10, 0, 0),
    "upperarm.L": (38, 0, -26), "forearm.L": (96, 0, 0), "hand.L": (-10, 0, 0),
    "chest": (2, 0, 0), "head": (-2, 0, 0),
}


def pose(base=READY, **bones):
    p = {k: tuple(v) for k, v in base.items()}
    for k, v in bones.items():
        p[k.replace("_R", ".R").replace("_L", ".L")] = v
    return p


def add(p, **bones):
    """Add angles on top of pose p."""
    q = dict(p)
    for k, v in bones.items():
        k = k.replace("_R", ".R").replace("_L", ".L")
        q[k] = tuple(a + b for a, b in zip(q.get(k, (0, 0, 0)), v))
    return q


def set_pose(rig, p):
    for pb in rig.pose.bones:
        pb.rotation_euler = [math.radians(a) for a in p.get(pb.name, (0, 0, 0))]
        pb.location = p.get("root_loc", (0, 0, 0)) if pb.name == "root" else (0, 0, 0)


def key_pose(rig, p, frame):
    set_pose(rig, p)
    for pb in rig.pose.bones:
        pb.keyframe_insert("rotation_euler", frame=frame)
        if pb.name in ("root", "hat"):
            pb.keyframe_insert("location", frame=frame)


def idle_poses():
    frames = CLIPS[0][2]
    out = []
    for f in range(0, frames + 1, 2):
        w = 2 * math.pi * f / frames
        breath = math.sin(2 * w)
        shift = math.sin(w)
        p = add(pose(),
                pelvis=(0, 0, 2.5 * shift), spine=(0, 0, -1.2 * shift), chest=(1.2 * breath, 0, -1.3 * shift),
                head=(1.5 * math.sin(w + 1), 9 * math.sin(w - 0.6), 0),
                thigh_R=(0, 0, -2.5 * shift), thigh_L=(0, 0, -2.5 * shift),
                upperarm_R=(2 * breath, 0, 0), upperarm_L=(2 * breath, 0, 0), forearm_R=(-2 * breath, 0, 0), forearm_L=(-2 * breath, 0, 0))
        out.append((f, p))
    return out


def walk_poses():
    frames = CLIPS[1][2]

    def step(t):
        """Leg for phase t (0 = heel strike): thigh forward -> back through the stance, knee bends in swing."""
        th = 24 * math.cos(2 * math.pi * t)
        swing = max(0.0, math.sin(2 * math.pi * (t - 0.5)))
        knee = -5 - 55 * swing
        foot = 10 * math.cos(2 * math.pi * t) - 12 * swing
        return th, knee, foot

    out = []
    for f in range(0, frames + 1, 2):
        t = f / frames + 0.25  # frame 0 = passing pose, close to the idle stance
        thL, knL, ftL = step(t)
        thR, knR, ftR = step(t + 0.5)
        c = math.cos(2 * math.pi * t)
        p = add(pose(),
                thigh_L=(thL, 0, 0), shin_L=(knL, 0, 0), foot_L=(ftL, 0, 0),
                thigh_R=(thR, 0, 0), shin_R=(knR, 0, 0), foot_R=(ftR, 0, 0),
                pelvis=(-4, 7 * c, 2 * c), spine=(-2, -3 * c, 0), chest=(-2, -4 * c, -2 * c), head=(2, 0, 0),
                upperarm_R=(4 * c, 0, 0), upperarm_L=(-4 * c, 0, 0))
        out.append((f, p))
    return out


def die_poses():
    ready = pose()
    hit = add(pose(), chest=(14, 0, 0), spine=(8, 0, 0), neck=(10, 0, 0), head=(16, 0, 0),
              upperarm_R=(50, 0, -40), upperarm_L=(50, 0, 40), forearm_R=(-50, 0, 0), forearm_L=(-50, 0, 0),
              thigh_R=(8, 0, 0), shin_R=(-8, 0, 0))
    hit["root_loc"] = (0, -0.07, 0)
    stagger = pose(chest=(-8, 10, 0), spine=(-6, 0, 0), neck=(-6, 0, 0), head=(-10, -10, 0),
                   upperarm_R=(20, 0, -15), forearm_R=(35, 0, 0), upperarm_L=(10, 0, 18), forearm_L=(30, 0, 0),
                   thigh_L=(30, 0, 0), shin_L=(-45, 0, 0), foot_L=(10, 0, 0), thigh_R=(20, 0, 0), shin_R=(-40, 0, 0), foot_R=(15, 0, 0))
    stagger["root_loc"] = (0, -0.04, 0)
    kneel = pose(pelvis=(-8, 0, 0), spine=(-8, 0, 0), chest=(-10, 0, 0), neck=(-8, 0, 0), head=(-12, 12, 0),
                 upperarm_R=(15, 0, -12), forearm_R=(25, 0, 0), upperarm_L=(8, 0, 14), forearm_L=(20, 0, 0),
                 thigh_L=(10, 0, 0), shin_L=(-100, 0, 0), foot_L=(-35, 0, 0), thigh_R=(8, 0, 0), shin_R=(-96, 0, 0), foot_R=(-35, 0, 0))
    kneel["root_loc"] = (0, 0.02, 0)
    topple = pose(pelvis=(-55, 0, 0), spine=(-10, 0, 0), chest=(-8, 0, 0), neck=(15, 0, 0), head=(20, 25, 0),
                  upperarm_R=(120, 0, -25), forearm_R=(20, 0, 0), upperarm_L=(110, 0, 25), forearm_L=(25, 0, 0),
                  thigh_L=(55, 0, 0), shin_L=(-90, 0, 0), foot_L=(-40, 0, 0), thigh_R=(50, 0, 0), shin_R=(-85, 0, 0), foot_R=(-40, 0, 0))
    topple["root_loc"] = (0, 0.22, 0)
    down = pose(pelvis=(-90, 0, 0), spine=(-2, 0, 0), chest=(0, 0, 0), neck=(22, 0, 0), head=(18, 70, 0),
                upperarm_R=(160, 0, -40), forearm_R=(20, 0, 0), hand_R=(-40, 0, 0), upperarm_L=(145, 0, 45), forearm_L=(35, 0, 0),
                thigh_L=(4, 0, 0), shin_L=(-10, 0, 0), foot_L=(-70, 0, 0), thigh_R=(12, 0, 0), shin_R=(-30, 0, 0), foot_R=(-60, 0, 0))
    down["root_loc"] = (0, 0.62, 0)
    settle = pose(down, pelvis=(-91, 0, 0), chest=(2, 0, 0), shin_R=(-34, 0, 0))
    settle["root_loc"] = (0, 0.64, 0)
    last = CLIPS[2][2] - 1
    return [(0, ready), (4, hit), (8, stagger), (12, kneel), (17, topple), (22, down), (last, settle)]


def jump_poses():
    """Push-off, knees tucked over the obstacle, legs reaching forward, landing crouch (~0.7 s of air time)."""
    push = add(pose(), pelvis=(-10, 0, 0), spine=(-4, 0, 0), chest=(-4, 0, 0), head=(6, 0, 0),
               thigh_R=(50, 0, 0), shin_R=(-80, 0, 0), foot_R=(-15, 0, 0),
               thigh_L=(-22, 0, 0), shin_L=(-12, 0, 0), foot_L=(-40, 0, 0),
               upperarm_R=(16, 0, 0), upperarm_L=(16, 0, 0))
    rise = add(pose(), pelvis=(-12, 0, 0), spine=(-6, 0, 0), chest=(-4, 0, 0), head=(10, 0, 0),
               thigh_R=(78, 0, 0), shin_R=(-110, 0, 0), foot_R=(-20, 0, 0),
               thigh_L=(40, 0, 0), shin_L=(-100, 0, 0), foot_L=(-30, 0, 0),
               upperarm_R=(20, 0, 0), upperarm_L=(20, 0, 0))
    tuck = add(pose(), pelvis=(-14, 0, 0), spine=(-6, 0, 0), chest=(-4, 0, 0), head=(12, 0, 0),
               thigh_R=(88, 0, 0), shin_R=(-120, 0, 0), foot_R=(-15, 0, 0),
               thigh_L=(80, 0, 0), shin_L=(-118, 0, 0), foot_L=(-15, 0, 0),
               upperarm_R=(14, 0, 0), upperarm_L=(14, 0, 0))
    reach = add(pose(), pelvis=(-6, 0, 0), spine=(-2, 0, 0), head=(6, 0, 0),
                thigh_R=(58, 0, 0), shin_R=(-45, 0, 0), foot_R=(8, 0, 0),
                thigh_L=(40, 0, 0), shin_L=(-62, 0, 0), foot_L=(5, 0, 0),
                upperarm_R=(8, 0, 0), upperarm_L=(8, 0, 0))
    touch = add(pose(), pelvis=(-4, 0, 0), head=(4, 0, 0),
                thigh_R=(38, 0, 0), shin_R=(-22, 0, 0), foot_R=(12, 0, 0),
                thigh_L=(24, 0, 0), shin_L=(-34, 0, 0), foot_L=(10, 0, 0))
    crouch = add(pose(), pelvis=(-16, 0, 0), spine=(-6, 0, 0), chest=(-4, 0, 0), head=(12, 0, 0),
                 thigh_R=(58, 0, 0), shin_R=(-80, 0, 0), foot_R=(20, 0, 0),
                 thigh_L=(46, 0, 0), shin_L=(-78, 0, 0), foot_L=(28, 0, 0),
                 upperarm_R=(-6, 0, 0), upperarm_L=(-6, 0, 0))
    return [(0, push), (2, rise), (4, tuck), (6, reach), (8, touch), (9, crouch)]


# Climbing: hands and feet driven by IK targets (empties parented to the rig, rig space before its 180 degree turn,
# so the ladder is at +Y). Diagonal gait: the left hand reaches up with the right foot while the other pair pulls.
# The player climbs ~1.5 tiles (~19 rungs) a second, so the limbs cannot stay on the rungs; one cycle spans a tile.
CLIMB_GRIP_Y = 0.28  # the fists close round the rungs this far in front (m); CLIMB_DEPTH in the engine matches it
CLIMB_HAND = {"x": 0.28, "low": 1.38, "high": 1.86, "lift": 0.07}  # fists clear of the hat brim seen from behind
CLIMB_FOOT = {"x": 0.13, "y": CLIMB_GRIP_Y - 0.11, "low": 0.02, "high": 0.42, "lift": 0.09}
# IK bone: (target, pole position, pole angle): elbows out to the sides, knees forward.
IK_CHAINS = {"forearm": ("hand", (0.75, -0.25, 1.0), -90), "shin": ("foot", (0.3, 0.9, 0.75), 90)}


def ease(t):
    return t * t * (3 - 2 * t)


def stroke(u, low, high):
    """Height and lift (0..1) of a limb at cycle phase u: pull down (0..0.5), then reach up clear of the ladder."""
    u %= 1.0
    if u < 0.5:
        return high + (low - high) * ease(u / 0.5), 0.0
    t = (u - 0.5) / 0.5
    return low + (high - low) * ease(t), math.sin(math.pi * t)


def build_ik(rig, coll):
    """IK targets and poles for the climb; every action keys the IK influence (1 in the climb, 0 elsewhere)."""
    for s in (1, -1):
        for bone, (target, pole, pole_angle) in IK_CHAINS.items():
            empties = []
            for kind in ("target", "pole"):
                name = "ik_{}_{}".format(side_name(target if kind == "target" else bone, s), kind)
                e = bpy.data.objects.get(name) or bpy.data.objects.new(name, None)
                if e.name not in coll.objects:
                    coll.objects.link(e)
                e.parent = rig
                e.empty_display_size = 0.05
                empties.append(e)
            empties[1].location = (pole[0] * s, pole[1], pole[2])
            pb = rig.pose.bones[side_name(bone, s)]
            con = pb.constraints.get("IK") or pb.constraints.new("IK")
            con.target, con.pole_target = empties
            con.pole_angle = math.radians(pole_angle)
            con.chain_count = 2
            con.use_tail = True


def key_ik(rig, on):
    for pb in rig.pose.bones:
        for con in pb.constraints:
            if con.type == "IK":
                con.influence = 1.0 if on else 0.0
                con.keyframe_insert("influence", frame=0)


def climb_targets(p):
    """Rig-space wrist and ankle targets at cycle phase p."""
    out = {}
    for s, phase in ((-1, 0.5), (1, 0.0)):  # left hand reaches first (u = 0.5 at p = 0)
        z, lift = stroke(p + phase, CLIMB_HAND["low"], CLIMB_HAND["high"])
        out["hand" + (".R" if s > 0 else ".L")] = V((CLIMB_HAND["x"] * s, CLIMB_GRIP_Y - CLIMB_HAND["lift"] * lift, z))
        z, lift = stroke(p + phase + 0.5, CLIMB_FOOT["low"], CLIMB_FOOT["high"])  # opposite foot with the hand
        out["foot" + (".R" if s > 0 else ".L")] = V((CLIMB_FOOT["x"] * s, CLIMB_FOOT["y"] - CLIMB_FOOT["lift"] * lift, z))
    return out


def climb_action(rig, name, frames):
    """FK part of the climb (trunk sway, head following the reaching hand) and the keyed IK targets."""
    for f in range(frames + 1):
        p = f / frames
        w = 2 * math.pi * p
        sway = math.sin(w)
        q = pose({}, pelvis=(4, 0, 3 * sway), spine=(-2, 0, -1.5 * sway), chest=(-2, 0, -2 * sway),
                 neck=(-6, 0, 0), head=(-10, -10 * math.cos(w), 0), foot_L=(-12, 0, 0), foot_R=(-12, 0, 0),
                 hand_L=(-20, 0, 0), hand_R=(-20, 0, 0))
        q["root_loc"] = (0.02 * sway, 0.0, -0.03 + 0.02 * math.cos(2 * w))
        key_pose(rig, q, f)
        for bone, pos in climb_targets(p % 1.0).items():
            e = bpy.data.objects["ik_{}_target".format(bone)]
            e.location = pos
            e.keyframe_insert("location", frame=f)


def make_cyclic(rig, act):
    from bpy_extras import anim_utils

    bag = anim_utils.action_get_channelbag_for_slot(act, rig.animation_data.action_slot)
    for fc in bag.fcurves:
        fc.modifiers.new("CYCLES")
        fc.update()


def lowest(obj, skip):
    ev = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mw = ev.matrix_world
    verts = ev.data.vertices
    return min((mw @ verts[i].co).z for i in range(len(verts)) if i not in skip)


def floor_fix(rig, obj, frames, skip):
    """Key the root height every frame so the lowest point touches the floor."""
    scene = bpy.context.scene
    root = rig.pose.bones["root"]
    for f in range(frames):
        scene.frame_set(f)
        root.location.z -= lowest(obj, skip)
        root.keyframe_insert("location", frame=f)


def jump_height(rig, obj, idle, act, frames, skip):
    """In the air the pelvis stays at standing height (the game lifts the whole body); the push-off
    and the landing crouch touch the floor."""
    scene = bpy.context.scene
    root = rig.pose.bones["root"]
    rig.animation_data.action = idle
    scene.frame_set(0)
    standing = root.location.z
    rig.animation_data.action = act
    for f in range(frames):
        scene.frame_set(f)
        root.location.z = standing
        if f in (0, frames - 1):  # push-off toe and landing crouch on the floor
            bpy.context.view_layer.update()
            root.location.z -= lowest(obj, skip)
        root.keyframe_insert("location", frame=f)


def drop_hat(rig, obj, hat_verts, start=13, land=20):
    """The fedora flies off as he pitches forward and lands crown-up in front of him."""
    scene = bpy.context.scene
    pb = rig.pose.bones["hat"]
    last = CLIPS[2][2] - 1
    scene.frame_set(start)
    held = pb.matrix.copy()
    scene.frame_set(last)
    head = rig.pose.bones["head"]
    head_pos = head.matrix @ V((0, head.bone.length * 0.5, 0))
    rest = pb.bone.matrix_local
    zmin = min(obj.data.vertices[i].co.z for i in hat_verts) - rest.translation.z
    lying = (Matrix.Rotation(math.radians(35), 4, "Z") @ Matrix.Rotation(math.radians(-8), 4, "X")) @ rest.to_3x3().to_4x4()
    lying.translation = V((head_pos.x + 0.22, head_pos.y + 0.26, -zmin + 0.004))
    targets = {}
    for f in range(start, last + 1):
        t = min(1.0, (f - start) / (land - start))
        rot = held.to_quaternion().slerp(lying.to_quaternion(), smoothstep(0, 1, t)).to_matrix().to_4x4()
        pos = held.translation.lerp(lying.translation, t) + V((0, 0, 0.25 * math.sin(math.pi * t) * (1 - 0.3 * t)))
        if land < f <= land + 4:  # small bounce
            pos.z += 0.035 * math.sin(math.pi * (f - land) / 4)
        rot.translation = pos
        targets[f] = rot
    for f in sorted(targets):
        scene.frame_set(f)
        pb.matrix = targets[f]
        bpy.context.view_layer.update()
        low = lowest(obj, set(range(len(obj.data.vertices))) - set(hat_verts))
        if low < 0:
            m = targets[f].copy()
            m.translation.z -= low
            pb.matrix = m
            bpy.context.view_layer.update()
        pb.keyframe_insert("rotation_euler", frame=f)
        pb.keyframe_insert("location", frame=f)


def make_actions(rig, obj, hat_verts):
    rig.animation_data_create()
    acts = {}
    skip = set(hat_verts)
    for (name, _, frames), poses in zip(CLIPS, (idle_poses(), walk_poses(), die_poses(), jump_poses())):
        act = bpy.data.actions.get(name)
        if act:
            bpy.data.actions.remove(act)
        act = bpy.data.actions.new(name)
        act.use_fake_user = True
        rig.animation_data.action = act
        key_ik(rig, False)
        for f, p in poses:
            key_pose(rig, p, f)
        if name == "archeologist_jump":
            jump_height(rig, obj, acts["archeologist_idle"], act, frames, skip)
            acts[name] = act
            continue
        if name != "archeologist_die":
            make_cyclic(rig, act)
        floor_fix(rig, obj, frames + (1 if name != "archeologist_die" else 0), skip)
        if name == "archeologist_die":
            drop_hat(rig, obj, hat_verts)
        acts[name] = act
    name, _, frames = CLIMB
    act = bpy.data.actions.get(name)
    if act:
        bpy.data.actions.remove(act)
    act = bpy.data.actions.new(name)
    act.use_fake_user = True
    rig.animation_data.action = act
    key_ik(rig, True)
    climb_action(rig, name, frames)
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
    build_body(b, M)
    build_gear(b, M)
    first = len(b.verts)
    build_head(b, M)
    b.tags[first:] = ["head"] * (len(b.verts) - first)
    b.verts[first:] = [V(v) + V((0, 0, HEAD_DZ)) for v in b.verts[first:]]
    build_hat(b, M)
    hat_verts = hat_indices(b)
    obj = common.finish_mesh(b, coll, "archeologist_new")
    common.uv_unwrap(obj, b.tags, {"head": ((0, 0.02, 1.69), 1.8)})
    if bake:
        tex = common.bake_texture(obj, tex_path or os.path.join(bpy.app.tempdir or "/tmp", "archeologist_preview.png"), TEX_SIZE, "archeologist")
        common.use_baked_material(obj, tex)
    common.rig_object(obj, rig)
    build_ik(rig, coll)
    acts = make_actions(rig, obj, hat_verts)
    rig.animation_data.action = acts["archeologist_idle"]
    scene = bpy.context.scene
    scene.frame_start, scene.frame_end = 0, CLIPS[0][2] - 1
    scene.frame_set(0)
    print("archeologist: {} verts, {} tris".format(len(obj.data.vertices), common.tri_count(obj)))
    return obj, rig


def export(models_dir=None):
    common.export_files(bpy.data.objects["archeologist_new"], bpy.data.objects["archeologist_rig"], "archeologist", CLIPS + [CLIMB], "characters", models_dir)


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    build(tex_path=os.path.join(REPO, "textures", "characters", "archeologist.png") if "--export" in args else None)
    if "--export" in args:
        export()
        bpy.ops.wm.save_as_mainfile(filepath=os.path.join(HERE, "archeologist.blend"))
