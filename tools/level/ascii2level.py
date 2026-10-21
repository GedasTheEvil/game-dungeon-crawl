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

Optional, after the object drawing (and after the structure drawing if there is one): a line 'wall', then the wall
drawing (plan wall-layer), as many rows as the object drawing: the fixtures on the back wall (lever, inscription, wall
spikes), '#' and '.' for none. A wall glyph in the object drawing (or a 'set' line with a wall type) goes to the wall
layer too, so a cell can hold a fixture and an object only with the wall drawing (or 'set').

The legend, the structure glyphs, the level size and the file format come from the game: `levelcheck --legend`
(make builds it).
"""

import os
import subprocess
import sys

LEVELCHECK = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "bin", "levelcheck")
NO_OBJECT = 1  # tile type


def game_legend():
    """(width, height, header, structure glyphs, wall types, {char: (type, attr, value, structure)}) from
    `levelcheck --legend`."""
    try:
        out = subprocess.run([LEVELCHECK, "--legend"], capture_output=True, text=True, check=True).stdout
    except (OSError, subprocess.CalledProcessError) as e:
        sys.exit(f"cannot run {LEVELCHECK} --legend ({e}); run make first")
    lines = out.splitlines()
    _, w, h = lines[0].split()
    _, magic, version = lines[1].split()
    structures = lines[2].split()[1]
    wall_types = {int(v) for v in lines[3].split()[1:]}
    # The glyph lines; "prop" and "decal" lines after them are for the --map drawings.
    legend = {line[0]: tuple(int(v) for v in line[2:].split()[:4]) for line in lines[4:] if line[1] == " "}
    return int(w), int(h), f"{magic} {version} {w} {h}", structures, wall_types, legend


def main(src, dst):
    W, H, header, STRUCTURES, WALL_TYPES, legend = game_legend()
    wall, empty = STRUCTURES.index("#"), STRUCTURES.index(".")
    objects, fixtures = {}, {}  # (col, row): (type, attr, value)
    structure = [[wall] * W for _ in range(H)]  # [row][col]
    drawing, structure_drawing, wall_drawing, sets = [], None, None, []
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
        elif line.strip() == "wall":
            wall_drawing = []
        elif wall_drawing is not None:
            wall_drawing.append(line.rstrip())
        elif structure_drawing is not None:
            structure_drawing.append(line.rstrip())
        else:
            drawing.append(line.rstrip())
    if len(drawing) > H - 1:
        sys.exit("too many rows")
    if structure_drawing is not None and len(structure_drawing) != len(drawing):
        sys.exit(f"the structure drawing has {len(structure_drawing)} rows, the object drawing {len(drawing)}")
    if wall_drawing is not None and len(wall_drawing) != len(drawing):
        sys.exit(f"the wall drawing has {len(wall_drawing)} rows, the object drawing {len(drawing)}")

    def layer(a):
        return fixtures if a in WALL_TYPES else objects

    for i, line in enumerate(drawing):
        row = len(drawing) - i
        for col, ch in enumerate(line[:W]):
            if ch not in legend:
                sys.exit(f"unknown character {ch!r} at row {row} col {col}")
            a, b, c, s = legend[ch]
            structure[row][col] = s
            if (a, b, c) != (NO_OBJECT, 0, 0):
                layer(a)[(col, row)] = (a, b, c)
    for i, line in enumerate(wall_drawing or []):
        row = len(wall_drawing) - i
        for col, ch in enumerate(line[:W]):
            if ch in "#.":
                continue
            if ch not in legend or legend[ch][0] not in WALL_TYPES:
                sys.exit(f"{ch!r} at row {row} col {col} of the wall drawing is not a wall type")
            fixtures[(col, row)] = legend[ch][:3]
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
            layer(a)[(col, row)] = (a, b, c)
            if structure[row][col] == wall:  # an object or fixture carves its cell
                structure[row][col] = empty
    with open(dst, "w") as f:
        f.write(f"{header}\nstructure\n")
        for row in range(H - 1, -1, -1):
            f.write("".join(STRUCTURES[s] for s in structure[row]) + "\n")
        for name, things in (("wall", fixtures), ("objects", objects)):
            f.write(f"{name} {len(things)}\n")
            for (col, row), (a, b, c) in sorted(things.items(), key=lambda kv: (kv[0][1], kv[0][0])):
                f.write(f"{col} {row} {a} {b} {c}\n")
        f.write("decor 0\ndecals 0\n")  # no placed props or decals: the editor paints them (plan placed-decor)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
