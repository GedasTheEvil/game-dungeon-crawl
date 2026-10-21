"""Generate the earlier expedition's torn notebook page media/textures/ui/expedition_page.png (RGBA, 256 x 320).

    python3 tools/textures/expedition_page.py [out.png]

A page torn out of a ruled pocket notebook (plan expedition-notes): yellowed paper, faded blue rules and a red margin
line, tea-coloured stains and foxing, a crease across the middle, the left edge ragged where it came out of the
binding, the other edges worn. No writing: the journal writes the page's text over it (JournalScreen), and in the
tomb it lies too small to read (Dungeon::drawPageTile). Transparent outside the paper.
"""

import os
import sys

import numpy as np
from PIL import Image, ImageFilter


W, H = 256, 320
SEED = 11
TEAR = 14  # px: how deep the torn edge bites in
WEAR = 4  # px: the worn edges
RULE_STEP = 22  # px between the rules
RULE_TOP = 46  # first rule below the top edge
MARGIN_X = 52  # the red margin line

rng = np.random.default_rng(SEED)


def smooth_noise(n, scale, seed):
    """1D value noise in -1..1, `n` samples, features `scale` samples long."""
    r = np.random.default_rng(seed)
    knots = r.uniform(-1, 1, int(n / scale) + 3)
    t = np.arange(n) / scale
    i = t.astype(int)
    f = t - i
    f = f * f * (3 - 2 * f)
    return knots[i] * (1 - f) + knots[i + 1] * f


def blotches(count, rmin, rmax, seed):
    """Soft round stains, 0..1."""
    r = np.random.default_rng(seed)
    yy, xx = np.mgrid[0:H, 0:W]
    out = np.zeros((H, W))
    for _ in range(count):
        cx, cy = r.uniform(0, W), r.uniform(0, H)
        rad = r.uniform(rmin, rmax)
        d = np.hypot(xx - cx, (yy - cy) * r.uniform(0.7, 1.3)) / rad
        ring = np.clip(1 - d, 0, 1) ** 1.5 + 0.6 * np.exp(-((d - 1) ** 2) / 0.004)  # a darker tide mark at the rim
        out = np.maximum(out, ring * r.uniform(0.3, 1.0))
    return np.clip(out, 0, 1)


def main(path):
    yy, xx = np.mgrid[0:H, 0:W].astype(float)

    # The paper's outline: a ragged tear on the left, worn and a little wavy elsewhere.
    tear = TEAR * (0.5 + 0.35 * smooth_noise(H, 9, SEED + 1) + 0.15 * smooth_noise(H, 2.5, SEED + 2))
    left = tear[:, None]
    right = W - WEAR - WEAR * smooth_noise(H, 30, SEED + 3)[:, None]
    top = WEAR + WEAR * smooth_noise(W, 40, SEED + 4)[None, :]
    bottom = H - WEAR - 2 * WEAR * smooth_noise(W, 25, SEED + 5)[None, :]
    inside = (xx > left) & (xx < right) & (yy > top) & (yy < bottom)
    alpha = Image.fromarray((inside * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(0.8))
    alpha = np.asarray(alpha).astype(float) / 255

    # Yellowed paper, darker towards the edges, fibres.
    edge = np.minimum.reduce([xx - left, right - xx, yy - top, bottom - yy])
    darken = np.clip(1 - edge / 28, 0, 1) ** 2
    paper = np.array([0.90, 0.84, 0.68])
    fibre = rng.normal(0, 0.025, (H, W))
    fibre = np.asarray(Image.fromarray(((fibre + 0.5) * 255).clip(0, 255).astype(np.uint8)).filter(
        ImageFilter.GaussianBlur(0.7))).astype(float) / 255 - 0.5
    rgb = paper[None, None, :] * (1 - 0.35 * darken[..., None]) + fibre[..., None] * 0.5

    # Faded rules and the margin line.
    for y in range(RULE_TOP, H - 10, RULE_STEP):
        wobble = y + 0.8 * smooth_noise(W, 60, y)
        line = np.exp(-((yy - wobble[None, :]) ** 2) / 0.9)
        rgb = rgb * (1 - 0.28 * line[..., None]) + np.array([0.45, 0.55, 0.75]) * 0.28 * line[..., None]
    margin = np.exp(-((xx - MARGIN_X) ** 2) / 0.8)
    rgb = rgb * (1 - 0.3 * margin[..., None]) + np.array([0.75, 0.35, 0.30]) * 0.3 * margin[..., None]

    # Stains: a few large tea marks, many small foxing spots.
    tea = blotches(3, 30, 70, SEED + 6) * 0.35 + blotches(25, 2, 6, SEED + 7) * 0.5
    rgb = rgb * (1 - 0.35 * tea[..., None]) + np.array([0.55, 0.38, 0.18]) * 0.35 * tea[..., None]

    # A crease across the middle: a dark line with a light side.
    crease_y = H * 0.52 + 3 * smooth_noise(W, 70, SEED + 8)[None, :]
    rgb *= (1 - 0.18 * np.exp(-((yy - crease_y) ** 2) / 1.5))[..., None]
    rgb += (0.06 * np.exp(-((yy - crease_y - 2.5) ** 2) / 2.0))[..., None]

    rgba = np.dstack([np.clip(rgb, 0, 1), alpha])
    os.makedirs(os.path.dirname(path), exist_ok=True)
    Image.fromarray((rgba * 255).round().astype(np.uint8), "RGBA").save(path)
    print(f"wrote {path}")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "media/textures/ui/expedition_page.png")
