"""Generate the inscription atlas media/textures/decorations/inscriptions.png (RGBA, 8 x 8 cells of 256 px, one per id).

    python3 tools/textures/inscriptions.py [inscriptions-dir] [out.png]

Reads the `id:`, `style:` and `G:` lines of content/inscriptions/*.txt (docs/inscriptions.md): cell id - 1, row by row from
the top left (INSCRIPTION_ATLAS_GRID in src/world/inscriptions.h). G: lists the signs by Gardiner code (A1, M23,
Aa1, N35A), drawn with the Noto Sans Egyptian Hieroglyphs font (SIL Open Font License; Debian/Ubuntu package
fonts-noto-extra) in painted colours with an ink outline, in the frame of the style:

    cartouche  an upright royal oval on its bar, the signs in a column (up to 5)
    column     a framed panel, the signs in a column (up to 7)
    row        a framed panel, the signs in a row (up to 7)
    stela      a round-topped slab with a winged sun, the signs in columns read right to left (up to 12)

Rerun after adding or changing an inscription's glyphs; the game reads the texts themselves at start-up.
"""

import glob
import os
import sys
import unicodedata

from PIL import Image, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from decals import CELL, INK, PAINT, bleed, layer, weather  # noqa: E402

GRID = 8
FONT = "/usr/share/fonts/truetype/noto/NotoSansEgyptianHieroglyphs-Regular.ttf"
COLOURS = ["blue", "red", "green", "black", "gold"]
STYLES = ("cartouche", "column", "row", "stela")


def sign(code):
    """'M23' -> the character of EGYPTIAN HIEROGLYPH M023; 'Aa1' -> AA001; 'N35A' -> N035A."""
    head = code[:2] if code[:2].lower() == "aa" else code[:1]
    rest = code[len(head):]
    digits = rest.rstrip("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz")
    suffix = rest[len(digits):]
    name = f"EGYPTIAN HIEROGLYPH {head.upper()}{int(digits):03d}{suffix.upper()}"
    return unicodedata.lookup(name)


def read_entries(directory):
    """{id: (style, [characters], source)} from every *.txt in directory."""
    entries = {}
    for path in sorted(glob.glob(os.path.join(directory, "*.txt"))):
        current = None
        with open(path, encoding="utf-8") as f:
            for n, raw in enumerate(f, 1):
                line = raw.strip()
                if not line or line.startswith("#") or ":" not in line:
                    continue
                key, value = (part.strip() for part in line.split(":", 1))
                where = f"{path}:{n}"
                if key == "id":
                    current = int(value)
                    if current in entries:
                        sys.exit(f"{where}: id {current} is taken by {entries[current][2]}")
                    if not 1 <= current <= GRID * GRID:
                        sys.exit(f"{where}: id must be 1 to {GRID * GRID}")
                    entries[current] = ["cartouche", [], where]
                elif current is None:
                    continue
                elif key == "style":
                    if value not in STYLES:
                        sys.exit(f"{where}: style must be one of {', '.join(STYLES)}")
                    entries[current][0] = value
                elif key == "G":
                    for code in value.split():
                        try:
                            entries[current][1].append(sign(code))
                        except (KeyError, ValueError):
                            sys.exit(f"{where}: no hieroglyph {code}")
    return entries


def glyph_mask(char, size):
    """The sign as an L image cropped to its ink, `size` px font."""
    font = ImageFont.truetype(FONT, size)
    left, top, right, bottom = font.getbbox(char)
    img = Image.new("L", (right - left + 8, bottom - top + 8), 0)
    ImageDraw.Draw(img).text((4 - left, 4 - top), char, font=font, fill=255)
    return img


def paint_sign(img, char, box, colour, size):
    """The sign centred in box (x, y, w, h), filled in colour with an ink outline, scaled to fill the box: a flat sign
    spans its width, a tall one its height."""
    x, y, w, h = box
    mask = glyph_mask(char, size)
    scale = min(w / mask.width, h / mask.height)
    mask = mask.resize((max(1, int(mask.width * scale)), max(1, int(mask.height * scale))), Image.LANCZOS)
    ox = int(x + (w - mask.width) / 2)
    oy = int(y + (h - mask.height) / 2)
    outline = mask.filter(ImageFilter.MaxFilter(7))
    img.paste(Image.new("RGBA", mask.size, INK), (ox, oy), outline)
    img.paste(Image.new("RGBA", mask.size, colour), (ox, oy), mask)


def panel(d, box):
    x0, y0, x1, y1 = box
    d.rectangle(box, fill=(232, 216, 170, 90))
    d.rectangle(box, outline=(120, 78, 40, 220), width=6)
    d.rectangle([x0 + 12, y0 + 12, x1 - 12, y1 - 12], outline=(120, 78, 40, 180), width=2)


def column_of(img, signs, x0, y0, x1, y1, first_colour=0):
    n = max(1, len(signs))
    slot = (y1 - y0) / n
    size = int(min(slot, x1 - x0) * 0.95)
    for k, char in enumerate(signs):
        colour = PAINT[COLOURS[(k + first_colour) % len(COLOURS)]]
        paint_sign(img, char, (x0, y0 + k * slot + slot * 0.06, x1 - x0, slot * 0.88), colour, size)


def cartouche(signs, seed):
    img = layer()
    d = ImageDraw.Draw(img)
    d.rounded_rectangle([168, 28, 344, 446], radius=86, fill=(232, 216, 170, 110), outline=INK, width=12)
    d.rounded_rectangle([168, 28, 344, 446], radius=86, outline=PAINT["gold"], width=6)
    d.rectangle([140, 446, 372, 476], fill=PAINT["gold"], outline=INK, width=4)
    column_of(img, signs[:5], 196, 66, 316, 412)
    return weather(img, seed, 0.35)


def column(signs, seed):
    img = layer()
    panel(ImageDraw.Draw(img), (150, 10, 362, 502))
    column_of(img, signs[:7], 182, 30, 330, 482, 1)
    return weather(img, seed, 0.4)


def row(signs, seed):
    img = layer()
    panel(ImageDraw.Draw(img), (8, 170, 504, 342))
    signs = signs[:7]
    n = max(1, len(signs))
    slot = (476 - 36) / n
    size = int(min(slot, 140) * 0.95)
    for k, char in enumerate(signs):
        paint_sign(img, char, (36 + k * slot + slot * 0.05, 192, slot * 0.9, 128), PAINT[COLOURS[(k + 2) % len(COLOURS)]], size)
    return weather(img, seed, 0.4)


def stela(signs, seed):
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
    # Columns of signs, read from the right, as on most stelae.
    signs = signs[:12]
    per = 4
    cols = max(1, (len(signs) + per - 1) // per)
    width = (400 - 112) / cols
    for c in range(cols):
        x1 = 400 - c * width
        column_of(img, signs[c * per:(c + 1) * per], x1 - width + 8, 176, x1 - 8, 488, c)
        if c > 0:
            d.line([(x1, 170), (x1, 490)], fill=(120, 78, 40, 180), width=3)
    return weather(img, seed, 0.35)


DRAW = {"cartouche": cartouche, "column": column, "row": row, "stela": stela}


def main(directory, path):
    entries = read_entries(directory)
    atlas = Image.new("RGBA", (CELL * GRID, CELL * GRID), (0, 0, 0, 0))
    for ident, (style, signs, _) in sorted(entries.items()):
        cell = DRAW[style](signs, 100 + ident).resize((CELL, CELL), Image.LANCZOS)
        k = ident - 1
        atlas.paste(cell, ((k % GRID) * CELL, (k // GRID) * CELL))
    bleed(atlas).save(path)
    print(f"wrote {path} ({len(entries)} inscriptions)")


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.join(here, "..", "..")
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(root, "content", "inscriptions"),
         sys.argv[2] if len(sys.argv) > 2 else os.path.join(root, "media", "textures", "decorations", "inscriptions.png"))
