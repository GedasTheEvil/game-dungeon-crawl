"""Size and extents of .md3 files, and how they changed against a git revision (plain Python, no Blender).

    python3 tools/blender/md3_stats.py [--diff [REV]] <file.md3> ...
Per file: frames, triangles, bytes, the extents of frame 0 and of all frames (original units, game space: Y up).
--diff compares each file with its version at REV (default HEAD): the same numbers then and now, and, when the
triangles are the same, the largest corner move at frame 0 and over all frames. Run it after an --export.
"""

import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from md3 import read_md3  # noqa: E402


def extents(points):
    lo = [min(p[i] for p in points) for i in range(3)]
    hi = [max(p[i] for p in points) for i in range(3)]
    return [h - l for l, h in zip(lo, hi)]


def stats(path):
    frames, _, _ = read_md3(path)
    return {
        "frames": frames,
        "tris": len(frames[0]) // 3,
        "bytes": os.path.getsize(path),
        "ext0": extents(frames[0]),
        "ext": extents([p for f in frames for p in f]),
    }


def fmt(s):
    return "{:3d} frames {:6d} tris {:8d} B  frame 0 {}  all {}".format(
        len(s["frames"]), s["tris"], s["bytes"], " x ".join("%.3f" % e for e in s["ext0"]), " x ".join("%.3f" % e for e in s["ext"]))


def at_revision(path, rev):
    """The file's stats at rev, or None when it is not in git there."""
    blob = subprocess.run(["git", "show", "{}:./{}".format(rev, path)], capture_output=True)
    if blob.returncode != 0:
        return None
    with tempfile.NamedTemporaryFile(suffix=".md3") as tmp:
        tmp.write(blob.stdout)
        tmp.flush()
        return stats(tmp.name)


def max_move(a, b):
    return max(sum((x - y) ** 2 for x, y in zip(p, q)) ** 0.5 for p, q in zip(a, b))


def main(argv):
    rev = None
    if argv and argv[0] == "--diff":
        argv = argv[1:]
        rev = argv.pop(0) if argv and not argv[0].endswith(".md3") else "HEAD"
    if not argv:
        sys.exit(__doc__)
    for path in argv:
        now = stats(path)
        print("{}\n  {:8s} {}".format(path, "now", fmt(now)))
        if rev is None:
            continue
        old = at_revision(path, rev)
        if old is None:
            print("  {}: not in git".format(rev))
            continue
        print("  {:8s} {}".format(rev, fmt(old)))
        change = ["{:+.1f}%".format(100 * (n / o - 1)) if o else "-" for n, o in zip(now["ext"], old["ext"])]
        print("  extents (all frames) changed {}".format(" ".join(change)))
        if now["tris"] == old["tris"]:
            moves = [max_move(a, b) for a, b in zip(now["frames"], old["frames"])]
            print("  largest corner move: frame 0 {:.4f}, all frames {:.4f}{}".format(
                moves[0], max(moves), "" if len(now["frames"]) == len(old["frames"]) else " (frame counts differ)"))


if __name__ == "__main__":
    main(sys.argv[1:])
