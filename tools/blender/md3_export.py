"""Export a Blender (5.x) mesh object to the game's .md3 format (layout and conventions: md3.py).

Every frame in [frame_start, frame_end] is sampled from the evaluated mesh, so shape keys,
armatures and modifiers all bake down. Topology must not change between frames.

Usage:
    MCP / Blender console:
        p = ".../tools/blender/md3_export.py"; g = {"__file__": p, "__name__": "md3_export"}
        exec(open(p).read(), g); g["export_md3"](bpy.data.objects["anubis"], "Models/monsters/anubis.md3")
    CLI:
        blender -b file.blend --python tools/blender/md3_export.py -- ObjectName out.md3 [start end]
"""

import os
import sys

import bpy

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import md3  # noqa: E402


def _to_game(x, y, z):
    # Blender Z-up -> game Y-up (inverse of md3_import._to_blender).
    return (x, z, -y)


def _sample(obj, depsgraph):
    ev = obj.evaluated_get(depsgraph)
    mesh = ev.to_mesh()
    try:
        mesh.calc_loop_triangles()
        mw = obj.matrix_world
        rot = mw.to_3x3().inverted_safe().transposed()
        loops = [l for tri in mesh.loop_triangles for l in tri.loops]
        pos = [_to_game(*(mw @ mesh.vertices[mesh.loops[l].vertex_index].co)) for l in loops]
        normals = [_to_game(*(rot @ mesh.corner_normals[l].vector).normalized()) for l in loops]
        uv_layer = mesh.uv_layers.active
        uvs = [tuple(uv_layer.data[l].uv) if uv_layer else (0.0, 0.0) for l in loops]
        return pos, normals, uvs
    finally:
        ev.to_mesh_clear()


def export_md3(obj, path, frame_start=None, frame_end=None):
    """Write obj to path. Frame range defaults to the scene's."""
    scene = bpy.context.scene
    frame_start = scene.frame_start if frame_start is None else frame_start
    frame_end = scene.frame_end if frame_end is None else frame_end
    current = scene.frame_current
    frames, normals, uvs = [], [], None
    try:
        for f in range(frame_start, frame_end + 1):
            scene.frame_set(f)
            pos, nrm, uv = _sample(obj, bpy.context.evaluated_depsgraph_get())
            if frames and len(pos) != len(frames[0]):
                raise ValueError("Topology changed at frame {}".format(f))
            frames.append(pos)
            normals.append(nrm)
            uvs = uvs or uv
    finally:
        scene.frame_set(current)
    stats = md3.write_md3(path, frames, normals, uvs, name=os.path.splitext(os.path.basename(path))[0])
    print("Exported {} -> {}: {verts} verts, {tris} tris, {surfaces} surfaces, {frames} frames, {bytes} bytes".format(obj.name, path, **stats))


if __name__ == "__main__" and "--" in sys.argv:
    args = sys.argv[sys.argv.index("--") + 1 :]
    rng = (int(args[2]), int(args[3])) if len(args) >= 4 else (None, None)
    export_md3(bpy.data.objects[args[0]], args[1], *rng)
