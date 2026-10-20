"""Synthesize the sand sounds (plan desert-chapter): a sand fall's hiss (grains trickling from a ceiling crack, faster
and faster), its pour (a dry rushing whoosh that settles into a soft heap, grit ticking on), and a step in sand (a soft
dry crunch with a slide).

    python3 tools/audio/sand_sounds.py [out_dir]      (default: media/sounds/, writes mechanisms/sand_hiss.wav,
                                                       mechanisms/sand_pour.wav, mechanisms/sand_step.wav)

Everything is generated (no samples), so this script is the source of the sounds; the helpers come from
mechanism_sounds.py.
Output: 16-bit PCM mono 22050 Hz, like jump_sound.py.
"""

import os
import sys
import wave

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mechanism_sounds import REPO, SR, band, env_ad, lowpass, normalize, place, t_axis  # noqa: E402

rng = np.random.default_rng(29)  # fixed seed: the same files on every run


def ticks(dur, rate0, rate1, lo=2500, hi=8000):
    """Grains on stone: short bright clicks, from rate0 to rate1 a second."""
    out = np.zeros(int(dur * SR))
    tt = 0.0
    while tt < dur:
        g = band(rng.standard_normal(int(0.005 * SR)), lo, hi) * env_ad(t_axis(0.005), 0.0003, 0.0012)
        place(out, g, tt, rng.uniform(0.2, 1.0))
        rate = rate0 + (rate1 - rate0) * tt / dur
        tt += rng.exponential(1.0 / rate)
    return out


def sand_hiss():
    """The crack gives: a thin trickle of grains that thickens into a hiss."""
    dur = 0.9
    t = t_axis(dur)
    hiss = band(rng.standard_normal(len(t)), 1800, 6500) * (t / dur) ** 1.5
    env = np.clip(t / 0.05, 0, 1) * np.clip((dur - t) / 0.08, 0, 1)
    return normalize((hiss * 0.6 + ticks(dur, 20, 400)) * env, -6)


def sand_pour():
    """The sand comes down: a dry rush, a soft thump into the heap, grit settling."""
    dur = 1.2
    t = t_axis(dur)
    rush = band(rng.standard_normal(len(t)), 300, 4000) * env_ad(t, 0.04, 0.35)
    thump = lowpass(rng.standard_normal(len(t)), 140, 4) * env_ad(t, 0.004, 0.09) * 4
    settle = ticks(dur, 300, 10) * np.clip(t / 0.1, 0, 1)
    env = np.clip((dur - t) / 0.2, 0, 1)
    return normalize((rush + thump + 0.5 * settle) * env, -3)


def sand_step():
    """A foot in a drift: a soft dry crunch, a short slide."""
    dur = 0.28
    t = t_axis(dur)
    crunch = band(rng.standard_normal(len(t)), 600, 5000) * env_ad(t, 0.008, 0.05)
    slide = band(rng.standard_normal(len(t)), 1500, 6000) * env_ad(t, 0.03, 0.08) * 0.4
    return normalize(crunch + slide + 0.4 * ticks(dur, 200, 20), -10)


SOUNDS = {
    "mechanisms/sand_hiss": sand_hiss,
    "mechanisms/sand_pour": sand_pour,
    "mechanisms/sand_step": sand_step,
}


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds")
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
