"""Generate a font sheet like media/textures/fonts/parchment.png from a TrueType font.

    python3 tools/textures/font_sheet.py FONT.ttf out.png [--size N] [--weight W] [--stroke S]

The sheet is N x N grey (default 512), white glyphs on black, printable ASCII from ' ' in a 10 x 10 grid; cell i
starts at round(i * N / 10), so the cells match the loader's 0.1 texture steps (Font::Load in
src/graphics/font.cpp). Each glyph is drawn at 4x and downsampled, its baseline and cap height where the old papyrus
sheet had them, so the fonts can swap at the same sizes. Font::Load measures the inked width of each cell for
proportional text, so the glyphs only need to start inside their cell.
"""

import argparse

from PIL import Image, ImageDraw, ImageFont

GRID = 10
SCALE = 4
# The layout in px of a 512 sheet, scaled to the sheet size.
REF_SHEET = 512
BASELINE = 34  # px from the cell top
CAP_HEIGHT = 24
LEFT = 8


def load_font(path, weight, size):
    font = ImageFont.truetype(path, size)
    if weight is not None:
        font.set_variation_by_axes([weight])
    return font


def cap_scaled_font(path, weight, cap):
    """The font size at which 'H' is cap px tall."""
    probe = 400
    font = load_font(path, weight, probe)
    top, bottom = font.getbbox("H", anchor="ls")[1], font.getbbox("H", anchor="ls")[3]
    size = round(probe * cap / (bottom - top))
    return load_font(path, weight, size)


def edge(i, sheet):
    return round(i * sheet / GRID)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("font")
    parser.add_argument("out")
    parser.add_argument("--size", type=int, default=REF_SHEET, help="the sheet side in px")
    parser.add_argument("--weight", type=float, help="weight axis of a variable font")
    parser.add_argument("--stroke", type=float, default=0, help="extra stroke width, in px of a 512 sheet")
    args = parser.parse_args()

    sheet = args.size
    unit = sheet / REF_SHEET * SCALE  # drawing px per px of a 512 sheet
    font = cap_scaled_font(args.font, args.weight, CAP_HEIGHT * unit)
    stroke = round(args.stroke * unit)
    big = Image.new("L", (sheet * SCALE, sheet * SCALE), 0)
    for glyph in range(95):
        col, row = glyph % GRID, glyph // GRID
        x0, y0 = edge(col, sheet) * SCALE, edge(row, sheet) * SCALE
        width = (edge(col + 1, sheet) - edge(col, sheet)) * SCALE
        height = (edge(row + 1, sheet) - edge(row, sheet)) * SCALE
        cell = Image.new("L", (width, height), 0)
        ImageDraw.Draw(cell).text(
            (LEFT * unit, BASELINE * unit),
            chr(32 + glyph),
            fill=255,
            font=font,
            anchor="ls",
            stroke_width=stroke,
            stroke_fill=255,
        )
        bbox = cell.getbbox()
        if bbox and bbox[2] > width - SCALE:
            print(f"warning: {chr(32 + glyph)!r} is cut at the cell's right edge")
        big.paste(cell, (x0, y0))
    big.resize((sheet, sheet), Image.LANCZOS).save(args.out)


if __name__ == "__main__":
    main()
