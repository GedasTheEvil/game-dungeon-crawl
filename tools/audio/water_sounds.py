"""Synthesize the sounds of half water (crocodiles-and-flooded-cells).

    python3 tools/audio/water_sounds.py [out_dir]      (default: sounds/)

Writes water/wade.wav (one splashing step: a slosh and a few droplets; played every WADE_SPLASH_MS while the
player wades) and water/splash.wav (the player lands in water from a fall: a heavy plunge, spray and droplets).
Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, like jump_sound.py.
"""

import os
import sys
import wave

import numpy as np
from scipy.signal import butter, sosfilt

SR = 22050
REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
rng = np.random.default_rng(23)  # fixed seed: the same files on every run


def t_axis(duration):
    return np.arange(int(duration * SR)) / SR


def band(x, lo, hi, order=2):
    return sosfilt(butter(order, [lo, hi], btype="band", fs=SR, output="sos"), x)


def place(out, clip, at, gain=1.0):
    i = int(at * SR)
    n = min(len(clip), len(out) - i)
    out[i:i + n] += gain * clip[:n]


def env_ad(t, attack, decay):
    return (1 - np.exp(-t / attack)) * np.exp(-t / decay)


def normalize(x, db=-3.0, fade=0.02):
    n = int(fade * SR)
    x = x.copy()
    x[-n:] *= np.linspace(1, 0, n)
    return x / np.max(np.abs(x)) * 10 ** (db / 20)


def droplet(f0, dur=0.05):
    """A drop hitting water: a short sine that glides up (the bubble resonance)."""
    t = t_axis(dur)
    f = f0 * (1 + 1.8 * t / dur)
    return np.sin(2 * np.pi * np.cumsum(f) / SR) * env_ad(t, 0.002, dur / 4)


def slosh(dur, lo, hi, attack, decay):
    t = t_axis(dur)
    noise = band(rng.standard_normal(len(t)), lo, hi)
    # A slow wobble makes the noise sound like moving water, not wind.
    wobble = 1 + 0.5 * np.sin(2 * np.pi * (9 + 6 * rng.random()) * t)
    return noise * env_ad(t, attack, decay) * wobble


def wade():
    out = np.zeros(int(0.4 * SR))
    place(out, slosh(0.3, 250, 2200, 0.02, 0.08), 0.0, 1.0)
    place(out, slosh(0.2, 1200, 5000, 0.005, 0.04), 0.03, 0.4)
    for _ in range(4):
        place(out, droplet(700 + 900 * rng.random()), 0.08 + 0.2 * rng.random(), 0.25)
    return normalize(out, -8)


def splash():
    out = np.zeros(int(0.9 * SR))
    t = t_axis(0.25)
    place(out, np.sin(2 * np.pi * 70 * t) * env_ad(t, 0.004, 0.06), 0.0, 0.8)  # the body hits the water
    place(out, slosh(0.6, 150, 1800, 0.01, 0.18), 0.0, 1.0)
    place(out, slosh(0.5, 1500, 7000, 0.004, 0.12), 0.02, 0.6)  # spray
    for _ in range(9):
        place(out, droplet(600 + 1200 * rng.random(), 0.06), 0.15 + 0.6 * rng.random(), 0.3)
    return normalize(out, -4)


SOUNDS = {"water/wade": wade, "water/splash": splash}


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "sounds")
    for name, fn in SOUNDS.items():
        pcm = (fn() * 32767).astype("<i2")
        path = os.path.join(out_dir, name + ".wav")
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with wave.open(path, "wb") as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(SR)
            w.writeframes(pcm.tobytes())
        print("wrote {} ({:.2f} s)".format(path, len(pcm) / SR))


if __name__ == "__main__":
    main()
