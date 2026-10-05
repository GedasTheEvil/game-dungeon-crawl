"""Synthesize the journal's page turn: the page lifting off with a crackle, a soft paper swish over the spine, and
the light slap of it landing on the other side.

    python3 tools/audio/page_sound.py [out.wav]      (default: sounds/ui/page_turn.wav)

Everything is generated (no samples), so this script is the source of the sound. The turn takes 650 ms in the game
(FLIP_MS, src/ui/journal_view.cpp): the landing sits near its end.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys
import wave

import numpy as np
from scipy.signal import butter, lfilter, sosfilt

SR = 22050
LENGTH = 0.7
REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
rng = np.random.default_rng(23)  # fixed seed: the same file on every run


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


def crackle(dur, rate):
    """Paper fibres bending: sparse tiny clicks, each a short burst of bright noise."""
    t = t_axis(dur)
    out = np.zeros(len(t))
    for _ in range(int(dur * rate)):
        at = rng.uniform(0, dur)
        n = int(rng.uniform(0.002, 0.007) * SR)
        click = rng.standard_normal(n) * np.exp(-np.arange(n) / (n / 4)) * rng.uniform(0.3, 1.0)
        place(out, click, at)
    return band(out, 1800, 9000)


def swish():
    """Air and paper sliding: noise sweeping up as the page rises, down as it falls over."""
    dur = 0.52
    t = t_axis(dur)
    noise = rng.standard_normal(len(t))
    low = band(noise, 400, 1600)
    high = band(noise, 1600, 6500)
    k = np.sin(np.pi * t / dur)
    env = k ** 1.6
    flutter = 1 + 0.2 * np.sin(2 * np.pi * 17 * t + 1.0)
    return (low * (1 - 0.5 * k) + high * 0.8 * k) * env * flutter


def slap():
    """The page landing flat: a soft papery thump."""
    t = t_axis(0.08)
    body = band(rng.standard_normal(len(t)), 180, 1400) * np.exp(-t / 0.018)
    air = band(rng.standard_normal(len(t)), 2000, 6000) * np.exp(-t / 0.008)
    return body + 0.4 * air


def render():
    out = np.zeros(int(LENGTH * SR))
    lift = crackle(0.16, 120) * np.linspace(1, 0.2, int(0.16 * SR))
    place(out, rms(lift, 0.05), 0.0)
    place(out, rms(swish(), 0.05), 0.04)
    place(out, rms(crackle(0.5, 25), 0.012), 0.08)
    place(out, rms(slap(), 0.06), 0.55)
    b, a = butter(2, 8500, fs=SR)
    out = lfilter(b, a, out)
    fade = int(0.04 * SR)
    out[-fade:] *= np.linspace(1, 0, fade)
    return out / np.max(np.abs(out)) * 10 ** (-6 / 20)


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "sounds", "ui", "page_turn.wav")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    pcm = (render() * 32767).astype("<i2")
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(pcm.tobytes())
    print("wrote {} ({:.2f} s)".format(path, len(pcm) / SR))


if __name__ == "__main__":
    main()
