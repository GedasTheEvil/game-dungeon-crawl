"""Generate a font sheet like media/textures/fonts/parchment.png from a TrueType font.

    python3 tools/textures/font_sheet.py FONT.ttf out.png [--size N] [--weight W] [--stroke S]
    python3 tools/textures/font_sheet.py --pad-glyphs SHEET.png [--stroke S]

The sheet is N x N grey (default 512), white glyphs on black, printable ASCII from ' ' in a 10 x 10 grid; cell i
starts at round(i * N / 10), so the cells match the loader's 0.1 texture steps (Font::Load in
src/render/font.cpp). Each glyph is drawn at 4x and downsampled, its baseline and cap height where the old papyrus
sheet had them, so the fonts can swap at the same sizes. Font::Load measures the inked width of each cell for
proportional text, so the glyphs only need to start inside their cell.

Cells 95-99 hold the gamepad's symbols (plan sdl3-gamepad, src/render/glyphs.h): the PlayStation face buttons'
cross, circle, square and triangle as outlines the cap height tall, then the d-pad's filled up arrow, centred in its
quad (the game draws it turned for down, left and right). --pad-glyphs draws them into an existing sheet (its
font file may be gone) and leaves the other cells as they are.
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


PAD_GLYPHS = ["cross", "circle", "square", "triangle", "up"]  # the game turns the up arrow for the others
FIRST_PAD_CELL = 95
PAD_STROKE = 3.5  # px of a 512 sheet, about a letter's stem


def draw_pad_glyph(draw, shape, cell, unit, stroke):
    """One symbol in the cell (x0, y0, x1, y1) of the drawing: from LEFT, the baseline up to the cap height; the arrow
    centred in the cell, so it stays in place turned."""
    x0, y0, x1, y1 = cell
    size = CAP_HEIGHT * unit
    left = x0 + LEFT * unit
    bottom = y0 + BASELINE * unit
    if shape == "up":
        left = (x0 + x1 - size) / 2
        bottom = (y0 + y1 + size) / 2
    box = [left, bottom - size, left + size, bottom]
    width = round((PAD_STROKE + stroke) * unit)
    inset = width / 2
    inner = [box[0] + inset, box[1] + inset, box[2] - inset, box[3] - inset]
    if shape == "cross":
        draw.line([inner[0], inner[1], inner[2], inner[3]], fill=255, width=width)
        draw.line([inner[0], inner[3], inner[2], inner[1]], fill=255, width=width)
    elif shape == "circle":
        draw.ellipse(box, outline=255, width=width)
    elif shape == "square":
        draw.rectangle(box, outline=255, width=width)
    elif shape == "triangle":
        mid = (box[0] + box[2]) / 2
        draw.polygon([(mid, box[1]), (box[2], box[3]), (box[0], box[3])], outline=255, width=width)
    else:  # the d-pad's up arrow, filled, a little smaller than the cap height
        l, t, r, b = box[0] + size * 0.12, box[1] + size * 0.12, box[2] - size * 0.12, box[3] - size * 0.12
        draw.polygon([((l + r) / 2, t), (r, b), (l, b)], fill=255)


def draw_pad_glyphs(big, sheet, stroke):
    """The pad symbols into cells 95-99 of a sheet drawn at SCALE."""
    unit = sheet / REF_SHEET * SCALE
    draw = ImageDraw.Draw(big)
    for i, shape in enumerate(PAD_GLYPHS):
        glyph = FIRST_PAD_CELL + i
        col, row = glyph % GRID, glyph // GRID
        x0, y0 = edge(col, sheet) * SCALE, edge(row, sheet) * SCALE
        x1, y1 = edge(col + 1, sheet) * SCALE, edge(row + 1, sheet) * SCALE
        draw.rectangle([x0, y0, x1 - 1, y1 - 1], fill=0)
        draw_pad_glyph(draw, shape, (x0, y0, x1, y1), unit, stroke)


def add_pad_glyphs(path, stroke):
    """Draws the pad symbols into an existing sheet, the other cells kept."""
    sheet_image = Image.open(path).convert("L")
    sheet = sheet_image.size[0]
    big = Image.new("L", (sheet * SCALE, sheet * SCALE), 0)
    draw_pad_glyphs(big, sheet, stroke)
    small = big.resize((sheet, sheet), Image.LANCZOS)
    for i in range(len(PAD_GLYPHS)):
        glyph = FIRST_PAD_CELL + i
        col, row = glyph % GRID, glyph // GRID
        cell = (edge(col, sheet), edge(row, sheet), edge(col + 1, sheet), edge(row + 1, sheet))
        sheet_image.paste(small.crop(cell), cell[:2])
    sheet_image.save(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("font", help="the TrueType font, or with --pad-glyphs the sheet to draw them into")
    parser.add_argument("out", nargs="?")
    parser.add_argument("--pad-glyphs", action="store_true", help="only draw the pad symbols into the sheet FONT")
    parser.add_argument("--size", type=int, default=REF_SHEET, help="the sheet side in px")
    parser.add_argument("--weight", type=float, help="weight axis of a variable font")
    parser.add_argument("--stroke", type=float, default=0, help="extra stroke width, in px of a 512 sheet")
    args = parser.parse_args()
    if args.pad_glyphs:
        add_pad_glyphs(args.font, args.stroke)
        return
    if args.out is None:
        parser.error("out is required")

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
    draw_pad_glyphs(big, sheet, args.stroke)
    big.resize((sheet, sheet), Image.LANCZOS).save(args.out)


if __name__ == "__main__":
    main()
