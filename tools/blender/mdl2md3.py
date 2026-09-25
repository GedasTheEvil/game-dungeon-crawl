"""Convert legacy text .mdl models to .md3 and verify the result (plain python3, Blender not needed).

    python3 tools/blender/mdl2md3.py Models/*.mdl

Writes <name>.md3 next to each input. Legacy files without per-frame normals get their frame-0
normals repeated, which is exactly how the old loader shaded them.
"""

import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import md3  # noqa: E402


def read_mdl(path):
    with open(path) as f:
        header = f.readline().split()
        vals = list(map(float, f.read().split()))
    vcount, fcount = int(header[0]), int(header[1])
    per_frame_normals = len(header) > 2 and header[2] == "1"
    pos = 0

    def take(n, width):
        nonlocal pos
        chunk = vals[pos : pos + n * width]
        pos += n * width
        return [tuple(chunk[i : i + width]) for i in range(0, len(chunk), width)]

    frames = [take(vcount, 3)]
    normals = [take(vcount, 3)]
    uvs = take(vcount, 2)
    for _ in range(fcount - 1):
        frames.append(take(vcount, 3))
        normals.append(take(vcount, 3) if per_frame_normals else normals[0])
    if pos != len(vals):
        raise ValueError("{}: {} floats left over".format(path, len(vals) - pos))
    return frames, normals, uvs


def angle(a, b):
    la = math.sqrt(sum(c * c for c in a)) or 1.0
    lb = math.sqrt(sum(c * c for c in b)) or 1.0
    return math.degrees(math.acos(max(-1.0, min(1.0, sum(x * y for x, y in zip(a, b)) / (la * lb)))))


def convert(path):
    frames, normals, uvs = read_mdl(path)
    out = os.path.splitext(path)[0] + ".md3"
    stats = md3.write_md3(out, frames, normals, uvs, name=os.path.splitext(os.path.basename(path))[0])
    rf, rn, ruv = md3.read_md3(out)
    # The writer only welds identical corners, so corner order survives the round trip.
    extent = max(abs(c) for fr in frames for p in fr for c in p)
    pos_err = max(abs(a - b) for f0, f1 in zip(frames, rf) for p, q in zip(f0, f1) for a, b in zip(p, q)) / extent
    nrm_err = max(angle(a, b) for f0, f1 in zip(normals, rn) for a, b in zip(f0, f1) if any(a))
    uv_err = max(abs(a - b) for p, q in zip(uvs, ruv) for a, b in zip(p, q))
    print("{:22s} {:>9d} -> {:>8d} bytes  verts {:>5d}  surfaces {}  frames {:>2d}  pos err {:.5%}  normal err {:.2f} deg  uv err {:.1e}".format(
        os.path.basename(path), os.path.getsize(path), stats["bytes"], stats["verts"], stats["surfaces"], stats["frames"], pos_err, nrm_err, uv_err))


if __name__ == "__main__":
    for p in sys.argv[1:]:
        convert(p)
