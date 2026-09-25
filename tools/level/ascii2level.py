"""Build a level file from an ASCII drawing (same legend as `levelcheck --map`).

    python3 tools/level/ascii2level.py IN.txt OUT

IN.txt: the drawing, top row first; rows are placed so the last drawn row is row 1 (row 0 and the rest stay wall),
columns start at 0. Shorter lines are padded with wall. Lines starting with ';' are comments.
Lines 'set COL ROW TYPE ATTR VALUE' override single cells (after the drawing), e.g. a lever's colour.

Legend: # wall  . open  S entrance  E exit  A ankh  ? riddle gate  D decoration gate  H ladder  $ treasure (small
potion)  ^ spikes  X death trap  v rock fall  s scarab  w worm  p plant  n anubis  t rat  T giant rat
r b g y keys  R B G Y gates  / lever (red, override with 'set')
"""

import sys

W, H = 40, 47
CELLS = W * H + 1
LEGEND = {
    "#": (0, 0, 0), ".": (1, 0, 0), "S": (2, 1, 0), "E": (2, 2, 0), "?": (2, 3, 0), "D": (2, 0, 0),
    "X": (3, 0, 0), "^": (5, 0, 0), "H": (6, 0, 0), "$": (8, 3, 0), "A": (9, 0, 0), "v": (13, 0, 0),
    "s": (4, 1, 0), "w": (4, 2, 0), "p": (4, 3, 0), "n": (4, 4, 0), "t": (4, 5, 0), "T": (4, 6, 0),
    "r": (10, 1, 0), "b": (10, 2, 0), "g": (10, 3, 0), "y": (10, 4, 0),
    "R": (11, 1, 0), "B": (11, 2, 0), "G": (11, 3, 0), "Y": (11, 4, 0), "/": (12, 1, 0),
}


def main(src, dst):
    cells = [(0, 0, 0)] * CELLS
    drawing, sets = [], []
    for line in open(src).read().splitlines():
        if line.startswith(";") or not line.strip():
            continue
        if line.startswith("set "):
            sets.append([int(v) for v in line.split()[1:]])
        else:
            drawing.append(line.rstrip())
    if len(drawing) > H - 1:
        sys.exit("too many rows")
    for i, line in enumerate(drawing):
        row = len(drawing) - i
        for col, ch in enumerate(line[:W]):
            if ch not in LEGEND:
                sys.exit(f"unknown character {ch!r} at row {row} col {col}")
            cells[row * W + col] = LEGEND[ch]
    for col, row, a, b, c in sets:
        cells[row * W + col] = (a, b, c)
    with open(dst, "w") as f:
        f.write(f"{CELLS}\n")
        f.writelines(f"{a} {b} {c} \n" for a, b, c in cells)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
