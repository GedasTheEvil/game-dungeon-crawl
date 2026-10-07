"""Generate the HUD and inventory tab icon atlas textures/ui/hud_icons.png (RGBA, 8 x 4 cells of 128 px).

    python3 tools/textures/hud_icons.py [out.png]

Flat glyphs with an ink outline, drawn at 4x and downsampled. Cell order must match PlayerHud::Icon in
src/ui/player_hud.h: the potion, amulet and ring, then from cell 8 on the weapons in ItemKind order, then from cell
21 on the amulets by AmuletType. The weapons lie diagonally, the grip bottom left. The flask is white: the HUD tints it with the
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

HORN = (60, 40, 28, 255)
BARK = (232, 218, 180, 255)
REDWOOD = (138, 52, 30, 255)
STONE = (150, 146, 138, 255)
RED = (176, 40, 30, 255)


def composite_bow():
    """Like the bow, with the grip set back between two humps, horn tips and bark bands."""
    img = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(img)
    tip, bulge = 250, 120
    d.line(to_image([(-tip, 0), (tip, 0)]), fill=INK, width=22)
    d.line(to_image([(-tip, 0), (tip, 0)]), fill=STRING, width=10)
    n = 32

    def depth(t):
        hump = 0.5 + 0.5 * (1 - math.exp(-(t / 0.32) ** 2))
        return bulge * (1 - t * t) * hump * 1.25

    spine = [(tip * (2 * i / n - 1), -depth(2 * i / n - 1)) for i in range(n + 1)]
    left, right = [], []
    for i, (x, y) in enumerate(spine):
        t = 2 * i / n - 1
        j0, j1 = max(i - 1, 0), min(i + 1, n)
        slope = (spine[j1][1] - spine[j0][1]) / (spine[j1][0] - spine[j0][0])
        nx, ny = -slope / math.hypot(1, slope), 1 / math.hypot(1, slope)
        half = 11 + 13 * (1 - t * t)
        left.append((x + nx * half, y + ny * half))
        right.append((x - nx * half, y - ny * half))
    part(d, left + right[::-1], REDWOOD)
    for t in (-0.43, 0.43):
        x, y = spine[round((t + 1) / 2 * n)]
        part(d, [(px + x, py + y) for px, py in box(-18, 18, 26)], BARK)
    part(d, [(x, y - depth(0)) for x, y in box(-30, 30, 24)], LEATHER)
    for s in (-1, 1):
        disc(d, s * tip, 0, 16, HORN)
    return img


def sling():
    """The finger loop bottom left, two cords up to a leather pouch with a stone."""
    img = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(img)
    for s in (-1, 1):
        cord = to_image([(-250, 0), (-60, s * 14), (150, s * 34)])
        d.line(cord, fill=INK, width=24, joint="curve")
        d.line(cord, fill=STRING, width=10, joint="curve")
    (lx, ly), = to_image([(-262, 0)])
    d.ellipse([lx - 34, ly - 34, lx + 34, ly + 34], outline=INK, width=24)
    d.ellipse([lx - 34, ly - 34, lx + 34, ly + 34], outline=STRING, width=10)
    part(d, [(130, -62), (230, -70), (282, 0), (230, 70), (130, 62)], LEATHER)
    disc(d, 205, 0, 52, STONE)
    return img


def throwing_stick():
    """A flat bent stick: leather grip, red and blue bands, a gold tip."""
    img = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(img)

    def pt(t, off):
        x = -235 + 470 * t
        y = -110 * math.sin(math.pi * t) + 50
        return (x, y + off)

    n = 24
    outline = [pt(i / n, -24) for i in range(n + 1)] + [pt(i / n, 24) for i in range(n, -1, -1)]
    part(d, outline, WOOD)
    for t0, t1, col in ((0.0, 0.14, LEATHER), (0.44, 0.51, RED), (0.51, 0.58, LAPIS), (0.58, 0.65, RED), (0.92, 1.0, GOLD)):
        seg = [pt(t0 + (t1 - t0) * k / 4, -24) for k in range(5)] + [pt(t0 + (t1 - t0) * k / 4, 24) for k in range(4, -1, -1)]
        part(d, seg, col)
    return img


def javelin():
    """Thinner and shorter than the spear, a long head, a cord grip in the middle."""
    img = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(img)
    part(d, box(-280, 120, 12), WOOD)
    part(d, box(-60, 20, 17), STRING)
    part(d, box(-292, -270, 15), GOLD)
    head = [(110, -16), (150, -44), (305, 0), (150, 44), (110, 16)]
    part(d, head, METAL)
    d.line(to_image([(125, 0), (285, 0)]), fill=INK, width=LINE)
    return img


BRONZE = (196, 132, 58, 255)
DARK = (60, 50, 40, 255)


def dagger():
    img = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(img)
    part(d, [(-60, -30), (170, -20), (245, 0), (170, 20), (-60, 30)], METAL)
    d.line(to_image([(-50, 0), (190, 0)]), fill=INK, width=LINE)
    part(d, box(-180, -80, 15), LEATHER)
    part(d, box(-90, -60, 62), GOLD)
    disc(d, -200, 0, 34, GOLD)
    return img


def khopesh():
    """Grip bottom left, a straight shank, the hooked blade curving out and back."""
    img = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(img)
    part(d, box(-280, -150, 17), (40, 34, 30, 255))
    for x in (-260, -215, -170):
        part(d, box(x, x + 14, 19), GOLD)
    outer = [(-150, -16), (0, -18), (80, -40), (170, -95), (240, -110), (280, -80), (270, -40)]
    inner = [(240, -60), (200, -60), (130, -25), (60, 6), (0, 14), (-150, 16)]
    part(d, [(0.8 * x - 30, 0.8 * y) for x, y in outer + inner], BRONZE)
    return img


def axe_haft(d, length=240):
    part(d, box(-280, length, 15), WOOD)
    part(d, box(-280, -170, 18), LEATHER)


def epsilon_axe():
    img = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(img)
    axe_haft(d)
    cx, r = 120, 105
    for x in (50, 120, 190):  # the tangs, from the haft to the blade
        part(d, [(x - 12, -14), (x + 12, -14), (x + 12, -60), (x - 12, -60)], BRONZE)
    arc_o = [(cx + r * math.cos(a), -60 - 80 * math.sin(a)) for a in [math.pi * k / 16 for k in range(17)]]
    arc_i = [(cx + (r - 30) * math.cos(a), -60 - 50 * math.sin(a)) for a in [math.pi * k / 16 for k in range(16, -1, -1)]]
    part(d, arc_o + arc_i, BRONZE)
    for x in (50, 120, 190):
        part(d, box(x - 10, x + 10, 19), STRING)
    return img


def duckbill_axe():
    img = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(img)
    axe_haft(d, 200)
    part(d, [(95, -16), (190, -16), (180, -110), (110, -100)], BRONZE)
    for x0 in (115, 148):
        part(d, [(x0, -38), (x0 + 20, -38), (x0 + 20, -88), (x0, -86)], DARK)
    for x in (105, 175):
        part(d, box(x - 10, x + 10, 19), STRING)
    return img


def mace():
    img = Image.new("RGBA", (S, S))
    d = ImageDraw.Draw(img)
    part(d, box(-280, 120, 16), WOOD)
    for x in (-260, -230, -200, -170):
        part(d, box(x, x + 14, 19), STRING)
    head = [(110 + 105 * math.cos(a) * (1.25 if math.cos(a) > 0 else 0.9), 85 * math.sin(a)) for a in [2 * math.pi * k / 32 for k in range(32)]]
    part(d, head, STONE)
    d.line(to_image([(120, -82), (210, 0), (120, 82)]), fill=LEATHER, width=16)
    d.line(to_image([(80, -80), (80, 80)]), fill=LEATHER, width=16)
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

# ---- the amulets, one per AmuletType: the cord of amulet(), a pendant of its own (tools/blender/models/amulets.py) ----
BONE = (238, 228, 200, 255)
BRONZE = (176, 120, 58, 255)
CARNELIAN = (190, 52, 34, 255)
TURQUOISE = (52, 168, 150, 255)
FAIENCE = (70, 170, 110, 255)
RED_JASPER = (160, 40, 40, 255)


def pendant(draw):
    """The cord loop, then draw(d, cx, cy): the pendant hanging at (cx, cy)."""
    def fn():
        img = Image.new("RGBA", (S, S))
        d = ImageDraw.Draw(img)
        cord = [(S / 2 + 150 * math.sin(a), 210 - 150 * math.cos(a))
                for a in [math.pi * (i / 24 - 0.5) * 1.6 for i in range(25)]]
        d.line(cord, fill=INK, width=26, joint="curve")
        d.line(cord, fill=LEATHER, width=12, joint="curve")
        draw(d, S / 2, 340)
        return img
    fn.__name__ = draw.__name__
    return fn


def tooth(d, cx, cy):
    """A jackal's fang, point down, a gold cap at the root."""
    pts = [(cx - 50, cy - 110), (cx + 50, cy - 110)]
    pts += [(cx + 50 - 60 * (t / 10) ** 1.4 + 25 * math.sin(math.pi * t / 10), cy - 110 + 26 * t) for t in range(1, 11)]
    pts += [(cx - 50 + 70 * (t / 10) ** 2.2, cy - 110 + 26 * t) for t in range(10, 0, -1)]
    d.polygon(pts, fill=BONE, outline=INK, width=LINE)
    d.rectangle([cx - 58, cy - 140, cx + 58, cy - 100], fill=GOLD, outline=INK, width=LINE)


def scarab(d, cx, cy):
    """A bronze scarab seen from above: head, thorax, the wing cases split down the middle."""
    d.ellipse([cx - 95, cy - 70, cx + 95, cy + 140], fill=BRONZE, outline=INK, width=LINE)
    d.ellipse([cx - 80, cy - 110, cx + 80, cy - 20], fill=BRONZE, outline=INK, width=LINE)
    d.ellipse([cx - 45, cy - 150, cx + 45, cy - 95], fill=BRONZE, outline=INK, width=LINE)
    d.line([(cx, cy - 20), (cx, cy + 138)], fill=INK, width=LINE)
    for side in (-1, 1):
        d.line([(cx + side * 70, cy - 60), (cx + side * 130, cy - 90)], fill=INK, width=14)
        d.line([(cx + side * 85, cy + 30), (cx + side * 140, cy + 40)], fill=INK, width=14)


def heart(d, cx, cy):
    """The ib heart amulet: a carnelian jar with two lugs and a neck."""
    d.ellipse([cx - 100, cy - 70, cx + 100, cy + 130], fill=CARNELIAN, outline=INK, width=LINE)
    for side in (-1, 1):
        d.ellipse([cx + side * 100 - 34, cy - 64, cx + side * 100 + 34, cy - 4], fill=CARNELIAN, outline=INK,
                  width=LINE)
    d.rectangle([cx - 48, cy - 130, cx + 48, cy - 64], fill=CARNELIAN, outline=INK, width=LINE)
    d.rectangle([cx - 62, cy - 150, cx + 62, cy - 124], fill=GOLD, outline=INK, width=LINE)


def scorpion(d, cx, cy):
    """Serket's scorpion in gold, head up: claws forward, legs, the tail curled to one side."""
    cy -= 40
    for side in (-1, 1):
        for k in range(3):
            y = cy + 10 + 30 * k
            d.line([(cx + side * 30, y), (cx + side * 105, y + 25)], fill=INK, width=12)
        arm = [(cx + side * 30, cy - 20), (cx + side * 95, cy - 70), (cx + side * 75, cy - 115)]
        d.line(arm, fill=INK, width=30)
        d.line(arm, fill=GOLD, width=16)
    tail = [(cx + 10, cy + 120), (cx + 50, cy + 150), (cx + 95, cy + 145), (cx + 120, cy + 110)]
    for k, (x, y) in enumerate(tail):
        r = 26 - 3 * k
        d.ellipse([x - r, y - r, x + r, y + r], fill=GOLD, outline=INK, width=LINE)
    d.polygon([(cx + 108, cy + 92), (cx + 140, cy + 70), (cx + 132, cy + 105)], fill=CARNELIAN, outline=INK,
              width=LINE)
    d.ellipse([cx - 45, cy - 50, cx + 45, cy + 125], fill=GOLD, outline=INK, width=LINE)


def wedjat(d, cx, cy):
    """The eye of Horus: lapis brow, the eye with its pupil, the falcon's cheek mark and spiral below."""
    for path in ([(cx - 30, cy), (cx - 50, cy + 140)],
                 [(cx + 40, cy)] + [(cx + 85 + (8 + 4 * t) * math.cos(t / 2.6), cy + 95 + (8 + 4 * t) * math.sin(t / 2.6))
                                    for t in range(14, -1, -1)]):
        d.line(path, fill=INK, width=30, joint="curve")
        d.line(path, fill=LAPIS, width=16, joint="curve")
    d.polygon([(cx - 140, cy - 40), (cx - 40, cy - 90), (cx + 80, cy - 80), (cx + 140, cy - 40), (cx + 60, cy + 20),
               (cx - 60, cy + 20)], fill=WHITE, outline=INK, width=LINE)
    d.ellipse([cx - 45, cy - 80, cx + 45, cy + 10], fill=LAPIS, outline=INK, width=LINE)
    d.line([(cx - 150, cy - 110), (cx + 140, cy - 110)], fill=INK, width=30)
    d.line([(cx - 150, cy - 110), (cx + 140, cy - 110)], fill=LAPIS, width=16)


def djed(d, cx, cy):
    """The djed pillar of Osiris in turquoise: a column with four bands at the top."""
    d.rectangle([cx - 40, cy - 60, cx + 40, cy + 150], fill=TURQUOISE, outline=INK, width=LINE)
    d.rectangle([cx - 70, cy + 130, cx + 70, cy + 160], fill=TURQUOISE, outline=INK, width=LINE)
    for k in range(4):
        y = cy - 140 + 30 * k
        d.rectangle([cx - 90, y, cx + 90, y + 22], fill=TURQUOISE, outline=INK, width=LINE)


def tyet(d, cx, cy):
    """The knot of Isis in red jasper: a loop on top, arms hanging down, a long tied sash."""
    d.ellipse([cx - 50, cy - 160, cx + 50, cy - 60], fill=RED_JASPER, outline=INK, width=LINE)
    d.ellipse([cx - 20, cy - 130, cx + 20, cy - 90], fill=(0, 0, 0, 0), outline=INK, width=LINE)
    for side in (-1, 1):
        d.polygon([(cx, cy - 60), (cx + side * 120, cy - 30), (cx + side * 110, cy + 20), (cx + side * 30, cy - 10)],
                  fill=RED_JASPER, outline=INK, width=LINE)
    d.polygon([(cx - 30, cy - 50), (cx + 30, cy - 50), (cx + 60, cy + 160), (cx - 60, cy + 160)], fill=RED_JASPER,
              outline=INK, width=LINE)


def shen(d, cx, cy):
    """The shen ring of eternity in gold: a rope circle tied to a bar below."""
    d.ellipse([cx - 110, cy - 130, cx + 110, cy + 90], fill=GOLD, outline=INK, width=LINE)
    d.ellipse([cx - 64, cy - 84, cx + 64, cy + 44], fill=CARNELIAN, outline=INK, width=LINE)
    d.rectangle([cx - 130, cy + 90, cx + 130, cy + 130], fill=GOLD, outline=INK, width=LINE)


def lotus(d, cx, cy):
    """A green faience lotus: a fan of petals on a short stalk."""
    for k in range(5):
        a = math.radians(-60 + 30 * k)
        tip = (cx + 150 * math.sin(a), cy - 20 - 150 * math.cos(a))
        side = (math.cos(a) * 38, math.sin(a) * 38)
        d.polygon([(cx - side[0], cy + 40 - side[1]), tip, (cx + side[0], cy + 40 + side[1])], fill=FAIENCE,
                  outline=INK, width=LINE)
    d.ellipse([cx - 60, cy + 10, cx + 60, cy + 80], fill=FAIENCE, outline=INK, width=LINE)
    d.rectangle([cx - 14, cy + 70, cx + 14, cy + 160], fill=FAIENCE, outline=INK, width=LINE)


# By AmuletType (src/world/items.h).
AMULET_ICONS = [pendant(f) for f in (tooth, scarab, heart, scorpion, wedjat, djed, tyet, shen, lotus)]

# Order = atlas cell = PlayerHud::Icon; None: an empty cell. The amulets from cell 21 on (Icon::FirstAmulet).
ICONS = [potion, amulet, ring, None, None, None, None, None,
         club, dagger, sword, khopesh, epsilon_axe, duckbill_axe, mace, spear,
         bow, composite_bow, sling, throwing_stick, javelin] + AMULET_ICONS

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
