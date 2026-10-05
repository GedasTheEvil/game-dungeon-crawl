"""Generate the journal's paper and cover textures (JournalScreen, src/ui/journal_view.cpp).

    python3 tools/textures/journal_book.py [out_dir]      (default: textures/ui/)

journal_paper.png (RGB, 1024 x 1086): one page of a field notebook, 66 x 70 journal units, the spine on the left.
White-cream paper with a fine blue-green grid (2.5 units a square) printed over the whole page, a light grain, and
the shading of the page curving down into the spine. The left page draws it mirrored.

journal_cloth.png (RGB, 512 x 512, tiles): the grey book cloth of the cover, a plain weave with slubs and a little
wear, drawn untinted.
"""

import os
import sys

import numpy as np
from PIL import Image, ImageFilter

SEED = 11
rng = np.random.default_rng(SEED)


def blurred_noise(h, w, radius, seed):
    """Smooth noise in -1..1, features about `radius` px."""
    r = np.random.default_rng(seed)
    n = Image.fromarray((r.random((h, w)) * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(radius))
    a = np.asarray(n, dtype=np.float32) / 255
    return (a - a.mean()) / max(a.std() * 3, 1e-6)


def save(rgb, path):
    img = Image.fromarray((np.clip(rgb, 0, 1) * 255).astype(np.uint8), "RGB")
    img.save(path)
    print(path, img.size)


# ---- paper ----------------------------------------------------------------

PAGE_W, PAGE_H = 66.0, 70.0  # units
PW = 1024
PH = round(PW * PAGE_H / PAGE_W)
PX = PW / PAGE_W  # pixels per unit
GRID = 2.5  # units per square
PAPER = np.array([0.965, 0.950, 0.905])  # white-cream
GRID_INK = np.array([0.50, 0.72, 0.72])  # light blue-green


def paper():
    xs = (np.arange(PW) + 0.5) / PX  # units from the spine
    ys = (np.arange(PH) + 0.5) / PX
    X, Y = np.meshgrid(xs, ys)

    # Grid lines, antialiased: coverage falls off over a pixel from each line's centre.
    line_w = 1.0 / PX  # ~1 px wide
    def coverage(c):
        d = np.abs(((c + GRID / 2) % GRID) - GRID / 2)  # distance to the nearest line
        return np.clip(1 - (d - line_w / 2) * PX, 0, 1)
    grid = np.maximum(coverage(X - 0.6), coverage(Y - 1.1))
    # Printed ink is not perfectly even.
    grid *= 0.38 + 0.06 * blurred_noise(PH, PW, 6, SEED + 1)

    rgb = PAPER[None, None, :] * (1 - grid[..., None]) + (PAPER * GRID_INK)[None, None, :] * grid[..., None]

    # Grain: fine fibres plus soft cloudiness.
    fine = rng.normal(0, 0.012, (PH, PW))
    cloud = 0.012 * blurred_noise(PH, PW, 30, SEED + 2)
    rgb *= (1 + fine + cloud)[..., None]

    # The page curves down into the spine: darker over the last few units, a deep fold right at it.
    curve = 1 - 0.30 * np.exp(-X / 1.2) - 0.10 * np.exp(-X / 5.0)
    # Lifted a little near the spine before it dips: a faint highlight.
    curve += 0.02 * np.exp(-((X - 6.0) / 3.0) ** 2)
    # The outer edge and the top and bottom edges turn away a hair.
    edge = 1 - 0.05 * np.exp(-(PAGE_W - X) / 0.8) - 0.03 * np.exp(-Y / 0.8) - 0.03 * np.exp(-(PAGE_H - Y) / 0.8)
    rgb *= (curve * edge)[..., None]
    return rgb


# ---- cloth ----------------------------------------------------------------

CW = 512
THREADS = 128  # per tile, each way
CLOTH = np.array([0.40, 0.40, 0.385])  # mid grey, a touch warm; the code darkens it


def cloth():
    t = (np.arange(CW) + 0.5) / CW * THREADS
    X, Y = np.meshgrid(t, t)
    ix, iy = np.floor(X).astype(int), np.floor(Y).astype(int)
    fx, fy = X - ix, Y - iy
    # Plain weave: the warp is on top where (ix + iy) is even, the weft where it is odd.
    warp_top = (ix + iy) % 2 == 0
    # Each thread a little lighter or darker, and thicker or thinner along its length (slubs), tiling.
    warp_shade = rng.normal(0, 0.018, THREADS)[ix % THREADS]
    weft_shade = rng.normal(0, 0.018, THREADS)[iy % THREADS]
    slub = 0.04 * blurred_noise(CW, CW, 3, SEED + 3)
    warp = np.sin(np.pi * fx) ** 0.6 * (0.85 + 0.15 * np.sin(np.pi * fy))
    weft = np.sin(np.pi * fy) ** 0.6 * (0.85 + 0.15 * np.sin(np.pi * fx))
    light = np.where(warp_top, 0.78 + 0.22 * warp + warp_shade, 0.74 + 0.22 * weft + weft_shade) + slub
    # Wear: a few lighter rubbed patches, soft, tiling (wrap the noise by blurring a tiled copy).
    big = rng.random((CW // 8, CW // 8))
    tiled = np.tile(big, (3, 3))
    patch = np.asarray(Image.fromarray((tiled * 255).astype(np.uint8)).resize((CW * 3, CW * 3), Image.BICUBIC)
                       .filter(ImageFilter.GaussianBlur(24)), dtype=np.float32)[CW:2 * CW, CW:2 * CW] / 255
    patch = (patch - patch.mean()) / max(patch.std(), 1e-6)
    light *= 1 + 0.035 * patch
    return CLOTH[None, None, :] * light[..., None]


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                                             "../../textures/ui")
    save(paper(), os.path.join(out, "journal_paper.png"))
    save(cloth(), os.path.join(out, "journal_cloth.png"))


if __name__ == "__main__":
    main()
