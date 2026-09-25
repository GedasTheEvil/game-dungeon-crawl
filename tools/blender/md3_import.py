"""Import a game .md3 model into Blender (5.x) as a mesh with one shape key per frame.

The file layout and game conventions are in md3.py (game space is Y-up).

Usage:
    MCP / Blender console:
        p = ".../tools/blender/md3_import.py"; g = {"__file__": p, "__name__": "md3_import"}
        exec(open(p).read(), g); g["import_md3"]("Models/anubis.md3")
    CLI:
        blender -b --python tools/blender/md3_import.py -- Models/anubis.md3 [out.blend]
"""

import os
import sys

import bpy

HERE = os.path.dirname(os.path.abspath(__file__)) if "__file__" in globals() else os.getcwd()
REPO_ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, HERE)
import md3  # noqa: E402


def _to_blender(x, y, z):
    # Game Y-up -> Blender Z-up (pure rotation, keeps triangle winding).
    return (x, -z, y)


def read_model(path):
    """(corner count, flat positions per frame, flat frame-0 normals, flat uvs) of a triangle list."""
    frames, normals, uvs = md3.read_md3(path)
    flat = lambda rows: [c for row in rows for c in row]  # noqa: E731
    return len(uvs), [flat(fr) for fr in frames], flat(normals[0]), flat(uvs)


def _guess_texture(path):
    base = os.path.splitext(os.path.basename(path))[0]
    for suffix in ("_att", "_die"):
        if base.endswith(suffix):
            base = base[: -len(suffix)]
    tex = os.path.join(REPO_ROOT, "Textures", base + ".png")
    return tex if os.path.exists(tex) else None


def _make_material(name, texture_path):
    mat = bpy.data.materials.new(name)
    tree = mat.node_tree
    bsdf = tree.nodes.get("Principled BSDF")
    img = tree.nodes.new("ShaderNodeTexImage")
    img.image = bpy.data.images.load(texture_path, check_existing=True)
    img.location = (-400, 0)
    tree.links.new(img.outputs["Color"], bsdf.inputs["Base Color"])
    return mat


def import_md3(path, name=None, texture=None):
    """Import one .md3 file. Returns the created object."""
    path = os.path.abspath(path)
    name = name or os.path.splitext(os.path.basename(path))[0]
    vcount, frames, normals, uvs = read_model(path)

    # Weld corners that coincide in every frame, so the mesh gets real topology.
    index_of = {}
    corner_to_vert = []
    for i in range(vcount):
        key = tuple(round(fr[i * 3 + k], 6) for fr in frames for k in range(3))
        corner_to_vert.append(index_of.setdefault(key, len(index_of)))
    first_corner = [0] * len(index_of)
    for i, v in enumerate(corner_to_vert):
        first_corner[v] = i

    def positions(fr):
        return [_to_blender(*fr[c * 3 : c * 3 + 3]) for c in first_corner]

    faces = [corner_to_vert[i : i + 3] for i in range(0, vcount, 3)]
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(positions(frames[0]), [], faces)

    # Loops follow corner order (from_pydata keeps face and vertex order).
    uv_layer = mesh.uv_layers.new(name="UVMap")
    uv_layer.data.foreach_set("uv", uvs)
    mesh.normals_split_custom_set([_to_blender(*normals[i * 3 : i * 3 + 3]) for i in range(vcount)])
    mesh.validate(clean_customdata=False)
    mesh.update()

    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    obj["md3_source"] = os.path.relpath(path, REPO_ROOT)
    obj["md3_frames"] = len(frames)

    texture = texture or _guess_texture(path)
    if texture:
        mesh.materials.append(_make_material(name, texture))

    if len(frames) > 1:
        obj.shape_key_add(name="frame_000")
        keys = []
        for f in range(1, len(frames)):
            sk = obj.shape_key_add(name="frame_{:03d}".format(f), from_mix=False)
            flat = [c for p in positions(frames[f]) for c in p]
            sk.data.foreach_set("co", flat)
            keys.append(sk)
        # One key fully on at its own frame, off at neighbours.
        for f, sk in enumerate(keys, start=1):
            for t, v in ((f - 1, 0.0), (f, 1.0), (f + 1, 0.0)):
                sk.value = v
                sk.keyframe_insert("value", frame=t)
        mesh.shape_keys.name = name + "_keys"
        scene = bpy.context.scene
        scene.frame_start = 0
        scene.frame_end = len(frames) - 1

    print("Imported {}: {} corners -> {} verts, {} faces, {} frames".format(
        name, vcount, len(index_of), len(faces), len(frames)))
    return obj


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    import_md3(args[0])
    if len(args) > 1:
        bpy.ops.wm.save_as_mainfile(filepath=os.path.abspath(args[1]))
