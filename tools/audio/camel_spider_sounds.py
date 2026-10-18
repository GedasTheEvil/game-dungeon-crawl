"""Synthesize the camel spider sounds: the attack (a rasping stridulation of the chelicerae rubbed together, then the
jaws snapping shut like shears, twice, and sawing) and the death (a thin rasping squeal breaking up, legs scraping,
a last few ticks). The chitin clicks, scrapes and rattles come from scorpion_sounds.py.

    python3 tools/audio/camel_spider_sounds.py [out_dir]  (default: media/sounds/monsters/, writes camel_spider_{att,die}.wav)

Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from scorpion_sounds import REPO, SR, band, clack, env, finish, place, rattle, rms, scrape, t_axis, write  # noqa: E402

rng = np.random.default_rng(61)  # fixed seed: the same files on every run


def rasp(dur, rate0, rate1, lo=1800, hi=7000):
    """Stridulation: the ridged jaws rubbed together, a buzz of tiny scrapes at rate0 gliding to rate1 per second."""
    t = t_axis(dur)
    phase = 2 * np.pi * np.cumsum(np.linspace(rate0, rate1, len(t))) / SR
    pulses = np.clip(np.sin(phase), 0, None) ** 6
    grit = band(rng.standard_normal(len(t)), lo, hi)
    shape = np.minimum(t / 0.02, 1.0) * np.clip((dur - t) / (0.3 * dur), 0, 1)
    return grit * pulses * shape


def shear(dur=0.09, pitch=3000):
    """Jaws closing like shears: a short metallic slide into a hard clack."""
    t = t_axis(dur)
    k = t / dur
    slide = band(rng.standard_normal(len(t)), 3000, 9000) * np.sin(np.pi * np.minimum(k / 0.7, 1.0)) ** 2 * (k < 0.7)
    out = 0.4 * slide
    place(out, clack(0.05, pitch, 1.2), 0.6 * dur)
    return out


def attack():
    """Rasps a warning, lunges: two shear snaps of the jaws, then a quick saw of them."""
    out = np.zeros(int(0.8 * SR))
    place(out, rms(rasp(0.22, 70, 110), 0.06), 0.0)
    place(out, rms(shear(0.09, 3200), 0.12), 0.24)
    place(out, rms(shear(0.08, 2800), 0.10), 0.34)
    place(out, rms(rasp(0.18, 40, 25, 1500, 5000), 0.04), 0.46)
    place(out, rms(scrape(0.15, 30), 0.02), 0.50)
    return finish(out)


def die():
    """A thin rasping squeal that breaks up, legs scraping, the rattle slowing to a last few ticks."""
    out = np.zeros(int(1.5 * SR))
    squeal = rasp(0.5, 140, 60, 2500, 8000) * env(t_axis(0.5), 0.01, 0.3)
    place(out, rms(squeal, 0.08), 0.0)
    place(out, rms(rattle(0.5, 35, 12), 0.06), 0.25)
    place(out, rms(scrape(0.8, 12) * env(t_axis(0.8), 0.05, 0.35), 0.04), 0.1)
    for at, level in ((1.15, 0.02), (1.35, 0.01)):
        place(out, rms(clack(0.04, 3300, 0.8), level), at)
    return finish(out)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds", "monsters")
    write(os.path.join(out_dir, "camel_spider_att.wav"), attack())
    write(os.path.join(out_dir, "camel_spider_die.wav"), die())


if __name__ == "__main__":
    main()
