"""Generate a font sheet like fonts/papyrus.png from a TrueType font.

    python3 tools/textures/font_sheet.py FONT.ttf out.png [--weight W]

The sheet is 512 x 512 grey, white glyphs on black, printable ASCII from ' ' in a 10 x 10 grid of 51 px cells
(Font::Load in src/graphics/font.cpp). Each glyph is drawn at 4x and downsampled, its baseline and cap height where
the papyrus sheet has them, so the fonts can swap at the same sizes. Font::Load measures the inked width of each
cell for proportional text, so the glyphs only need to start inside their cell.
"""

import argparse

from PIL import Image, ImageDraw, ImageFont

SHEET = 512
GRID = 10
CELL = SHEET // GRID  # 51, as the loader reads it
SCALE = 4
BASELINE = 34  # px from the cell top, as in papyrus.png
CAP_HEIGHT = 24
LEFT = 8


def load_font(path, weight, size):
    font = ImageFont.truetype(path, size)
    if weight is not None:
        font.set_variation_by_axes([weight])
    return font


def cap_scaled_font(path, weight):
    """The font size at which 'H' is CAP_HEIGHT tall (at the drawing scale)."""
    probe = 100 * SCALE
    font = load_font(path, weight, probe)
    top, bottom = font.getbbox("H", anchor="ls")[1], font.getbbox("H", anchor="ls")[3]
    size = round(probe * CAP_HEIGHT * SCALE / (bottom - top))
    return load_font(path, weight, size)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("font")
    parser.add_argument("out")
    parser.add_argument("--weight", type=float, help="weight axis of a variable font")
    args = parser.parse_args()

    font = cap_scaled_font(args.font, args.weight)
    big = Image.new("L", (SHEET * SCALE, SHEET * SCALE), 0)
    draw = ImageDraw.Draw(big)
    for glyph in range(95):
        ch = chr(32 + glyph)
        x0 = (glyph % GRID) * CELL * SCALE
        y0 = (glyph // GRID) * CELL * SCALE
        cell = Image.new("L", (CELL * SCALE, CELL * SCALE), 0)
        ImageDraw.Draw(cell).text((LEFT * SCALE, BASELINE * SCALE), ch, fill=255, font=font, anchor="ls")
        bbox = cell.getbbox()
        if bbox and bbox[2] > CELL * SCALE - SCALE:
            print(f"warning: {ch!r} is cut at the cell's right edge")
        big.paste(cell, (x0, y0))
    del draw
    big.resize((SHEET, SHEET), Image.LANCZOS).save(args.out)


if __name__ == "__main__":
    main()
