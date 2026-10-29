"""Walk stride of a walk clip: how far a planted foot goes back in one round (plain Python, no Blender).

    python3 tools/blender/md3_stride.py media/models/monsters/anubis.md3 ...
Prints the stride in model units (the largest extent of frame 0 is 1, as the game's Centrify), the number
MonsterKind::walkStride takes (plan walk-clip-stride). Per step between two frames: the median move along the walk (z)
of the vertices within 1% of the height above that frame's floor in both frames, the planted feet. The stride is the
75th percentile of those steps times the frames: the feet in the air (no vertex planted) or two feet at once do not
shorten it. "flat": the share of steps at the stride's pace, 1 for an even walk (the scorpion), about 0.4 for a walk
that slows between steps (Anubis, the mummy). A clip that is not a stride (a gallop, a slither, a waddle) measures as
nonsense, its flat low (rat 0.2, worm 0.1): give it the stride that looks right instead.
"""

import os
import statistics
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from md3 import read_md3  # noqa: E402

FOOT_BAND = 0.01  # of the height: a vertex this near the floor is planted


def steps(path):
    frames, _, _ = read_md3(path)
    first = frames[0]
    extent = max(max(p[i] for p in first) - min(p[i] for p in first) for i in range(3))
    height = max(p[1] for p in first) - min(p[1] for p in first)
    floors = [min(p[1] for p in f) for f in frames]
    moves = []
    for f in range(len(frames)):
        g = (f + 1) % len(frames)
        a, b = frames[f], frames[g]
        planted = [i for i in range(len(a))
                   if a[i][1] < floors[f] + FOOT_BAND * height and b[i][1] < floors[g] + FOOT_BAND * height]
        moves.append(abs(statistics.median(b[i][2] - a[i][2] for i in planted)) / extent if planted else 0.0)
    return moves


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    for path in sys.argv[1:]:
        moves = steps(path)
        n = len(moves)
        stride = sorted(moves)[int(0.75 * n)] * n
        # Flat: the share of steps within 15% of the stride's pace, 1 for a clean walk at an even pace.
        pace = stride / n
        flat = sum(1 for m in moves if pace > 0 and abs(m - pace) <= 0.15 * pace) / n
        print("{:40s} {:3d} frames  stride {:.3f}  flat {:.2f}".format(path, n, stride, flat))


if __name__ == "__main__":
    main()
