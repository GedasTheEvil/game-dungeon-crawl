"""Procedural mummy priest (in game "High Priest of Anubis", the mummy boss): a mummified high priest in dark,
resin-stained linen, a leopard skin over the left shoulder, a gold scarab on the chest, gilded finger stalls, a shaved
priest's skull under tight wrappings and a long ebony staff crowned with a gold ankh round an amethyst. Built on
mummy.py (class Mummy): the same body, rig, strips, walk legs, die, idle, rise and climb, with his own head, staff,
arm poses, texture and a larger coffin (decor.py `coffin_large`).

    MCP:  p = ".../tools/blender/models/mummy_priest.py"; g = {"__file__": p, "__name__": "mummy_priest"}
          exec(open(p).read(), g); g["build"](bake=False)      # then g["export"]()
    CLI:  blender -b --python tools/blender/models/mummy_priest.py -- [--export] [--bake] [--export-climb]
          [--coffin-check out_dir] [--measure]
          (--measure prints the staff head per attack frame: the ranged attack's release, RangedRules)

Space as mummy.py: Z up, facing +Y (game: towards the camera, drawn at rotA 180), right side +X (game -X).
Clips (ENTOMBED_CLIPS, as the mummy's):
  mummy_priest.md3        walk loop 24 (the reference), the staff upright in the right fist, the left hand reaching,
  mummy_priest_att.md3    attack loop 26: the staff swung up over the head and down until the ankh points at the
                          player, the left hand thrust forward. It is the melee clip (looped) and the ranged clip
                          (played once, RANGED_CLIPS): the bolt leaves the ankh at the release frame (--measure),
  mummy_priest_die.md3    die 30, the mummy's collapse; the staff tips out of the hand and falls to his right,
  mummy_priest_idle.md3   idle 32, dormant on his back in the large coffin, arms crossed, the staff clutched in the
                          right fist along the body, its ankh past the head; centred, the back IDLE_BACK world units up,
  mummy_priest_rise.md3   rise 26, the mummy's climb out of the coffin, the staff taken upright (its foot on the coffin
                          floor, then on the floor); last frame = walk frame 0. The coffin depth is the mummy's
                          (TOMB_DEPTH 14.4 = MUMMY_COFFIN_DEPTH), the engine slide as the mummy's,
  mummy_priest_climb.md3  climb 24 (loops), the mummy's; the staff slung across the back.
KINDS: scale 20.5 (walk frame 0's largest dimension is its height, 1.86 m, the staff below the crown: the body 2.5
world units, a head, taller than the mummy at 18), riseMs as the mummy's (the same rise keys and frame count),
climbRise 0.650, climbGrip 0.118 (printed by the build), the ranged release (--measure): frame 12 of 26, release
12 / 25, releaseY 0.52, releaseAhead 3.0 (the staff slides forward through the fist as it swings, STAFF slide). The
dormant body's centre and depth are the mummy's: MUMMY_COFFIN_DEPTH (TOMB_DEPTH 14.4) works for both.
Texture media/textures/monsters/mummy_priest.png (1024); sounds tools/audio/mummy_priest_sounds.py.
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
import mummy  # noqa: E402

importlib.reload(common)
importlib.reload(mummy)
from common import ellipsoid, loft, smoothstep, transform, tube  # noqa: E402
from mummy import TILE, TORSO, TORSO_CHAIN, add, band, chain_weights, joints, pose, set_pose, side_name  # noqa: E402

COL = dict(mummy.COL, **{
    "linen": (0.30, 0.215, 0.12),
    "linen_hi": (0.43, 0.33, 0.19),
    "linen_dark": (0.09, 0.055, 0.025),
    "stain": (0.15, 0.07, 0.015),  # resin
    "skin": (0.06, 0.035, 0.02),
    "eye": (0.62, 0.22, 1.0),  # violet, the colour of his bolt
    "gold": (0.80, 0.55, 0.12),
    "gold_dark": (0.42, 0.25, 0.05),
    "lapis": (0.03, 0.07, 0.38),
    "fur": (0.62, 0.40, 0.13),
    "fur_spot": (0.035, 0.022, 0.012),
    "ebony": (0.035, 0.022, 0.016),
    "amethyst": (0.42, 0.08, 0.72),
})

# Leopard-skin sash: a ring round the torso in a plane rising from the right hip to the left shoulder.
SASH = {"hip": (0.15, 0.97), "shoulder": (-0.16, 1.47), "width": 0.075, "thick": 0.006, "lift": 0.016}
# Amulet: a gold cord round the neck (lower in front) with a lapis scarab in a gold setting on the sternum.
CORD = {"back": 1.50, "front": 1.36, "thick": 0.0045, "lift": 0.012}
STAFF_LEN = {"bottom": 0.05, "shaft_top": 1.63}  # held upright in the ready pose (metres over the floor)


def spotted_material(name, pal):
    """Leopard fur: dark rosette spots (Voronoi cells) on tawny, darkened by the resin like the linen."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nt = mat.node_tree
    for node in list(nt.nodes):
        if node.type not in ("OUTPUT_MATERIAL", "BSDF_PRINCIPLED"):
            nt.nodes.remove(node)
    bsdf = nt.nodes["Principled BSDF"]
    bsdf.inputs["Roughness"].default_value = 0.9
    new, link = nt.nodes.new, nt.links.new
    coord = new("ShaderNodeTexCoord")
    vor = new("ShaderNodeTexVoronoi")
    vor.inputs["Scale"].default_value = 34.0
    link(coord.outputs["Object"], vor.inputs["Vector"])
    edge = new("ShaderNodeTexVoronoi")
    edge.feature = "DISTANCE_TO_EDGE"
    edge.inputs["Scale"].default_value = 34.0
    link(coord.outputs["Object"], edge.inputs["Vector"])
    spot = new("ShaderNodeMapRange")  # rosette ring: dark near the cell edges, a lighter centre
    spot.inputs["From Min"].default_value = 0.05
    spot.inputs["From Max"].default_value = 0.11
    link(edge.outputs["Distance"], spot.inputs["Value"])
    mix = new("ShaderNodeMix")
    mix.data_type = "RGBA"
    mix.inputs["A"].default_value = (*pal["fur_spot"], 1)
    mix.inputs["B"].default_value = (*pal["fur"], 1)
    link(spot.outputs["Result"], mix.inputs["Factor"])
    noise = new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = 4.0
    link(coord.outputs["Object"], noise.inputs["Vector"])
    dark = new("ShaderNodeMapRange")
    dark.inputs["From Min"].default_value = 0.35
    dark.inputs["From Max"].default_value = 0.65
    dark.inputs["To Min"].default_value = 0.45
    link(noise.outputs["Fac"], dark.inputs["Value"])
    mul = new("ShaderNodeMix")
    mul.data_type = "RGBA"
    mul.blend_type = "MULTIPLY"
    mul.inputs["Factor"].default_value = 1.0
    link(mix.outputs["Result"], mul.inputs["A"])
    link(dark.outputs["Result"], mul.inputs["B"])
    link(mul.outputs["Result"], bsdf.inputs["Base Color"])
    return mat


def torso_at(z):
    """Torso section (y centre, rx, ry) at height z, linear between TORSO's sections."""
    secs = TORSO
    if z <= secs[0][0][2]:
        c, rx, ry = secs[0]
        return c[1], rx, ry
    for (c0, rx0, ry0), (c1, rx1, ry1) in zip(secs, secs[1:]):
        if c0[2] <= z <= c1[2]:
            t = (z - c0[2]) / (c1[2] - c0[2])
            return c0[1] + (c1[1] - c0[1]) * t, rx0 + (rx1 - rx0) * t, ry0 + (ry1 - ry0) * t
    c, rx, ry = secs[-1]
    return c[1], rx, ry


def torso_ring(plane, lift, n=48):
    """Closed path on the torso surface (lifted by lift) where it meets the surface z = plane(x, y); returns the
    points and the outward normals."""
    pts, nrms = [], []
    for k in range(n):
        a = 2 * math.pi * k / n
        z = plane(0.1 * math.cos(a), 0.1 * math.sin(a))
        for _ in range(8):
            yc, rx, ry = torso_at(z)
            x, y = rx * math.cos(a), yc + ry * math.sin(a)
            z = plane(x, y)
        nrm = V((math.cos(a) / rx, math.sin(a) / ry, 0)).normalized()
        pts.append(V((x, y, z)) + nrm * lift)
        nrms.append(nrm)
    return pts, nrms


class MummyPriest(mummy.Mummy):
    name = "mummy_priest"
    clip_list = [("walk", "", 24), ("attack", "_att", 26), ("die", "_die", 30), ("idle", "_idle", 32), ("rise", "_rise", 26), ("climb", "_climb", 24)]
    review_view = {"target": (0, 0.1, 0.95), "ortho": 2.6, "res": (360, 420)}
    scale = 20.5  # KINDS scale: the body (its walk frame 0 height) a head taller than the mummy's 18
    # decor.py coffin_large interior around the dormant body centre (tile units), like the mummy's coffin.
    coffin = {"half_x": 0.33, "half_y": 0.09, "wall": 0.018, "floor": 0.012, "rim": 0.09}
    coffin_prop = "coffin_large"
    col = COL
    strips = mummy.STRIPS[:3] + [
        ("strip_head", "head", (-0.085, 0.03, 1.64), (0.07, 0.065)),
        ("strip_paw", "pelvis", (0.165, 0.035, 0.955), (0.13, 0.11), "fur"),  # the leopard skin's hind leg
    ]
    ready = {  # upright, the staff upright in the right fist, the left hand reaching: walk frame 0, attack frame 0
        "upperarm.R": (14, 0, -4), "forearm.R": (80, 0, 0), "hand.R": (0, 0, 0),
        "upperarm.L": (74, 0, 8), "forearm.L": (22, 0, 0), "hand.L": (-18, 0, 0),
        "spine": (-2, 0, 0), "chest": (-4, 0, 0), "neck": (-2, 0, 0), "head": (-4, 3, 6),
    }
    head_boost = ((0, 0.03, 1.69), 1.8)

    def __init__(self):
        super().__init__()
        self.staff = {}  # rest-space (hand.R) points: "tip" (the ankh's centre: the bolt's origin), "bottom", "top"

    # ------------------------------------------------ materials and geometry
    def materials(self):
        M = super().materials()
        pal, p = self.col, self.name + "_"
        for name in ("gold", "gold_dark", "lapis", "ebony", "amethyst"):
            M[name] = common.make_material(p + name, ("solid", name), pal)
        M["fur"] = spotted_material(p + "fur", pal)
        M["cord"] = common.make_material(p + "cord", ("stripes", "v", 0.012, [(2, "gold"), (1, "gold_dark")]), pal)
        # Darker linen, more resin.
        for key, period, turn, base, grime in (("wrap", 0.055, 1.0, "linen", 1.2), ("wrap_limb", 0.04, -1.0, "linen", 1.2),
                                                ("wrap_head", 0.024, 1.0, "linen", 1.0), ("strip", 0.09, 0.0, "linen_hi", 1.2)):
            M[key] = mummy.wrap_material(p + key, period, turn, base, grime, pal)
        return M

    def build_body(self, b, M):
        mummy.build_body(b, M, claw="gold")  # gilded finger stalls

    def build_head(self, b, M):
        """A priest's shaved skull: a high, smooth dome swept back, tightly wrapped, one band at the brow."""
        fwd = V((0, 1, 0))
        head = [
            ((0, 0.015, 1.555), 0.046, 0.046),
            ((0, 0.03, 1.60), 0.062, 0.072),
            ((0, 0.03, 1.64), 0.074, 0.088),
            ((0, 0.02, 1.69), 0.082, 0.097),
            ((0, 0.004, 1.74), 0.085, 0.100),
            ((0, -0.012, 1.785), 0.080, 0.096),
            ((0, -0.024, 1.825), 0.064, 0.078),
            ((0, -0.032, 1.852), 0.036, 0.044),
        ]
        b.add(tube(head, sub=3, n=18, ref=fwd, caps=(False, True), bulge=(0, 0.010)), M["wrap_head"],
              lambda co: {"head": smoothstep(1.56, 1.61, co.z), "neck": 1 - smoothstep(1.56, 1.61, co.z)})
        b.add(ellipsoid((0, 0.112, 1.662), (0.014, 0.014, 0.026), n=8, rings=5), M["wrap_head"], "head")  # nose
        b.add(band((0, 0.03, 1.615), (0.2, 0.25, 1), 0.068, 0.082, 0.006, 14), M["band"], "head")  # jaw
        b.add(band((0, 0.02, 1.728), (0, -0.32, 1), 0.087, 0.100, 0.005, 16), M["band"], "head")  # brow
        self.build_face(b, M)

    def build_extras(self, b, M, rig):
        self.build_sash(b, M)
        self.build_amulet(b, M)
        self.build_staff(b, M, rig)

    def build_sash(self, b, M):
        (hx, hz), (sx, sz) = SASH["hip"], SASH["shoulder"]
        slope = (sz - hz) / (sx - hx)
        pts, nrms = torso_ring(lambda x, y: hz + slope * (x - hx), SASH["lift"])
        w = SASH["width"] / 2
        geo = loft(pts, [(SASH["thick"], w)] * len(pts), n=6, ref=lambda i: nrms[i].cross((pts[(i + 1) % len(pts)] - pts[i - 1]).normalized()), closed=True)
        b.add(geo, M["fur"], chain_weights(*TORSO_CHAIN, 0.06))
        # A gold clasp where the sash crosses the chest.
        k = min(range(len(pts)), key=lambda i: (pts[i] - V((-0.08, 0.2, 1.33))).length)
        c = pts[k] + nrms[k] * 0.006
        b.add(ellipsoid(c, (0.022, 0.022, 0.008), n=12, rings=4, axis=nrms[k], ref=V((0, 0, 1))), M["gold"], "chest")
        b.add(ellipsoid(c + nrms[k] * 0.006, (0.009, 0.009, 0.005), n=8, rings=3, axis=nrms[k], ref=V((0, 0, 1))), M["lapis"], "chest")

    def build_amulet(self, b, M):
        back, front = CORD["back"], CORD["front"]
        pts, _ = torso_ring(lambda x, y: back + (front - back) * smoothstep(-0.06, 0.11, y), CORD["lift"], n=32)
        b.add(tube([(p, CORD["thick"], CORD["thick"]) for p in pts], sub=1, n=5, closed=True), M["cord"], chain_weights(*TORSO_CHAIN, 0.04))
        low = min(pts, key=lambda p: p.z)
        c = V((0, low.y + 0.006, low.z - 0.045))
        b.add(ellipsoid(c, (0.034, 0.040, 0.008), n=14, rings=4, axis=V((0, 1, 0)), ref=V((0, 0, 1))), M["gold"], "chest")  # setting
        b.add(ellipsoid(c + V((0, 0.007, 0.002)), (0.024, 0.031, 0.008), n=12, rings=4, axis=V((0, 1, 0)), ref=V((0, 0, 1))), M["lapis"], "chest")  # scarab
        b.add(ellipsoid(c + V((0, 0.011, 0.024)), (0.012, 0.008, 0.005), n=8, rings=3, axis=V((0, 1, 0)), ref=V((0, 0, 1))), M["lapis"], "chest")  # head
        b.add(ellipsoid(c + V((0, 0.013, -0.004)), (0.002, 0.026, 0.004), n=6, rings=3, axis=V((0, 1, 0)), ref=V((0, 0, 1))), M["gold"], "chest")  # wing line
        for s in (1, -1):  # gold wings to the setting's sides
            b.add(ellipsoid(c + V((0.045 * s, -0.002, 0.008)), (0.022, 0.012, 0.004), n=10, rings=3, axis=V((0, 1, 0)), ref=V((0, 0, 1))), M["gold"], "chest")

    def fist_centre(self, rig, s=1):
        """The right fist's centre in pose space (armature), the staff's grip."""
        pb = rig.pose.bones[side_name("hand", s)]
        j = joints(s)
        rest = j["wrist"] + (j["hand_end"] - j["wrist"]).normalized() * 0.06 + V((-0.012 * s, 0.008, 0))
        return pb.matrix @ pb.bone.matrix_local.inverted() @ rest

    def build_staff(self, b, M, rig):
        """Ebony staff, upright through the right fist in the ready pose: a gold ankh round an amethyst on top, gold
        bands, a bronze-gold foot. Modelled in the ready pose, moved to hand.R's rest space; its own bone (parented to
        hand.R, head at the fist) lets it slide in the fist, be dropped, laid in the coffin or slung on the back."""
        set_pose(rig, self.ready)
        bpy.context.view_layer.update()
        pb = rig.pose.bones["hand.R"]
        to_rest_m = pb.bone.matrix_local @ pb.matrix.inverted()

        def to_rest(geo):
            return transform(geo, lambda v: to_rest_m @ v)

        p = self.fist_centre(rig)
        bottom, top = STAFF_LEN["bottom"], STAFF_LEN["shaft_top"]
        x, y = p.x, p.y
        shaft = [(V((x, y, bottom + 0.04)), 0.014, 0.014), (V((x, y, 0.9)), 0.017, 0.017), (V((x, y, top)), 0.015, 0.015)]
        b.add(to_rest(tube(shaft, sub=6, n=10, caps=(False, False))), M["ebony"], "staff", "staff")
        foot = [(V((x, y, bottom)), 0.008, 0.008), (V((x, y, bottom + 0.012)), 0.015, 0.015), (V((x, y, bottom + 0.06)), 0.017, 0.017)]
        b.add(to_rest(tube(foot, sub=2, n=10, bulge=(0.004, 0))), M["gold_dark"], "staff", "staff")
        for z, r in ((0.55, 0.020), (p.z + 0.07, 0.020), (p.z - 0.07, 0.020), (top - 0.03, 0.021)):
            b.add(to_rest(tube([(V((x, y, z - 0.012)), r, r), (V((x, y, z + 0.012)), r, r)], sub=1, n=10)), M["gold"], "staff", "staff")
        # Ankh: crossbar, a tear-shaped loop round the amethyst, in the X-Z plane (facing forward, +Y).
        bar_z = top + 0.02
        bar = [(V((x - 0.075, y, bar_z)), 0.013, 0.011), (V((x, y, bar_z)), 0.015, 0.012), (V((x + 0.075, y, bar_z)), 0.013, 0.011)]
        b.add(to_rest(tube(bar, sub=2, n=8, ref=V((0, 1, 0)), bulge=(0.004, 0.004))), M["gold"], "staff", "staff")
        neck = [(V((x, y, top - 0.02)), 0.017, 0.013), (V((x, y, bar_z + 0.02)), 0.014, 0.011)]
        b.add(to_rest(tube(neck, sub=2, n=8, ref=V((0, 1, 0)))), M["gold"], "staff", "staff")
        lc = V((x, y, bar_z + 0.075))
        loop = []
        for i in range(20):
            a = 2 * math.pi * i / 20
            r = 0.040 + 0.012 * max(0.0, -math.cos(a))  # wider at the bottom (a tear)
            loop.append(lc + V((r * 0.85 * math.sin(a), 0, 0.058 * math.cos(a) - 0.004 * math.cos(2 * a))))
        b.add(to_rest(loft(loop, [(0.011, 0.010)] * 20, n=8, ref=V((0, 1, 0)), closed=True)), M["gold"], "staff", "staff")
        b.add(to_rest(ellipsoid(lc + V((0, 0, -0.004)), (0.030, 0.012, 0.044), n=14, rings=6)), M["amethyst"], "staff", "staff")
        self.staff = {
            "tip": to_rest_m @ lc,
            "bottom": to_rest_m @ V((x, y, bottom)),
            "top": to_rest_m @ V((x, y, bar_z + 0.075 + 0.07)),
            "grip": p.z - bottom,
        }
        set_pose(rig, {})
        bpy.context.view_layer.objects.active = rig
        bpy.ops.object.mode_set(mode="EDIT")
        head = to_rest_m @ p
        mummy.edit_bone(rig.data.edit_bones, "staff", head, head + (to_rest_m.to_3x3() @ V((0, 0, 0.3))), "hand.R")
        bpy.ops.object.mode_set(mode="OBJECT")
        rig.pose.bones["staff"].rotation_mode = "XYZ"
        bpy.context.view_layer.update()
        self.staff["ready"] = None  # the staff bone's pose-space rotation in the ready pose, set in make_actions

    # ------------------------------------------------ staff keying
    def floor_skip(self, obj):
        return mummy.group_verts(obj, lambda g: mummy.is_strip(g) or g == "staff")

    def staff_ends(self, rig, m=None):
        """Model-space bottom, top and tip of the staff for its bone's pose matrix m (default: the current one)."""
        pb = rig.pose.bones["staff"]
        m = rig.matrix_world @ (m if m is not None else pb.matrix) @ pb.bone.matrix_local.inverted()
        return m @ self.staff["bottom"], m @ self.staff["top"], m @ self.staff["tip"]

    def staff_frame(self, axis, face, grip_at):
        """Pose-space matrix of the staff bone: shaft along axis (bottom -> top), the ankh facing face, the bone's head
        (the grip point) at grip_at."""
        def frame(f, a):
            yv = V(a).normalized()
            h = (V(f) - V(f).dot(yv) * yv).normalized()
            return Matrix((h, yv, h.cross(yv))).transposed()

        r0 = self.staff["ready"]
        rot = frame(face, axis) @ frame(V((0, 1, 0)), V((0, 0, 1))).inverted() @ r0
        m = rot.to_4x4()
        m.translation = grip_at
        return m

    def body_matrix(self, rig):
        """Pose-space transform of the body (the root bone's pose)."""
        root = rig.pose.bones["root"]
        return root.matrix @ root.bone.matrix_local.inverted()

    def floor_at(self, key, f, p):
        """Floor height under model-space point p (metres): the coffin's inner floor inside the coffin (idle, rise)."""
        if key not in ("idle", "rise"):
            return 0.0
        t = TILE * self.U["m"]
        q = p - self.slide(f) if key == "rise" else p  # the coffin's frame (the idle is authored in it)
        hx, hy = (self.coffin["half_x"] + self.coffin["wall"]) * t, (self.coffin["half_y"] + self.coffin["wall"]) * t
        ix, iy = self.coffin["half_x"] * t, self.coffin["half_y"] * t
        if key == "rise" and abs(q.x) < hx and abs(q.y) < hy and (abs(q.x) > ix or abs(q.y) > iy):
            return self.coffin["rim"] * t  # over a wall: on its rim
        if key == "idle" or (abs(q.x) < hx and abs(q.y) < hy):
            return self.coffin["floor"] * t
        return 0.0

    def staff_target(self, rig, key, f, n):
        """The staff bone's pose matrix at frame f of clip key, None to leave it in the hand at rest."""
        pb = rig.pose.bones["staff"]
        hand = rig.pose.bones["hand.R"]
        if key == "idle":
            body = self.body_matrix(rig).to_3x3()
            axis = body @ V((0.05, -0.035, 1.0))  # along the body, head end past the skull
            return self.staff_frame(axis, body @ V((0, 1, 0)), self.fist_centre(rig) + axis.normalized() * 0.02)
        if key == "rise":
            if f >= 19:
                return None
            fist = self.fist_centre(rig)
            if f == 0:
                return hand.matrix @ self.staff["idle_local"]
            if f <= 9:
                k = smoothstep(0, 3, f)
                face = self.staff["idle_face"].lerp(V((0, 1, 0)), k)
                return self.staff_frame(self.rise_axis(rig, f, fist), face, fist)
            up = self.staff_frame(V((0, 0, 1)), V((0, 1, 0)), fist)
            rest = hand.matrix @ (hand.bone.matrix_local.inverted() @ pb.bone.matrix_local)
            k2 = smoothstep(13, 19, f)
            m = up.to_quaternion().slerp(rest.to_quaternion(), k2).to_matrix().to_4x4()
            m.translation = up.translation.lerp(rest.translation, k2)
            return m
        if key == "die":
            return self.staff_drop(rig, f)
        if key == "climb":
            chest = rig.pose.bones["chest"]
            axis = V((0.32, 0.0, 1.0)).normalized()  # across the back: the ankh over the right shoulder
            placed = self.staff_frame(axis, V((0, -1, 0)), V((-0.24, -0.15, 0.30)) + axis * self.staff["grip"])
            # placed is in the standing body's space; follow the chest.
            rest_chest = chest.bone.matrix_local
            return chest.matrix @ rest_chest.inverted() @ placed
        return None

    def rise_axis(self, rig, f, fist):
        """The staff's axis while he climbs out: upright, its foot kept inside the coffin (off the front wall) until
        he sits on the rim, then lifted over it."""
        if f > 9:
            return V((0, 0, 1))
        t = TILE * self.U["m"]
        C = self.coffin
        floor, rim, inner, outer = C["floor"] * t, C["rim"] * t, C["half_y"] * t, (C["half_y"] + C["wall"]) * t
        hand = fist + rig.matrix_world.translation - self.slide(f)  # in the coffin's frame
        lim_x = (C["half_x"] - 0.012) * t
        x = max(-lim_x, min(lim_x, hand.x))
        if f >= 5 and hand.y > (inner + outer) / 2:  # the fist over / past the front wall: the foot on the floor in front
            edge, foot_z = outer + 0.025, 0.0
            y = max(hand.y, edge) if hand.z <= rim + 0.02 else max(hand.y, hand.y + (edge - hand.y) * hand.z / (hand.z - rim))
        else:  # inside: the shaft passes inside the front wall's rim
            edge, foot_z = inner - 0.025, floor
            y = min(hand.y, edge - 0.03)
            if hand.y > edge and hand.z > rim + 0.02:
                y = min(y, hand.y - (hand.y - edge) * (hand.z - foot_z) / (hand.z - rim))
            y = max(-inner + 0.03, y)
        planted = V((x, y, foot_z))
        if f <= 3:  # from where the foot lay, along the coffin floor
            planted = self.staff["idle_foot"].lerp(planted, smoothstep(0, 3, f))
        return (hand - planted).normalized()

    def staff_slide(self, key, f):
        """How far the staff slides through the fist towards its head (metres): the swing thrusts it forward so it
        does not run back through the body when it points at the player."""
        if key == "attack":
            return 0.45 * smoothstep(9.5, 12, f) * (1 - smoothstep(16, 23, f))
        return 0.0

    def staff_drop(self, rig, f):
        """Die: still in the hand until frame 4, then an accelerating tip-over to the floor at his right by frame 12."""
        if f <= 4:
            return None
        if "drop" not in self.staff:
            scene = bpy.context.scene
            cur = scene.frame_current
            scene.frame_set(4)
            pb = rig.pose.bones["staff"]
            held = pb.matrix.copy()
            scene.frame_set(cur)
            axis = V((0.22, 0.97, 0.0)).normalized()
            lying = self.staff_frame(axis, V((0, 0, 1)), V((0.46, -0.30, 0.0)) + axis * self.staff["grip"])
            self.staff["drop"] = (held, lying)
        held, lying = self.staff["drop"]
        t = min(1.0, ((f - 4) / 8) ** 2)
        m = held.to_quaternion().slerp(lying.to_quaternion(), t).to_matrix().to_4x4()
        m.translation = held.translation.lerp(lying.translation, t)
        return m

    def after_clip(self, rig, obj, key, act):
        """Key the staff bone on every frame: its target (or at rest in the fist), slid along its shaft so that neither
        end goes below the floor (the coffin's floor inside the coffin)."""
        scene = bpy.context.scene
        pb = rig.pose.bones["staff"]
        n = self.n(key)
        if self.staff["ready"] is None:
            set_pose(rig, self.ready)
            bpy.context.view_layer.update()
            self.staff["ready"] = pb.matrix.to_3x3().normalized()
            set_pose(rig, {})
        self.staff.pop("drop", None)
        last = n if key in ("walk", "attack", "idle", "climb") else n - 1
        for f in range(last + 1):
            scene.frame_set(f)
            pb.rotation_euler, pb.location = (0, 0, 0), (0, 0, 0)
            bpy.context.view_layer.update()
            m = self.staff_target(rig, key, f, n)
            if m is not None:
                pb.matrix = m
            else:
                pb.location = (0, self.staff_slide(key, f), 0)  # along the shaft, towards the ankh
            bpy.context.view_layer.update()
            bottom, top, _tip = self.staff_ends(rig)
            low = min(bottom, top, key=lambda q: q.z)
            d = (top - bottom).normalized()
            along = d if low is bottom else -d
            floor = self.floor_at(key, f, low) + (0.006 if along.z > 0.2 else 0.02)  # lying: the shaft's radius
            if low.z < floor:  # slide it up along the shaft (lying flat: lift it)
                lift = (floor - low.z) / along.z if along.z > 0.2 else floor - low.z
                mm = pb.matrix.copy()
                mm.translation += (along * lift) if along.z > 0.2 else V((0, 0, lift))
                pb.matrix = mm
                bpy.context.view_layer.update()
            pb.keyframe_insert("rotation_euler", frame=f)
            pb.keyframe_insert("location", frame=f)
            if key == "idle" and f == 0:
                hand = rig.pose.bones["hand.R"]
                self.staff["idle_local"] = hand.matrix.inverted() @ pb.matrix
                self.staff["idle_foot"] = bottom.copy()  # the idle is authored in the coffin's frame
                self.staff["idle_face"] = (rig.matrix_world.to_3x3() @ pb.matrix.to_3x3() @ self.staff["ready"].inverted() @ V((0, 1, 0))).normalized()

    # ------------------------------------------------ poses
    def attack_poses(self):
        """The staff swung up behind the head and down over it until the ankh points at the player (the release),
        the left hand thrust out with it; a short follow-through and back to the ready pose."""
        n = self.n("attack")
        w0 = self.walk_pose(0.0)
        windup = pose(w0, upperarm_R=(150, 0, -12), forearm_R=(10, 0, 0), hand_R=(-50, 0, 0),
                      upperarm_L=(40, 0, 14), forearm_L=(70, 0, 0), hand_L=(-10, 0, 0),
                      pelvis=(4, -12, 0), spine=(6, -8, 0), chest=(8, -10, 0), neck=(2, 0, 0), head=(-2, 12, 4),
                      thigh_L=(20, 0, 0), shin_L=(-14, 0, 0), foot_L=(-6, 0, 0), thigh_R=(-14, 0, 0), shin_R=(-6, 0, 0), foot_R=(10, 0, 0))
        windup["root_loc"] = (0, -0.03, 0)
        strike = pose(w0, upperarm_R=(80, 0, -30), forearm_R=(12, 0, 0), hand_R=(-60, 0, 0),
                      upperarm_L=(96, 0, 18), forearm_L=(4, 0, 0), hand_L=(-62, 0, 0),
                      pelvis=(-8, -2, 0), spine=(-10, -2, 0), chest=(-12, -2, 0), neck=(-4, 0, 0), head=(-6, 0, 4),
                      thigh_L=(36, 0, 0), shin_L=(-30, 0, 0), foot_L=(-4, 0, 0), thigh_R=(-22, 0, 0), shin_R=(-10, 0, 0), foot_R=(18, 0, 0))
        strike["root_loc"] = (0, 0.06, 0)
        follow = pose(strike, upperarm_R=(72, 0, -30), hand_R=(-66, 0, 0), chest=(-16, -2, 0), spine=(-12, -2, 0), hand_L=(-50, 0, 0))
        follow["root_loc"] = (0, 0.07, 0)
        return [(0, w0), (8, windup), (12, strike), (16, follow), (n, w0)]

    def rise_keys(self, rig, idle0, walk0):
        """The mummy's climb out; from standing up on, the right arm holds the staff as in the ready pose."""
        keys = super().rise_keys(rig, idle0, walk0)
        hold = {b: self.ready[b] for b in ("upperarm.R", "forearm.R", "hand.R")}
        for f, p in keys:
            if 4 <= f < self.n("rise") - 1:
                p.update({"upperarm.R": (20, 0, -6), "forearm.R": (60, 0, 0), "hand.R": (0, 0, 0)} if f <= 10 else hold)
        return keys

    def idle_pose(self, t):
        p = super().idle_pose(t)
        return add(p, hand_R=(-14 * math.exp(-((t - 0.62) / 0.04) ** 2), 0, 0))  # the fist on the staff keeps still

    # ------------------------------------------------ the ranged attack's release
    def measure(self):
        """The ranged attack's release (as anubis.py --measure): the ankh's centre per attack frame, its height as a
        share of the reference (walk frame 0) height (RangedRules::releaseY), how far ahead of the reference's centre in
        the engine's half widths (the larger of the half width and half length, ModelInfo::HalfWidth;
        RangedRules::releaseAhead) and where the staff points."""
        obj, rig = bpy.data.objects[self.coll], bpy.data.objects[self.name + "_rig"]
        scene = bpy.context.scene

        def mesh_box():
            pts = mummy.evaluated(obj)
            return (min(q.z for q in pts), max(q.z for q in pts), min(q.y for q in pts), max(q.y for q in pts),
                    max(q.x for q in pts) - min(q.x for q in pts))

        rig.animation_data.action = bpy.data.actions[self.act("walk")]
        scene.frame_set(0)
        z0, z1, y0, y1, width = mesh_box()
        height, centre, half = z1 - z0, (y0 + y1) / 2, max(width, y1 - y0) / 2
        print("MEASURE reference: height {:.3f}, centre y {:.3f}, half width {:.3f} (x) / length {:.3f} (y): the engine's {:.3f}".format(
            height, centre, width / 2, (y1 - y0) / 2, half))
        rig.animation_data.action = bpy.data.actions[self.act("attack")]
        n = self.n("attack")
        best = None
        for f in range(n):
            scene.frame_set(f)
            bottom, top, tip = self.staff_ends(rig)
            d = (top - bottom).normalized()
            ang = math.degrees(math.atan2(d.z, d.y))
            if best is None and abs(ang) <= 10 and tip.y > centre:  # the first frame it points at the player
                best = (f, ang, (tip.z - z0) / height, (tip.y - centre) / half)
            print("TIP attack f{:02d} ({:.2f} of the clip): height {:.3f} = {:.3f} of the reference, ahead {:+.3f} = {:+.2f} half "
                  "widths, points {:+4.0f} deg (0 = forward, 90 = up), {:+4.0f} deg sideways (+ = his right), tip x {:+.3f}".format(
                      f, f / (n - 1), tip.z - z0, (tip.z - z0) / height, tip.y - centre, (tip.y - centre) / half, ang,
                      math.degrees(math.atan2(d.x, d.y)), tip.x))
        f, ang, ry, ra = best
        print("RELEASE frame {} = {}/{} = {:.3f} of the clip, releaseY {:.2f}, releaseAhead {:.2f}, points {:+.0f} deg".format(f, f, n - 1, f / (n - 1), ry, ra, ang))
        rig.animation_data.action = bpy.data.actions[self.act("walk")]
        scene.frame_set(0)

    def report(self, rig=None, obj=None):
        super().report(rig, obj)
        obj = obj or bpy.data.objects[self.coll]
        rig = rig or bpy.data.objects[self.name + "_rig"]
        scene = bpy.context.scene
        rig.animation_data.action = bpy.data.actions[self.act("walk")]
        scene.frame_set(0)
        skip = mummy.group_verts(obj, lambda g: g == "staff")
        lo, hi = mummy.bbox(obj, skip)
        lo2, hi2 = mummy.bbox(obj)
        print("{}: walk frame 0 body height {:.3f} m, with the staff {:.3f} x {:.3f} x {:.3f} m; body {:.2f} world units at scale {}".format(
            self.name, hi.z - lo.z, *(hi2 - lo2), (hi.z - lo.z) / self.U["m"], self.scale))

    def main(self, args, **kw):
        if "--measure" in args:
            bpy.ops.wm.read_factory_settings(use_empty=True)
            self.build(bake=False)
            self.measure()
            return
        super().main(args, **kw)


MODEL = MummyPriest()
CLIPS = MODEL.CLIPS
FRAMES = {name: n for name, _, n in CLIPS}
REVIEW_VIEW = MODEL.review_view
SCALE = MODEL.scale
build, export, report, coffin_check, measure = MODEL.build, MODEL.export, MODEL.report, MODEL.coffin_check, MODEL.measure


if __name__ == "__main__" and "--" in sys.argv:
    MODEL.main(sys.argv[sys.argv.index("--") + 1 :])
