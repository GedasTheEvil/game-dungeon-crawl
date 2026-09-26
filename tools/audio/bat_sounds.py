"""Synthesize the bat sounds: the bite (chittering screech + leathery flap) and the death (falling squeal + soft thud).

    python3 tools/audio/bat_sounds.py [out_dir]      (default: Sounds/, writes bat_att.wav and bat_die.wav)

Everything is generated (no samples), so this script is the source of the sounds. The giant bat uses the same files.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys
import wave

import numpy as np
from scipy.signal import butter, lfilter, sosfilt

SR = 22050
REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
rng = np.random.default_rng(11)  # fixed seed: the same files on every run


def t_axis(duration):
    return np.arange(int(duration * SR)) / SR


def band(x, lo, hi, order=2):
    return sosfilt(butter(order, [lo, hi], btype="band", fs=SR, output="sos"), x)


def rms(x, level):
    return x * level / np.sqrt(np.mean(x ** 2))


def place(out, clip, at):
    i = int(at * SR)
    n = min(len(clip), len(out) - i)
    out[i:i + n] += clip[:n]


def env(t, attack, decay):
    return np.minimum(t / attack, 1.0) * np.exp(-np.maximum(t - attack, 0) / decay)


def squeak(dur, f_start, f_end, vibrato=0.0, rough=0.0):
    """Pitched squeal: a sawtooth-ish tone with a few harmonics, pitch glides f_start -> f_end."""
    t = t_axis(dur)
    f = f_start * (f_end / f_start) ** (t / dur) * (1 + vibrato * np.sin(2 * np.pi * 38 * t))
    ph = 2 * np.pi * np.cumsum(f) / SR
    x = np.sin(ph) + 0.5 * np.sin(2 * ph + 0.3) + 0.25 * np.sin(3 * ph + 0.9)
    if rough:
        x *= 1 + rough * band(rng.standard_normal(len(t)), 60, 400)
    return x


def chitter():
    """Three to four fast, high chirps (the bite)."""
    out = np.zeros(int(0.3 * SR))
    for k, (at, f0) in enumerate(((0.0, 4200), (0.055, 4700), (0.11, 4000), (0.17, 3600))):
        d = 0.05 if k < 3 else 0.09
        s = squeak(d, f0, f0 * 0.7, vibrato=0.04, rough=0.3) * env(t_axis(d), 0.004, d / 3)
        place(out, s * (1.0 - 0.15 * k), at)
    return band(out, 1800, 9000)


def flap(dur=0.12):
    """Leathery wing beat: a band-limited noise burst with a quick swell."""
    t = t_axis(dur)
    n = band(rng.standard_normal(len(t)), 250, 1800)
    return n * env(t, 0.03, 0.03)


def thud(dur=0.18):
    t = t_axis(dur)
    body = np.sin(2 * np.pi * (110 - 50 * t / dur) * t) * env(t, 0.003, 0.04)
    grit = band(rng.standard_normal(len(t)), 300, 2500) * env(t, 0.002, 0.015)
    return body + 0.4 * grit


def finish(out):
    b, a = butter(2, 9500, fs=SR)
    out = lfilter(b, a, out)
    fade = int(0.02 * SR)
    out[-fade:] *= np.linspace(1, 0, fade)
    return out / np.max(np.abs(out)) * 10 ** (-3 / 20)


def attack():
    out = np.zeros(int(0.36 * SR))
    place(out, rms(flap(), 0.05), 0.0)
    place(out, rms(flap(), 0.04), 0.16)
    place(out, rms(chitter(), 0.14), 0.02)
    return finish(out)


def die():
    out = np.zeros(int(0.62 * SR))
    d = 0.38
    s = squeak(d, 3800, 1300, vibrato=0.06, rough=0.4) * env(t_axis(d), 0.01, 0.16)
    place(out, rms(band(s, 900, 8000), 0.15), 0.0)
    place(out, rms(flap(0.1), 0.035), 0.02)
    place(out, rms(thud(), 0.12), 0.42)
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
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "Sounds")
    write(os.path.join(out_dir, "bat_att.wav"), attack())
    write(os.path.join(out_dir, "bat_die.wav"), die())


if __name__ == "__main__":
    main()
