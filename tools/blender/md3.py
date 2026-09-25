"""Quake 3 MD3 reader/writer for the game's vertex-animated models (pure Python, Blender not needed).

Spec: https://www.icculus.org/homepages/phaethon/q3a/formats/md3format.html
Game conventions on top of standard MD3:
* Coordinates are game space (Y up), triangles keep the game's counter-clockwise winding.
* The engine rescales every model (AnimatedModel::Centrify), so each file is scaled to use the
  full int16 range (1/64 steps, |coord| <= 511). The original unit is kept in the header name as
  "<name>;unit=<float>" so tools can restore real sizes: original = stored * unit.
* Texture t is stored flipped (1 - v) like standard MD3; the engine flips it back.
* Normals are stored for every frame. Surfaces are split at the MD3 limit of 4096 vertices.
"""

import math
import struct

XYZ_SCALE = 1.0 / 64
MAX_VERTS = 4096
MAX_TRIS = 8192
HEADER = struct.Struct("<4si64s9i")
FRAME = struct.Struct("<3f3f3ff16s")
SURFACE = struct.Struct("<4s64s10i")
SHADER = struct.Struct("<64si")
TRI = struct.Struct("<3i")
ST = struct.Struct("<2f")
VERT = struct.Struct("<4h")
IDENT, VERSION = b"IDP3", 15


def encode_normal(n):
    """Unit vector -> 16-bit lat/long: high byte azimuth, low byte polar angle, both 255 per 360 deg."""
    x, y, z = n
    length = math.sqrt(x * x + y * y + z * z) or 1.0
    x, y, z = x / length, y / length, z / length
    azimuth = int(round(math.degrees(math.atan2(y, x)) * 255 / 360)) & 0xFF
    polar = int(round(math.degrees(math.acos(max(-1.0, min(1.0, z)))) * 255 / 360)) & 0xFF
    code = (azimuth << 8) | polar
    return code - 0x10000 if code >= 0x8000 else code


def decode_normal(code):
    code &= 0xFFFF
    azimuth = ((code >> 8) & 0xFF) * 2 * math.pi / 255
    polar = (code & 0xFF) * 2 * math.pi / 255
    return (math.cos(azimuth) * math.sin(polar), math.sin(azimuth) * math.sin(polar), math.cos(polar))


def _name(s, size):
    return s.encode("ascii", "replace")[: size - 1].ljust(size, b"\0")


def write_md3(path, frames, normals, uvs, name="model", shader=""):
    """Write a triangle-list model.

    frames[f][c] = (x, y, z) and normals[f][c] = (x, y, z) per frame f and corner c
    (3 corners per triangle, counter-clockwise); uvs[c] = (u, v) with v up (OpenGL).
    Returns a dict with vertex/surface counts and the stored unit.
    """
    corners, nframes = len(uvs), len(frames)
    extent = max(abs(c) for fr in frames for p in fr for c in p) or 1.0
    scale = 511.0 / extent  # stored units per original unit
    qpos = [[tuple(int(round(c * scale / XYZ_SCALE)) for c in p) for p in fr] for fr in frames]
    qnrm = [[encode_normal(n) for n in fr] for fr in normals]

    # Weld corners that match in every frame (position, normal) and in uv.
    index_of, verts, tris, tri = {}, [], [], []
    for c in range(corners):
        key = (tuple(qpos[f][c] for f in range(nframes)), tuple(qnrm[f][c] for f in range(nframes)), (round(uvs[c][0], 6), round(uvs[c][1], 6)))
        if key not in index_of:
            index_of[key] = len(verts)
            verts.append(c)
        tri.append(index_of[key])
        if len(tri) == 3:
            tris.append(tuple(tri))
            tri = []

    # Split into surfaces within the MD3 limits.
    surfaces, cur_tris, cur_map = [], [], {}
    for tri in tris:
        new = [v for v in tri if v not in cur_map]
        if cur_tris and (len(cur_map) + len(set(new)) > MAX_VERTS or len(cur_tris) >= MAX_TRIS):
            surfaces.append((cur_tris, cur_map))
            cur_tris, cur_map = [], {}
        for v in tri:
            cur_map.setdefault(v, len(cur_map))
        cur_tris.append(tuple(cur_map[v] for v in tri))
    surfaces.append((cur_tris, cur_map))

    frame_blob = b""
    for f in range(nframes):
        pts = [tuple(c * XYZ_SCALE for c in p) for p in qpos[f]]
        lo = [min(p[i] for p in pts) for i in range(3)]
        hi = [max(p[i] for p in pts) for i in range(3)]
        radius = max(math.sqrt(sum(c * c for c in p)) for p in pts)
        frame_blob += FRAME.pack(*lo, *hi, 0.0, 0.0, 0.0, radius, _name("frame%d" % f, 16))

    surf_blobs = []
    for s, (stris, smap) in enumerate(surfaces):
        order = sorted(smap, key=smap.get)  # global vertex ids in local order
        tri_blob = b"".join(TRI.pack(*t) for t in stris)
        st_blob = b"".join(ST.pack(uvs[verts[v]][0], 1.0 - uvs[verts[v]][1]) for v in order)
        xyz_blob = b"".join(VERT.pack(*qpos[f][verts[v]], qnrm[f][verts[v]]) for f in range(nframes) for v in order)
        shader_blob = SHADER.pack(_name(shader, 64), 0)
        ofs_shaders = SURFACE.size
        ofs_tris = ofs_shaders + len(shader_blob)
        ofs_st = ofs_tris + len(tri_blob)
        ofs_xyz = ofs_st + len(st_blob)
        ofs_end = ofs_xyz + len(xyz_blob)
        head = SURFACE.pack(IDENT, _name("%s_%d" % (name, s), 64), 0, nframes, 1, len(order), len(stris), ofs_tris, ofs_shaders, ofs_st, ofs_xyz, ofs_end)
        surf_blobs.append(head + shader_blob + tri_blob + st_blob + xyz_blob)

    ofs_frames = HEADER.size
    ofs_surfaces = ofs_frames + len(frame_blob)
    ofs_end = ofs_surfaces + sum(len(b) for b in surf_blobs)
    unit = 1.0 / scale
    header = HEADER.pack(IDENT, VERSION, _name("%s;unit=%.9g" % (name, unit), 64), 0, nframes, 0, len(surf_blobs), 0,
                         ofs_frames, ofs_surfaces, ofs_surfaces, ofs_end)
    with open(path, "wb") as out:
        out.write(header + frame_blob + b"".join(surf_blobs))
    return {"verts": len(verts), "tris": len(tris), "surfaces": len(surf_blobs), "frames": nframes, "unit": unit, "bytes": ofs_end}


def read_md3(path):
    """Read a model back as triangle lists: (frames, normals, uvs) like write_md3's input.

    Positions are in original units when the header carries ";unit=", else in MD3 units.
    """
    data = open(path, "rb").read()
    ident, version, name, _, nframes, _, nsurf, _, _, _, ofs_surf, _ = HEADER.unpack_from(data, 0)
    if ident != IDENT or version != VERSION:
        raise ValueError("{}: not an MD3 v15 file".format(path))
    name = name.split(b"\0")[0].decode("ascii", "replace")
    unit = float(name.split(";unit=")[1]) if ";unit=" in name else 1.0
    frames = [[] for _ in range(nframes)]
    normals = [[] for _ in range(nframes)]
    uvs = []
    ofs = ofs_surf
    for _ in range(nsurf):
        (_, _, _, sframes, _, nverts, ntris, ofs_tris, _, ofs_st, ofs_xyz, ofs_end) = SURFACE.unpack_from(data, ofs)
        st = [ST.unpack_from(data, ofs + ofs_st + i * ST.size) for i in range(nverts)]
        xyz = [VERT.unpack_from(data, ofs + ofs_xyz + i * VERT.size) for i in range(nverts * sframes)]
        for t in range(ntris):
            for v in TRI.unpack_from(data, ofs + ofs_tris + t * TRI.size):
                uvs.append((st[v][0], 1.0 - st[v][1]))
                for f in range(nframes):
                    x, y, z, n = xyz[f * nverts + v]
                    frames[f].append((x * XYZ_SCALE * unit, y * XYZ_SCALE * unit, z * XYZ_SCALE * unit))
                    normals[f].append(decode_normal(n))
        ofs += ofs_end
    return frames, normals, uvs
