"""Synthesize Babi's sounds (plan babi-boss): the baboon's, from a body twice its size, an octave lower and rougher.
The wake (a deep wa-hoo bark, three times, the last one a roar), the attack (a roar as he rears with both hands high,
the two-handed slam on the stone at the clip's frame 10 of 20, the jaws snapping shut on the fangs at frame 16), the
ranged throw (a low screech rising while he holds the coal high, the whoosh at the release), the leap (a grunt of
effort, the thud of the landing) and the death (a long roar sagging into grunts). The voice, the shriek, the whoosh
and the snap come from baboon_sounds.py.

    python3 tools/audio/babi_sounds.py [out_dir]  (default: media/sounds/monsters/, writes babi_{wake,att,att_ranged,jump,die}.wav)

Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import baboon_sounds as bs  # noqa: E402
from baboon_sounds import shriek, snap, voice, whoosh  # noqa: E402
from scorpion_sounds import REPO, SR, band, env, finish, place, rms, t_axis, write  # noqa: E402

rng = np.random.default_rng(97)  # fixed seed: the same files on every run
bs.rng = np.random.default_rng(98)

FPS = 14  # the clips' frames a second (ModelInfo::CLIP_SPEED)
RELEASE_S = 8 / FPS  # the coal leaves the hand at frame 8 of the throw (baboon.py RELEASE_FRAME)
SLAM_S = 10 / FPS  # both hands on the floor (baboon.py ATTACK_BOSS)
BITE_S = 16 / FPS  # the jaws shut
TARGET_RMS = {"wake": 0.12, "att": 0.11, "att_ranged": 0.12, "jump": 0.08, "die": 0.11}


def level(x, name):
    """Peak-normalized by finish, then down to the target rms (never louder than the peak normalization)."""
    x = finish(x)
    return x * min(1.0, TARGET_RMS[name] / np.sqrt(np.mean(x ** 2)))


def roar(dur, f0, f1):
    """A deep, rough primate roar: a low voice gliding f0 -> f1, overdriven, with chest resonance and a rasp."""
    t = t_axis(dur)
    v = voice(dur, f0, f1, [(380, 220, 1.8), (800, 350, 1.1), (1700, 700, 0.5)], 0.7, 0.2)
    v = np.tanh(2.2 * v / np.max(np.abs(v)))
    rasp = band(rng.standard_normal(len(t)), 900, 4000) * (1 + 0.6 * np.sin(2 * np.pi * 23 * t))
    return (v + 0.15 * rasp / np.max(np.abs(rasp))) * np.minimum(t / 0.04, 1.0) * np.clip((dur - t) / (0.3 * dur), 0, 1)


def bark(lvl=1.0):
    """The baboon's wa-hoo an octave down."""
    d1, d2 = 0.16, 0.3
    wa = voice(d1, 210, 260, [(600, 300, 1.6), (1100, 500, 0.9), (2200, 800, 0.3)], 0.5, 0.15) * env(t_axis(d1), 0.008, 0.09)
    t = t_axis(d2)
    hoo = voice(d2, 240, 150, [(320, 180, 1.7), (650, 260, 0.7), (1800, 700, 0.12)], 0.35, 0.08)
    hoo *= np.minimum(t / 0.025, 1.0) * np.clip((d2 - t) / (0.6 * d2), 0, 1)
    out = np.zeros(int((d1 + d2 + 0.02) * SR))
    place(out, rms(wa, 0.25 * lvl), 0.0)
    place(out, rms(hoo, 0.2 * lvl), d1 - 0.02)
    return out


def thud(dur=0.25, f=70):
    """A heavy body or both hands on the stone: a low knock and a short gritty rumble."""
    t = t_axis(dur)
    knock = (np.sin(2 * np.pi * f * t) + 0.6 * np.sin(2 * np.pi * 2.3 * f * t)) * env(t, 0.002, 0.06)
    grit = band(rng.standard_normal(len(t)), 150, 1200) * env(t, 0.002, 0.04)
    return knock + 0.5 * grit / np.max(np.abs(grit))


def wake():
    """Two deep barks, then a roar."""
    out = np.zeros(int(1.9 * SR))
    place(out, bark(1.0), 0.0)
    place(out, bark(0.9), 0.55)
    place(out, rms(roar(0.75, 170, 120), 0.2), 1.1)
    return level(out, "wake")


def attack():
    """A roar as he rears, both hands slam the floor, a snarl into the lunge, the snap of the fangs."""
    out = np.zeros(int(1.45 * SR))
    place(out, rms(roar(0.55, 150, 190), 0.17), 0.08)
    place(out, rms(thud(0.3, 62), 0.3), SLAM_S)
    place(out, rms(roar(0.3, 120, 140), 0.08), SLAM_S + 0.12)
    place(out, rms(snap(), 0.24), BITE_S)
    return level(out, "att")


def att_ranged():
    """A low screech rising while the coal is held high, breaking off into the whoosh of the throw at the release."""
    out = np.zeros(int(1.0 * SR))
    start = 0.05
    d = RELEASE_S - 0.06 - start
    t = t_axis(d)
    scream = shriek(d, 380, 720, 1.2) * np.minimum(t / 0.05, 1.0) * (0.5 + 0.5 * t / d) * np.clip((d - t) / 0.04, 0, 1)
    place(out, rms(scream, 0.15), start)
    place(out, rms(roar(d, 160, 220), 0.08), start)
    wd = 0.42
    place(out, rms(whoosh(wd), 0.15), RELEASE_S - 0.45 * wd)
    place(out, rms(roar(0.16, 200, 140), 0.06), RELEASE_S + 0.02)
    return level(out, "att_ranged")


def jump():
    """A grunt of effort at the spring, the thud of the landing (MONSTER_JUMP_MS 650, touchdown at 0.83 of it)."""
    out = np.zeros(int(0.8 * SR))
    tt = t_axis(0.16)
    g = voice(0.16, 190, 140, [(500, 260, 1.4), (1000, 380, 0.6)], 0.6, 0.25) * env(tt, 0.01, 0.07)
    place(out, rms(g, 0.14), 0.1)
    place(out, rms(thud(0.22, 75), 0.22), 0.54)
    return level(out, "jump")


def die():
    """A long roar sagging in pitch and breaking up, then three weakening grunts."""
    out = np.zeros(int(2.1 * SR))
    place(out, rms(roar(0.85, 200, 95), 0.18), 0.0)
    place(out, rms(shriek(0.2, 330, 210), 0.07), 0.75)
    for at, f0, lvl, dur in ((1.05, 120, 0.14, 0.18), (1.35, 105, 0.10, 0.16), (1.66, 92, 0.06, 0.2)):
        tt = t_axis(dur)
        g = voice(dur, f0, f0 * 0.8, [(420, 240, 1.4), (850, 320, 0.6)], 0.6, 0.2) * env(tt, 0.01, dur * 0.4)
        place(out, rms(g, lvl), at)
    return level(out, "die")


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds", "monsters")
    write(os.path.join(out_dir, "babi_wake.wav"), wake())
    write(os.path.join(out_dir, "babi_att.wav"), attack())
    write(os.path.join(out_dir, "babi_att_ranged.wav"), att_ranged())
    write(os.path.join(out_dir, "babi_jump.wav"), jump())
    write(os.path.join(out_dir, "babi_die.wav"), die())


if __name__ == "__main__":
    main()
