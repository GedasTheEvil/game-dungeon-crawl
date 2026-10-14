"""Synthesize the blue lotus sounds (plan water-lotus): the wake (water pouring off as it rears up out of the pool, a
leafy rustle, a wet hiss), the bite (a swish, a wet snap of the petal jaws, drips) and the death (a gurgling wilt, the
head flopping onto the water, bubbles as it sinks).

    python3 tools/audio/lotus_sounds.py [out_dir]   (default: media/sounds/monsters/, writes lotus_{wake,att,die}.wav)

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
rng = np.random.default_rng(57)  # fixed seed: the same files on every run


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


def pour(dur=0.6):
    """Water running off: band noise, rising then falling away, with a trickle of droplets."""
    t = t_axis(dur)
    k = t / dur
    body = band(rng.standard_normal(len(t)), 300, 2500) * np.sin(np.pi * k) ** 0.6
    drops = band(rng.standard_normal(len(t)) * (rng.random(len(t)) > 0.97), 1500, 6000) * (0.3 + k)
    return body + 0.6 * drops


def drop(f=900.0, dur=0.08):
    """One drip: a short rising blip (the bubble's resonance)."""
    t = t_axis(dur)
    return np.sin(2 * np.pi * np.cumsum(f * (1 + 1.5 * t / dur)) / SR) * env(t, 0.002, 0.018)


def rustle(dur=0.3):
    """Petals and leaves brushing: crackly high noise in bursts."""
    t = t_axis(dur)
    gate = (rng.random(len(t)) > 0.6) * (0.5 + 0.5 * np.sin(2 * np.pi * 23 * t))
    return band(rng.standard_normal(len(t)) * gate, 2500, 8000) * np.sin(np.pi * t / dur)


def hiss(dur=0.4):
    t = t_axis(dur)
    return band(rng.standard_normal(len(t)), 3000, 7500) * env(t, 0.08, 0.15)


def snap(dur=0.12):
    """The jaws shutting: a wet thump and a click."""
    t = t_axis(dur)
    thump = np.sin(2 * np.pi * (220 * np.exp(-t / 0.03) + 90) * t) * env(t, 0.001, 0.03)
    click = band(rng.standard_normal(len(t)), 1200, 6000) * env(t, 0.0005, 0.006)
    return thump + 0.7 * click


def swish(dur=0.2):
    t = t_axis(dur)
    k = t / dur
    noise = rng.standard_normal(len(t))
    out = np.zeros(len(t))
    steps = 6
    for s in range(steps):  # a band sweeping up as the head lunges
        a, b = int(len(t) * s / steps), int(len(t) * (s + 1) / steps)
        f = 600 + 2200 * (s + 0.5) / steps
        out[a:b] = band(noise, f * 0.6, f * 1.5)[a:b]
    return out * np.sin(np.pi * k) ** 1.5


def gurgle(dur=0.6):
    """Bubbles: a run of drips at random pitches, thinning out."""
    out = np.zeros(int(dur * SR))
    at = 0.0
    while at < dur - 0.08:
        place(out, drop(rng.uniform(300, 700), 0.07) * rng.uniform(0.4, 1.0), at)
        at += rng.uniform(0.02, 0.06) * (1 + 2 * at / dur)
    return out


def finish(out):
    out = lowpass(out, 9000)
    fade = int(0.03 * SR)
    out[-fade:] *= np.linspace(1, 0, fade)
    return out / np.max(np.abs(out)) * 10 ** (-3 / 20)


def wake():
    out = np.zeros(int(0.9 * SR))
    place(out, rms(pour(0.7), 0.10), 0.0)
    place(out, rms(rustle(0.35), 0.05), 0.15)
    place(out, rms(hiss(0.45), 0.05), 0.4)
    for at in (0.55, 0.66, 0.8):
        place(out, rms(drop(rng.uniform(800, 1300)), 0.03), at)
    return finish(out)


def attack():
    out = np.zeros(int(0.6 * SR))
    place(out, rms(swish(0.18), 0.06), 0.0)
    place(out, rms(snap(0.12), 0.14), 0.15)
    place(out, rms(rustle(0.2), 0.03), 0.2)
    for at in (0.33, 0.45):
        place(out, rms(drop(rng.uniform(900, 1400)), 0.03), at)
    return finish(out)


def die():
    out = np.zeros(int(1.6 * SR))
    place(out, rms(hiss(0.5), 0.05), 0.0)
    place(out, rms(rustle(0.4), 0.05), 0.1)
    t = t_axis(0.3)
    slap = band(rng.standard_normal(len(t)), 250, 2200) * env(t, 0.003, 0.06)
    place(out, rms(slap, 0.12), 0.55)
    place(out, rms(pour(0.4), 0.04), 0.58)
    place(out, rms(gurgle(0.8), 0.06), 0.75)
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
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds", "monsters")
    write(os.path.join(out_dir, "lotus_wake.wav"), wake())
    write(os.path.join(out_dir, "lotus_att.wav"), attack())
    write(os.path.join(out_dir, "lotus_die.wav"), die())


if __name__ == "__main__":
    main()
