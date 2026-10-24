"""Generate the parts the game composes the inscriptions from (plan composed-inscriptions):

    python3 tools/textures/inscriptions.py [content-dir] [out-dir]

    inscription_plates.png  the empty frames, 2 x 2 cells of 512 px in INSCRIPTION_STYLES order (cartouche, column,
                            row, stela; src/world/inscription_layout.h)
    hieroglyphs.png         the hieroglyph sheet: 16 x 16 cells of 128 px, one sign each, white fill and ink outline
                            (the game tints the fill with the paint colour)
    hieroglyphs.txt         its manifest: one line per cell, row by row from the top left, the Gardiner code and the
                            sign's width / height in its cell

The signs are every code on a `G:` line of content/inscriptions/*.txt and content/names/*.txt (docs/inscriptions.md),
sorted, so the cell order is stable. G: lists the signs by Gardiner code (A1, M23, Aa1, N35A), drawn with the Noto
Sans Egyptian Hieroglyphs font (SIL Open Font License; Debian/Ubuntu package fonts-noto-extra). The game lays them
out in the plate of the inscription's style (inscriptionLayout in src/world/inscription_layout.cpp):

    cartouche  an upright royal oval on its bar, the signs in a column (up to 5)
    column     a framed panel, the signs in a column (up to 7)
    row        a framed panel, the signs in a row (up to 7)
    stela      a round-topped slab with a winged sun, the signs in columns read right to left (up to 12)

Rerun after adding a sign no G: line used before; the game reads the texts, styles and G: lines at start-up.
"""

import glob
import os
import sys
import unicodedata

from PIL import Image, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from decals import INK, PAINT, S, bleed, layer, weather  # noqa: E402

PLATE_GRID = 2  # PLATE_GRID in src/world/inscription_layout.h; the plates are drawn at S = 512 px
SIGN_GRID = 16  # HIEROGLYPH_SHEET_GRID
SIGN_PX = 128  # a sign's cell; drawn at S and scaled down
SIGN_GUTTER = 4  # px of a cell kept clear around the sign (HIEROGLYPH_GUTTER), so filtering does not reach the next
SIGN_OUTLINE = 21  # the ink outline's MaxFilter size at S: about a cartouche's outline before
FONT = "/usr/share/fonts/truetype/noto/NotoSansEgyptianHieroglyphs-Regular.ttf"
STYLES = ("cartouche", "column", "row", "stela")
WHITE = (255, 255, 255, 235)  # the fill; the paint colours have this alpha too


def sign(code):
    """'M23' -> the character of EGYPTIAN HIEROGLYPH M023; 'Aa1' -> AA001; 'N35A' -> N035A."""
    head = code[:2] if code[:2].lower() == "aa" else code[:1]
    rest = code[len(head):]
    digits = rest.rstrip("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz")
    suffix = rest[len(digits):]
    name = f"EGYPTIAN HIEROGLYPH {head.upper()}{int(digits):03d}{suffix.upper()}"
    return unicodedata.lookup(name)


def read_codes(directories):
    """{code: first file:line} of every G: line in the *.txt files of the directories."""
    codes = {}
    for directory in directories:
        for path in sorted(glob.glob(os.path.join(directory, "*.txt"))):
            with open(path, encoding="utf-8") as f:
                for n, raw in enumerate(f, 1):
                    key, _, value = raw.strip().partition(":")
                    if key.strip() != "G":
                        continue
                    for code in value.split():
                        try:
                            sign(code)
                        except (KeyError, ValueError):
                            sys.exit(f"{path}:{n}: no hieroglyph {code}")
                        codes.setdefault(code, f"{path}:{n}")
    return codes


def glyph_mask(char, size):
    """The sign as an L image cropped to its ink, `size` px font."""
    font = ImageFont.truetype(FONT, size)
    left, top, right, bottom = font.getbbox(char)
    img = Image.new("L", (right - left + 8, bottom - top + 8), 0)
    ImageDraw.Draw(img).text((4 - left, 4 - top), char, font=font, fill=255)
    return img


def sign_cell(char, seed):
    """The sign centred in an S x S layer, as large as fits with its outline, and its width / height."""
    inner = S - 2 * SIGN_GUTTER * S // SIGN_PX
    pad = SIGN_OUTLINE // 2 + 2
    mask = glyph_mask(char, inner)
    scale = min((inner - 2 * pad) / mask.width, (inner - 2 * pad) / mask.height)
    mask = mask.resize((max(1, int(mask.width * scale)), max(1, int(mask.height * scale))), Image.LANCZOS)
    framed = Image.new("L", (mask.width + 2 * pad, mask.height + 2 * pad), 0)
    framed.paste(mask, (pad, pad))
    outline = framed.filter(ImageFilter.MaxFilter(SIGN_OUTLINE))
    img = layer()
    at = ((S - framed.width) // 2, (S - framed.height) // 2)
    img.paste(Image.new("RGBA", framed.size, INK), at, outline)
    img.paste(Image.new("RGBA", framed.size, WHITE), at, framed)
    return weather(img, seed, 0.25, grain=6), framed.width / framed.height


def panel(d, box):
    x0, y0, x1, y1 = box
    d.rectangle(box, fill=(232, 216, 170, 90))
    d.rectangle(box, outline=(120, 78, 40, 220), width=6)
    d.rectangle([x0 + 12, y0 + 12, x1 - 12, y1 - 12], outline=(120, 78, 40, 180), width=2)


def cartouche():
    img = layer()
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([168, 28, 344, 446], radius=86, fill=(232, 216, 170, 110), outline=INK, width=12)
    d.rounded_rectangle([168, 28, 344, 446], radius=86, outline=PAINT["gold"], width=6)
    d.rectangle([140, 446, 372, 476], fill=PAINT["gold"], outline=INK, width=4)
    return weather(img, 101, 0.35)


def column():
    img = layer()
    panel(ImageDraw.Draw(img), (150, 10, 362, 502))
    return weather(img, 102, 0.4)


def row():
    img = layer()
    panel(ImageDraw.Draw(img), (8, 170, 504, 342))
    return weather(img, 103, 0.4)


def stela():
    img = layer()
    d = ImageDraw.Draw(img)
    slab = (232, 216, 170, 120)
    d.rectangle([70, 150, 442, 500], fill=slab)
    d.pieslice([70, 20, 442, 280], 180, 360, fill=slab)
    d.arc([70, 20, 442, 280], 180, 360, fill=INK, width=8)
    d.line([(70, 150), (70, 500), (442, 500), (442, 150)], fill=INK, width=8)
    # The winged sun in the lunette.
    for s in (-1, 1):
        d.polygon([(256, 100), (256 + s * 150, 82), (256 + s * 160, 110), (256, 128)], fill=PAINT["blue"], outline=INK, width=3)
    d.ellipse([234, 84, 278, 128], fill=PAINT["red"], outline=INK, width=4)
    d.line([(96, 160), (416, 160)], fill=INK, width=4)
    return weather(img, 104, 0.35)


DRAW = {"cartouche": cartouche, "column": column, "row": row, "stela": stela}


def main(content, out):
    plates = Image.new("RGBA", (S * PLATE_GRID, S * PLATE_GRID), (0, 0, 0, 0))
    for k, style in enumerate(STYLES):
        plates.paste(DRAW[style](), ((k % PLATE_GRID) * S, (k // PLATE_GRID) * S))
    path = os.path.join(out, "inscription_plates.png")
    bleed(plates).save(path)
    print(f"wrote {path}")

    codes = sorted(read_codes([os.path.join(content, "inscriptions"), os.path.join(content, "names")]))
    if len(codes) > SIGN_GRID * SIGN_GRID:
        sys.exit(f"{len(codes)} signs, the sheet holds {SIGN_GRID * SIGN_GRID}")
    sheet = Image.new("RGBA", (SIGN_PX * SIGN_GRID, SIGN_PX * SIGN_GRID), (0, 0, 0, 0))
    lines = ["# The hieroglyph sheet's cells (tools/textures/inscriptions.py): Gardiner code, width / height"]
    for k, code in enumerate(codes):
        cell, aspect = sign_cell(sign(code), 500 + k)
        sheet.paste(cell.resize((SIGN_PX, SIGN_PX), Image.LANCZOS), ((k % SIGN_GRID) * SIGN_PX, (k // SIGN_GRID) * SIGN_PX))
        lines.append(f"{code} {aspect:.3f}")
    path = os.path.join(out, "hieroglyphs.png")
    bleed(sheet).save(path)
    with open(os.path.join(out, "hieroglyphs.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    print(f"wrote {path} and its manifest ({len(codes)} signs)")


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.join(here, "..", "..")
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "content"),
         sys.argv[2] if len(sys.argv) > 2 else os.path.join(root, "media", "textures", "decorations"))
