"""Generate the HUD and inventory tab icon atlas textures/ui/hud_icons.png (RGBA, 8 x 4 cells of 128 px).

    python3 tools/textures/hud_icons.py [out.png]

Flat glyphs with an ink outline, drawn at 4x and downsampled. Cell order must match PlayerHud::Icon in
src/ui/player_hud.h: the potion, amulet and ring, then from cell 8 on the weapons in ItemKind order. The weapons lie diagonally, the grip bottom left. The flask is white: the HUD tints it with the
potion colour.
"""

import math
import os
import sys

from PIL import Image, ImageDraw, ImageFilter

from decals import bleed

CELL = 128
S = CELL * 4  # drawing resolution per cell
GRID_X, GRID_Y = 8, 4
OUTLINE = 18  # ink round the whole glyph, in drawing pixels
LINE = 6  # ink between the parts

INK = (40, 28, 18, 255)
WOOD = (150, 98, 52, 255)
LEATHER = (110, 62, 34, 255)
METAL = (222, 216, 196, 255)
GOLD = (214, 160, 52, 255)
GLASS = (200, 200, 200, 255)
WHITE = (255, 255, 255, 255)
STRING = (236, 226, 196, 255)
LAPIS = (40, 78, 170, 255)

ANGLE = -math.pi / 4  # the weapon axis, up to the right (image y points down)


def to_image(pts):
    """Weapon frame (x along the weapon, centred; y across) to image pixels."""
    c, s = math.cos(ANGLE), math.sin(ANGLE)
    return [(S / 2 + x * c - y * s, S / 2 + x * s + y * c) for x, y in pts]


def box(x0, x1, half):
    return [(x0, -half), (x1, -half), (x1, half), (x0, half)]


def part(d, pts, colour):
    d.polygon(to_image(pts), fill=colour, outline=INK, width=LINE)

def disc(d, x, y, r, colour):
    (cx, cy), = to_image([(x, y)])
    d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=colour, outline=INK, width=LINE)

def outlined(img):
    """Ink round the glyph: its alpha grown by OUTLINE, under the glyph."""
    grown = img.getchannel("A").filter(ImageFilter.MaxFilter(2 * OUTLINE + 1)).filter(ImageFilter.GaussianBlur(1.5))
    ink = Image.new("RGBA", img.size, INK)
    ink.putalpha(grown)
    return Image.alpha_composite(ink, img)

def club():
    img = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(img)
    part(d, box(-270, -130, 17), LEATHER)
    part(d, [(-130, -20), (190, -58), (250, -46), (272, 0), (250, 46), (190, 58), (-130, 20)], WOOD)
    part(d, box(-150, -120, 26), GOLD)
    for x, y in ((90, -22), (150, 26), (210, -18)):
        disc(d, x, y, 16, GOLD)
    return img

def sword():
    img = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(img)
    part(d, [(-165, -34), (200, -30), (285, 0), (200, 30), (-165, 34)], METAL)
    d.line(to_image([(-150, 0), (215, 0)]), fill=INK, width=LINE)
    part(d, box(-262, -185, 15), LEATHER)
    part(d, box(-192, -160, 78), GOLD)
    disc(d, -275, 0, 26, GOLD)
    return img

def spear():
    img = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(img)
    part(d, box(-295, 150, 16), WOOD)
    part(d, box(-305, -270, 21), GOLD)
    head = [(150, -20), (195, -60), (300, 0), (195, 60), (150, 20)]
    part(d, head, METAL)
    d.line(to_image([(165, 0), (275, 0)]), fill=INK, width=LINE)
    part(d, box(130, 165, 24), GOLD)
    return img

def bow():
    img = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(img)
    tip, bulge = 250, 120
    d.line(to_image([(-tip, 0), (tip, 0)]), fill=INK, width=22)
    d.line(to_image([(-tip, 0), (tip, 0)]), fill=STRING, width=10)
    n = 24
    spine = [(tip * (2 * i / n - 1), -bulge * (1 - (2 * i / n - 1) ** 2)) for i in range(n + 1)]
    left, right = [], []
    for i, (x, y) in enumerate(spine):
        t = 2 * i / n - 1
        slope = 2 * bulge * t / tip  # dy/dx of the spine
        nx, ny = -slope / math.hypot(1, slope), 1 / math.hypot(1, slope)
        half = 11 + 15 * (1 - t * t)
        left.append((x + nx * half, y + ny * half))
        right.append((x - nx * half, y - ny * half))
    part(d, left + right[::-1], WOOD)
    part(d, [(x, y - bulge) for x, y in box(-34, 34, 25)], LEATHER)
    for s in (-1, 1):
        disc(d, s * tip, 0, 14, GOLD)
    return img

def potion():
    img = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(img)
    cx, cy, r = S / 2, 330, 140
    d.rectangle([cx - 44, 120, cx + 44, 220], fill=GLASS, outline=INK, width=LINE)
    d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=GLASS, outline=INK, width=LINE)
    d.chord([cx - r + LINE, cy - r + LINE, cx + r - LINE, cy + r - LINE], 5, 175, fill=WHITE)  # the liquid
    d.line([(cx - r * 0.99, cy + 12), (cx + r * 0.99, cy + 12)], fill=INK, width=LINE)
    d.rectangle([cx - 66, 100, cx + 66, 132], fill=GLASS, outline=INK, width=LINE)
    d.rectangle([cx - 36, 44, cx + 36, 104], fill=WHITE, outline=INK, width=LINE)  # the cork
    return img

def amulet():
    """A cord hanging in a loop with a gold pendant: a lapis stone in a drop-shaped setting."""
    img = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(img)
    cord = [(S / 2 + 150 * math.sin(a), 230 - 150 * math.cos(a)) for a in [math.pi * (i / 24 - 0.5) * 1.6 for i in range(25)]]
    d.line(cord, fill=INK, width=26, joint="curve")
    d.line(cord, fill=LEATHER, width=12, joint="curve")
    cx, cy = S / 2, 330
    drop = [(cx + 120 * math.sin(a) * (1 - 0.35 * math.cos(a)), cy + 120 * math.cos(a) * 0.95 - 20)
            for a in [2 * math.pi * i / 48 for i in range(48)]]
    d.polygon(drop, fill=GOLD, outline=INK, width=LINE)
    d.ellipse([cx - 52, cy - 52, cx + 52, cy + 52], fill=LAPIS, outline=INK, width=LINE)
    d.rectangle([cx - 26, cy - 160, cx + 26, cy - 110], fill=GOLD, outline=INK, width=LINE)  # the bail
    return img

def ring():
    """A gold band seen from a little above, a lapis stone on top."""
    img = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(img)
    cx, cy, rx, ry, band = S / 2, 300, 170, 140, 44
    d.ellipse([cx - rx, cy - ry, cx + rx, cy + ry], fill=GOLD, outline=INK, width=LINE)
    d.ellipse([cx - rx + band, cy - ry + band, cx + rx - band, cy + ry - band], fill=(0, 0, 0, 0), outline=INK,
              width=LINE)
    d.polygon([(cx - 70, 170), (cx + 70, 170), (cx + 46, 90), (cx - 46, 90)], fill=GOLD, outline=INK, width=LINE)
    d.ellipse([cx - 62, 30, cx + 62, 130], fill=LAPIS, outline=INK, width=LINE)
    return img

# Order = atlas cell = PlayerHud::Icon; None: an empty cell.
ICONS = [potion, amulet, ring, None, None, None, None, None,
         club, sword, spear, bow]

def main(path):
    atlas = Image.new("RGBA", (CELL * GRID_X, CELL * GRID_Y), (0, 0, 0, 0))
    for k, fn in enumerate(ICONS):
        if fn is None:
            continue
        cell = outlined(fn()).resize((CELL, CELL), Image.LANCZOS)
        atlas.paste(cell, ((k % GRID_X) * CELL, (k // GRID_X) * CELL))
    bleed(atlas).save(path)
    print("wrote", path)

if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "..", "textures", "ui", "hud_icons.png"))
