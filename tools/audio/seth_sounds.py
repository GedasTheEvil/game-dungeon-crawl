"""Synthesize Seth's sounds (plan hippo-boss): the hippo's waking call, bite and death pitched down (a god in the
red beast's shape, a bigger chest), and his storm roar: a bellow that swells into a howling gust of sand, grains
hissing and ticking on the walls as it dies away.

    python3 tools/audio/seth_sounds.py [out_dir]      (default: media/sounds/monsters/, writes seth_{wake,att,die,roar}.wav)

Everything is generated (no samples), so this script is the source of the sounds; the hippo's come from
hippo_sounds.py.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import hippo_sounds  # noqa: E402
from crocodile_sounds import REPO, SR, band, env, finish, lowpass, place, rms, t_axis, write  # noqa: E402

rng = np.random.default_rng(97)  # fixed seed: the same files on every run
PITCH = 0.78  # of the hippo's: slower and deeper


def pitched(x, factor=PITCH):
    """Played back at factor of its speed: longer and lower."""
    n = int(len(x) / factor)
    return np.interp(np.arange(n) * factor, np.arange(len(x)), x)


def gust(dur, lo, hi):
    """Wind with sand in it: band noise whose band and loudness wander, swelling in and dying away."""
    t = t_axis(dur)
    swell = np.clip(t / (0.25 * dur), 0, 1) * np.clip((dur - t) / (0.55 * dur), 0, 1)
    wobble = 1 + 0.5 * lowpass(rng.standard_normal(len(t)), 3) * 4
    howl = band(rng.standard_normal(len(t)), lo, hi) * wobble
    whistle = band(rng.standard_normal(len(t)), hi * 0.9, hi * 1.3) * (0.4 + 0.6 * np.sin(2 * np.pi * 0.7 * t) ** 2)
    return (howl + 0.5 * whistle) * swell


def grains(dur, count):
    """Sand ticking on stone: short bright clicks, thinning out."""
    out = np.zeros(int(dur * SR))
    for _ in range(count):
        at = dur * rng.uniform(0, 1) ** 1.6
        g = band(rng.standard_normal(int(0.006 * SR)), 2500, 8000) * env(t_axis(0.006), 0.0003, 0.0015)
        place(out, g * rng.uniform(0.2, 1.0), at)
    return out


def roar():
    """The storm roar: a deep bellow, the gust of sand it throws, grains on the walls."""
    out = np.zeros(int(2.4 * SR))
    d = 1.1
    t = t_axis(d)
    bellow = hippo_sounds.voice(d, 58, 44, [(360, 220, 1.6), (800, 420, 1.1), (1900, 800, 0.35)], 0.9, env(t, 0.05, 0.5))
    place(out, rms(bellow, 0.24), 0.0)
    place(out, rms(gust(1.9, 250, 1400), 0.12), 0.25)
    place(out, rms(grains(1.6, 160), 0.03), 0.5)
    return finish(out)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds", "monsters")
    write(os.path.join(out_dir, "seth_wake.wav"), finish(pitched(hippo_sounds.wake())))
    write(os.path.join(out_dir, "seth_att.wav"), finish(pitched(hippo_sounds.attack())))
    write(os.path.join(out_dir, "seth_die.wav"), finish(pitched(hippo_sounds.die())))
    write(os.path.join(out_dir, "seth_roar.wav"), roar())


if __name__ == "__main__":
    main()
