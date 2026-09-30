"""Generate the HUD quick slot icon atlas textures/ui/hud_icons.png (RGBA, 4 x 2 cells of 128 px).

    python3 tools/textures/hud_icons.py [out.png]

Flat glyphs with an ink outline, drawn at 4x and downsampled. Cell order must match PlayerHud::Icon in
src/ui/player_hud.h. The weapons lie diagonally, the grip bottom left. The flask is white: the HUD tints it with the
potion colour.
"""

import math
import os
import sys

from PIL import Image, ImageDraw, ImageFilter

from decals import bleed

CELL = 128
S = CELL * 4  # drawing resolution per cell
GRID_X, GRID_Y = 4, 2
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

# Order = atlas cell = PlayerHud::Icon.
ICONS = [club, sword, spear, bow, potion]

def main(path):
    atlas = Image.new("RGBA", (CELL * GRID_X, CELL * GRID_Y), (0, 0, 0, 0))
    for k, fn in enumerate(ICONS):
        cell = outlined(fn()).resize((CELL, CELL), Image.LANCZOS)
        atlas.paste(cell, ((k % GRID_X) * CELL, (k // GRID_X) * CELL))
    bleed(atlas).save(path)
    print("wrote", path)

if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "..", "textures", "ui", "hud_icons.png"))
