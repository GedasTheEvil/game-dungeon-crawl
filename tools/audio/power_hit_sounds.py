"""Synthesize the power hit's sounds (plan skill-power-hit): the charge, the heavy swing and the heavy impact.

    python3 tools/audio/power_hit_sounds.py [out_dir]      (default: media/sounds/)

Writes items/power_charge.wav (a low hum and a strained breath rising in pitch for the charge's 0.85 s, then held,
trembling, till the release stops it), items/power_swing.wav (a deep, long whoosh) and items/power_hit.wav (a heavy
thump with a low boom under the weapon's own hit sound).
Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, like weapon_sounds.py.
"""

import os
import sys
import wave

import numpy as np
from scipy.signal import butter, sosfilt

SR = 22050
REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
rng = np.random.default_rng(41)  # fixed seed: the same files on every run

CHARGE_S = 0.85  # the charge's rise to its cap (POWER_CAP_PERCENT / POWER_DRAIN_PERCENT)
HOLD_S = 2.4  # held at the cap: the tremble goes on this long, then fades


def t_axis(duration):
    return np.arange(int(duration * SR)) / SR


def band(x, lo, hi, order=2):
    return sosfilt(butter(order, [lo, hi], btype="band", fs=SR, output="sos"), x)


def lowpass(x, hi, order=2):
    return sosfilt(butter(order, hi, fs=SR, output="sos"), x)


def normalize(x, db=-3.0, fade=0.02):
    x = x / (np.max(np.abs(x)) + 1e-12) * 10 ** (db / 20)
    n = int(fade * SR)
    x[-n:] *= np.linspace(1, 0, n)
    return x


def charge():
    """A hum rising from 70 to 190 Hz over the charge, a breathy hiss rising with it, then held with a tremble."""
    t = t_axis(CHARGE_S + HOLD_S)
    rise = np.minimum(t / CHARGE_S, 1.0)
    freq = 70 + 120 * rise ** 1.5
    tremble = 1 + 0.03 * np.sin(2 * np.pi * 7 * t) * (rise >= 1.0)
    phase = 2 * np.cumsum(np.pi * freq * tremble) / SR
    hum = np.sin(phase) + 0.45 * np.sin(2 * phase) + 0.2 * np.sin(3 * phase)
    breath = band(rng.standard_normal(len(t)), 500, 2500) * (0.15 + 0.35 * rise)
    level = np.minimum(t / 0.06, 1.0) * (0.35 + 0.65 * rise)
    level *= np.where(t > CHARGE_S + HOLD_S - 0.6, np.linspace(1, 0, len(t)) ** 0.2, 1.0)
    return normalize((hum * 0.6 + breath) * level, db=-6.0)


def swing():
    """A deep whoosh: low-passed noise swelling and falling, longer and lower than the club's."""
    t = t_axis(0.42)
    env = np.sin(np.pi * np.minimum(t / 0.42, 1.0)) ** 1.5
    air = lowpass(rng.standard_normal(len(t)), 900) + 0.4 * band(rng.standard_normal(len(t)), 900, 2200)
    return normalize(air * env, db=-2.0)


def hit():
    """A heavy thump: a pitch-dropping boom with a crack of noise on top."""
    t = t_axis(0.6)
    freq = 95 * np.exp(-t * 4) + 38
    boom = np.sin(2 * np.cumsum(np.pi * freq) / SR) * np.exp(-t / 0.18)
    crack = band(rng.standard_normal(len(t)), 200, 3000) * np.exp(-t / 0.02)
    return normalize(boom + 0.7 * crack, db=-1.0)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds")
    for name, sound in [("power_charge", charge()), ("power_swing", swing()), ("power_hit", hit())]:
        path = os.path.join(out_dir, "items", name + ".wav")
        with wave.open(path, "wb") as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(SR)
            w.writeframes((np.clip(sound, -1, 1) * 32767).astype(np.int16).tobytes())
        print(path)


if __name__ == "__main__":
    main()
