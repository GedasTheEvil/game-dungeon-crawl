#!/usr/bin/env python3
"""Generate the level editor palette icons for the mechanism tiles.

Writes key.tga, gate_lock.tga, lever.tga and rockfall.tga to DungeonEditor/Textures/.
Format matches the other small editor icons (ankh.tga, ladder.tga, ...):
32x32, uncompressed 24-bit TGA (type 2), top-left origin (descriptor 0x20), BGR pixels,
simple pictogram on a white background. LoadTGA in DungeonEditor/textures.h reads it.

Usage: python3 tools/level/editor_icons.py
"""

import os
import struct

from PIL import Image, ImageDraw

SIZE = 32
SS = 8  # supersampling factor, drawn at SIZE*SS and downscaled for smooth edges
OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "DungeonEditor", "Textures")

WHITE = (255, 255, 255)
GOLD = (230, 170, 20)
GOLD_DARK = (150, 100, 10)
IRON = (70, 70, 80)
STONE = (120, 110, 100)
STONE_DARK = (60, 55, 50)
RED = (200, 30, 30)
WOOD = (140, 90, 40)


def s(v):
    """Scale an icon coordinate (0..32) to the supersampled canvas."""
    return int(round(v * SS))


def box(x0, y0, x1, y1):
    return [s(x0), s(y0), s(x1), s(y1)]


def canvas():
    img = Image.new("RGB", (SIZE * SS, SIZE * SS), WHITE)
    return img, ImageDraw.Draw(img)


def draw_key():
    img, d = canvas()
    # bow (ring) on the left, shaft to the right, two bits hanging down
    d.ellipse(box(2, 9, 16, 23), fill=GOLD, outline=GOLD_DARK, width=s(1))
    d.ellipse(box(6, 13, 12, 19), fill=WHITE, outline=GOLD_DARK, width=s(0.75))
    d.rectangle(box(15, 14, 30, 18), fill=GOLD, outline=GOLD_DARK, width=s(0.75))
    d.rectangle(box(22, 18, 25, 24), fill=GOLD, outline=GOLD_DARK, width=s(0.75))
    d.rectangle(box(27, 18, 30, 22), fill=GOLD, outline=GOLD_DARK, width=s(0.75))
    return img


def draw_gate():
    img, d = canvas()
    # stone arch frame with an iron portcullis and a padlock in the middle
    d.rectangle(box(1, 1, 31, 31), fill=STONE, outline=STONE_DARK, width=s(1))
    d.rectangle(box(5, 5, 27, 31), fill=WHITE)
    for x in (7, 11.5, 16, 20.5, 25):
        d.rectangle(box(x - 1, 5, x + 1, 31), fill=IRON)
    for y in (11, 20):
        d.rectangle(box(5, y - 1, 27, y + 1), fill=IRON)
    # padlock
    d.arc(box(12, 11, 20, 19), 180, 360, fill=GOLD_DARK, width=s(1.5))
    d.rectangle(box(11, 15, 21, 23), fill=GOLD, outline=GOLD_DARK, width=s(0.75))
    d.ellipse(box(15, 17, 17, 19), fill=GOLD_DARK)
    d.rectangle(box(15.5, 18, 16.5, 21), fill=GOLD_DARK)
    return img


def draw_lever():
    img, d = canvas()
    # stone base plate, tilted wooden handle with a red knob
    d.line([(s(16), s(24)), (s(25), s(6))], fill=WOOD, width=s(3))
    d.ellipse(box(21.5, 2, 28.5, 9), fill=RED, outline=(120, 10, 10), width=s(0.75))
    d.pieslice(box(8, 18, 24, 34), 180, 360, fill=STONE, outline=STONE_DARK, width=s(1))
    d.rectangle(box(4, 26, 28, 30), fill=STONE, outline=STONE_DARK, width=s(1))
    return img


def draw_rockfall():
    img, d = canvas()
    # ceiling slab with a crack, falling rock, motion lines, floor
    d.rectangle(box(0, 0, 32, 7), fill=STONE, outline=STONE_DARK, width=s(1))
    d.line([(s(10), s(7)), (s(13), s(4)), (s(12), s(2)), (s(15), s(0))], fill=(20, 20, 20), width=s(1))
    d.line([(s(21), s(7)), (s(19), s(4)), (s(22), s(1))], fill=(20, 20, 20), width=s(1))
    d.polygon([(s(11), s(8)), (s(21), s(8)), (s(18), s(9)), (s(13), s(9))], fill=WHITE)  # gap left by the rock
    for x in (12, 16, 20):
        d.line([(s(x), s(10)), (s(x), s(14))], fill=(150, 150, 150), width=s(1))
    d.polygon([(s(10), s(19)), (s(14), s(15)), (s(20), s(15)), (s(24), s(20)),
               (s(21), s(26)), (s(13), s(26))], fill=STONE, outline=STONE_DARK)
    d.line([(s(14), s(19)), (s(18), s(21))], fill=STONE_DARK, width=s(1))
    d.rectangle(box(0, 28, 32, 32), fill=STONE_DARK)
    return img


def write_tga(path, img):
    img = img.resize((SIZE, SIZE), Image.LANCZOS).convert("RGB")
    header = struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, SIZE, SIZE, 24, 0x20)
    pixels = bytearray()
    for r, g, b in img.getdata():
        pixels += bytes((b, g, r))
    with open(path, "wb") as f:
        f.write(header)
        f.write(pixels)


def main():
    icons = {
        "key.tga": draw_key,
        "gate_lock.tga": draw_gate,
        "lever.tga": draw_lever,
        "rockfall.tga": draw_rockfall,
    }
    for name, draw in icons.items():
        path = os.path.normpath(os.path.join(OUT_DIR, name))
        write_tga(path, draw())
        print("wrote", path)


if __name__ == "__main__":
    main()
