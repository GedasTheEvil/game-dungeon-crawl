"""Synthesize the scorpion sounds: the attack (chitin clacks of the claws grabbing and pinching, a quick hiss and whip
of the tail strike, the sting's tick) and the death (dry chitin rattle of the convulsion, legs scraping, fading out).

    python3 tools/audio/scorpion_sounds.py [out_dir]      (default: sounds/monsters/, writes scorpion_{att,die}.wav)

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


def clack(dur=0.06, pitch=2600, hard=1.0):
    """Hard chitin tap: a short click with two resonances of the shell, no low body."""
    t = t_axis(dur)
    click = band(rng.standard_normal(len(t)), 2000, 9000) * env(t, 0.0002, 0.003 * hard)
    ring = (np.sin(2 * np.pi * pitch * t) + 0.6 * np.sin(2 * np.pi * pitch * 1.73 * t + 1)) * env(t, 0.0005, 0.012)
    knock = np.sin(2 * np.pi * pitch * 0.33 * t) * env(t, 0.0005, 0.008) * 0.5
    return click + 0.35 * ring + knock


def whip(dur=0.22, f0=900, f1=3800):
    """Tail strike: rising band of air noise, sharpening into a crack at the end."""
    t = t_axis(dur)
    k = t / dur
    noise = rng.standard_normal(len(t))
    lo = band(noise, f0, f0 * 2.5)
    hi = band(noise, f1 * 0.6, min(f1 * 1.6, 10000))
    swell = np.sin(np.pi * np.minimum(k, 1.0) ** 1.6) ** 2
    return (lo * (1 - k) + hi * k) * swell


def hiss(dur, lo=2500, hi=8000, attack=0.02):
    t = t_axis(dur)
    shape = np.minimum(t / attack, 1.0) * np.clip((dur - t) / (0.5 * dur), 0, 1) ** 1.5
    return band(rng.standard_normal(len(t)), lo, hi) * shape


def rattle(dur, rate0, rate1, jitter=0.3):
    """Dry chitin rattle: a train of tiny clacks, the rate gliding from rate0 to rate1 per second."""
    out = np.zeros(int(dur * SR))
    t = 0.0
    while t < dur - 0.03:
        k = t / dur
        rate = rate0 + (rate1 - rate0) * k
        c = clack(0.03, rng.uniform(2200, 4200), rng.uniform(0.6, 1.3))
        place(out, c * rng.uniform(0.4, 1.0) * (1 - 0.7 * k), t)
        t += (1 + rng.uniform(-jitter, jitter)) / rate
    return out


def scrape(dur, rate=18):
    """Legs scraping over stone: band noise in rough strokes."""
    t = t_axis(dur)
    strokes = np.clip(lowpass(rng.standard_normal(len(t)), rate) * 6, 0, None)
    grit = band(rng.standard_normal(len(t)), 1500, 6000) * (1 + 0.8 * (rng.random(len(t)) > 0.97))
    return grit * strokes


def finish(out):
    out = lowpass(out, 10000)
    fade = int(0.03 * SR)
    out[-fade:] *= np.linspace(1, 0, fade)
    return out / np.max(np.abs(out)) * 10 ** (-3 / 20)


def attack():
    """Claws snap open, clack shut on the grab, a quick hiss and whip of the tail over the head, the sting's tick."""
    out = np.zeros(int(1.0 * SR))
    place(out, rms(clack(0.05, 3200, 0.7), 0.05), 0.0)
    place(out, rms(clack(0.05, 3500, 0.7), 0.04), 0.025)
    place(out, rms(clack(0.07, 2400), 0.12), 0.40)
    place(out, rms(clack(0.07, 2700), 0.10), 0.42)
    place(out, rms(hiss(0.25, 3000, 9000, 0.05), 0.035), 0.50)
    place(out, rms(whip(0.2), 0.10), 0.60)
    place(out, rms(clack(0.05, 4200, 1.4), 0.09), 0.80)
    place(out, rms(scrape(0.2, 25), 0.02), 0.78)
    return finish(out)


def die():
    """A burst of convulsive chitin rattling, legs scraping, the rattle slowing and fading to a last few ticks."""
    out = np.zeros(int(1.7 * SR))
    place(out, rms(rattle(0.5, 45, 30), 0.10), 0.0)
    place(out, rms(scrape(0.9, 14) * env(t_axis(0.9), 0.05, 0.4), 0.05), 0.1)
    place(out, rms(rattle(0.8, 22, 6), 0.06), 0.45)
    place(out, rms(scrape(0.5, 8) * env(t_axis(0.5), 0.05, 0.2), 0.02), 0.9)
    for at, level in ((1.35, 0.025), (1.55, 0.012)):
        place(out, rms(clack(0.04, 3000, 0.8), level), at)
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
    write(os.path.join(out_dir, "scorpion_att.wav"), attack())
    write(os.path.join(out_dir, "scorpion_die.wav"), die())


if __name__ == "__main__":
    main()
