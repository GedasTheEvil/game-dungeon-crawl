"""Synthesize the player's jump sound: boot scuff on push-off, an effort "hup", and a cloth whoosh.

    python3 tools/audio/jump_sound.py [out.wav]      (default: Sounds/Jump.wav)

Everything is generated (no samples), so this script is the source of the sound.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys
import wave

import numpy as np
from scipy.signal import butter, lfilter, sosfilt

SR = 22050
LENGTH = 0.5
REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
rng = np.random.default_rng(7)  # fixed seed: the same file on every run


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


def scuff():
    """Short gritty scrape of a boot sole leaving the stone floor."""
    t = t_axis(0.09)
    grit = band(rng.standard_normal(len(t)), 700, 5000) * (1 + 0.6 * (rng.random(len(t)) > 0.97))
    thump = np.sin(2 * np.pi * 85 * t) * np.exp(-t / 0.025)
    env = np.exp(-t / 0.028) * (1 - np.exp(-t / 0.002))
    return 0.55 * grit * env + 0.5 * thump * (1 - np.exp(-t / 0.003))


def formants(src, freqs, bws, gains):
    out = np.zeros_like(src)
    for f, bw, g in zip(freqs, bws, gains):
        out += g * band(src, f - bw / 2, f + bw / 2)
    return out


def hup():
    """Male effort grunt 'hh-UH-p': breathy onset, voiced vowel with falling pitch, lips closing."""
    dur = 0.2
    t = t_axis(dur)
    f0 = 165 - 45 * (t / dur) + 3 * np.sin(2 * np.pi * 9 * t)  # falling pitch with slight vibrato
    phase = 2 * np.pi * np.cumsum(f0) / SR
    phase += 0.04 * np.cumsum(rng.standard_normal(len(t))) / np.sqrt(len(t))  # jitter
    # Glottal pulses: a sharpened sawtooth (bright, like a real voice source).
    saw = (phase / (2 * np.pi)) % 1.0
    glottal = np.where(saw < 0.6, np.sin(np.pi * saw / 0.6) ** 2, 0.0)
    glottal = np.diff(glottal, prepend=0.0) * 40
    breath = rng.standard_normal(len(t)) * 0.12
    voice_env = np.clip((t - 0.025) / 0.018, 0, 1) * np.clip((dur - t) / 0.05, 0, 1) ** 1.5
    aspir_env = np.exp(-((t - 0.02) / 0.018) ** 2)  # the "h"
    src = glottal * voice_env + breath * (0.35 * voice_env + 1.4 * aspir_env)
    # "uh" formants gliding slightly towards the closed lips of "p".
    k = np.clip(t / dur, 0, 1)
    a = formants(src, (650, 1150, 2500), (140, 180, 260), (1.0, 0.55, 0.22))
    b = formants(src, (480, 950, 2300), (120, 160, 240), (1.0, 0.45, 0.18))
    vowel = a * (1 - k) + b * k
    # Lip release: tiny low burst after closure.
    pop_t = t_axis(0.03)
    pop = band(rng.standard_normal(len(pop_t)), 150, 900) * np.exp(-pop_t / 0.006) * 0.35
    out = np.zeros(int((dur + 0.04) * SR))
    out[:len(vowel)] += vowel
    place(out, pop, dur + 0.004)
    return out


def whoosh():
    """Clothes and air rushing as the body springs forward: filtered noise sweeping up then down."""
    dur = 0.36
    t = t_axis(dur)
    noise = rng.standard_normal(len(t))
    low = band(noise, 250, 900)
    high = band(noise, 900, 3200)
    k = np.sin(np.pi * t / dur)
    env = k ** 2 * (1 - 0.3 * t / dur)
    flutter = 1 + 0.25 * np.sin(2 * np.pi * 23 * t)  # jacket flapping
    return (low * (1 - 0.6 * k) + high * 0.6 * k) * env * flutter


def render():
    out = np.zeros(int(LENGTH * SR))
    # Levels by loudness: the voice leads, the scuff marks the push-off, the whoosh sits underneath.
    place(out, rms(scuff(), 0.07), 0.0)
    place(out, rms(whoosh(), 0.035), 0.02)
    place(out, rms(hup(), 0.16), 0.012)
    # Gentle high cut and a short fade so nothing clicks.
    b, a = butter(2, 7500, fs=SR)
    out = lfilter(b, a, out)
    out[-int(0.03 * SR):] *= np.linspace(1, 0, int(0.03 * SR))
    return out / np.max(np.abs(out)) * 10 ** (-3 / 20)


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "Sounds", "Jump.wav")
    pcm = (render() * 32767).astype("<i2")
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())
    print("wrote {} ({:.2f} s)".format(path, len(pcm) / SR))


if __name__ == "__main__":
    main()
