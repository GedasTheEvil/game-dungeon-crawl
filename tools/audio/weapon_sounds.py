"""Synthesize the weapon sounds: a swing and a hit per melee weapon, the bow's draw and release, the arrow's impacts.

    python3 tools/audio/weapon_sounds.py [out_dir]      (default: sounds/)

Writes items/club_swing.wav (heavy, low whoosh), club_hit.wav (dull wooden thud on flesh), sword_swing.wav (quick bright
swish with a faint ring), sword_hit.wav (slash: cut, wet slap, blade ring), spear_swing.wav (short airy thrust),
spear_hit.wav (stab: punch and tearing hiss), bow_draw.wav (the draw: creaking wood, string tension),
bow_release.wav (string twang and snap, the arrow's whoosh), arrow_hit.wav (thunk into a body),
arrow_wall.wav (tock into stone and the shaft buzzing).
Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, like mechanism_sounds.py.
"""

import os
import sys
import wave

import numpy as np
from scipy.signal import butter, sosfilt

SR = 22050
REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
rng = np.random.default_rng(23)  # fixed seed: the same files on every run


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


def normalize(x, db=-3.0, fade=0.02):
    n = int(fade * SR)
    x = x.copy()
    x[-n:] *= np.linspace(1, 0, n)
    return x / np.max(np.abs(x)) * 10 ** (db / 20)


def whoosh(dur, lo0, hi0, lo1, hi1, peak=0.5, steps=24):
    """Noise through a band sweeping from (lo0, hi0) to (lo1, hi1), swelling to its loudest at peak (0..1)."""
    t = t_axis(dur)
    noise = rng.standard_normal(len(t))
    out = np.zeros(len(t))
    edges = np.linspace(0, len(t), steps + 1).astype(int)
    for k in range(steps):  # piecewise filtered, crossfaded by the envelope below
        a = k / (steps - 1)
        seg = band(noise, lo0 + (lo1 - lo0) * a, hi0 + (hi1 - hi0) * a)
        out[edges[k]:edges[k + 1]] = seg[edges[k]:edges[k + 1]]
    x = t / dur
    swell = np.where(x < peak, (x / peak) ** 2, np.exp(-(x - peak) / (1 - peak) * 4))
    return out * swell


def thud(f0, f1, dur, decay):
    """Pitched body of an impact: a sine gliding from f0 to f1."""
    t = t_axis(dur)
    f = f1 + (f0 - f1) * np.exp(-t / (decay * 0.6))
    return np.sin(2 * np.pi * np.cumsum(f) / SR) * env_ad(t, 0.002, decay)


def burst(dur, lo, hi, decay, attack=0.001):
    t = t_axis(dur)
    return band(rng.standard_normal(len(t)), lo, hi) * env_ad(t, attack, decay)


def ring(freqs, dur, decay):
    t = t_axis(dur)
    return sum(np.sin(2 * np.pi * f * t) / (k + 1) for k, f in enumerate(freqs)) * env_ad(t, 0.003, decay)


def club_swing():
    return normalize(whoosh(0.42, 120, 500, 250, 1100, peak=0.6), -5)


def club_hit():
    out = np.zeros(int(0.35 * SR))
    place(out, thud(140, 55, 0.3, 0.09), 0.0, 1.0)
    place(out, burst(0.12, 250, 900, 0.03), 0.0, 0.7)  # wood knock
    place(out, burst(0.08, 900, 3000, 0.015), 0.004, 0.25)  # slap
    return normalize(out, -3)


def sword_swing():
    out = np.zeros(int(0.3 * SR))
    place(out, whoosh(0.24, 1200, 4000, 2500, 7500, peak=0.45), 0.0, 1.0)
    place(out, ring([3100, 4650], 0.25, 0.08), 0.06, 0.04)
    return normalize(out, -6)


def sword_hit():
    out = np.zeros(int(0.45 * SR))
    place(out, burst(0.12, 1500, 5500, 0.035, attack=0.004), 0.0, 0.8)  # the cut
    place(out, thud(180, 90, 0.15, 0.05), 0.0, 0.7)  # wet slap
    place(out, lowpass(burst(0.1, 300, 1200, 0.03), 900), 0.01, 0.5)
    place(out, ring([2350, 3510, 5230], 0.4, 0.14), 0.0, 0.18)  # the blade rings on
    return normalize(out, -3)


def spear_swing():
    return normalize(whoosh(0.2, 700, 2200, 1400, 4200, peak=0.3), -6)


def spear_hit():
    out = np.zeros(int(0.3 * SR))
    place(out, thud(160, 70, 0.2, 0.06), 0.0, 1.0)  # the punch
    place(out, burst(0.18, 1800, 6000, 0.06, attack=0.01), 0.005, 0.5)  # tearing hiss
    place(out, burst(0.05, 500, 1500, 0.012), 0.0, 0.4)
    return normalize(out, -3)


def bow_draw():
    """The draw (BOW_DRAW_MS = 450): wood creaking in a few grains, rising, and the string's tension hum."""
    dur = 0.45
    out = np.zeros(int(dur * SR))
    t = t_axis(dur)
    for k, at in enumerate(np.sort(rng.uniform(0.02, 0.4, 14))):
        f = 380 + 500 * at / dur + rng.uniform(-40, 40)
        creak = burst(0.03, f * 0.8, f * 1.6, 0.008)
        place(out, creak, at, 0.35 + 0.5 * at / dur)
    hum = np.sin(2 * np.pi * (140 + 60 * t / dur) * t) * (t / dur) ** 2 * 0.15
    out += hum
    return normalize(out, -9)


def bow_release():
    out = np.zeros(int(0.6 * SR))
    t = t_axis(0.5)
    f = 150 + 70 * np.exp(-t / 0.02)  # the string snaps back, settles to its pitch
    phase = 2 * np.pi * np.cumsum(f) / SR
    twang = sum(np.sin(k * phase) * np.exp(-t * (8 + 7 * k)) / k for k in range(1, 7))
    twang *= 1 + 0.4 * np.sin(2 * np.pi * 9 * t)  # thrum
    place(out, twang, 0.0, 1.0)
    place(out, burst(0.03, 1500, 6000, 0.006), 0.0, 0.6)  # snap
    place(out, whoosh(0.22, 1500, 4500, 900, 3000, peak=0.2), 0.01, 0.35)  # the arrow leaves
    return normalize(out, -4)


def arrow_hit():
    out = np.zeros(int(0.3 * SR))
    place(out, thud(210, 90, 0.2, 0.05), 0.0, 1.0)
    place(out, burst(0.06, 700, 2500, 0.012), 0.0, 0.6)
    return normalize(out, -4)


def arrow_wall():
    out = np.zeros(int(0.5 * SR))
    place(out, burst(0.04, 1500, 5000, 0.006), 0.0, 1.0)  # the tock
    place(out, thud(900, 700, 0.06, 0.012), 0.0, 0.5)
    t = t_axis(0.45)
    buzz = np.sin(2 * np.pi * 330 * t) * (0.5 + 0.5 * np.sign(np.sin(2 * np.pi * 38 * t))) * env_ad(t, 0.01, 0.12)
    place(out, buzz, 0.01, 0.25)  # the shaft quivering
    return normalize(out, -6)


# Paths under sounds/.
SOUNDS = {
    "items/club_swing": club_swing, "items/club_hit": club_hit, "items/sword_swing": sword_swing,
    "items/sword_hit": sword_hit, "items/spear_swing": spear_swing, "items/spear_hit": spear_hit,
    "items/bow_draw": bow_draw, "items/bow_release": bow_release, "items/arrow_hit": arrow_hit,
    "items/arrow_wall": arrow_wall,
}


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "sounds")
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
