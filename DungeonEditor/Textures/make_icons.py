#!/usr/bin/env python3
"""Draws the editor's tile icons (64 x 64 PNG) in the game's UI palette: gold, bronze and lapis on dark open space.

Run from anywhere: python3 DungeonEditor/Textures/make_icons.py
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
    img = Image.new("RGB", (S, S), OPEN)
    # Soft glow behind the symbol, so it reads as an object lit in the dark.
    halo = Image.new("L", (S, S), 0)
    ImageDraw.Draw(halo).ellipse(p(10, 10, 54, 54), fill=glow_alpha)
    halo = halo.filter(ImageFilter.GaussianBlur(10 * SS))
    img.paste(Image.new("RGB", (S, S), glow), (0, 0), halo)
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


def area3d():  # unused tile: faded wireframe cube
    img, d = canvas(GOLD, 15)
    front = [(12, 24), (40, 24), (40, 52), (12, 52)]
    back = [(x + 12, y - 12) for x, y in front]
    for a, b in zip(front, back):
        d.line(p(*a, *b), fill=GOLD_DIM, width=2 * SS)
    for quad in (back, front):
        d.line([v * SS for xy in quad + [quad[0]] for v in xy], fill=GOLD_DIM, width=2 * SS)
    save(img, "3D.png")


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


if __name__ == "__main__":
    door()
    spikes("death.png", tall=True)
    spikes("spikes.png", tall=False)
    monster()
    ladder()
    area3d()
    treasure()
    ankh()
    key()
    gate()
    lever()
    rockfall()
