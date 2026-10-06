"""Synthesize the scorpion egg cluster sounds: the heave while a scorpion hatches (wet squelches of the leathery eggs,
a dry crack of a shell splitting) and the death (eggs bursting one after another, a wet splatter, the husks settling).

    python3 tools/audio/egg_cluster_sounds.py [out_dir]   (default: sounds/monsters/, writes egg_cluster_{att,die}.wav)

Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys
import wave

import numpy as np
from scipy.signal import butter, sosfilt

SR = 22050
REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
rng = np.random.default_rng(91)  # fixed seed: the same files on every run


def t_axis(duration):
    return np.arange(int(duration * SR)) / SR


def band(x, lo, hi, order=2):
    return sosfilt(butter(order, [lo, hi], btype="band", fs=SR, output="sos"), x)


def lowpass(x, hi, order=2):
    return sosfilt(butter(order, hi, fs=SR, output="sos"), x)


def rms(x, level):
    return x * level / (np.sqrt(np.mean(x ** 2)) + 1e-12)


def place(out, clip, at):
    i = int(at * SR)
    n = min(len(clip), len(out) - i)
    out[i:i + n] += clip[:n]


def env(t, attack, decay):
    return np.minimum(t / attack, 1.0) * np.exp(-np.maximum(t - attack, 0) / decay)


def squelch(dur=0.25, f0=180, f1=520):
    """Wet squelch: a low noise band sweeping up (a membrane stretching), with a gurgle of amplitude wobble."""
    t = t_axis(dur)
    k = t / dur
    noise = rng.standard_normal(len(t))
    out = np.zeros(len(t))
    steps = 8
    for s in range(steps):  # a band that moves up through the sound
        a, b = int(len(t) * s / steps), int(len(t) * (s + 1) / steps)
        f = f0 + (f1 - f0) * (s + 0.5) / steps
        out[a:b] = band(noise, f * 0.6, f * 1.6)[a:b]
    wobble = 0.6 + 0.4 * np.sin(2 * np.pi * (14 + 10 * k) * t)
    tone = np.sin(2 * np.pi * np.cumsum(f0 * 0.7 + (f1 - f0) * 0.4 * k) / SR) * 0.4
    return (out + tone) * wobble * np.sin(np.pi * k) ** 0.7


def crack(dur=0.07, bright=1.0):
    """A dry shell splitting: a click and a short crackle."""
    t = t_axis(dur)
    click = band(rng.standard_normal(len(t)), 1500, 7000) * env(t, 0.0003, 0.004 * bright)
    crackle = band(rng.standard_normal(len(t)) * (rng.random(len(t)) > 0.92), 900, 5000) * env(t, 0.001, 0.02)
    return click + 0.6 * crackle


def burst(dur=0.35):
    """An egg bursting: a soft pop (low thump), then wet spray noise falling off."""
    t = t_axis(dur)
    pop = np.sin(2 * np.pi * (140 * np.exp(-t / 0.05) + 60) * t) * env(t, 0.001, 0.03)
    spray = band(rng.standard_normal(len(t)), 600, 4500) * env(t, 0.004, 0.08)
    drops = band(rng.standard_normal(len(t)) * (rng.random(len(t)) > 0.985), 1200, 6000) * env(t, 0.02, 0.15)
    return pop + 0.5 * spray + 0.4 * drops


def finish(out):
    out = lowpass(out, 9000)
    fade = int(0.03 * SR)
    out[-fade:] *= np.linspace(1, 0, fade)
    return out / np.max(np.abs(out)) * 10 ** (-3 / 20)


def attack():
    """The heave: two squelches of the stretching eggs, a crack of a shell giving, a last wet slide."""
    out = np.zeros(int(0.9 * SR))
    place(out, rms(squelch(0.32, 150, 420), 0.10), 0.0)
    place(out, rms(squelch(0.28, 200, 600), 0.08), 0.25)
    place(out, rms(crack(0.08), 0.07), 0.45)
    place(out, rms(squelch(0.3, 260, 180), 0.05), 0.55)
    return finish(out)


def die():
    """Eggs burst one after another, a splatter, the husks settle with a last few drips."""
    out = np.zeros(int(1.6 * SR))
    for k, (at, level) in enumerate(((0.0, 0.12), (0.12, 0.09), (0.21, 0.10), (0.36, 0.08), (0.5, 0.06), (0.68, 0.04))):
        place(out, rms(burst(0.3 + 0.05 * (k % 2)), level), at)
        place(out, rms(crack(0.05, 0.8), level * 0.5), at + 0.01)
    t = t_axis(0.8)
    splatter = band(rng.standard_normal(len(t)), 400, 3000) * env(t, 0.02, 0.25)
    place(out, rms(splatter, 0.05), 0.15)
    for at in (1.05, 1.22, 1.4):
        place(out, rms(crack(0.03, 0.5), 0.015), at)
    return finish(out)


def write(path, x):
    pcm = (x * 32767).astype("<i2")
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())
    print("wrote {} ({:.2f} s)".format(path, len(pcm) / SR))


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "sounds", "monsters")
    write(os.path.join(out_dir, "egg_cluster_att.wav"), attack())
    write(os.path.join(out_dir, "egg_cluster_die.wav"), die())


if __name__ == "__main__":
    main()
