"""Generate the journal's ribbon bookmark texture textures/ui/ribbon.png (RGBA, 544 x 224).

    python3 tools/textures/ribbon.py [out.png]

A woven silk ribbon lying flat, the swallowtail end on the right, in light greys: the journal tints it per section
(JournalScreen::DrawRibbons). The image covers 17 x 7 ribbon units at 32 px each: the ribbon is x 0..16, y 1..7 from
the bottom; the extra unit right and below holds its soft drop shadow. Cut edges fray, a few loose threads stick out.
"""

import os
import sys

import numpy as np
from PIL import Image, ImageFilter


PX = 32  # pixels per ribbon unit
W, H = 17 * PX, 7 * PX
LEN, WIDTH = 16.0, 6.0  # the ribbon, in units
NOTCH = 2.0  # swallowtail depth
SHADOW = (0.4, -0.6)  # offset in units, x right, y up
SHADOW_ALPHA = 0.4
SEED = 7

rng = np.random.default_rng(SEED)

# Ribbon coordinates per pixel centre: x along, y across from the bottom edge of the ribbon.
xs = (np.arange(W) + 0.5) / PX
ys = WIDTH - ((np.arange(H) + 0.5) / PX)  # image row 0 is the top edge (y = 6)
X, Y = np.meshgrid(xs, ys)


def smooth_noise(n, scale, seed):
    """1D value noise, `n` samples, features `scale` samples long."""
    r = np.random.default_rng(seed)
    knots = r.uniform(-1, 1, int(n / scale) + 3)
    t = np.arange(n) / scale
    i = t.astype(int)
    f = t - i
    f = f * f * (3 - 2 * f)
    return knots[i] * (1 - f) + knots[i + 1] * f


def shape_mask(fray):
    """Ribbon coverage: the strip minus the swallowtail notch, cut edges roughened by `fray` (units)."""
    v = Y / WIDTH  # 0 bottom, 1 top
    notch_x = LEN - NOTCH * (1 - np.abs(2 * v - 1))  # the cut: tips at the corners, deepest in the middle
    cut = notch_x + fray
    inside = (Y >= 0) & (Y <= WIDTH) & (X >= 0) & (X <= cut)
    return inside.astype(np.float32)


# ---- weave ----------------------------------------------------------------
# Warp threads run along the ribbon, 5 per unit, each a little lighter or darker; a fine cross rib (grosgrain)
# over them.
THREADS = 5.0
warp_phase = Y * THREADS
thread_id = np.floor(warp_phase).astype(int) + 1  # >= 0: y goes down to -1
n_threads = thread_id.max() + 2
thread_shade = rng.normal(0, 0.035, n_threads)[thread_id]
warp = np.sin(np.pi * (warp_phase % 1.0)) ** 0.7  # bright on the thread, dark between
rib = 0.5 + 0.5 * np.cos(2 * np.pi * (X * 7.0 + 0.15 * thread_id % 2))
weave = 0.80 + 0.13 * warp + 0.05 * rib * warp + thread_shade
grain = rng.random((H, W)).astype(np.float32)
grain = np.asarray(Image.fromarray((grain * 255).astype(np.uint8)).resize((W // 4, H)).resize((W, H), Image.BILINEAR),
                   dtype=np.float32) / 255
weave += 0.06 * (grain - 0.5)  # slubs: thicker and thinner bits along each thread

# ---- shading --------------------------------------------------------------
v = Y / WIDTH
across = 0.88 + 0.12 * np.sin(np.pi * np.clip(v, 0, 1)) ** 0.5  # a slight curl: edges turn away from the light
# Soft creases lying on the page, slanted a little: the cloth was folded once and pressed flat by the pages.
crease_t = X + 0.35 * (Y - WIDTH / 2)
crease = np.interp(crease_t, np.arange(-4, 22, 0.5), rng.uniform(-1, 1, 52))
crease = np.asarray(Image.fromarray(((crease + 1) * 127.5).astype(np.uint8)).filter(ImageFilter.GaussianBlur(0.4 * PX)),
                    dtype=np.float32) / 127.5 - 1
folds = 1.0 + 0.09 * crease
sheen = 0.13 * np.exp(-((v - 0.64 - 0.06 * crease) / 0.11) ** 2) * (0.6 + 0.4 * crease)  # silk sheen, bent by the creases
selvedge = 1.0 - 0.2 * np.clip(1 - np.minimum(v, 1 - v) / 0.045, 0, 1)  # the denser woven edges
light = weave * across * folds * selvedge + np.maximum(sheen, 0)
light = np.clip(light, 0, 1)

# ---- shape, fraying -------------------------------------------------------
# The cut frays: each fibre (3 per thread) ends a little before or after the cut, some hang out further. Only the
# fibre's core sticks out, so the frayed ends are thin wisps.
FIBRES = THREADS * 3
fibre_phase = Y * FIBRES
fibre_id = np.floor(fibre_phase).astype(int) + 4
n_fibres = fibre_id.max() + 2
length = rng.exponential(0.10, n_fibres) - 0.05
long_ = rng.random(n_fibres) < 0.10
length = np.where(long_, rng.uniform(0.3, 0.75, n_fibres), length)
core = np.abs((fibre_phase % 1.0) - 0.5) < 0.3
fray = np.where(core, length[fibre_id], np.minimum(length[fibre_id], 0.0))
fray += 0.04 * smooth_noise(H, 0.2 * PX, SEED + 2)[:, None]  # the scissors did not cut quite straight
mask = shape_mask(fray)
clean = shape_mask(np.zeros_like(X))
light = np.where((mask > 0) & (clean == 0), light * 0.85, light)  # loose ends: thinner, darker

# Antialias the mask by drawing it at the pixel scale and blurring a hair.
alpha = np.asarray(Image.fromarray((mask * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(0.6)),
                   dtype=np.float32) / 255

# ---- shadow ---------------------------------------------------------------
dx, dy = int(round(SHADOW[0] * PX)), int(round(-SHADOW[1] * PX))
shadow = np.zeros_like(alpha)
shadow[dy:, dx:] = clean[:H - dy, :W - dx]
shadow = np.asarray(Image.fromarray((shadow * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(0.35 * PX)),
                    dtype=np.float32) / 255 * SHADOW_ALPHA

# Ribbon over its shadow: the shadow is black, so the tint leaves it black.
out_a = alpha + shadow * (1 - alpha)
rgb = np.where(out_a > 0, light * alpha / np.maximum(out_a, 1e-6), 0)

img = np.dstack([rgb, rgb, rgb, out_a])
img = Image.fromarray((np.clip(img, 0, 1) * 255).astype(np.uint8), "RGBA")


out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "../../textures/ui/ribbon.png")
img.save(out)
print(out, img.size)
