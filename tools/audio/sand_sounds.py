"""Synthesize the sand sounds (plan desert-chapter): a sand fall's hiss (grains trickling from a ceiling crack, faster
and faster), its pour (a dry rushing whoosh that settles into a soft heap, grit ticking on), and a step in sand (a soft
dry crunch with a slide). The sandstorm halls' (plan sandstorm-halls): the wind rising before a gust (a hiss that
swells, grit starting to skitter) and the gust itself (a howling rush of wind and sand, grains rattling on stone).

    python3 tools/audio/sand_sounds.py [out_dir]      (default: media/sounds/, writes mechanisms/sand_hiss.wav,
                                                       mechanisms/sand_pour.wav, mechanisms/sand_step.wav,
                                                       mechanisms/gust_rise.wav, mechanisms/gust_blow.wav)

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


def gust_rise():
    """The tell (GUST_TELEGRAPH_MS, 1.2 s): a low hiss swells, grit starts to skitter, faster towards the gust."""
    dur = 1.2
    t = t_axis(dur)
    k = t / dur
    hiss = band(rng.standard_normal(len(t)), 900, 5000) * k**2
    moan = band(rng.standard_normal(len(t)), 250, 700) * k**1.5 * 0.8
    env = np.clip(t / 0.1, 0, 1)
    return normalize((hiss + moan + 0.5 * ticks(dur, 10, 250)) * env, -8)


def wind(dur, lo, hi, gusts):
    """Noise through a band whose middle wanders: the howl of the wind, gusts the bumps of its strength."""
    t = t_axis(dur)
    out = np.zeros(len(t))
    steps = 8
    for k in range(steps):  # the band moves in steps, cross-faded: a slow howl
        f = lo + (hi - lo) * (0.5 + 0.5 * np.sin(2 * np.pi * (k / steps) * 1.3 + rng.uniform(0, 1)))
        part = band(rng.standard_normal(len(t)), f * 0.7, f * 1.4)
        w = np.clip(1 - np.abs(t / dur * steps - k - 0.5), 0, 1)
        out += part * w
    strength = 1.0 + sum(0.3 * np.sin(2 * np.pi * rng.uniform(0.6, 2.0) * t + rng.uniform(0, 6)) for _ in range(gusts))
    return out * strength


def gust_blow():
    """The gust (GUST_OUT_MS, 2 s; 2.4 slow): a rush of wind that hits, howls and dies down, sand rattling on stone."""
    dur = 2.4
    t = t_axis(dur)
    rush = band(rng.standard_normal(len(t)), 400, 6000) * env_ad(t, 0.08, 1.0)
    howl = wind(dur, 350, 900, 3) * 0.7
    grit = ticks(dur, 900, 120, 2000, 7000)
    env = np.clip(t / 0.06, 0, 1) * np.clip((dur - t) / 0.6, 0, 1)
    return normalize((rush + howl + 0.4 * grit) * env, -4)


SOUNDS = {  # in this order: the generator's draws, so the older files stay the same
    "mechanisms/sand_hiss": sand_hiss,
    "mechanisms/sand_pour": sand_pour,
    "mechanisms/sand_step": sand_step,
    "mechanisms/gust_rise": gust_rise,
    "mechanisms/gust_blow": gust_blow,
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
