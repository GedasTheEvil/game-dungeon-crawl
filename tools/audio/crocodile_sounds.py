"""Synthesize the crocodile sounds: waking (water surging off its back, a splash, a low hiss), the bite (hiss, heavy
jaw snap with a bone-deep thud, teeth clack, a short thrash in the water) and the death (deep rumbling bellow fading, a
last splash as it rolls over).

    python3 tools/audio/crocodile_sounds.py [out_dir]      (default: sounds/monsters/, writes crocodile_{wake,att,die}.wav)

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
rng = np.random.default_rng(41)  # fixed seed: the same files on every run


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


def hiss(dur, lo=1800, hi=7000, attack=0.05):
    """Breathy hiss: band-limited noise with a soft swell and a fade."""
    t = t_axis(dur)
    shape = np.minimum(t / attack, 1.0) * np.clip((dur - t) / (0.4 * dur), 0, 1) ** 1.5
    flutter = 1 + 0.25 * lowpass(rng.standard_normal(len(t)), 12) * 4
    return band(rng.standard_normal(len(t)), lo, hi) * shape * flutter


def drops(dur, count, lo=900, hi=2600):
    """Droplets falling back: short pitched blips that glide up (bubble-like)."""
    out = np.zeros(int(dur * SR))
    for _ in range(count):
        d = rng.uniform(0.02, 0.05)
        t = t_axis(d)
        f0 = rng.uniform(lo, hi)
        f = f0 * (1 + 1.5 * t / d)
        blip = np.sin(2 * np.pi * np.cumsum(f) / SR) * env(t, 0.001, d / 3)
        place(out, blip * rng.uniform(0.3, 1.0), rng.uniform(0, dur - d))
    return out


def splash(dur=0.6, weight=1.0):
    """Water slap: a broadband burst with a low body, then a washy tail and droplets."""
    t = t_axis(dur)
    burst = band(rng.standard_normal(len(t)), 300, 6000) * env(t, 0.004, 0.06 * weight)
    wash = band(rng.standard_normal(len(t)), 500, 3500) * env(t, 0.03, 0.18 * weight) * 0.5
    body = lowpass(rng.standard_normal(len(t)), 250) * env(t, 0.005, 0.08 * weight) * 3
    return burst + wash + body + 0.25 * rms(drops(dur, int(14 * weight)), np.sqrt(np.mean(burst ** 2)) + 1e-9)


def surge(dur):
    """Water streaming off a rising body: swelling low-passed noise with a gurgle."""
    t = t_axis(dur)
    swell = np.sin(np.pi * np.minimum(t / dur, 1.0)) ** 1.2
    gurgle = 1 + 0.6 * np.sin(2 * np.pi * (7 + 5 * t / dur) * t + 2 * lowpass(rng.standard_normal(len(t)), 6))
    return band(rng.standard_normal(len(t)), 150, 2200) * swell * gurgle


def rumble(dur, f0, f1, rough=0.5):
    """Infrasonic-ish bellow: a low buzzy tone with slow tremolo and jitter, heard as a deep growl."""
    t = t_axis(dur)
    jitter = lowpass(rng.standard_normal(len(t)), 20) * 5
    f = f0 * (f1 / f0) ** (t / dur) * (1 + 0.03 * jitter)
    ph = 2 * np.pi * np.cumsum(f) / SR
    x = sum(np.sin(k * ph + 0.5 * k) / k ** 0.7 for k in range(1, 14))
    trem = 1 + rough * np.sin(2 * np.pi * 9 * t + 2 * lowpass(rng.standard_normal(len(t)), 4))
    return band(x * trem, 35, 900)


def snap(dur=0.25):
    """Jaws slamming: a dull bony thud plus a hard crack and a clack of teeth."""
    t = t_axis(dur)
    thud = (np.sin(2 * np.pi * 95 * t) + 0.5 * np.sin(2 * np.pi * 210 * t)) * env(t, 0.002, 0.045)
    crack = band(rng.standard_normal(len(t)), 1200, 6500) * env(t, 0.0004, 0.012)
    clack = band(rng.standard_normal(len(t)), 2800, 8500) * env(np.maximum(t - 0.012, 1e-6), 0.0003, 0.005) * (t > 0.012)
    return thud + 0.9 * crack + 0.6 * clack


def finish(out):
    out = lowpass(out, 9500)
    fade = int(0.03 * SR)
    out[-fade:] *= np.linspace(1, 0, fade)
    return out / np.max(np.abs(out)) * 10 ** (-3 / 20)


def wake():
    """It surges out of the water: rising wash, a splash as the body breaks the surface, droplets, a low hiss."""
    out = np.zeros(int(1.4 * SR))
    place(out, rms(surge(0.7), 0.08), 0.0)
    place(out, rms(splash(0.7, 1.2), 0.2), 0.32)
    place(out, rms(drops(0.6, 20), 0.03), 0.55)
    place(out, rms(hiss(0.6, 900, 5000, 0.12), 0.07), 0.7)
    place(out, rms(rumble(0.5, 48, 42, 0.4) * env(t_axis(0.5), 0.1, 0.2), 0.06), 0.75)
    return finish(out)


def attack():
    """A sharp hiss as the jaws gape, the heavy snap, a thrash of water from the head shake."""
    out = np.zeros(int(0.9 * SR))
    place(out, rms(hiss(0.32, 1500, 7500, 0.03), 0.09), 0.0)
    place(out, rms(snap(), 0.3), 0.3)
    place(out, rms(snap(0.12), 0.08), 0.37)
    place(out, rms(splash(0.4, 0.6), 0.07), 0.42)
    place(out, rms(splash(0.35, 0.5), 0.05), 0.58)
    return finish(out)


def die():
    """A deep rumbling bellow that sags and fades, a weak hiss, then the splash of the body rolling over."""
    out = np.zeros(int(1.9 * SR))
    d = 1.2
    place(out, rms(rumble(d, 62, 34, 0.7) * env(t_axis(d), 0.08, 0.45), 0.2), 0.0)
    place(out, rms(hiss(0.5, 700, 3500, 0.1), 0.03), 0.6)
    place(out, rms(splash(0.8, 1.4), 0.17), 1.05)
    place(out, rms(drops(0.5, 12), 0.02), 1.3)
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
    write(os.path.join(out_dir, "crocodile_wake.wav"), wake())
    write(os.path.join(out_dir, "crocodile_att.wav"), attack())
    write(os.path.join(out_dir, "crocodile_die.wav"), die())


if __name__ == "__main__":
    main()
