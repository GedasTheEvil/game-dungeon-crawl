"""Synthesize the cobra sounds: the wake (a hiss swelling as it rears up and spreads the hood), the attack (a sharp
hiss and the snap of the strike), the spit (a short wet 'pff' and a hiss) and the death (a hiss fading out and the
thud of the body falling).

    python3 tools/audio/cobra_sounds.py [out_dir]      (default: sounds/monsters/, writes cobra_{wake,att,spit,die}.wav)

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
rng = np.random.default_rng(73)  # fixed seed: the same files on every run


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


def hiss(dur, lo=2800, hi=9000, attack=0.05, release=0.4, wobble=0.0):
    """Breath forced through the glottis: band noise, a slow rise, a long release; wobble adds a breathy tremor."""
    t = t_axis(dur)
    shape = np.minimum(t / attack, 1.0) * np.clip((dur - t) / (release * dur), 0, 1) ** 1.3
    if wobble:
        shape *= 1 + wobble * np.sin(2 * np.pi * 7 * t)
    noise = rng.standard_normal(len(t))
    return (band(noise, lo, hi) + 0.35 * band(noise, lo * 0.5, lo)) * shape


def snap(dur=0.05):
    """The jaws closing on the strike: a short hard click with a little body."""
    t = t_axis(dur)
    click = band(rng.standard_normal(len(t)), 1500, 8000) * env(t, 0.0002, 0.004)
    knock = np.sin(2 * np.pi * 420 * t) * env(t, 0.0005, 0.012)
    return click + 0.6 * knock


def lunge(dur=0.16):
    """Air pushed aside by the strike: a low swish rising in pitch."""
    t = t_axis(dur)
    k = t / dur
    noise = rng.standard_normal(len(t))
    swell = np.sin(np.pi * k) ** 2
    return (band(noise, 300, 900) * (1 - k) + band(noise, 900, 3000) * k) * swell


def pff(dur=0.12):
    """Venom forced out through the fangs: a burst of wet, bubbly noise."""
    t = t_axis(dur)
    burst = band(rng.standard_normal(len(t)), 600, 5000) * env(t, 0.003, 0.03)
    bubbles = np.zeros(len(t))
    for _ in range(9):
        at = rng.uniform(0.0, dur * 0.6)
        f = rng.uniform(900, 2200)
        bt = t_axis(0.025)
        b = np.sin(2 * np.pi * f * bt * (1 + 2.5 * bt)) * env(bt, 0.001, 0.006)
        i = int(at * SR)
        n = min(len(b), len(bubbles) - i)
        bubbles[i:i + n] += b[:n] * rng.uniform(0.3, 1.0)
    return burst + 0.4 * bubbles


def thud(dur=0.3, f=95):
    """The body dropping on stone: a dull low thump with a little grit."""
    t = t_axis(dur)
    body = np.sin(2 * np.pi * f * t * (1 - 0.3 * t)) * env(t, 0.002, 0.06)
    grit = band(rng.standard_normal(len(t)), 400, 2500) * env(t, 0.001, 0.02)
    return body + 0.3 * grit


def finish(out):
    out = lowpass(out, 10000)
    fade = int(0.03 * SR)
    out[-fade:] *= np.linspace(1, 0, fade)
    return out / np.max(np.abs(out)) * 10 ** (-3 / 20)


def wake():
    """A long hiss swelling as it rears up, rising a little in pitch as the hood spreads."""
    out = np.zeros(int(1.2 * SR))
    place(out, rms(hiss(1.1, 2200, 7000, attack=0.45, release=0.35, wobble=0.15), 0.06), 0.0)
    place(out, rms(hiss(0.6, 3500, 9500, attack=0.25, release=0.5), 0.05), 0.45)
    return finish(out)


def attack():
    """A sharp hiss, the strike's swish and the snap of the jaws."""
    out = np.zeros(int(0.8 * SR))
    place(out, rms(hiss(0.35, 3000, 9000, attack=0.02, release=0.5), 0.07), 0.0)
    place(out, rms(lunge(0.16), 0.06), 0.30)
    place(out, rms(snap(), 0.12), 0.45)
    place(out, rms(hiss(0.25, 2500, 7000, attack=0.01, release=0.8), 0.025), 0.5)
    return finish(out)


def spit():
    """A short wet 'pff' and a hiss behind it."""
    out = np.zeros(int(0.6 * SR))
    place(out, rms(hiss(0.18, 3000, 8000, attack=0.08, release=0.2), 0.035), 0.0)
    place(out, rms(pff(0.14), 0.12), 0.15)
    place(out, rms(hiss(0.3, 2800, 9000, attack=0.01, release=0.7), 0.05), 0.2)
    return finish(out)


def die():
    """A hiss gasping out in a few weakening breaths, the body falling with a thud and a last slither."""
    out = np.zeros(int(1.6 * SR))
    place(out, rms(hiss(0.5, 2600, 8500, attack=0.02, release=0.6), 0.07), 0.0)
    place(out, rms(hiss(0.35, 2000, 6000, attack=0.05, release=0.6), 0.035), 0.55)
    place(out, rms(thud(0.3, 90), 0.12), 0.62)
    place(out, rms(thud(0.25, 120), 0.05), 0.80)
    t = t_axis(0.45)
    slither = band(rng.standard_normal(len(t)), 1200, 5000) * np.clip(lowpass(rng.standard_normal(len(t)), 12) * 5, 0, None)
    place(out, rms(slither * env(t, 0.05, 0.2), 0.02), 0.9)
    place(out, rms(hiss(0.3, 1800, 5000, attack=0.08, release=0.8), 0.012), 1.25)
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
    write(os.path.join(out_dir, "cobra_wake.wav"), wake())
    write(os.path.join(out_dir, "cobra_att.wav"), attack())
    write(os.path.join(out_dir, "cobra_spit.wav"), spit())
    write(os.path.join(out_dir, "cobra_die.wav"), die())


if __name__ == "__main__":
    main()
