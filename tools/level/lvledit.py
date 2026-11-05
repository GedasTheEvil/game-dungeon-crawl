"""Set or clear single wall-layer or object-layer entries of a level file, in place.

    python3 tools/level/lvledit.py LEVEL OP...

OPs (COL ROW as `levelcheck --map` prints them, row 0 at the bottom):
    obj COL ROW TYPE ATTR VALUE    put an object (replaces the cell's object)
    wall COL ROW TYPE ATTR VALUE   put a back-wall fixture (replaces the cell's fixture)
    noobj COL ROW                  remove the cell's object
    nowall COL ROW                 remove the cell's fixture
    show COL ROW                   print the cell's object and fixture

Type numbers: `DungeonTileType` in src/world/level.h (or `levelcheck --legend`). An OP on a wall cell refuses: carve
the structure in the editor first. Run `bin/levelcheck LEVEL` after.
"""

import sys


def load(path):
    lines = open(path).read().splitlines()
    header = lines[0].split()
    width, height = int(header[2]), int(header[3])
    start = lines.index("structure") + 1
    structure = lines[start : start + height]  # top row first
    sections = {}
    order = []
    i = start + height
    while i < len(lines):
        name, count = lines[i].split()
        count = int(count)
        sections[name] = lines[i + 1 : i + 1 + count]
        order.append(name)
        i += 1 + count
    return lines[:start], structure, sections, order, width, height


def save(path, head, structure, sections, order):
    out = head + structure
    for name in order:
        out.append(f"{name} {len(sections[name])}")
        out += sections[name]
    open(path, "w").write("\n".join(out) + "\n")


def cell_key(line):
    col, row = line.split()[:2]
    return int(row), int(col)


def main(path, ops):
    head, structure, sections, order, _, height = load(path)
    i = 0
    while i < len(ops):
        op = ops[i]
        n = {"obj": 5, "wall": 5, "noobj": 2, "nowall": 2, "show": 2}.get(op)
        if n is None:
            sys.exit(f"unknown op {op}")
        args = [int(v) for v in ops[i + 1 : i + 1 + n]]
        i += 1 + n
        col, row = args[0], args[1]
        layer = "wall" if op in ("wall", "nowall") else "objects"
        if op == "show":
            for name in ("wall", "objects"):
                for line in sections[name]:
                    if cell_key(line) == (row, col):
                        print(f"{name}: {line}")
            print("structure:", structure[height - 1 - row][col])
            continue
        if op in ("obj", "wall") and structure[height - 1 - row][col] in "#=":
            sys.exit(f"{col} {row} is rock or deep water")
        entries = [line for line in sections[layer] if cell_key(line) != (row, col)]
        if op in ("obj", "wall"):
            entries.append(" ".join(str(v) for v in args))
        entries.sort(key=cell_key)
        sections[layer] = entries
    save(path, head, structure, sections, order)


if __name__ == "__main__":
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2:])
