"""Synthesize the sounds of the rats, keys, gates, levers and rock falls.

    python3 tools/audio/mechanism_sounds.py [out_dir]      (default: Sounds/)

Writes rat_att.wav (squeak + hiss, also used by the giant rat), rat_die.wav (falling squeal),
key_pickup.wav (metal chink + chime), gate_open.wav (stone grinding + chain rattle), gate_locked.wav (dull rattle),
lever.wav (wooden clunk + latch), rock_rumble.wav (low rumble with trickling grit), rock_crash.wav (impact + debris).
Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, like jump_sound.py.
"""

import os
import sys
import wave

import numpy as np
from scipy.signal import butter, sosfilt

SR = 22050
REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
rng = np.random.default_rng(11)  # fixed seed: the same files on every run


def t_axis(duration):
    return np.arange(int(duration * SR)) / SR


def band(x, lo, hi, order=2):
    return sosfilt(butter(order, [lo, hi], btype="band", fs=SR, output="sos"), x)


def lowpass(x, hi, order=2):
    return sosfilt(butter(order, hi, fs=SR, output="sos"), x)


def place(out, clip, at, gain=1.0):
    i = int(at * SR)
    n = min(len(clip), len(out) - i)
    out[i:i + n] += gain * clip[:n]


def env_ad(t, attack, decay):
    return (1 - np.exp(-t / attack)) * np.exp(-t / decay)


def chirp(f_start, f_end, dur, curve=1.0):
    t = t_axis(dur)
    k = (t / dur) ** curve
    f = f_start + (f_end - f_start) * k
    return np.sin(2 * np.pi * np.cumsum(f) / SR), t


def normalize(x, db=-3.0, fade=0.02):
    n = int(fade * SR)
    x = x.copy()
    x[-n:] *= np.linspace(1, 0, n)
    return x / np.max(np.abs(x)) * 10 ** (db / 20)


def squeak(f0, dur, wobble=40):
    s, t = chirp(f0, f0 * 1.25, dur, 0.5)
    s *= 1 + 0.3 * np.sin(2 * np.pi * wobble * t)
    return s * np.sin(np.pi * t / dur) ** 1.5


def rat_att():
    out = np.zeros(int(0.45 * SR))
    place(out, squeak(2600, 0.09), 0.0, 0.6)
    place(out, squeak(3100, 0.07), 0.1, 0.5)
    t = t_axis(0.25)
    hiss = band(rng.standard_normal(len(t)), 2500, 7000) * env_ad(t, 0.01, 0.08)
    place(out, hiss, 0.16, 0.5)
    return normalize(out, -4)


def rat_die():
    s, t = chirp(3400, 900, 0.5, 1.6)
    s *= 1 + 0.35 * np.sin(2 * np.pi * 28 * t)
    s *= np.exp(-t / 0.25) * (1 - np.exp(-t / 0.01))
    return normalize(s + 0.1 * band(rng.standard_normal(len(t)), 2000, 6000) * np.exp(-t / 0.1), -4)


def ring(freqs, dur, decay):
    t = t_axis(dur)
    return sum(np.sin(2 * np.pi * f * t) * np.exp(-t / (decay * (1 + i * 0.3))) / (1 + i)
               for i, f in enumerate(freqs)), t


def key_pickup():
    out = np.zeros(int(0.7 * SR))
    chink, t = ring((2350, 3710, 5230), 0.25, 0.05)
    place(out, chink, 0.0, 0.5)
    for i, f in enumerate((1320, 1760, 2640)):  # rising chime
        tone, _ = ring((f, 2 * f), 0.45, 0.18)
        place(out, tone, 0.05 + 0.07 * i, 0.35)
    return normalize(out, -5)


def grind(dur):
    t = t_axis(dur)
    noise = rng.standard_normal(len(t))
    grain = band(noise, 120, 700) * (1 + 0.6 * np.sin(2 * np.pi * 7 * t + 2 * np.sin(2 * np.pi * 1.3 * t)))
    return grain, t


def rattle(dur, rate, lo=1800, hi=6000):
    out = np.zeros(int(dur * SR))
    tt = 0.0
    while tt < dur - 0.05:
        clink, _ = ring((rng.uniform(lo, hi), rng.uniform(lo, hi)), 0.05, 0.01)
        place(out, clink, tt, rng.uniform(0.2, 0.5))
        tt += rng.exponential(1 / rate)
    return out


def gate_open():
    dur = 1.3
    grain, t = grind(dur)
    env = np.clip(t / 0.08, 0, 1) * np.clip((dur - t) / 0.2, 0, 1)
    out = grain * env * 0.8 + rattle(dur, 18) * 0.5
    thud = lowpass(rng.standard_normal(int(0.2 * SR)), 180) * env_ad(t_axis(0.2), 0.003, 0.05)
    place(out, thud, dur - 0.25, 2.5)
    return normalize(out, -4)


def gate_locked():
    out = rattle(0.35, 40, 900, 3000)
    t = t_axis(0.3)
    place(out, lowpass(rng.standard_normal(len(t)), 250) * env_ad(t, 0.002, 0.04), 0.0, 2.0)
    return normalize(out, -6)


def lever():
    out = np.zeros(int(0.5 * SR))
    t = t_axis(0.3)
    creak, _ = chirp(420, 260, 0.18, 1.0)
    place(out, band(creak * (1 + rng.standard_normal(len(creak)) * 0.3), 250, 1500) * np.sin(np.pi * t_axis(0.18) / 0.18), 0.0, 0.4)
    clunk = lowpass(rng.standard_normal(len(t)), 400) * env_ad(t, 0.002, 0.035)
    place(out, clunk, 0.17, 2.2)
    latch, _ = ring((1900, 3100), 0.15, 0.02)
    place(out, latch, 0.19, 0.4)
    return normalize(out, -4)


def rock_rumble():
    dur = 0.9
    t = t_axis(dur)
    rumble = lowpass(rng.standard_normal(len(t)), 110, 4) * (0.5 + 0.5 * np.clip(t / 0.6, 0, 1))
    grit = band(rng.standard_normal(len(t)), 2000, 7000) * (rng.random(len(t)) > 0.985) * 3
    env = np.clip(t / 0.1, 0, 1) * np.clip((dur - t) / 0.15, 0, 1)
    return normalize((rumble * 6 + grit * 0.3 * np.clip(t / 0.4, 0, 1)) * env, -3)


def rock_crash():
    dur = 1.0
    t = t_axis(dur)
    boom = lowpass(rng.standard_normal(len(t)), 160, 4) * env_ad(t, 0.002, 0.12) * 8
    crack = band(rng.standard_normal(len(t)), 800, 5000) * env_ad(t, 0.001, 0.04)
    out = boom + crack
    debris = np.zeros(len(t))
    tt = 0.05
    while tt < 0.7:
        pebble = band(rng.standard_normal(int(0.04 * SR)), 1000, 4500) * env_ad(t_axis(0.04), 0.001, 0.008)
        place(debris, pebble, tt, rng.uniform(0.1, 0.4) * (1 - tt))
        tt += rng.exponential(0.035)
    return normalize(out + debris, -2)


SOUNDS = {
    "rat_att": rat_att, "rat_die": rat_die, "key_pickup": key_pickup, "gate_open": gate_open,
    "gate_locked": gate_locked, "lever": lever, "rock_rumble": rock_rumble, "rock_crash": rock_crash,
}


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "Sounds")
    for name, fn in SOUNDS.items():
        pcm = (fn() * 32767).astype("<i2")
        path = os.path.join(out_dir, name + ".wav")
        with wave.open(path, "wb") as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(SR)
            w.writeframes(pcm.tobytes())
        print("wrote {} ({:.2f} s)".format(path, len(pcm) / SR))


if __name__ == "__main__":
    main()
