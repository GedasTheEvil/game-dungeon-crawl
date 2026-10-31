"""Synthesize the sand worm's wake (plan missing-monster-sounds), played as it dives and as it bursts up through the
sand: a low rumble from below, sand bursting up, grains pattering down. Smaller and quicker than Rerek's
(rerek_sounds.py).

    python3 tools/audio/worm_sounds.py [out_dir]      (default: media/sounds/monsters/, writes worm_wake.wav)

Everything is generated (no samples), so this script is the source of worm_wake.wav; the worm's bite and death are
older files.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from crocodile_sounds import REPO, SR, band, env, finish, lowpass, place, rms, rumble, t_axis, write  # noqa: E402
from seth_sounds import grains, gust  # noqa: E402

rng = np.random.default_rng(131)  # fixed seed: the same files on every run


def wake():
    """A short rumble under the floor, the burst of sand thrown up, a soft hiss of it sliding off, grains falling."""
    out = np.zeros(int(1.2 * SR))
    place(out, rms(rumble(0.45, 60, 80, 0.6) * env(t_axis(0.45), 0.15, 0.2), 0.12), 0.0)
    b = t_axis(0.25)
    burst = lowpass(rng.standard_normal(len(b)), 1200) * env(b, 0.008, 0.07)
    place(out, rms(burst, 0.12), 0.18)
    place(out, rms(gust(0.6, 400, 2500), 0.07), 0.2)
    slide = t_axis(0.5)
    place(out, rms(band(rng.standard_normal(len(slide)), 1500, 6000) * env(slide, 0.05, 0.2), 0.03), 0.3)
    place(out, rms(grains(0.8, 60), 0.025), 0.35)
    return finish(out)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds", "monsters")
    write(os.path.join(out_dir, "worm_wake.wav"), wake())


if __name__ == "__main__":
    main()
