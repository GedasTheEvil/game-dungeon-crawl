"""Synthesize the Mother of Webs' sounds (plan camel-spider-boss): the camel spider's, from a far bigger body. The
attack (a slow, deep stridulation, then the jaws shearing shut, heavier, and sawing), the death (a long rasping
groan breaking up, legs scraping and drumming, a few last ticks) and the wake (a low dry hiss rising into a rasp, the
legs drumming on the stone). The parts come from camel_spider_sounds.py and scorpion_sounds.py.

    python3 tools/audio/mother_sounds.py [out_dir]  (default: media/sounds/monsters/, writes mother_{att,die,wake}.wav)

Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from camel_spider_sounds import rasp, shear  # noqa: E402
from scorpion_sounds import REPO, SR, band, clack, env, finish, place, rattle, rms, scrape, t_axis, write  # noqa: E402

rng = np.random.default_rng(83)  # fixed seed: the same files on every run


def hiss(dur, lo=600, hi=3200):
    """A dry breath of air through the spiracles: band-limited noise swelling and dying."""
    t = t_axis(dur)
    return band(rng.standard_normal(len(t)), lo, hi) * np.sin(np.pi * t / dur) ** 1.5


def drum(dur, rate):
    """Eight legs drumming on the stone: dull low clacks, rate per second, a little uneven."""
    out = np.zeros(int(dur * SR))
    at = 0.0
    while at < dur - 0.05:
        place(out, clack(0.05, 700 + 300 * rng.random(), 1.6), at)
        at += (0.7 + 0.6 * rng.random()) / rate
    return out


def attack():
    """A deep rasp of warning, the lunge: two heavy shear snaps of the jaws, then a slow saw of them."""
    out = np.zeros(int(1.0 * SR))
    place(out, rms(rasp(0.32, 40, 70, 900, 4500), 0.07), 0.0)
    place(out, rms(shear(0.12, 1900), 0.14), 0.32)
    place(out, rms(shear(0.11, 1600), 0.12), 0.46)
    place(out, rms(rasp(0.26, 25, 15, 800, 3500), 0.05), 0.60)
    place(out, rms(scrape(0.2, 20), 0.025), 0.64)
    return finish(out)


def die():
    """A long rasping groan that breaks up, the legs scraping and drumming slower, a few last ticks."""
    out = np.zeros(int(2.2 * SR))
    groan = rasp(0.9, 80, 25, 900, 5000) * env(t_axis(0.9), 0.02, 0.5)
    place(out, rms(groan, 0.09), 0.0)
    place(out, rms(drum(0.9, 9), 0.05), 0.3)
    place(out, rms(rattle(0.8, 25, 6), 0.05), 0.5)
    place(out, rms(scrape(1.2, 8) * env(t_axis(1.2), 0.05, 0.5), 0.04), 0.2)
    for at, level in ((1.7, 0.025), (1.95, 0.012)):
        place(out, rms(clack(0.05, 2200, 1.0), level), at)
    return finish(out)


def wake():
    """A low dry hiss rising into a rasp as she wakes, the legs drumming on the stone."""
    out = np.zeros(int(1.3 * SR))
    place(out, rms(hiss(0.7), 0.06), 0.0)
    place(out, rms(rasp(0.5, 30, 90, 1000, 5000), 0.06), 0.45)
    place(out, rms(drum(0.8, 14), 0.05), 0.4)
    return finish(out)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds", "monsters")
    write(os.path.join(out_dir, "mother_att.wav"), attack())
    write(os.path.join(out_dir, "mother_die.wav"), die())
    write(os.path.join(out_dir, "mother_wake.wav"), wake())


if __name__ == "__main__":
    main()
