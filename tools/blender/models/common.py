"""Shared helpers for procedural game models (see anubis.py for a complete example).

Model scripts do: sys.path.insert(0, HERE); import common; importlib.reload(common)
"""

import math
import os

import bpy
from mathutils import Vector as V

REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))


# ---------------------------------------------------------------- geometry helpers


def _cr(p0, p1, p2, p3, t):
    """Catmull-Rom interpolation, works for floats and Vectors."""
    return 0.5 * ((2 * p1) + (p2 - p0) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t + (3 * p1 - p0 - 3 * p2 + p3) * t * t * t)


def loft(path, secs, n=12, ref=V((0, 1, 0)), closed=False, caps=(True, True), bulge=(0.0, 0.0)):
    """Sweep sections along path. secs[i] = (rx, ry[, shape(theta)->k[, shear]]).

    Ring frame: t = tangent, nrm = ref made perpendicular to t, side = nrm x t.
    rx runs along side, ry along nrm. Returns (verts, faces, face_uvs); the uv is the
    'pattern' space: u = angle / 2pi, v = arc length in metres.
    """
    path = [V(p) for p in path]
    m = len(path)
    arc = [0.0]
    for i in range(1, m):
        arc.append(arc[-1] + (path[i] - path[i - 1]).length)
    verts, faces, fuv = [], [], []
    tangents = []
    for i, c in enumerate(path):
        if closed:
            t = path[(i + 1) % m] - path[i - 1]
        else:
            t = path[min(i + 1, m - 1)] - path[max(i - 1, 0)]
        t.normalize()
        tangents.append(t)
        r = ref(i) if callable(ref) else ref
        nrm = (r - r.dot(t) * t).normalized()
        side = nrm.cross(t)
        sec = secs[i]
        rx, ry = sec[0], sec[1]
        shape = sec[2] if len(sec) > 2 else None
        shear = sec[3] if len(sec) > 3 else 0.0
        for j in range(n):
            th = 2 * math.pi * j / n
            k = shape(th) if shape else 1.0
            cs, sn = math.cos(th), math.sin(th)
            verts.append(c + side * (rx * cs * k) + nrm * (ry * sn * k + shear * rx * cs))
    seg = m if closed else m - 1
    total = arc[-1] + ((path[0] - path[-1]).length if closed else 0.0)
    for i in range(seg):
        i2 = (i + 1) % m
        v0, v1 = arc[i], (arc[i2] if i2 else total)
        for j in range(n):
            j2 = (j + 1) % n
            faces.append([i * n + j, i * n + j2, i2 * n + j2, i2 * n + j])
            fuv.append([(j / n, v0), ((j + 1) / n, v0), ((j + 1) / n, v1), (j / n, v1)])
    if not closed:
        for end in (0, 1):
            if not caps[end]:
                continue
            i = 0 if end == 0 else m - 1
            ring = [i * n + j for j in range(n)]
            centre = sum((verts[r] for r in ring), V()) / n
            centre += tangents[i] * (bulge[end] if end else -bulge[end])
            ci = len(verts)
            verts.append(centre)
            for j in range(n):
                a, b = ring[j], ring[(j + 1) % n]
                faces.append([ci, b, a] if end == 0 else [ci, a, b])
                fuv.append([(0.5, arc[i]), ((j + 1) / n, arc[i]), (j / n, arc[i])] if end == 0 else [(0.5, arc[i]), (j / n, arc[i]), ((j + 1) / n, arc[i])])
    return verts, faces, fuv


def tube(keys, sub=2, n=12, ref=V((0, 1, 0)), shape=None, caps=(True, True), bulge=(0.0, 0.0), closed=False):
    """Loft through key sections (pos, rx, ry[, a]) with Catmull-Rom smoothing.

    shape(theta, a) -> radius multiplier, where a is interpolated between keys.
    """
    keys = [(V(k[0]), k[1], k[2], k[3] if len(k) > 3 else 0.0) for k in keys]
    m = len(keys)
    path, secs = [], []

    def sec(rx, ry, a):
        return (max(rx, 0.001), max(ry, 0.001), (lambda th, a=a: shape(th, a)) if shape else None)

    for i in range(m if closed else m - 1):
        if closed:
            idx = [(i - 1) % m, i, (i + 1) % m, (i + 2) % m]
        else:
            idx = [max(i - 1, 0), i, min(i + 1, m - 1), min(i + 2, m - 1)]
        k = [keys[j] for j in idx]
        for s in range(sub):
            t = s / sub
            path.append(_cr(k[0][0], k[1][0], k[2][0], k[3][0], t))
            secs.append(sec(*(_cr(k[0][c], k[1][c], k[2][c], k[3][c], t) for c in (1, 2, 3))))
    if not closed:
        path.append(keys[-1][0])
        secs.append(sec(keys[-1][1], keys[-1][2], keys[-1][3]))
    return loft(path, secs, n, ref, closed, caps, bulge)


def ellipsoid(c, r, n=10, rings=6, axis=V((0, 0, 1)), ref=V((0, 1, 0))):
    """Ellipsoid with radii r = (side, ref, axis) around centre c."""
    c, axis = V(c), V(axis).normalized()
    path, secs = [], []
    for i in range(1, rings):
        phi = math.pi * i / rings
        path.append(c - axis * (math.cos(phi) * r[2]))
        secs.append((r[0] * math.sin(phi), r[1] * math.sin(phi)))
    b = r[2] * (1 - math.cos(math.pi / rings))
    return loft(path, secs, n, ref, caps=(True, True), bulge=(b, b))


def orient(verts, face, expected):
    a, b, c = (verts[i] for i in face[:3])
    return face if (b - a).cross(c - a).dot(expected) >= 0 else face[::-1]


def cone(base, direction, length, r, n=6):
    d = V(direction).normalized()
    ref = V((0, 0, 1)) if abs(d.z) < 0.9 else V((1, 0, 0))
    return tube([(base - d * 0.01, r, r), (base + d * length * 0.5, r * 0.55, r * 0.55), (base + d * length, 0.002, 0.002)],
                sub=1, n=n, ref=ref, caps=(True, False))


def perp(d):
    ref = V((0, 0, 1)) if abs(d.z) < 0.9 else V((1, 0, 0))
    return (ref - ref.dot(d) * d).normalized()


def lathe(profile, vs, n, mats, z_mod=None):
    """Revolve (r, z) profile around Z. Face normals follow the profile's right-hand side.

    mats[i] names the material of profile segment i. Returns {mat: (verts, faces, uvs)}.
    """
    verts = []
    for r, z in profile:
        for j in range(n):
            th = 2 * math.pi * j / n
            zz = z_mod(th, r, z) if z_mod else z
            verts.append(V((r * math.cos(th), r * math.sin(th), zz)))
    out = {}
    for i in range(len(profile) - 1):
        (r0, z0), (r1, z1) = profile[i], profile[i + 1]
        nr, nz = z1 - z0, -(r1 - r0)
        faces, fuv = out.setdefault(mats[i], (verts, [], []))[1:]
        for j in range(n):
            j2 = (j + 1) % n
            th = 2 * math.pi * (j + 0.5) / n
            expected = V((nr * math.cos(th), nr * math.sin(th), nz))
            face = [i * n + j, i * n + j2, (i + 1) * n + j2, (i + 1) * n + j]
            uv = [(j / n, vs[i]), ((j + 1) / n, vs[i]), ((j + 1) / n, vs[i + 1]), (j / n, vs[i + 1])]
            f = orient(verts, face, expected) if (r0 > 1e-6 or r1 > 1e-6) else face
            faces.append(f)
            fuv.append(uv if f is face else uv[::-1])
    return out


def transform(geo, fn):
    verts, faces, fuv = geo
    return [fn(v) for v in verts], faces, fuv


def smoothstep(e0, e1, x):
    t = min(max((x - e0) / (e1 - e0), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def chain_weights(joints, bones, blend=0.05):
    """Weight function for a tube along a bone chain; blends across joints."""
    joints = [V(j) for j in joints]

    def fn(co):
        best = None
        for k in range(len(bones)):
            a, b = joints[k], joints[k + 1]
            ab = b - a
            L = ab.length
            t = min(max((co - a).dot(ab) / (L * L), 0.0), 1.0)
            d = (co - (a + ab * t)).length
            if best is None or d < best[0]:
                best = (d, k, t * L, L)
        _, k, s, L = best
        if s < blend and k > 0:
            a = 0.5 + 0.5 * s / blend
            return {bones[k]: a, bones[k - 1]: 1 - a}
        if L - s < blend and k < len(bones) - 1:
            a = 0.5 + 0.5 * (L - s) / blend
            return {bones[k]: a, bones[k + 1]: 1 - a}
        return {bones[k]: 1.0}

    return fn


class Builder:
    """Accumulates parts into one mesh: faces, pattern uvs, material and bone weights."""

    def __init__(self):
        self.verts, self.faces, self.fuv, self.fmat, self.weights, self.tags = [], [], [], [], [], []
        self.mats = []

    def add(self, geo, mat, weights, tag="body"):
        verts, faces, fuv = geo
        base = len(self.verts)
        if mat not in self.mats:
            self.mats.append(mat)
        mi = self.mats.index(mat)
        self.verts.extend(verts)
        for f, uv in zip(faces, fuv):
            self.faces.append([base + i for i in f])
            self.fuv.append(uv)
            self.fmat.append(mi)
        if isinstance(weights, str):
            weights = {weights: 1.0}
        wf = weights if callable(weights) else (lambda co, w=weights: w)
        self.weights.extend(wf(v) for v in verts)
        self.tags.extend([tag] * len(verts))


# ---------------------------------------------------------------- materials


def make_material(name, spec, palette):
    """spec: ("solid", colour) or ("stripes", "u"|"v", period, [(width, colour), ...][, "LINEAR"]); colours index palette."""
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nt = mat.node_tree
    for node in list(nt.nodes):
        if node.type not in ("OUTPUT_MATERIAL", "BSDF_PRINCIPLED"):
            nt.nodes.remove(node)
    bsdf = nt.nodes.get("Principled BSDF")
    bsdf.inputs["Roughness"].default_value = 0.6
    if spec[0] == "solid":
        bsdf.inputs["Base Color"].default_value = (*palette[spec[1]], 1.0)
        return mat
    _, axis, period, bands = spec[:4]
    uv = nt.nodes.new("ShaderNodeUVMap")
    uv.uv_map = "pattern"
    sep = nt.nodes.new("ShaderNodeSeparateXYZ")
    mul = nt.nodes.new("ShaderNodeMath")
    mul.operation = "MULTIPLY"
    mul.inputs[1].default_value = 1.0 / period
    fract = nt.nodes.new("ShaderNodeMath")
    fract.operation = "FRACT"
    ramp = nt.nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.interpolation = spec[4] if len(spec) > 4 else "CONSTANT"
    els = ramp.color_ramp.elements
    while len(els) > 1:
        els.remove(els[-1])
    total = sum(w for w, _ in bands)
    els[0].position = 0.0
    els[0].color = (*palette[bands[0][1]], 1.0)
    pos = bands[0][0] / total
    for w, c in bands[1:]:
        els.new(pos).color = (*palette[c], 1.0)
        pos += w / total
    nt.links.new(uv.outputs["UV"], sep.inputs[0])
    nt.links.new(sep.outputs["X" if axis == "u" else "Y"], mul.inputs[0])
    nt.links.new(mul.outputs[0], fract.inputs[0])
    nt.links.new(fract.outputs[0], ramp.inputs["Fac"])
    nt.links.new(ramp.outputs["Color"], bsdf.inputs["Base Color"])
    return mat


def finish_mesh(b, coll, name):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([tuple(v) for v in b.verts], [], b.faces)
    pattern = mesh.uv_layers.new(name="pattern")
    flat = [c for uvs in b.fuv for uv in uvs for c in uv]
    pattern.data.foreach_set("uv", flat)
    mesh.polygons.foreach_set("material_index", b.fmat)
    for m in b.mats:
        mesh.materials.append(m)
    mesh.shade_smooth()
    mesh.validate()
    obj = bpy.data.objects.new(name, mesh)
    coll.objects.link(obj)
    groups = {}
    for i, w in enumerate(b.weights):
        for bone, val in w.items():
            if val <= 0:
                continue
            if bone not in groups:
                groups[bone] = obj.vertex_groups.new(name=bone)
            groups[bone].add([i], val, "REPLACE")
    obj["tags"] = sorted(set(b.tags))
    return obj


def uv_unwrap(obj, tags, boost=None):
    """Smart-project the final UV map. boost = {tag: (centre, factor)} scales tagged parts up
    during projection so they get more texels."""
    mesh = obj.data
    uv = mesh.uv_layers.new(name="UVMap")
    mesh.uv_layers.active = uv
    uv.active_render = True
    orig = [v.co.copy() for v in mesh.vertices]
    boost = boost or {}
    for i, v in enumerate(mesh.vertices):
        if tags[i] in boost:
            c, k = boost[tags[i]]
            v.co = V(c) + (v.co - V(c)) * k
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(60), island_margin=0.004, area_weight=0.0, scale_to_bounds=True)
    bpy.ops.object.mode_set(mode="OBJECT")
    for v, co in zip(mesh.vertices, orig):
        v.co = co


def bake_texture(obj, out_path, size=1024, prefix="model", ao_distance=0.15):
    import numpy as np

    scene = bpy.context.scene
    engine = scene.render.engine
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = 64
    if scene.world is None:
        scene.world = bpy.data.worlds.new("World")
    scene.world.light_settings.distance = ao_distance

    col = bpy.data.images.new(prefix + "_bake_col", size, size, alpha=False)
    ao = bpy.data.images.new(prefix + "_bake_ao", size, size, alpha=False)
    for m in obj.data.materials:
        node = m.node_tree.nodes.get("bake_target") or m.node_tree.nodes.new("ShaderNodeTexImage")
        node.name = "bake_target"
        m.node_tree.nodes.active = node
    for o in bpy.context.view_layer.objects:
        o.select_set(False)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    for img, kind in ((col, "DIFFUSE"), (ao, "AO")):
        for m in obj.data.materials:
            m.node_tree.nodes["bake_target"].image = img
        kw = {"pass_filter": {"COLOR"}} if kind == "DIFFUSE" else {}
        bpy.ops.object.bake(type=kind, margin=6, use_clear=True, **kw)
    c = np.array(col.pixels[:]).reshape(-1, 4)
    a = np.array(ao.pixels[:]).reshape(-1, 4)[:, :1]
    out = c.copy()
    out[:, :3] = c[:, :3] * (0.45 + 0.55 * a)
    tex = bpy.data.images.get(prefix + "_tex") or bpy.data.images.new(prefix + "_tex", size, size, alpha=False)
    tex.pixels = out.ravel().tolist()
    tex.filepath_raw = out_path
    tex.file_format = "PNG"
    tex.save()
    scene.render.engine = engine
    for m in obj.data.materials:
        m.node_tree.nodes.remove(m.node_tree.nodes["bake_target"])
    bpy.data.images.remove(col)
    bpy.data.images.remove(ao)
    return tex


def use_baked_material(obj, tex):
    name = tex.name.replace("_tex", "") + "_baked"
    mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    nt = mat.node_tree
    img = nt.nodes.get("tex") or nt.nodes.new("ShaderNodeTexImage")
    img.name = "tex"
    img.image = tex
    nt.links.new(img.outputs["Color"], nt.nodes["Principled BSDF"].inputs["Base Color"])
    obj.data.materials.clear()
    obj.data.materials.append(mat)
    obj.data.polygons.foreach_set("material_index", [0] * len(obj.data.polygons))


def clear_collection(name):
    """Empty (or create) the model collection so a rebuild replaces it."""
    coll = bpy.data.collections.get(name)
    if coll:
        for o in list(coll.objects):
            data = o.data
            bpy.data.objects.remove(o)
            if data and data.users == 0:
                (bpy.data.meshes if isinstance(data, bpy.types.Mesh) else bpy.data.armatures).remove(data)
    else:
        coll = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(coll)
    return coll



def rig_object(obj, rig):
    obj.parent = rig
    mod = obj.modifiers.new("Armature", "ARMATURE")
    mod.object = rig


def tri_count(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def export_files(obj, rig, base, clips, category, models_dir=None):
    """clips = [(action_name, file_suffix, frame_count)]. Writes Models/<category>/<base><suffix>.md3."""
    models_dir = models_dir or os.path.join(REPO, "Models", category)
    p = os.path.join(REPO, "tools", "blender", "md3_export.py")
    g = {"__file__": p, "__name__": "md3_export"}
    exec(open(p).read(), g)
    for action, suffix, frames in clips:
        rig.animation_data.action = bpy.data.actions[action]
        g["export_md3"](obj, os.path.join(models_dir, base + suffix + ".md3"), 0, frames - 1)
    rig.animation_data.action = bpy.data.actions[clips[0][0]]
