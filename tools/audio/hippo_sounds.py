"""Synthesize the hippo sounds: waking (water streaming off its head, a splash, the wheeze and the deep honking grunts
of a hippo's call), the bite (a gaping roar, then the jaws slamming shut with a crushing, bone-deep crunch and a thrash
of water) and the death (a long groaning bellow sagging in pitch, a last wheezing breath and the splash of the body
rolling over). The water sounds come from crocodile_sounds.py.

    python3 tools/audio/hippo_sounds.py [out_dir]      (default: media/sounds/monsters/, writes hippo_{wake,att,die}.wav)

Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from crocodile_sounds import REPO, SR, band, drops, env, finish, lowpass, place, rms, splash, surge, t_axis, write  # noqa: E402

rng = np.random.default_rng(73)  # fixed seed: the same files on every run


def voice(dur, f0, f1, formants, rough=0.3, shape=None):
    """A huge chest voice: a buzzy pulse train gliding f0 -> f1 with jitter and a rough tremolo, coloured by
    resonances (centre Hz, bandwidth Hz, gain) of the throat and the open mouth."""
    t = t_axis(dur)
    jitter = lowpass(rng.standard_normal(len(t)), 25) * 6
    f = f0 * (f1 / f0) ** (t / dur) * (1 + 0.025 * jitter)
    ph = 2 * np.pi * np.cumsum(f) / SR
    src = sum(np.sin(k * ph + 0.3 * k) / k ** 0.6 for k in range(1, 30))
    src *= 1 + rough * np.sin(2 * np.pi * 11 * t + 3 * lowpass(rng.standard_normal(len(t)), 5))
    out = band(src, 30, 220) * 1.4
    for fc, bw, g in formants:
        out = out + g * band(src, max(fc - bw / 2, 40), fc + bw / 2)
    if shape is not None:
        out = out * shape
    return out


def wheeze(dur, lo=500, hi=2600):
    """Air forced through the nostrils: band noise with a swell and a fade."""
    t = t_axis(dur)
    sh = np.minimum(t / (0.3 * dur), 1.0) * np.clip((dur - t) / (0.5 * dur), 0, 1)
    return band(rng.standard_normal(len(t)), lo, hi) * sh * (1 + 0.4 * np.sin(2 * np.pi * 23 * t))


def grunt(dur, f0, f1):
    """One honking grunt: a short nasal bellow with a hard onset."""
    t = t_axis(dur)
    sh = env(t, 0.015, dur * 0.35) * np.clip((dur - t) / (0.25 * dur), 0, 1)
    return voice(dur, f0, f1, [(320, 160, 1.6), (720, 300, 0.8), (1500, 600, 0.25)], 0.45, sh)


def crunch(dur=0.35):
    """The jaws slamming: a deep thud of the whole head, a bony crack and a grinding crunch."""
    t = t_axis(dur)
    thud = (np.sin(2 * np.pi * 58 * t) + 0.6 * np.sin(2 * np.pi * 120 * t) + 0.3 * np.sin(2 * np.pi * 190 * t)) * env(t, 0.002, 0.07)
    crack = band(rng.standard_normal(len(t)), 900, 5500) * env(t, 0.0005, 0.018)
    grains = np.zeros(len(t))
    for _ in range(14):  # gritty crunch after the slam
        at = rng.uniform(0.01, 0.12)
        g = band(rng.standard_normal(int(0.012 * SR)), 1500, 6000) * env(t_axis(0.012), 0.0005, 0.003)
        place(grains, g * rng.uniform(0.3, 1.0), at)
    return thud * 1.2 + 0.8 * crack + 0.5 * grains


def wake():
    """It heaves its head out of the water: a surge and splash, a wheeze, then the honking call, four grunts."""
    out = np.zeros(int(2.1 * SR))
    place(out, rms(surge(0.6), 0.07), 0.0)
    place(out, rms(splash(0.6, 1.0), 0.14), 0.22)
    place(out, rms(drops(0.5, 14), 0.02), 0.4)
    place(out, rms(wheeze(0.45), 0.05), 0.45)
    for i, (at, f0, lvl) in enumerate(((0.80, 92, 0.24), (1.08, 86, 0.22), (1.34, 80, 0.19), (1.58, 72, 0.15))):
        place(out, rms(grunt(0.26 + 0.03 * i, f0, f0 * 0.82), lvl), at)
    return finish(out)


def attack():
    """The threat: a gaping roar, the jaws slam shut with a crushing crunch, water thrashed by the head shake."""
    out = np.zeros(int(1.3 * SR))
    d = 0.6
    t = t_axis(d)
    roar = voice(d, 75, 62, [(420, 260, 1.4), (900, 500, 1.0), (2200, 900, 0.3)], 0.6, env(t, 0.06, 0.35))
    place(out, rms(roar, 0.2), 0.0)
    place(out, rms(wheeze(0.5, 300, 3000), 0.04), 0.05)
    place(out, rms(crunch(), 0.32), 0.58)
    place(out, rms(splash(0.45, 0.7), 0.08), 0.66)
    place(out, rms(splash(0.4, 0.55), 0.06), 0.84)
    return finish(out)


def die():
    """A long groaning bellow that sags and breaks, a last wheeze, the body rolls over into the water."""
    out = np.zeros(int(2.4 * SR))
    d = 1.4
    t = t_axis(d)
    groan = voice(d, 78, 36, [(300, 180, 1.4), (650, 300, 0.7)], 0.8, env(t, 0.08, 0.6))
    place(out, rms(groan, 0.22), 0.0)
    place(out, rms(grunt(0.22, 60, 48), 0.08), 1.25)
    place(out, rms(wheeze(0.6, 300, 1800), 0.035), 1.3)
    place(out, rms(splash(0.9, 1.6), 0.17), 1.5)
    place(out, rms(drops(0.6, 14), 0.02), 1.75)
    return finish(out)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds", "monsters")
    write(os.path.join(out_dir, "hippo_wake.wav"), wake())
    write(os.path.join(out_dir, "hippo_att.wav"), attack())
    write(os.path.join(out_dir, "hippo_die.wav"), die())


if __name__ == "__main__":
    main()
