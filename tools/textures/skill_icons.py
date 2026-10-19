"""Generate the skill icon atlas media/textures/ui/skill_icons.png (RGBA, 4 x 4 cells of 128 px).

    python3 tools/textures/skill_icons.py [out.png]

The look of the HUD icons (hud_icons.py): flat glyphs with an ink outline, drawn at 4x and downsampled. Cell 0 is the
heka mark (the ka arms: the Heka screen's tab, its points, the HUD's waiting point); from cell 1 on the skills in
SkillId order (src/world/skills.h, SKILL_ICON_FIRST).
"""

import math
import os
import sys

from PIL import Image, ImageDraw

from decals import bleed
from hud_icons import (BONE, BRONZE, CARNELIAN, CELL, DARK, FAIENCE, GOLD, INK, LAPIS, LINE, METAL, S, TURQUOISE, WHITE,
                       WOOD, outlined)

GRID = 4
WATER = (60, 130, 200, 255)
FOAM = (170, 215, 240, 255)
VENOM = (110, 200, 60, 255)
PAPYRUS = (226, 204, 150, 255)


def canvas():
    img = Image.new("RGBA", (S, S))
    return img, ImageDraw.Draw(img)


def stroke(d, pts, colour, width):
    """A line in colour with an ink edge."""
    d.line(pts, fill=INK, width=width + 2 * LINE, joint="curve")
    d.line(pts, fill=colour, width=width, joint="curve")


def arc(cx, cy, r, a0, a1, n=24):
    """Points on a circle, angles in degrees, image y down."""
    return [(cx + r * math.cos(math.radians(a0 + (a1 - a0) * t / n)), cy + r * math.sin(math.radians(a0 + (a1 - a0) * t / n)))
            for t in range(n + 1)]


def star(d, cx, cy, r, colour, points=5):
    pts = []
    for k in range(points * 2):
        rr = r if k % 2 == 0 else r * 0.45
        a = math.pi * k / points - math.pi / 2
        pts.append((cx + rr * math.cos(a), cy + rr * math.sin(a)))
    d.polygon(pts, fill=colour, outline=INK, width=LINE)


def heka():
    """The ka: two arms raised from the shoulders, gold, a lapis bar where they meet."""
    img, d = canvas()
    for side in (-1, 1):
        arm = [(256 + side * 40, 400), (256 + side * 150, 390), (256 + side * 170, 300), (256 + side * 160, 110)]
        stroke(d, arm, GOLD, 56)
        d.ellipse([256 + side * 160 - 42, 70, 256 + side * 160 + 42, 130], fill=GOLD, outline=INK, width=LINE)
    d.rectangle([186, 380, 326, 440], fill=LAPIS, outline=INK, width=LINE)
    return img


def stun():
    """A mace head over a daze ring of stars."""
    img, d = canvas()
    d.ellipse([96, 90, 416, 190], outline=INK, width=16)
    for a in (160, 270, 20):
        star(d, 256 + 160 * math.cos(math.radians(a)), 140 + 50 * math.sin(math.radians(a)), 48, GOLD)
    d.rectangle([236, 330, 276, 480], fill=WOOD, outline=INK, width=LINE)
    d.ellipse([166, 220, 346, 370], fill=METAL, outline=INK, width=LINE)
    for x, y in ((206, 280), (256, 255), (306, 280), (256, 325)):
        d.ellipse([x - 16, y - 16, x + 16, y + 16], fill=BRONZE, outline=INK, width=LINE)
    return img


def thoth():
    """An unrolled papyrus with three lines of glyphs, Thoth's reed pen across it."""
    img, d = canvas()
    d.rectangle([110, 120, 402, 392], fill=PAPYRUS, outline=INK, width=LINE)
    for y in (110, 402):
        d.rounded_rectangle([86, y - 28, 426, y + 28], radius=28, fill=PAPYRUS, outline=INK, width=LINE)
    for row, y in enumerate((170, 240, 310)):
        for k in range(4):
            x = 150 + 60 * k + 12 * row
            shape = (row + k) % 3
            if shape == 0:
                d.ellipse([x, y, x + 34, y + 34], fill=LAPIS)
            elif shape == 1:
                d.polygon([(x, y + 34), (x + 17, y), (x + 34, y + 34)], fill=CARNELIAN)
            else:
                d.rectangle([x + 4, y + 6, x + 30, y + 30], fill=INK)
    stroke(d, [(330, 460), (470, 70)], WOOD, 26)
    return img


def dodge():
    """An arrow passing by a dodging body: the arrow, speed lines and the body's curved lean."""
    img, d = canvas()
    stroke(d, arc(330, 260, 150, 120, 240, 20), BONE, 70)
    d.ellipse([140, 60, 230, 150], fill=BONE, outline=INK, width=LINE)
    for y, x0 in ((180, 300), (250, 340), (320, 300)):
        d.line([(x0, y), (x0 + 120, y)], fill=INK, width=18)
    stroke(d, [(70, 420), (460, 420)], WOOD, 22)
    d.polygon([(470, 420), (400, 380), (410, 420), (400, 460)], fill=METAL, outline=INK, width=LINE)
    d.polygon([(70, 420), (40, 385), (110, 400), (110, 440), (40, 455)], fill=CARNELIAN, outline=INK, width=LINE)
    return img


def power_hit():
    """A fist on a carnelian burst."""
    img, d = canvas()
    burst = []
    for k in range(24):
        r = 230 if k % 2 == 0 else 140
        a = math.pi * k / 12
        burst.append((256 + r * math.cos(a), 256 + r * math.sin(a)))
    d.polygon(burst, fill=CARNELIAN, outline=INK, width=LINE)
    d.rounded_rectangle([150, 180, 362, 360], radius=50, fill=BONE, outline=INK, width=LINE)
    for k in range(3):
        x = 203 + 53 * k
        d.line([(x, 180), (x, 260)], fill=INK, width=LINE)
    d.rounded_rectangle([130, 270, 250, 340], radius=30, fill=BONE, outline=INK, width=LINE)  # the thumb
    d.rectangle([190, 360, 322, 440], fill=BRONZE, outline=INK, width=LINE)  # the bracer
    return img


def sharp_eye():
    """The eye of Horus with an arrow flying out to the right."""
    img, d = canvas()
    stroke(d, [(200, 330), (180, 450)], LAPIS, 30)
    spiral = [(270 + (10 + 5 * t) * math.cos(t / 2.4), 400 + (10 + 5 * t) * math.sin(t / 2.4)) for t in range(14, -1, -1)]
    stroke(d, [(250, 330)] + spiral, LAPIS, 30)
    d.polygon([(70, 250), (170, 180), (300, 180), (370, 250), (280, 320), (150, 320)], fill=WHITE, outline=INK,
              width=LINE)
    d.ellipse([175, 190, 285, 300], fill=LAPIS, outline=INK, width=LINE)
    stroke(d, [(60, 140), (360, 140)], LAPIS, 28)
    stroke(d, [(330, 250), (470, 250)], WOOD, 18)
    d.polygon([(490, 250), (440, 220), (450, 250), (440, 280)], fill=METAL, outline=INK, width=LINE)
    return img


def hapi():
    """Three waves of the Nile with a papyrus umbel growing out of them."""
    img, d = canvas()
    stroke(d, [(256, 300), (256, 120)], FAIENCE, 30)
    fan = [(256, 130)] + arc(256, 130, 110, 200, 340, 12)
    d.polygon(fan, fill=FAIENCE, outline=INK, width=LINE)
    for k, y in enumerate((300, 360, 420)):
        wave = [(60 + 6 * t, y + 26 * math.sin(t / 8 + k)) for t in range(66)]
        stroke(d, wave, WATER if k % 2 == 0 else FOAM, 34)
    return img


def serket():
    """A scorpion's tail curled over a venom drop that is crossed out."""
    img, d = canvas()
    d.polygon([(330, 190), (276, 320), (384, 320)], fill=VENOM)
    d.ellipse([272, 270, 388, 400], fill=VENOM, outline=INK, width=LINE)
    d.line([(330, 190), (274, 324)], fill=INK, width=LINE)
    d.line([(330, 190), (386, 324)], fill=INK, width=LINE)
    d.polygon([(330, 200), (282, 318), (378, 318)], fill=VENOM)
    tail = [(90, 440), (110, 340), (150, 250), (210, 170), (290, 120), (360, 120)]
    for k, (x, y) in enumerate(tail):
        r = 44 - 4 * k
        d.ellipse([x - r, y - r, x + r, y + r], fill=GOLD, outline=INK, width=LINE)
    d.polygon([(390, 110), (450, 150), (400, 160)], fill=CARNELIAN, outline=INK, width=LINE)
    stroke(d, [(250, 430), (420, 230)], CARNELIAN, 26)
    return img


def isis():
    """Isis's spread wings in lapis and turquoise round a gold sun."""
    img, d = canvas()
    for side in (-1, 1):
        for k, colour in enumerate((LAPIS, TURQUOISE, LAPIS)):
            y = 230 + 50 * k
            reach = 220 - 30 * k
            feather = [(256 + side * 50, y - 20), (256 + side * reach, y - 70 + 20 * k), (256 + side * (reach - 20), y + 30),
                       (256 + side * 50, y + 40)]
            d.polygon(feather, fill=colour, outline=INK, width=LINE)
    d.ellipse([196, 150, 316, 270], fill=GOLD, outline=INK, width=LINE)
    d.rectangle([226, 260, 286, 420], fill=DARK, outline=INK, width=LINE)
    return img


def flaming_arrows():
    """An arrow flying right with a flame on its tip."""
    img, d = canvas()
    flame = [(300, 270), (320, 190), (380, 110), (370, 200), (430, 150), (440, 260), (470, 330), (400, 380), (330, 360)]
    d.polygon(flame, fill=CARNELIAN, outline=INK, width=LINE)
    inner = [(340, 300), (370, 250), (410, 230), (420, 300), (390, 340), (350, 335)]
    d.polygon(inner, fill=GOLD, outline=INK, width=LINE)
    stroke(d, [(50, 330), (400, 330)], WOOD, 22)
    d.polygon([(470, 330), (390, 285), (405, 330), (390, 375)], fill=METAL, outline=INK, width=LINE)
    d.polygon([(50, 330), (20, 290), (95, 305), (95, 355), (20, 370)], fill=BONE, outline=INK, width=LINE)
    return img


# Order = atlas cell: the heka mark, then SkillId order.
ICONS = [heka, stun, thoth, dodge, power_hit, sharp_eye, hapi, serket, isis, flaming_arrows]


def main(path):
    atlas = Image.new("RGBA", (CELL * GRID, CELL * GRID), (0, 0, 0, 0))
    for k, fn in enumerate(ICONS):
        cell = outlined(fn()).resize((CELL, CELL), Image.LANCZOS)
        atlas.paste(cell, ((k % GRID) * CELL, (k // GRID) * CELL))
    bleed(atlas).save(path)
    print("wrote", path)


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "..", "media", "textures", "ui", "skill_icons.png"))
