"""Generate the HUD's status card backgrounds media/textures/ui/status_cards.png (RGBA, 3 cells of 128 px in one row).

    python3 tools/textures/status_cards.py [out.png]

One cell per StatusKind in enum order (src/ui/status_list.h, plan curse-blessing-cards): blessing (yellow-gold),
condition (neutral grey stone), curse (purple). Same shape and frame width on all three: a bevelled frame, a face with
a little grain, the middle calm (the icon goes there), the bottom strip a shade darker (the pips go there). The frames
differ in brightness too (blessing brightest, curse darkest), not only in hue, so they tell apart without colour.
Drawn at 4x and downsampled.
"""

import os
import random
import sys

from PIL import Image, ImageDraw, ImageFilter

from decals import bleed

CELL = 128
S = CELL * 4
FRAME = 44  # frame width in drawing pixels
STRIP = 120  # the pips' strip height at the bottom, drawing pixels
INK = (32, 22, 16)

# kind: (frame light, frame dark, face top, face bottom)
KINDS = [
    ((255, 232, 120), (150, 100, 20), (246, 204, 80), (196, 140, 36)),  # blessing
    ((190, 188, 182), (74, 72, 70), (140, 138, 132), (96, 94, 90)),  # condition
    ((140, 70, 168), (30, 8, 44), (84, 36, 108), (44, 14, 66)),  # curse
]


def lerp(a, b, t):
    return tuple(round(x + (y - x) * t) for x, y in zip(a, b))


def card(light, dark, top, bottom, seed):
    img = Image.new("RGBA", (S, S), INK + (255,))
    d = ImageDraw.Draw(img)
    pad = 10  # the ink line round the card
    # bevelled frame: light top and left, dark bottom and right, mitred corners
    x0, y0, x1, y1 = pad, pad, S - pad, S - pad
    d.polygon([(x0, y0), (x1, y0), (x1 - FRAME, y0 + FRAME), (x0 + FRAME, y0 + FRAME)], fill=light)
    d.polygon([(x0, y0), (x0 + FRAME, y0 + FRAME), (x0 + FRAME, y1 - FRAME), (x0, y1)], fill=lerp(light, dark, 0.35))
    d.polygon([(x1, y0), (x1, y1), (x1 - FRAME, y1 - FRAME), (x1 - FRAME, y0 + FRAME)], fill=lerp(light, dark, 0.7))
    d.polygon([(x0, y1), (x1, y1), (x1 - FRAME, y1 - FRAME), (x0 + FRAME, y1 - FRAME)], fill=dark)
    # the face: a vertical gradient, the strip darker
    fx0, fy0, fx1, fy1 = x0 + FRAME, y0 + FRAME, x1 - FRAME, y1 - FRAME
    for y in range(fy0, fy1):
        t = (y - fy0) / (fy1 - fy0)
        c = lerp(top, bottom, t)
        if y > fy1 - STRIP:
            c = lerp(c, (0, 0, 0), 0.22)
        d.line([(fx0, y), (fx1 - 1, y)], fill=c)
    # grain: faint speckles
    rnd = random.Random(seed)
    for _ in range(2600):
        x, y = rnd.randrange(fx0, fx1), rnd.randrange(fy0, fy1)
        r = rnd.randrange(2, 6)
        k = rnd.choice((-1, 1))
        base = img.getpixel((x, y))[:3]
        c = lerp(base, (255, 255, 255) if k > 0 else (0, 0, 0), 0.07)
        d.ellipse([x - r, y - r, x + r, y + r], fill=c)
    # the face's inner edge: a thin ink line, so the icon sits in a well
    d.rectangle([fx0, fy0, fx1 - 1, fy1 - 1], outline=lerp(dark, INK, 0.5), width=6)
    return img.filter(ImageFilter.GaussianBlur(1.2))


def main(path):
    atlas = Image.new("RGBA", (CELL * len(KINDS), CELL), (0, 0, 0, 0))
    for k, (light, dark, top, bottom) in enumerate(KINDS):
        atlas.paste(card(light, dark, top, bottom, 7 + k).resize((CELL, CELL), Image.LANCZOS), (k * CELL, 0))
    bleed(atlas).save(path)
    print("wrote", path)


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "..", "media", "textures", "ui", "status_cards.png"))
