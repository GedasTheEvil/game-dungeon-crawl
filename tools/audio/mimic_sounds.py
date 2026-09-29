"""Synthesize the mimic sounds: waking (wood creak + wet snarl), the bite (lid slam, teeth clack, growl) and the death
(shuddering groan, rattling wood, the lid thudding shut, a trickle of coins).

    python3 tools/audio/mimic_sounds.py [out_dir]      (default: sounds/monsters/, writes mimic_{wake,att,die}.wav)

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
rng = np.random.default_rng(23)  # fixed seed: the same files on every run


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


def clack(dur=0.05):
    """Teeth clacking together: short bright clicks."""
    t = t_axis(dur)
    return band(rng.standard_normal(len(t)), 2500, 8000) * env(t, 0.0003, 0.004)


def coin(dur=0.25, f=None):
    t = t_axis(dur)
    f = f or rng.uniform(3200, 5200)
    return sum(np.sin(2 * np.pi * f * m * t) * np.exp(-t / (0.06 / m)) for m in (1.0, 1.47, 2.09)) * env(t, 0.0005, 1.0)


def finish(out):
    b, a = butter(2, 9500, fs=SR)
    out = lfilter(b, a, out)
    fade = int(0.02 * SR)
    out[-fade:] *= np.linspace(1, 0, fade)
    return out / np.max(np.abs(out)) * 10 ** (-3 / 20)


def wake():
    """The lid creaks, the wood groans, a wet snarl rises out of the box."""
    out = np.zeros(int(1.0 * SR))
    place(out, rms(creak(0.55, 520, 330), 0.09), 0.0)
    place(out, rms(creak(0.25, 700, 600, 70, 60), 0.05), 0.42)
    g = growl(0.6, 85, 120, rough=0.9) * env(t_axis(0.6), 0.18, 0.3)
    place(out, rms(g, 0.16), 0.34)
    return finish(out)


def attack():
    """Gape creak, lunge snarl, the lid slams on the teeth, a rattle of clacks."""
    out = np.zeros(int(0.7 * SR))
    place(out, rms(creak(0.18, 600, 420, 80, 50), 0.05), 0.0)
    g = growl(0.42, 110, 150, rough=1.0, bright=3000) * env(t_axis(0.42), 0.06, 0.2)
    place(out, rms(g, 0.17), 0.08)
    place(out, rms(knock(0.18, 150), 0.2), 0.33)
    for k, at in enumerate((0.335, 0.4, 0.45, 0.49)):
        place(out, rms(clack(), 0.1 * (1 - 0.2 * k)), at)
    place(out, rms(knock(0.1, 210), 0.07), 0.47)
    return finish(out)


def die():
    """A shuddering groan with the box rattling, the lid thuds shut, coins trickle back, a last creak."""
    out = np.zeros(int(1.6 * SR))
    d = 0.7
    g = growl(d, 120, 55, rough=1.2) * env(t_axis(d), 0.03, 0.35)
    place(out, rms(g, 0.16), 0.0)
    for k in range(9):  # rattling while it shudders
        place(out, rms(knock(0.06, rng.uniform(220, 320), 4000), 0.05 * (1 - k / 12)), 0.05 + 0.065 * k + rng.uniform(-0.01, 0.01))
    place(out, rms(knock(0.25, 120), 0.22), 0.72)  # the lid slams shut
    for k in range(14):  # gold settling under the lid
        at = 0.85 + 0.02 * k + 0.25 * rng.random() ** 2
        place(out, rms(lowpass(coin(), 5000), 0.02 * (1 - k / 18)), at)
    place(out, rms(creak(0.4, 420, 300, 40, 22), 0.05), 1.15)  # the dead lid falls open
    place(out, rms(knock(0.12, 160), 0.06), 1.5)
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
    write(os.path.join(out_dir, "mimic_wake.wav"), wake())
    write(os.path.join(out_dir, "mimic_att.wav"), attack())
    write(os.path.join(out_dir, "mimic_die.wav"), die())


if __name__ == "__main__":
    main()
