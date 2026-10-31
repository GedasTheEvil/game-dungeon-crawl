"""Synthesize the man-eater plant's sounds (plan missing-monster-sounds): the bite (vines whipping, leaves rustling, the
pod's jaws snapping shut with a wet crunch) and the death (a long wilting rustle, a creak of the stalk giving, a
last soft sigh of air from the pod). It is rooted and never lurks, so it has no wake.

    python3 tools/audio/plant_sounds.py [out_dir]      (default: media/sounds/monsters/, writes plant_{att,die}.wav)

Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from crocodile_sounds import REPO, SR, band, env, finish, lowpass, place, rms, t_axis, write  # noqa: E402

rng = np.random.default_rng(149)  # fixed seed: the same files on every run


def rustle(dur, rate=30, lo=1500, hi=7000):
    """Leaves brushing: band noise in fast irregular crackles."""
    t = t_axis(dur)
    crackle = np.clip(lowpass(rng.standard_normal(len(t)), rate) * 5, 0, None) + 0.15
    return band(rng.standard_normal(len(t)), lo, hi) * crackle


def whip(dur=0.18):
    """A vine lashing: a falling band of air, a soft slap at the end."""
    t = t_axis(dur)
    k = t / dur
    noise = rng.standard_normal(len(t))
    air = band(noise, 600, 2400) * (1 - k) + band(noise, 1800, 5000) * k
    return air * np.sin(np.pi * k) ** 2


def snap(dur=0.2):
    """The pod's jaws shut: a fibrous crack, a wet squelch, a low knock of the pod."""
    t = t_axis(dur)
    crack = band(rng.standard_normal(len(t)), 1200, 6000) * env(t, 0.0005, 0.01)
    squelch = band(rng.standard_normal(len(t)), 300, 1600) * env(t, 0.004, 0.04)
    knock = np.sin(2 * np.pi * (180 * np.exp(-t / 0.05) + 90) * t) * env(t, 0.002, 0.04)
    return 0.8 * crack + 0.7 * squelch + knock


def creak(dur, f0=140, f1=90):
    """A green stalk bending: a slow stick-slip creak gliding down."""
    t = t_axis(dur)
    f = f0 * (f1 / f0) ** (t / dur)
    pulses = np.maximum(np.sin(2 * np.pi * np.cumsum(f) / SR), 0) ** 12
    return band(pulses + 0.2 * rng.standard_normal(len(t)), 200, 2500)


def attack():
    """Leaves stir, two vines lash in, the jaws snap shut, the leaves settle."""
    out = np.zeros(int(0.8 * SR))
    place(out, rms(rustle(0.3, 25) * env(t_axis(0.3), 0.05, 0.12), 0.03), 0.0)
    place(out, rms(whip(0.18), 0.08), 0.12)
    place(out, rms(whip(0.15), 0.06), 0.2)
    place(out, rms(snap(0.2), 0.16), 0.32)
    place(out, rms(rustle(0.35, 18) * env(t_axis(0.35), 0.02, 0.12), 0.025), 0.36)
    return finish(out)


def die():
    """The plant wilts: leaves rustling as they droop, the stalk creaking over, the pod sighing out its air, a last
    rustle."""
    out = np.zeros(int(1.6 * SR))
    place(out, rms(rustle(0.9, 22) * env(t_axis(0.9), 0.04, 0.4), 0.05), 0.0)
    place(out, rms(creak(0.7) * env(t_axis(0.7), 0.1, 0.35), 0.06), 0.15)
    s = t_axis(0.6)
    sigh = band(rng.standard_normal(len(s)), 300, 1400) * env(s, 0.12, 0.2)
    place(out, rms(sigh, 0.04), 0.6)
    place(out, rms(rustle(0.4, 10) * env(t_axis(0.4), 0.02, 0.12), 0.015), 1.1)
    return finish(out)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds", "monsters")
    write(os.path.join(out_dir, "plant_att.wav"), attack())
    write(os.path.join(out_dir, "plant_die.wav"), die())


if __name__ == "__main__":
    main()
