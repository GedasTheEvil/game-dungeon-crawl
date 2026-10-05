"""Build a level file from an ASCII drawing (same legend as `levelcheck --map`).

    python3 tools/level/ascii2level.py IN.txt OUT

IN.txt: the object drawing, top row first; rows are placed so the last drawn row is row 1 (row 0 and the rest stay
wall), columns start at 0. Shorter lines are padded with wall. Lines starting with ';' are comments.
Lines 'set COL ROW TYPE ATTR VALUE' override the object of single cells (after the drawing), e.g. a lever's colour.
Lines 'def CHAR TYPE ATTR VALUE' add a character to the legend for this file, e.g. 'def L 12 2 0' (blue lever),
'def 1 8 1 1' (sword chest). Put them before the drawing.

Optional: a line 'structure', then the structure drawing, as many rows as the object drawing: '#' wall, '.' empty,
'~' half water, '=' deep water (the structure glyphs come from the game). Then '#' and '.' in the object drawing are
just "no object". Without it, the structure follows from the object drawing: '#' wall, anything else empty.

The legend, the structure glyphs, the level size and the file format come from the game: `levelcheck --legend`
(make builds it).
"""

import os
import subprocess
import sys

LEVELCHECK = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "levelcheck")
NO_OBJECT = 1  # tile type


def game_legend():
    """(width, height, header, structure glyphs, {char: (type, attr, value, structure)}) from `levelcheck --legend`."""
    try:
        out = subprocess.run([LEVELCHECK, "--legend"], capture_output=True, text=True, check=True).stdout
    except (OSError, subprocess.CalledProcessError) as e:
        sys.exit(f"cannot run {LEVELCHECK} --legend ({e}); run make first")
    lines = out.splitlines()
    _, w, h = lines[0].split()
    _, magic, version = lines[1].split()
    structures = lines[2].split()[1]
    legend = {line[0]: tuple(int(v) for v in line[2:].split()[:4]) for line in lines[3:]}
    return int(w), int(h), f"{magic} {version} {w} {h}", structures, legend


def main(src, dst):
    W, H, header, STRUCTURES, legend = game_legend()
    wall, empty = STRUCTURES.index("#"), STRUCTURES.index(".")
    objects = {}  # (col, row): (type, attr, value)
    structure = [[wall] * W for _ in range(H)]  # [row][col]
    drawing, structure_drawing, sets = [], None, []
    for line in open(src).read().splitlines():
        if line.startswith(";") or not line.strip():
            continue
        if line.startswith("def "):
            ch, *tile = line.split()[1:]
            legend[ch] = tuple(int(v) for v in tile) + (empty,)
        elif line.startswith("set "):
            sets.append([int(v) for v in line.split()[1:]])
        elif line.strip() == "structure":
            structure_drawing = []
        elif structure_drawing is not None:
            structure_drawing.append(line.rstrip())
        else:
            drawing.append(line.rstrip())
    if len(drawing) > H - 1:
        sys.exit("too many rows")
    if structure_drawing is not None and len(structure_drawing) != len(drawing):
        sys.exit(f"the structure drawing has {len(structure_drawing)} rows, the object drawing {len(drawing)}")
    for i, line in enumerate(drawing):
        row = len(drawing) - i
        for col, ch in enumerate(line[:W]):
            if ch not in legend:
                sys.exit(f"unknown character {ch!r} at row {row} col {col}")
            a, b, c, s = legend[ch]
            structure[row][col] = s
            if (a, b, c) != (NO_OBJECT, 0, 0):
                objects[(col, row)] = (a, b, c)
    if structure_drawing is not None:
        for i, line in enumerate(structure_drawing):
            row = len(structure_drawing) - i
            for col in range(W):
                ch = line[col] if col < len(line) else "#"
                if ch not in STRUCTURES:
                    sys.exit(f"unknown structure {ch!r} at row {row} col {col}")
                structure[row][col] = STRUCTURES.index(ch)
    for col, row, a, b, c in sets:
        if (a, b, c) == (NO_OBJECT, 0, 0):
            objects.pop((col, row), None)
        else:
            objects[(col, row)] = (a, b, c)
            if structure[row][col] == wall:  # an object carves its cell
                structure[row][col] = empty
    with open(dst, "w") as f:
        f.write(f"{header}\nstructure\n")
        for row in range(H - 1, -1, -1):
            f.write("".join(STRUCTURES[s] for s in structure[row]) + "\n")
        f.write(f"objects {len(objects)}\n")
        for (col, row), (a, b, c) in sorted(objects.items(), key=lambda kv: (kv[0][1], kv[0][0])):
            f.write(f"{col} {row} {a} {b} {c}\n")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
