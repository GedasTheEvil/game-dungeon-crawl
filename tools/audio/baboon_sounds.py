"""Synthesize the baboon sounds (plan baboon): waking (the hamadryas's sharp two-phase bark, "wa-hoo", twice), the bite
(a rough snarl, then the jaws snapping shut on the fangs), the ranged throw (a loud rising shriek while it holds the coal
over its head, then the whoosh of the overarm throw at the clip's release) and the death (a scream that breaks down
into grunts). The snap comes from scorpion_sounds.py.

    python3 tools/audio/baboon_sounds.py [out_dir]  (default: media/sounds/monsters/, writes baboon_{wake,att,att_ranged,die}.wav)

Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from scorpion_sounds import REPO, SR, band, clack, env, finish, lowpass, place, rms, t_axis, write  # noqa: E402

rng = np.random.default_rng(83)  # fixed seed: the same files on every run

# The ranged sound starts with the clip (Monster::AttackRanged); the coal leaves the hand at frame 8 of 14 (baboon.py
# RELEASE_FRAME), at 14 frames a second (ModelInfo::CLIP_SPEED): the throw's whoosh peaks there.
RELEASE_S = 8 / 14
# Loudness after the peak normalization: scaled to about the other monsters' rms (hippo 0.06-0.08, anubis 0.1-0.2).
TARGET_RMS = {"wake": 0.10, "att": 0.09, "att_ranged": 0.11, "die": 0.09}


def voice(dur, f0, f1, formants, rough=0.2, breath=0.1, curve=1.0):
    """A primate voice: a buzzy pulse train gliding f0 -> f1 (curve > 1 holds and moves late) with jitter and a rough
    tremolo, coloured by resonances (centre Hz, bandwidth Hz, gain) of the throat and the open mouth, a little breath."""
    t = t_axis(dur)
    jitter = lowpass(rng.standard_normal(len(t)), 30) * 8
    f = (f0 + (f1 - f0) * (t / dur) ** curve) * (1 + 0.03 * jitter)
    ph = 2 * np.pi * np.cumsum(f) / SR
    src = sum(np.sin(k * ph + 0.4 * k) / k ** 0.7 for k in range(1, 24))
    src *= 1 + rough * np.sin(2 * np.pi * 27 * t + 4 * lowpass(rng.standard_normal(len(t)), 8))
    src += breath * rng.standard_normal(len(t)) * np.max(np.abs(src))
    out = 0.15 * src
    for fc, bw, g in formants:
        out = out + g * band(src, max(fc - bw / 2, 60), min(fc + bw / 2, 10500))
    return out


def bark(level=1.0):
    """Wa-hoo: a sharp open "wa" with a hard onset, then a lower, rounder "hoo" sliding down."""
    d1, d2 = 0.13, 0.24
    wa = voice(d1, 420, 520, [(900, 400, 1.6), (1500, 600, 0.9), (2800, 900, 0.3)], 0.35, 0.15)
    wa *= env(t_axis(d1), 0.006, 0.07)
    hoo = voice(d2, 470, 300, [(420, 220, 1.6), (850, 300, 0.7), (2400, 800, 0.12)], 0.25, 0.08)
    t = t_axis(d2)
    hoo *= np.minimum(t / 0.02, 1.0) * np.clip((d2 - t) / (0.6 * d2), 0, 1)
    out = np.zeros(int((d1 + d2 + 0.02) * SR))
    place(out, rms(wa, 0.25 * level), 0.0)
    place(out, rms(hoo, 0.2 * level), d1 - 0.015)
    return out


def snarl(dur):
    """A rough throaty snarl: a low growl beating fast, hissing breath through bared teeth."""
    t = t_axis(dur)
    growl = voice(dur, 150, 170, [(600, 300, 1.4), (1300, 500, 0.8), (3000, 1200, 0.4)], 0.9, 0.25)
    growl = np.tanh(2.0 * growl / np.max(np.abs(growl)))
    hiss = band(rng.standard_normal(len(t)), 2500, 7500) * (1 + 0.6 * np.sin(2 * np.pi * 31 * t))
    shape = np.minimum(t / 0.05, 1.0) * np.clip((dur - t) / (0.2 * dur), 0, 1)
    return (growl + 0.12 * hiss / np.max(np.abs(hiss))) * shape


def snap():
    """The jaws shutting on the fangs: a bony clack with a low knock of the skull."""
    t = t_axis(0.12)
    knock = (np.sin(2 * np.pi * 180 * t) + 0.5 * np.sin(2 * np.pi * 410 * t)) * env(t, 0.001, 0.025)
    out = 0.6 * knock
    place(out, clack(0.06, 2300, 1.3), 0.0)
    return out


def shriek(dur, f0, f1, curve=1.0):
    """A loud harsh shriek: a high voice gliding f0 -> f1, overdriven, with a scraping noise in the high formants."""
    t = t_axis(dur)
    v = voice(dur, f0, f1, [(1600, 700, 1.5), (2900, 1100, 1.0), (4300, 1400, 0.5)], 0.45, 0.2, curve)
    v = np.tanh(2.5 * v / np.max(np.abs(v)))
    rasp = band(rng.standard_normal(len(t)), 2000, 6500) * (1 + 0.7 * np.sin(2 * np.pi * 45 * t))
    return v + 0.25 * rasp / np.max(np.abs(rasp))


def whoosh(dur):
    """The arm and the coal through the air: band noise swelling and sweeping up, then a fading hiss."""
    t = t_axis(dur)
    k = t / dur
    noise = rng.standard_normal(len(t))
    low = band(noise, 250, 900) * np.sin(np.pi * np.minimum(k / 0.6, 1.0)) ** 2 * (k < 0.6)
    high = band(noise, 900, 3200) * np.sin(np.pi * np.clip((k - 0.2) / 0.8, 0, 1)) ** 2
    crackle = np.zeros(len(t))
    for _ in range(10):  # the ember spitting as it flies
        at = rng.uniform(0.35, 0.9) * dur
        place(crackle, band(rng.standard_normal(int(0.006 * SR)), 2500, 8000) * rng.uniform(0.4, 1.0), at)
    return low + high + 0.4 * crackle


def level(x, name):
    """Peak-normalized by finish, then down to the target rms (never louder than the peak normalization)."""
    x = finish(x)
    return x * min(1.0, TARGET_RMS[name] / np.sqrt(np.mean(x ** 2)))


def wake():
    """Two sharp wa-hoo barks."""
    out = np.zeros(int(1.05 * SR))
    place(out, bark(1.0), 0.0)
    place(out, bark(0.85), 0.5)
    return level(out, "wake")


def attack():
    """A snarl rising as it lunges, the jaws snap shut on the fangs (about the bite clip's frame 11 of 18)."""
    out = np.zeros(int(1.0 * SR))
    place(out, rms(snarl(0.62), 0.12), 0.0)
    place(out, rms(shriek(0.16, 700, 900), 0.06), 0.42)  # a yelp into the lunge
    place(out, rms(snap(), 0.22), 0.74)
    place(out, rms(snarl(0.2), 0.04), 0.8)
    return level(out, "att")


def att_ranged():
    """It rears and screeches at once, rising and louder, the shriek breaks off into the throw's whoosh at the release."""
    out = np.zeros(int(1.0 * SR))
    start = 0.05
    d = RELEASE_S - 0.06 - start
    t = t_axis(d)
    scream = shriek(d, 700, 1350, 1.2) * np.minimum(t / 0.05, 1.0) * (0.5 + 0.5 * t / d) * np.clip((d - t) / 0.04, 0, 1)
    place(out, rms(scream, 0.16), start)
    wd = 0.36
    place(out, rms(whoosh(wd), 0.14), RELEASE_S - 0.45 * wd)  # the swell (low, then high) is loudest at the release
    place(out, rms(shriek(0.12, 900, 600), 0.05), RELEASE_S + 0.02)  # the effort grunt-yelp of the throw
    return level(out, "att_ranged")


def die():
    """A scream sagging in pitch, breaking up, then three weakening grunts."""
    out = np.zeros(int(1.7 * SR))
    d = 0.6
    t = t_axis(d)
    scream = shriek(d, 1200, 520, 0.8) * env(t, 0.02, 0.35)
    place(out, rms(scream, 0.16), 0.0)
    place(out, rms(shriek(0.18, 600, 380), 0.08), 0.55)  # the break
    for at, f0, lvl, dur in ((0.85, 210, 0.14, 0.14), (1.1, 185, 0.10, 0.13), (1.36, 160, 0.06, 0.16)):
        tt = t_axis(dur)
        g = voice(dur, f0, f0 * 0.8, [(550, 300, 1.4), (1100, 400, 0.6)], 0.5, 0.2) * env(tt, 0.01, dur * 0.4)
        place(out, rms(g, lvl), at)
    return level(out, "die")


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds", "monsters")
    write(os.path.join(out_dir, "baboon_wake.wav"), wake())
    write(os.path.join(out_dir, "baboon_att.wav"), attack())
    write(os.path.join(out_dir, "baboon_att_ranged.wav"), att_ranged())
    write(os.path.join(out_dir, "baboon_die.wav"), die())


if __name__ == "__main__":
    main()
