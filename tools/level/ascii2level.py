"""Build a level file from an ASCII drawing (same legend as `levelcheck --map`).

    python3 tools/level/ascii2level.py IN.txt OUT

IN.txt: the drawing, top row first; rows are placed so the last drawn row is row 1 (row 0 and the rest stay wall),
columns start at 0. Shorter lines are padded with wall. Lines starting with ';' are comments.
Lines 'set COL ROW TYPE ATTR VALUE' override single cells (after the drawing), e.g. a lever's colour.
Lines 'def CHAR TYPE ATTR VALUE' add a character to the legend for this file, e.g. 'def L 12 2 0' (blue lever),
'def 1 8 1 1' (sword chest). Put them before the drawing.

The legend (and the level size) come from the game: `levelcheck --legend` (make builds it).
"""

import os
import subprocess
import sys

LEVELCHECK = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "levelcheck")


def game_legend():
    """(width, height, cells, {char: (type, attr, value)}) from `levelcheck --legend`."""
    try:
        out = subprocess.run([LEVELCHECK, "--legend"], capture_output=True, text=True, check=True).stdout
    except (OSError, subprocess.CalledProcessError) as e:
        sys.exit(f"cannot run {LEVELCHECK} --legend ({e}); run make first")
    lines = out.splitlines()
    _, w, h, cells = lines[0].split()
    legend = {line[0]: tuple(int(v) for v in line[2:].split()[:3]) for line in lines[1:]}
    return int(w), int(h), int(cells), legend


def main(src, dst):
    W, H, CELLS, legend = game_legend()
    cells = [(0, 0, 0)] * CELLS
    drawing, sets = [], []
    for line in open(src).read().splitlines():
        if line.startswith(";") or not line.strip():
            continue
        if line.startswith("def "):
            ch, *tile = line.split()[1:]
            legend[ch] = tuple(int(v) for v in tile)
        elif line.startswith("set "):
            sets.append([int(v) for v in line.split()[1:]])
        else:
            drawing.append(line.rstrip())
    if len(drawing) > H - 1:
        sys.exit("too many rows")
    for i, line in enumerate(drawing):
        row = len(drawing) - i
        for col, ch in enumerate(line[:W]):
            if ch not in legend:
                sys.exit(f"unknown character {ch!r} at row {row} col {col}")
            cells[row * W + col] = legend[ch]
    for col, row, a, b, c in sets:
        cells[row * W + col] = (a, b, c)
    with open(dst, "w") as f:
        f.write(f"{CELLS}\n")
        f.writelines(f"{a} {b} {c} \n" for a, b, c in cells)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
