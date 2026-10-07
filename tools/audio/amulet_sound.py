"""Synthesize the amulet sound (put on or taken off): a cord slipping over the head, then beads and a small metal
pendant clinking against each other.

    python3 tools/audio/amulet_sound.py [out.wav]      (default: sounds/items/amulet.wav)

Everything is generated (no samples), so this script is the source of the sound.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys
import wave

import numpy as np
from scipy.signal import butter, sosfilt

SR = 22050
LENGTH = 0.55
REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
rng = np.random.default_rng(11)  # fixed seed: the same file on every run


def t_axis(duration):
    return np.arange(int(duration * SR)) / SR


def band(x, lo, hi, order=2):
    return sosfilt(butter(order, [lo, hi], btype="band", fs=SR, output="sos"), x)


def place(out, clip, at):
    i = int(at * SR)
    n = min(len(clip), len(out) - i)
    out[i:i + n] += clip[:n]


def cord():
    """Soft rustle of a linen cord sliding over hair and cloth."""
    t = t_axis(0.22)
    env = np.sin(np.pi * t / t[-1]) ** 2
    return band(rng.standard_normal(len(t)), 1200, 6000) * env * 0.18


def clink(freq, decay, level):
    """A small struck metal or faience piece: a few inharmonic partials with a fast decay."""
    t = t_axis(decay * 6)
    partials = [(1.0, 1.0), (2.76, 0.5), (5.4, 0.25), (8.9, 0.12)]
    tone = sum(g * np.sin(2 * np.pi * freq * r * t + rng.random() * 6.28) * np.exp(-t * r ** 0.5 / decay)
               for r, g in partials)
    tick = band(rng.standard_normal(len(t)), 3000, 9000) * np.exp(-t / 0.002) * 0.3
    return (tone + tick) * level


def main(path):
    out = np.zeros(int(LENGTH * SR))
    place(out, cord(), 0.0)
    # Beads settle: quick knocks, then the pendant rings a little longer.
    for at, freq, decay, level in [(0.17, 2300, 0.018, 0.25), (0.21, 2900, 0.015, 0.2), (0.24, 1900, 0.02, 0.22),
                                   (0.29, 1450, 0.06, 0.42), (0.36, 2600, 0.015, 0.12)]:
        place(out, clink(freq, decay, level), at)
    out *= 0.8 / np.max(np.abs(out))
    fade = int(0.02 * SR)
    out[-fade:] *= np.linspace(1, 0, fade)
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes((out * 32767).astype(np.int16).tobytes())
    print("wrote", path)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "sounds", "items", "amulet.wav"))
