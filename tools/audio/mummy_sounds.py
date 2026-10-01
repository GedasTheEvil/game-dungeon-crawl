"""Synthesize the mummy sounds: waking (dry coffin creak, linen rustle, long rasping groan), the swing (cloth whoosh,
effort grunt, dull thud) and the death (fading rasp, crumbling dust, cloth flopping on stone).

    python3 tools/audio/mummy_sounds.py [out_dir]      (default: sounds/monsters/, writes mummy_{wake,att,die}.wav)

Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys
import wave

import numpy as np
from scipy.signal import butter, lfilter, sosfilt

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


def creak(dur, f0, f1, rate0=55, rate1=30):
    """Wooden hinge creak: a train of stick-slip pulses (rate glides rate0 -> rate1) exciting two wood resonances."""
    t = t_axis(dur)
    rate = rate0 * (rate1 / rate0) ** (t / dur)
    ph = np.cumsum(rate) / SR
    pulses = np.zeros(len(t))
    idx = np.nonzero(np.diff(np.floor(ph)) > 0)[0]
    pulses[idx] = 1 + 0.4 * rng.standard_normal(len(idx))
    f = f0 * (f1 / f0) ** (t / dur)
    x = np.zeros(len(t))
    for mult, q in ((1.0, 0.004), (2.7, 0.0025)):  # resonances follow the pitch glide (decaying sines per pulse)
        tone = np.sin(2 * np.pi * np.cumsum(f * mult) / SR)
        ring = lfilter([1.0], [1.0, -np.exp(-1 / (q * SR))], pulses)
        x += tone * ring / mult
    swell = np.sin(np.pi * np.minimum(t / dur, 1.0)) ** 0.6
    return band(x, 250, 5000) * swell


def growl(dur, f0, f1, rough=0.8, bright=2200):
    """Throaty snarl: a buzzy low tone with jittered pitch and amplitude flutter, through a mouth-ish band."""
    t = t_axis(dur)
    jitter = lowpass(rng.standard_normal(len(t)), 30) * 6
    f = f0 * (f1 / f0) ** (t / dur) * (1 + 0.04 * jitter)
    ph = 2 * np.pi * np.cumsum(f) / SR
    x = sum(np.sin(k * ph + 0.3 * k) / k ** 0.8 for k in range(1, 12))
    flutter = 1 + rough * np.sin(2 * np.pi * 27 * t + 3 * lowpass(rng.standard_normal(len(t)), 8))
    breath = band(rng.standard_normal(len(t)), 400, bright) * 0.6
    y = band(x * flutter, 90, bright) + breath * (0.5 + 0.5 * flutter)
    return y


def knock(dur=0.16, f=180, bright=3000):
    """Wood slam: a damped low body tone plus a bright crack."""
    t = t_axis(dur)
    body = np.sin(2 * np.pi * f * t) * env(t, 0.002, 0.035) + 0.5 * np.sin(2 * np.pi * f * 2.3 * t) * env(t, 0.001, 0.02)
    crack = band(rng.standard_normal(len(t)), 900, bright) * env(t, 0.0005, 0.008)
    return body + 0.7 * crack


def rustle(dur, lo=1500, hi=7000, rate=35):
    """Dry linen rustle: bandpassed noise chopped into random scuffs."""
    t = t_axis(dur)
    chop = np.abs(lowpass(rng.standard_normal(len(t)), rate)) ** 1.5
    swell = np.sin(np.pi * np.minimum(t / dur, 1.0)) ** 0.5
    return band(rng.standard_normal(len(t)), lo, hi) * chop * swell


def whoosh(dur, lo, hi):
    """Heavy cloth swinging through air: noise whose band sweeps down as the arm passes."""
    t = t_axis(dur)
    n = rng.standard_normal(len(t))
    x = band(n, lo, hi) * np.sin(np.pi * np.minimum(t / dur, 1.0)) ** 1.5
    return x + 0.5 * lowpass(n, 500) * np.sin(np.pi * np.minimum(t / dur, 1.0)) ** 2


def rasp(dur, f0, f1, dry=0.7):
    """Dry throat groan: a weak low tone with ragged pitch, mostly breathy band noise (no wet buzz)."""
    t = t_axis(dur)
    jitter = lowpass(rng.standard_normal(len(t)), 20) * 5
    f = f0 * (f1 / f0) ** (t / dur) * (1 + 0.06 * jitter)
    ph = 2 * np.pi * np.cumsum(f) / SR
    tone = band(sum(np.sin(k * ph) / k for k in range(1, 8)), 80, 1400)
    flutter = 0.6 + 0.4 * np.sin(2 * np.pi * 19 * t + 4 * lowpass(rng.standard_normal(len(t)), 6))
    air = band(rng.standard_normal(len(t)), 600, 3200) * (0.6 + 0.4 * flutter)
    return tone * flutter + dry * air * 1.4


def thud(dur=0.2, f=70):
    """Dull heavy hit: a low damped tone with a soft muffled body."""
    t = t_axis(dur)
    sweep = np.sin(2 * np.pi * (f + 40 * np.exp(-t / 0.03)) * t) * env(t, 0.003, 0.06)
    body = lowpass(rng.standard_normal(len(t)), 400) * env(t, 0.001, 0.025)
    return sweep + 0.6 * body


def dust(dur, density=220):
    """Crumbling dust: sparse tiny grains of bright noise that thin out."""
    out = np.zeros(int(dur * SR))
    for at in np.sort(rng.random(int(density * dur)) ** 1.6) * dur:
        g = band(rng.standard_normal(int(0.012 * SR)), 1800, 7500) * env(t_axis(0.012), 0.0005, 0.003)
        place(out, g * rng.uniform(0.3, 1.0), at)
    return out


def finish(out):
    b, a = butter(2, 9500, fs=SR)
    out = lfilter(b, a, out)
    fade = int(0.02 * SR)
    out[-fade:] *= np.linspace(1, 0, fade)
    return out / np.max(np.abs(out)) * 10 ** (-3 / 20)


def wake():
    """Wood creaks, a hand knocks the rim, linen rustles as it climbs out, a long dry groan."""
    out = np.zeros(int(1.9 * SR))
    place(out, rms(creak(0.5, 380, 260, 45, 25), 0.07), 0.0)
    place(out, rms(knock(0.12, 140, 2500), 0.12), 0.38)
    place(out, rms(knock(0.1, 170, 2500), 0.09), 0.55)
    place(out, rms(rustle(0.9), 0.07), 0.3)
    place(out, rms(creak(0.3, 500, 380, 60, 35), 0.05), 0.95)
    place(out, rms(knock(0.14, 120, 2500), 0.1), 1.05)
    d = 1.1
    g = rasp(d, 70, 52) * env(t_axis(d), 0.35, 0.55)
    place(out, rms(g, 0.15), 0.7)
    return finish(out)


def attack():
    """Slow wind-up rustle, cloth whoosh with a grunt, then a dull thud on impact."""
    out = np.zeros(int(0.7 * SR))
    place(out, rms(rustle(0.2, 1200, 5000), 0.04), 0.0)
    place(out, rms(whoosh(0.4, 600, 3500), 0.12), 0.12)
    g = rasp(0.28, 85, 62, dry=0.3) * env(t_axis(0.28), 0.05, 0.12)
    place(out, rms(g, 0.16), 0.12)
    place(out, rms(thud(0.25, 70), 0.24), 0.45)
    place(out, rms(rustle(0.15, 1500, 5000), 0.03), 0.5)
    return finish(out)


def die():
    """Rasping exhale fading, the body gives way in a crumble of dust, cloth flops on stone."""
    out = np.zeros(int(1.5 * SR))
    d = 0.8
    g = rasp(d, 80, 40, dry=1.0) * env(t_axis(d), 0.05, 0.3)
    place(out, rms(g, 0.15), 0.0)
    place(out, rms(dust(1.0, 260), 0.06), 0.25)
    place(out, rms(rustle(0.4, 800, 4500, 25), 0.06), 0.35)
    place(out, rms(thud(0.3, 55), 0.2), 0.7)  # it drops to its knees
    place(out, rms(lowpass(rustle(0.35, 600, 3500, 20), 3000), 0.07), 0.75)  # linen flops on stone
    place(out, rms(thud(0.2, 45), 0.1), 0.95)
    place(out, rms(dust(0.5, 120), 0.03), 0.95)
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
    write(os.path.join(out_dir, "mummy_wake.wav"), wake())
    write(os.path.join(out_dir, "mummy_att.wav"), attack())
    write(os.path.join(out_dir, "mummy_die.wav"), die())


if __name__ == "__main__":
    main()
