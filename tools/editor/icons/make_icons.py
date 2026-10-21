#!/usr/bin/env python3
"""Draws the editor's tile icons (64 x 64 PNG) in the game's UI palette: gold, bronze and lapis, on a transparent
background (the editor draws the cell's open space or water under them).

Run from anywhere: python3 tools/editor/icons/make_icons.py
"""

import math
import os

from PIL import Image, ImageDraw, ImageFilter

SIZE = 64
SS = 4  # supersampling
S = SIZE * SS

OPEN = (9, 7, 5)  # editor's OPEN_COLOR
GOLD = (242, 194, 97)
GOLD_LIGHT = (255, 228, 150)
GOLD_DIM = (148, 112, 56)
BRONZE = (107, 77, 38)
BRONZE_DARK = (62, 44, 22)
LAPIS = (31, 69, 148)
LAPIS_LIGHT = (80, 130, 220)
RED = (170, 40, 22)
RED_LIGHT = (230, 80, 40)
STONE = (102, 79, 54)
STONE_DARK = (60, 46, 31)
WOOD = (130, 86, 44)
WOOD_DARK = (78, 50, 24)
TURQUOISE = (50, 180, 165)

OUT = os.path.dirname(os.path.abspath(__file__))


def p(*xy):
    """Design coordinates (0..64) to supersampled pixels."""
    return [v * SS for v in xy]


def canvas(glow=GOLD, glow_alpha=40):
    # Transparent, so the editor draws the cell's structure (open space, water) under the symbol.
    img = Image.new("RGBA", (S, S), glow + (0,))
    # Soft glow behind the symbol, so it reads as an object lit in the dark.
    halo = Image.new("L", (S, S), 0)
    ImageDraw.Draw(halo).ellipse(p(10, 10, 54, 54), fill=glow_alpha)
    halo = halo.filter(ImageFilter.GaussianBlur(10 * SS))
    img.paste(Image.new("RGBA", (S, S), glow + (255,)), (0, 0), halo)
    return img, ImageDraw.Draw(img)


def save(img, name):
    img.resize((SIZE, SIZE), Image.LANCZOS).save(os.path.join(OUT, name), optimize=True)


def poly(d, pts, fill, outline=None, width=0):
    d.polygon([v * SS for xy in pts for v in xy], fill=fill)
    if outline:
        d.line([v * SS for xy in pts + [pts[0]] for v in xy], fill=outline, width=width * SS, joint="curve")


def door():  # sphinx gate: pylon gateway with a lapis doorway
    img, d = canvas()
    poly(d, [(6, 56), (10, 12), (27, 12), (29, 56)], GOLD_DIM, BRONZE_DARK, 1)
    poly(d, [(35, 56), (37, 12), (54, 12), (58, 56)], GOLD_DIM, BRONZE_DARK, 1)
    d.rectangle(p(24, 20, 40, 56), fill=BRONZE)
    d.rectangle(p(27, 28, 37, 56), fill=LAPIS)
    d.rectangle(p(29, 32, 35, 56), fill=(12, 30, 80))
    d.rectangle(p(4, 8, 60, 13), fill=GOLD)  # cornice
    for x in (13, 44):  # carved bands
        d.rectangle(p(x, 20, x + 7, 22), fill=BRONZE)
        d.rectangle(p(x, 28, x + 7, 30), fill=BRONZE)
    d.ellipse(p(29, 14, 35, 20), fill=RED_LIGHT)  # sun disc
    save(img, "gate.png")


def spikes(name, tall):
    img, d = canvas(RED, 35 if tall else 25)
    d.rectangle(p(4, 52, 60, 58), fill=STONE_DARK)
    count = 4 if tall else 6
    top = 10 if tall else 32
    w = 56 / count
    for i in range(count):
        x0 = 4 + i * w
        cx = x0 + w / 2
        poly(d, [(x0 + 1, 52), (cx, top), (x0 + w - 1, 52)], GOLD_DIM)
        poly(d, [(cx, top), (x0 + w - 1, 52), (cx, 52)], BRONZE)  # shaded side
        tip = (52 - top) * 0.28  # bloodied tip, following the spike's sides
        half = (w / 2 - 1) * 0.28
        poly(d, [(cx - half, top + tip), (cx, top), (cx + half, top + tip)], RED_LIGHT if tall else RED)
    save(img, name)


def monster():  # scarab
    img, d = canvas(RED, 45)
    for side in (-1, 1):  # legs
        for y, dx in ((26, 12), (35, 14), (44, 12)):
            d.line(p(32, y, 32 + side * dx, y + (y - 35) * 0.6 + 2, 32 + side * (dx + 5), y + (y - 35) * 0.9 + 8),
                   fill=BRONZE, width=3 * SS, joint="curve")
    d.ellipse(p(19, 22, 45, 56), fill=LAPIS, outline=GOLD, width=2 * SS)  # wing cases
    d.line(p(32, 24, 32, 55), fill=GOLD, width=2 * SS)
    d.ellipse(p(23, 13, 41, 27), fill=LAPIS, outline=GOLD, width=2 * SS)  # thorax
    d.ellipse(p(27, 7, 37, 15), fill=GOLD)  # head
    d.ellipse(p(28, 9, 31, 12), fill=RED_LIGHT)
    d.ellipse(p(33, 9, 36, 12), fill=RED_LIGHT)
    d.ellipse(p(25, 32, 30, 37), fill=LAPIS_LIGHT)  # sheen
    save(img, "monster.png")


def ladder():
    img, d = canvas(GOLD, 25)
    for x in (17, 44):
        d.rectangle(p(x, 3, x + 4, 61), fill=WOOD, outline=WOOD_DARK, width=SS)
    for y in range(9, 60, 11):
        d.rectangle(p(19, y, 46, y + 3), fill=WOOD, outline=WOOD_DARK, width=SS)
        d.line(p(18, y + 1, 21, y + 2), fill=GOLD_DIM, width=SS)  # lashing
        d.line(p(44, y + 1, 47, y + 2), fill=GOLD_DIM, width=SS)
    save(img, "ladder.png")


def treasure():
    img, d = canvas(GOLD, 70)
    d.rectangle(p(8, 30, 56, 56), fill=BRONZE, outline=BRONZE_DARK, width=SS)
    d.chord(p(8, 12, 56, 44), 180, 360, fill=GOLD_DIM, outline=BRONZE_DARK, width=SS)  # lid
    d.rectangle(p(8, 28, 56, 32), fill=GOLD)
    for x in (13, 47):  # straps
        d.rectangle(p(x, 14, x + 4, 56), fill=GOLD)
    d.rectangle(p(27, 29, 37, 40), fill=GOLD_LIGHT, outline=BRONZE_DARK, width=SS)  # lock
    d.ellipse(p(30, 32, 34, 36), fill=LAPIS)
    for x, y in ((22, 24), (40, 22), (31, 19)):  # coins on the rim
        d.ellipse(p(x - 3, y - 2, x + 3, y + 2), fill=GOLD_LIGHT)
    save(img, "treasure.png")


def trapped_treasure():
    """The chest darker, a red glow, a broken red seal over the lock and scratches round it (plan trapped-chests)."""
    img, d = canvas(RED, 70)
    d.rectangle(p(8, 30, 56, 56), fill=WOOD_DARK, outline=BRONZE_DARK, width=SS)
    d.chord(p(8, 12, 56, 44), 180, 360, fill=BRONZE, outline=BRONZE_DARK, width=SS)  # lid
    d.rectangle(p(8, 28, 56, 32), fill=GOLD_DIM)
    for x in (13, 47):  # straps
        d.rectangle(p(x, 14, x + 4, 56), fill=GOLD_DIM)
    d.rectangle(p(27, 29, 37, 40), fill=GOLD_DIM, outline=BRONZE_DARK, width=SS)  # lock
    poly(d, [(25, 31), (31, 28), (30, 35), (25, 37)], RED_LIGHT, BRONZE_DARK, 1)  # the seal, broken in two
    poly(d, [(33, 30), (39, 33), (38, 38), (32, 36)], RED, BRONZE_DARK, 1)
    for x0, y0, x1, y1 in ((22, 42, 26, 48), (24, 41, 27, 46), (40, 42, 37, 49), (42, 43, 39, 47)):  # scratches
        d.line(p(x0, y0, x1, y1), fill=GOLD_LIGHT, width=SS)
    save(img, "trapped_treasure.png")


def ankh():
    img, d = canvas(GOLD, 80)
    d.ellipse(p(20, 4, 44, 32), outline=GOLD, width=6 * SS)
    d.rectangle(p(10, 28, 54, 35), fill=GOLD)
    poly(d, [(28, 33), (36, 33), (39, 60), (25, 60)], GOLD)
    d.ellipse(p(21, 5, 43, 31), outline=GOLD_LIGHT, width=SS)  # highlight
    d.line(p(12, 29, 52, 29), fill=GOLD_LIGHT, width=SS)
    save(img, "ankh.png")


def key():
    img, d = canvas(GOLD, 55)
    d.ellipse(p(6, 18, 30, 42), outline=GOLD, width=6 * SS)  # bow
    d.ellipse(p(14, 26, 22, 34), fill=LAPIS)
    d.rectangle(p(28, 27, 58, 33), fill=GOLD)  # shaft
    d.rectangle(p(44, 33, 48, 44), fill=GOLD)  # bit
    d.rectangle(p(52, 33, 56, 41), fill=GOLD)
    d.line(p(29, 28, 57, 28), fill=GOLD_LIGHT, width=SS)
    save(img, "key.png")


def gate():  # portcullis with a lapis gem lock
    img, d = canvas(LAPIS, 50)
    d.rectangle(p(4, 4, 60, 10), fill=STONE, outline=STONE_DARK, width=SS)  # lintel
    for x in range(9, 60, 9):
        d.rectangle(p(x - 2, 10, x + 2, 54), fill=BRONZE, outline=BRONZE_DARK, width=SS)
        poly(d, [(x - 2, 54), (x + 2, 54), (x, 60)], BRONZE)  # spiked feet
    for y in (22, 42):
        d.rectangle(p(5, y, 59, y + 4), fill=GOLD_DIM, outline=BRONZE_DARK, width=SS)
    d.ellipse(p(24, 25, 40, 41), fill=GOLD, outline=BRONZE_DARK, width=SS)
    poly(d, [(32, 28), (37, 33), (32, 38), (27, 33)], LAPIS_LIGHT)
    save(img, "gate_lock.png")


def lever():
    img, d = canvas(GOLD, 35)
    d.line(p(32, 48, 46, 12), fill=GOLD_DIM, width=5 * SS)  # handle
    d.ellipse(p(40, 5, 54, 19), fill=LAPIS, outline=GOLD, width=2 * SS)  # knob
    d.ellipse(p(43, 8, 47, 12), fill=LAPIS_LIGHT)
    d.pieslice(p(18, 36, 46, 64), 180, 360, fill=STONE, outline=STONE_DARK, width=SS)  # pivot base
    d.rectangle(p(10, 50, 54, 58), fill=STONE, outline=STONE_DARK, width=SS)
    d.ellipse(p(28, 44, 36, 52), fill=GOLD)
    save(img, "lever.png")


def rockfall():  # cracked ceiling slab, grit, a falling rock
    img, d = canvas(GOLD, 25)
    d.rectangle(p(0, 0, 64, 14), fill=STONE, outline=STONE_DARK, width=SS)
    d.line(p(10, 14, 16, 7, 22, 11, 30, 3), fill=OPEN, width=2 * SS, joint="curve")
    d.line(p(36, 14, 42, 8, 50, 12, 56, 5), fill=OPEN, width=2 * SS, joint="curve")
    for x, y, r in ((31, 18, 1.4), (33, 23, 1.1), (30, 27, 1.2), (32, 31, 0.9)):  # grit trickling down
        d.ellipse(p(x - r, y - r, x + r, y + r), fill=GOLD_DIM)
    poly(d, [(22, 42), (30, 36), (42, 38), (46, 48), (38, 58), (26, 57), (19, 50)], STONE, STONE_DARK, 1)
    poly(d, [(30, 36), (42, 38), (46, 48), (36, 45)], (140, 110, 78))  # lit face
    save(img, "rockfall.png")


def dartplate():  # a slab in the floor, the panel of three dart holes in the wall above it
    img, d = canvas(GOLD, 25)
    d.rectangle(p(12, 18, 52, 34), fill=STONE, outline=STONE_DARK, width=SS)  # the panel
    for x in (20, 32, 44):
        d.ellipse(p(x - 4, 22, x + 4, 30), fill=OPEN, outline=BRONZE, width=SS)  # its holes
    d.line(p(32, 34, 32, 44), fill=RED, width=2 * SS)  # a dart on its way out
    poly(d, [(29, 44), (35, 44), (32, 48)], GOLD)
    d.rectangle(p(12, 50, 52, 58), fill=STONE, outline=GOLD_DIM, width=SS)
    d.line(p(18, 54, 24, 52, 30, 56, 36, 52, 42, 54), fill=GOLD, width=SS, joint="curve")  # the carved cobra
    save(img, "dartplate.png")


def inscription():  # a cartouche on its bar: three signs, sun, water, reed
    img, d = canvas(GOLD, 35)
    d.rounded_rectangle(p(20, 4, 44, 52), radius=12 * SS, fill=STONE, outline=GOLD, width=3 * SS)
    d.rectangle(p(14, 52, 50, 58), fill=GOLD, outline=BRONZE_DARK, width=SS)
    d.ellipse(p(27, 10, 37, 20), fill=RED, outline=BRONZE_DARK, width=SS)
    d.line(p(25, 28, 28, 25, 31, 28, 34, 25, 37, 28, 40, 25), fill=LAPIS_LIGHT, width=2 * SS, joint="curve")
    poly(d, [(32, 34), (35, 40), (33, 48), (31, 48), (29, 40)], TURQUOISE)
    save(img, "inscription.png")


# ---- the decor and decal layers (plan placed-decor): one icon per prop (prop_<DECOR_NAMES>.png), the torch, bare --

FLOOR_Y = 56  # the props stand on it
CLAY = (160, 92, 50)
CLAY_DARK = (104, 56, 28)
BONE = (226, 214, 180)
BONE_DARK = (150, 138, 108)
BLACK = (24, 20, 16)
GREEN = (70, 140, 60)
GREEN_DARK = (40, 90, 36)
LINEN = (205, 190, 150)
LINEN_DARK = (140, 124, 90)
WATER = (30, 76, 105)


def floor(d):
    d.rectangle(p(4, FLOOR_Y, 60, FLOOR_Y + 3), fill=STONE_DARK)


def water(d, level=30):
    d.rectangle(p(2, level, 62, 62), fill=WATER)
    d.line(p(2, level, 62, level), fill=LAPIS_LIGHT, width=SS)


def jar(d, cx, top, w, h, fill, dark, lid=None):
    """A jar standing on the floor: body, neck and an optional lid colour."""
    d.ellipse(p(cx - w / 2, top + h * 0.25, cx + w / 2, FLOOR_Y), fill=fill, outline=dark, width=SS)
    d.rectangle(p(cx - w / 4, top + h * 0.1, cx + w / 4, top + h * 0.35), fill=fill, outline=dark, width=SS)
    if lid:
        d.ellipse(p(cx - w / 3, top - 2, cx + w / 3, top + h * 0.2), fill=lid, outline=dark, width=SS)


def prop_web():
    img, d = canvas(GOLD, 20)
    for k in range(5):  # threads from the corner
        a = math.radians(k * 22.5)
        d.line(p(4, 4, 4 + 54 * math.cos(a), 4 + 54 * math.sin(a)), fill=BONE, width=SS)
    for r in (14, 26, 38, 50):  # the rings
        pts = [(4 + r * math.cos(math.radians(k * 22.5)), 4 + r * math.sin(math.radians(k * 22.5))) for k in range(5)]
        d.line([v * SS for xy in pts for v in xy], fill=BONE_DARK, width=SS)
    save(img, "prop_web.png")


def prop_pottery():
    img, d = canvas(GOLD, 25)
    floor(d)
    jar(d, 22, 22, 22, 34, CLAY, CLAY_DARK)
    jar(d, 44, 34, 16, 22, CLAY, CLAY_DARK)
    d.line(p(14, 40, 30, 40), fill=CLAY_DARK, width=SS)
    save(img, "prop_pottery.png")


def prop_canopic():
    img, d = canvas(GOLD, 25)
    floor(d)
    for cx, lid in ((16, GOLD_DIM), (32, BONE), (48, LAPIS_LIGHT)):
        jar(d, cx, 22, 13, 34, BONE_DARK, BRONZE_DARK, lid)
    save(img, "prop_canopic.png")


def prop_rubble():
    img, d = canvas(GOLD, 20)
    floor(d)
    for pts in ([(8, 56), (12, 44), (24, 42), (28, 56)], [(26, 56), (32, 38), (44, 36), (50, 46), (48, 56)],
                [(46, 56), (50, 48), (58, 50), (58, 56)]):
        poly(d, pts, STONE, STONE_DARK, 1)
    save(img, "prop_rubble.png")


def prop_sand():
    img, d = canvas(GOLD, 25)
    floor(d)
    d.chord(p(6, 34, 58, 78), 180, 360, fill=(190, 150, 90), outline=(130, 96, 50), width=SS)
    d.line(p(18, 46, 28, 44, 36, 46), fill=(220, 184, 120), width=SS)
    save(img, "prop_sand.png")


def prop_skeleton():
    img, d = canvas(GOLD, 25)
    floor(d)
    d.ellipse(p(6, 40, 18, 52), fill=BONE, outline=BONE_DARK, width=SS)  # skull, lying
    d.ellipse(p(9, 44, 12, 47), fill=BLACK)
    d.line(p(18, 48, 50, 50), fill=BONE, width=2 * SS)  # spine
    for x in (24, 30, 36):  # ribs
        d.arc(p(x - 4, 40, x + 4, 56), 200, 340, fill=BONE, width=SS)
    d.line(p(50, 50, 58, 44), fill=BONE, width=2 * SS)  # legs
    d.line(p(50, 50, 60, 54), fill=BONE, width=2 * SS)
    save(img, "prop_skeleton.png")


def prop_brazier():
    img, d = canvas(RED_LIGHT, 60)
    floor(d)
    for x0, x1 in ((22, 16), (42, 48)):  # legs
        d.line(p(x0, 40, x1, 56), fill=BRONZE, width=2 * SS)
    d.chord(p(12, 22, 52, 46), 0, 180, fill=BRONZE, outline=BRONZE_DARK, width=SS)  # bowl
    poly(d, [(18, 34), (24, 14), (30, 26), (34, 8), (40, 24), (46, 34)], RED_LIGHT)  # fire
    poly(d, [(26, 34), (32, 18), (38, 34)], GOLD_LIGHT)
    save(img, "prop_brazier.png")


def prop_lamp():
    img, d = canvas(GOLD, 50)
    floor(d)
    d.rectangle(p(29, 30, 35, 56), fill=BRONZE, outline=BRONZE_DARK, width=SS)  # stand
    d.rectangle(p(22, 52, 42, 56), fill=BRONZE)
    d.chord(p(16, 18, 48, 38), 0, 180, fill=CLAY, outline=CLAY_DARK, width=SS)  # the lamp
    poly(d, [(14, 28), (18, 14), (22, 28)], GOLD_LIGHT)  # flame at the wick
    save(img, "prop_lamp.png")


def prop_scrolls():
    img, d = canvas(GOLD, 25)
    floor(d)
    for x0, y in ((8, 48), (20, 40), (30, 48)):
        d.rounded_rectangle(p(x0, y, x0 + 26, y + 8), radius=4 * SS, fill=(220, 200, 150), outline=BRONZE, width=SS)
        d.ellipse(p(x0 + 22, y, x0 + 30, y + 8), fill=(190, 170, 120), outline=BRONZE, width=SS)
    save(img, "prop_scrolls.png")


def figure(d, cx, top, fill, dark, head=None):
    """A standing mummiform figure: head, body tapering to the feet on the floor."""
    d.ellipse(p(cx - 6, top, cx + 6, top + 12), fill=head or fill, outline=dark, width=SS)
    poly(d, [(cx - 8, top + 11), (cx + 8, top + 11), (cx + 5, FLOOR_Y), (cx - 5, FLOOR_Y)], fill, dark, 1)


def prop_ushabti():
    img, d = canvas(TURQUOISE, 35)
    floor(d)
    figure(d, 32, 8, TURQUOISE, (20, 90, 80), head=(30, 40, 70))
    for y in (30, 38, 46):  # columns of text
        d.line(p(28, y, 36, y), fill=(20, 90, 80), width=SS)
    save(img, "prop_ushabti.png")


def prop_cat():
    img, d = canvas(GOLD, 30)
    floor(d)
    d.rectangle(p(18, 50, 46, 56), fill=STONE)  # base
    poly(d, [(22, 50), (24, 26), (34, 22), (42, 50)], BLACK)  # seated body
    d.ellipse(p(25, 12, 37, 26), fill=BLACK)  # head
    poly(d, [(26, 15), (27, 7), (30, 13)], BLACK)  # ears
    poly(d, [(32, 13), (35, 7), (36, 15)], BLACK)
    d.ellipse(p(32, 16, 34, 18), fill=GOLD)
    save(img, "prop_cat.png")


def prop_jackal():
    img, d = canvas(GOLD, 30)
    floor(d)
    d.rectangle(p(6, 46, 58, 56), fill=GOLD_DIM, outline=BRONZE_DARK, width=SS)  # shrine chest
    poly(d, [(12, 46), (14, 36), (40, 34), (44, 46)], BLACK)  # lying body
    poly(d, [(40, 34), (46, 24), (56, 30), (48, 36)], BLACK)  # head, muzzle out
    poly(d, [(44, 26), (44, 16), (48, 24)], BLACK)  # ear
    save(img, "prop_jackal.png")


def prop_osiris():
    img, d = canvas(GOLD, 30)
    floor(d)
    figure(d, 32, 14, LINEN, LINEN_DARK, head=GREEN)
    poly(d, [(28, 15), (32, 2), (36, 15)], BONE)  # the white crown
    d.line(p(26, 28, 38, 34), fill=GOLD, width=2 * SS)  # crook and flail
    d.line(p(38, 28, 26, 34), fill=LAPIS_LIGHT, width=2 * SS)
    save(img, "prop_osiris.png")


def ibis(d, seated):
    body_top = 32 if seated else 20
    d.ellipse(p(20, body_top, 44, body_top + 16), fill=BONE, outline=BONE_DARK, width=SS)
    d.line(p(38, body_top + 2, 42, body_top - 10), fill=BONE, width=3 * SS)  # neck
    d.ellipse(p(38, body_top - 16, 46, body_top - 8), fill=BLACK)  # head
    d.line(p(44, body_top - 12, 54, body_top - 2), fill=BLACK, width=2 * SS)  # curved beak
    if not seated:
        for x in (28, 34):
            d.line(p(x, body_top + 14, x, FLOOR_Y), fill=BLACK, width=2 * SS)
    d.rectangle(p(16, FLOOR_Y - 4 if seated else FLOOR_Y - 2, 48, FLOOR_Y), fill=STONE)


def baboon(d, seated):
    top = 22 if seated else 12
    d.ellipse(p(20, top + 8, 44, FLOOR_Y - (2 if seated else 12)), fill=STONE, outline=STONE_DARK, width=SS)
    d.ellipse(p(24, top, 40, top + 14), fill=STONE, outline=STONE_DARK, width=SS)  # head and mane
    d.ellipse(p(28, top + 4, 36, top + 12), fill=(150, 120, 90))  # face
    if not seated:
        for x in (26, 38):
            d.line(p(x, FLOOR_Y - 14, x, FLOOR_Y), fill=STONE_DARK, width=3 * SS)
    d.ellipse(p(26, top - 8, 38, top + 2), fill=GOLD, outline=BRONZE_DARK, width=SS)  # the moon disc


def prop_thoth(name, draw, seated):
    img, d = canvas(GOLD, 30)
    floor(d)
    draw(d, seated)
    save(img, f"prop_{name}.png")


def coffin(d, fill, dark, large=False, sleeper=False):
    """A coffin seen from the front, lying on the floor: the lid with a face at one end."""
    x0, x1 = (4, 60) if large else (8, 56)
    d.rounded_rectangle(p(x0, 38, x1, FLOOR_Y), radius=6 * SS, fill=fill, outline=dark, width=SS)
    d.ellipse(p(x1 - 16, 36, x1 - 2, 50), fill=GOLD if large else (190, 140, 80), outline=dark, width=SS)
    if large:
        for x in range(x0 + 8, x1 - 16, 6):  # gilded bands
            d.line(p(x, 40, x, 54), fill=GOLD, width=SS)
    if sleeper:  # the mummy in it, wrapped
        d.rounded_rectangle(p(x0 + 4, 32, x1 - 14, 44), radius=5 * SS, fill=LINEN, outline=LINEN_DARK, width=SS)
        for x in range(x0 + 8, x1 - 14, 5):
            d.line(p(x, 33, x + 3, 43), fill=LINEN_DARK, width=SS)
        d.ellipse(p(x1 - 20, 30, x1 - 8, 42), fill=LINEN, outline=LINEN_DARK, width=SS)  # head
        d.line(p(x1 - 18, 36, x1 - 10, 36), fill=BLACK, width=SS)  # eyes closed


def prop_sarcophagus():
    img, d = canvas(GOLD, 25)
    floor(d)
    d.rectangle(p(6, 30, 58, 56), fill=STONE, outline=STONE_DARK, width=SS)
    d.rectangle(p(4, 26, 60, 32), fill=(130, 104, 74), outline=STONE_DARK, width=SS)  # lid
    for x in (14, 24, 34, 44):
        d.line(p(x, 38, x + 6, 38), fill=GOLD_DIM, width=SS)
    save(img, "prop_sarcophagus.png")


def prop_coffin(name, **kind):
    img, d = canvas(GOLD, 25)
    floor(d)
    coffin(d, WOOD, WOOD_DARK, **kind)
    save(img, f"prop_{name}.png")


def prop_water_reeds():
    img, d = canvas(GREEN, 25)
    water(d, 40)
    for x, top in ((18, 6), (26, 12), (34, 4), (42, 14)):
        d.line(p(x, 62, x + 2, top), fill=GREEN, width=2 * SS)
        poly(d, [(x - 3, top + 2), (x + 2, top - 4), (x + 7, top + 2)], GREEN_DARK)  # the papyrus umbel
    save(img, "prop_water_reeds.png")


def prop_water_weed():
    img, d = canvas(GREEN, 20)
    water(d, 14)
    for x in (16, 28, 40, 50):
        d.line(p(x, 62, x - 4, 50, x + 3, 38, x - 2, 26), fill=GREEN, width=2 * SS, joint="curve")
    save(img, "prop_water_weed.png")


def pads(d):
    water(d, 38)
    for x0, x1 in ((6, 30), (26, 48), (42, 60)):
        d.pieslice(p(x0, 34, x1, 44), 20, 340, fill=GREEN, outline=GREEN_DARK, width=SS)


def prop_water_pads():
    img, d = canvas(GREEN, 20)
    pads(d)
    save(img, "prop_water_pads.png")


def prop_water_lotus():
    img, d = canvas(LAPIS_LIGHT, 40)
    pads(d)
    for pts, c in (([(24, 36), (28, 18), (32, 36)], LAPIS_LIGHT), ([(30, 36), (34, 14), (38, 36)], LAPIS_LIGHT),
                   ([(36, 36), (40, 18), (44, 36)], LAPIS)):
        poly(d, pts, c)
    d.ellipse(p(31, 30, 37, 36), fill=GOLD)
    save(img, "prop_water_lotus.png")


def torch():
    img, d = canvas(GOLD, 50)
    d.rectangle(p(24, 30, 40, 34), fill=BRONZE, outline=BRONZE_DARK, width=SS)  # the bracket on the wall
    poly(d, [(28, 30), (36, 30), (34, 58), (30, 58)], WOOD, WOOD_DARK, 1)
    d.rectangle(p(26, 24, 38, 30), fill=BRONZE_DARK)  # the head
    poly(d, [(24, 24), (30, 4), (33, 14), (36, 6), (40, 24)], RED_LIGHT)
    poly(d, [(28, 24), (32, 10), (36, 24)], GOLD_LIGHT)
    return img, d


def torch_icon():
    img, _ = torch()
    save(img, "torch.png")


def no_torch_icon():
    img, d = torch()
    d.line(p(8, 56, 56, 8), fill=RED, width=5 * SS)
    save(img, "no_torch.png")


def bare_icon():  # nothing goes here
    img, d = canvas(GOLD, 0)
    d.ellipse(p(14, 14, 50, 50), outline=GOLD_DIM, width=4 * SS)
    d.line(p(20, 44, 44, 20), fill=GOLD_DIM, width=4 * SS)
    save(img, "bare.png")


def props():
    for f in (prop_web, prop_pottery, prop_canopic, prop_rubble, prop_sand, prop_skeleton, prop_brazier, prop_lamp,
              prop_scrolls, prop_ushabti, prop_cat, prop_jackal, prop_osiris, prop_sarcophagus, prop_water_reeds,
              prop_water_weed, prop_water_pads, prop_water_lotus):
        f()
    prop_thoth("thoth_ibis_standing", ibis, False)
    prop_thoth("thoth_ibis_seated", ibis, True)
    prop_thoth("thoth_baboon_seated", baboon, True)
    prop_thoth("thoth_baboon_standing", baboon, False)
    prop_coffin("coffin")
    prop_coffin("coffin_large", large=True)
    prop_coffin("coffin_mummy", sleeper=True)
    torch_icon()
    no_torch_icon()
    bare_icon()


if __name__ == "__main__":
    door()
    spikes("death.png", tall=True)
    spikes("spikes.png", tall=False)
    monster()
    ladder()
    treasure()
    trapped_treasure()
    ankh()
    key()
    gate()
    lever()
    rockfall()
    inscription()
    dartplate()
    props()
