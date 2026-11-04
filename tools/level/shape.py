#!/usr/bin/env python3
"""Set or clear cell shapes in a v6 level file (plan subcell-campaign).

usage: tools/level/shape.py LEVEL COL,ROW=MASK ...

MASK: 4 hex digits (bit = column + 4 x row, row 0 at the bottom, as the level file) or a brush name:
step, stepl, stepr, half, halfl, halfr, stairr, stairl, low1, low2, low3, lipl, lipr; 0000 or none clears it.
COL may be a range (3-6). Keeps the structure drawing's shaped glyph and the shape list in step, as the editor does.
"""

import sys

BRUSHES = {
	"none": 0x0000,
	"step": 0x000F,
	"stepl": 0x0003,
	"stepr": 0x000C,
	"half": 0x00FF,
	"halfl": 0x0033,
	"halfr": 0x00CC,
	"stairr": 0x08CE,
	"stairl": 0x0137,
	"low1": 0xF000,
	"low2": 0xFF00,
	"low3": 0xFFF0,
	"lipl": 0x0001,
	"lipr": 0x0008,
}
PLAIN_TO_SHAPED = {".": "_", "~": "-", ",": ";", "%": "&"}
SHAPED_TO_PLAIN = {v: k for k, v in PLAIN_TO_SHAPED.items()}


def parse_mask(text):
	if text.lower() in BRUSHES:
		return BRUSHES[text.lower()]
	return int(text, 16)


def main(argv):
	if len(argv) < 3:
		sys.exit(__doc__)
	path = argv[1]
	lines = open(path).read().split("\n")
	width, height = int(lines[0].split()[2]), int(lines[0].split()[3])
	top = lines.index("structure") + 1  # the drawing's first line is row height-1
	shapes_at = next(i for i, line in enumerate(lines) if line.startswith("shapes "))
	count = int(lines[shapes_at].split()[1])
	shapes = {}
	for line in lines[shapes_at + 1 : shapes_at + 1 + count]:
		col, row, mask = line.split()
		shapes[(int(col), int(row))] = int(mask, 16)
	for arg in argv[2:]:
		cell, mask_text = arg.split("=")
		cols, row = cell.split(",")
		first, _, last = cols.partition("-")
		mask = parse_mask(mask_text)
		for col in range(int(first), int(last or first) + 1):
			at = top + height - 1 - int(row)
			glyph = lines[at][col]
			plain = SHAPED_TO_PLAIN.get(glyph, glyph)
			if plain not in PLAIN_TO_SHAPED:
				sys.exit(f"{path}: cell {col},{row} is '{glyph}', not an open structure")
			new = PLAIN_TO_SHAPED[plain] if mask else plain
			lines[at] = lines[at][:col] + new + lines[at][col + 1 :]
			if mask:
				shapes[(col, int(row))] = mask
			else:
				shapes.pop((col, int(row)), None)
	ordered = sorted(shapes.items(), key=lambda item: item[0][1] * width + item[0][0])
	body = [f"{col} {row} {mask:04x}" for (col, row), mask in ordered]
	lines[shapes_at : shapes_at + 1 + count] = [f"shapes {len(body)}"] + body
	open(path, "w").write("\n".join(lines))


if __name__ == "__main__":
	main(sys.argv)
