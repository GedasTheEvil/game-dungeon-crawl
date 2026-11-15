"""Synthesize the landing sounds of a fall (plan soft-landing).

    python3 tools/audio/landing_sounds.py [out_dir]      (default: media/sounds/)

Writes characters/archeologist_land_soft.wav (a muffled, low thud into something soft, a short rustle of its matter
after it: a landing heap, sand) and characters/archeologist_land_hard.wav (a plain thud of boots on stone, a little
grit). Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, like weapon_sounds.py.
"""

import os
import sys
import wave

import numpy as np
from scipy.signal import butter, sosfilt

SR = 22050
REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
rng = np.random.default_rng(57)  # fixed seed: the same files on every run


def t_axis(duration):
    return np.arange(int(duration * SR)) / SR


def band(x, lo, hi, order=2):
    return sosfilt(butter(order, [lo, hi], btype="band", fs=SR, output="sos"), x)


def lowpass(x, hi, order=2):
    return sosfilt(butter(order, hi, fs=SR, output="sos"), x)


def normalize(x, db=-3.0, fade=0.02):
    x = x / (np.max(np.abs(x)) + 1e-12) * 10 ** (db / 20)
    n = int(fade * SR)
    x[-n:] *= np.linspace(1, 0, n)
    return x


def thump(t, freq, decay):
    """A body hitting the floor: a sine dropping in pitch under a fast decay."""
    f = freq * (1 + 0.6 * np.exp(-t / 0.02))
    return np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / decay)


def soft():
    t = t_axis(0.55)
    body = thump(t, 55, 0.09)
    muffle = lowpass(rng.standard_normal(len(t)), 400) * np.exp(-t / 0.05) * 0.8
    rustle_env = np.clip((t - 0.03) / 0.04, 0, 1) * np.exp(-np.maximum(t - 0.07, 0) / 0.12)
    rustle = band(rng.standard_normal(len(t)), 1200, 5000) * rustle_env * 0.18
    return normalize(body + muffle + rustle, db=-5.0)


def hard():
    t = t_axis(0.4)
    body = thump(t, 80, 0.06)
    knock = band(rng.standard_normal(len(t)), 300, 1500) * np.exp(-t / 0.015) * 0.9
    grit = band(rng.standard_normal(len(t)), 2500, 7000) * np.exp(-t / 0.05) * 0.12
    return normalize(body + knock + grit, db=-4.0)


def write(path, x):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes((np.clip(x, -1, 1) * 32767).astype("<i2").tobytes())
    print("wrote", path)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds")
    write(os.path.join(out, "characters", "archeologist_land_soft.wav"), soft())
    write(os.path.join(out, "characters", "archeologist_land_hard.wav"), hard())


if __name__ == "__main__":
    main()
