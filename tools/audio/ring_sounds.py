"""Synthesize the sounds of the ring transporter and the Stargate chamber.

    python3 tools/audio/ring_sounds.py [out_dir]      (default: media/sounds/)

Writes mechanisms/rings_down.wav (a ring platform: five metal whooshes as the rings drop one after another, each
landing with a soft clang, a rising hum under them, a bright zing into the white flash; plan ring-transporter),
rings_up.wav (at the partner: the zing dying out, the five whooshes going up, the hum falling away),
chevron_lock.wav (the Stargate's inner ring turns with a stone rumble, stops, a heavy clunk and a short power swell
as a chevron locks; plan stargate-chamber), chevron_fail.wav (the seventh: the clunk does not catch, the ring grinds
to a halt, the power dies in a falling whine) and dial_press.wav (the dial device's red dome: a dull clunk, a crackle,
nothing).
Everything is generated (no samples) and made to evoke the rings and the gate, not ripped; this script is the source.
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

# The engine's timings (src/core/gameplay_config.h, RING_*): the rings drop RING_STAGGER s apart, RING_FALL s each.
RING_COUNT = 5
RING_STAGGER = 0.15
RING_FALL = 0.4
RING_HOLD = 0.25
CHEVRON_MS = 0.7  # STARGATE_CHEVRON_MS: one chevron's turn and lock


def t_axis(duration):
    return np.arange(int(duration * SR)) / SR


def band(x, lo, hi, order=2):
    return sosfilt(butter(order, [lo, hi], btype="band", fs=SR, output="sos"), x)


def lowpass(x, hi, order=2):
    return sosfilt(butter(order, hi, fs=SR, output="sos"), x)


def place(out, clip, at, gain=1.0):
    i = int(at * SR)
    n = min(len(clip), len(out) - i)
    if n > 0:
        out[i:i + n] += gain * clip[:n]


def env_ad(t, attack, decay):
    return (1 - np.exp(-t / attack)) * np.exp(-t / decay)


def chirp(f_start, f_end, dur, curve=1.0):
    t = t_axis(dur)
    k = (t / dur) ** curve
    f = f_start + (f_end - f_start) * k
    return np.sin(2 * np.pi * np.cumsum(f) / SR), t


def ring(freqs, dur, decay):
    t = t_axis(dur)
    return sum(np.sin(2 * np.pi * f * t) * np.exp(-t / (decay * (1 + i * 0.3))) / (1 + i)
               for i, f in enumerate(freqs)), t


def normalize(x, db=-3.0, fade=0.02):
    n = int(fade * SR)
    x = x.copy()
    x[-n:] *= np.linspace(1, 0, n)
    return x / np.max(np.abs(x)) * 10 ** (db / 20)


def whoosh(dur, lo, hi, rising):
    """A ring rushing through the air: band noise swelling and dying, its band sliding up (rising) or down."""
    t = t_axis(dur)
    noise = rng.standard_normal(len(t))
    a = band(noise, lo, hi)
    b = band(noise, lo * 1.8, hi * 1.6)
    k = t / dur if rising else 1 - t / dur
    return (a * (1 - k) + b * k) * np.sin(np.pi * t / dur) ** 2


def clang(f0):
    """A ring settling on the one below: a soft metallic clang, inharmonic partials."""
    s, t = ring((f0, f0 * 2.76, f0 * 5.4), 0.6, 0.12)
    return s * (1 - np.exp(-t / 0.002))


def hum(dur, f0, f1):
    """The power of the rings: a low hum with a beating overtone, sliding from f0 to f1."""
    s, t = chirp(f0, f1, dur, 1.0)
    s2, _ = chirp(f0 * 2.01, f1 * 2.01, dur, 1.0)
    s3, _ = chirp(f0 * 3.0, f1 * 3.0, dur, 1.0)
    return s + 0.5 * s2 + 0.25 * s3 * (1 + 0.5 * np.sin(2 * np.pi * 6 * t))


def rings_down():
    drop = (RING_COUNT - 1) * RING_STAGGER + RING_FALL
    dur = drop + RING_HOLD + 0.6
    out = np.zeros(int(dur * SR))
    for k in range(RING_COUNT):
        at = k * RING_STAGGER
        place(out, whoosh(RING_FALL, 500, 2400, False), at, 0.45)
        place(out, clang(620 + 40 * (RING_COUNT - 1 - k)), at + RING_FALL * 0.92, 0.35)
    t = t_axis(drop + RING_HOLD)
    h = hum(drop + RING_HOLD, 70, 140) * np.clip(t / drop, 0, 1) ** 1.5
    place(out, h, 0.0, 0.5)
    zing, tz = chirp(900, 4200, 0.3, 2.0)  # into the flash
    place(out, zing * np.sin(np.pi * tz / 0.3) ** 0.7, drop + RING_HOLD - 0.15, 0.55)
    flash, tf = ring((2100, 3150, 4200), 0.5, 0.1)
    place(out, flash * (1 - np.exp(-tf / 0.003)), drop + RING_HOLD + 0.1, 0.4)
    return normalize(out, -2)


def rings_up():
    rise = (RING_COUNT - 1) * RING_STAGGER + RING_FALL
    dur = rise + 0.5
    out = np.zeros(int(dur * SR))
    flash, tf = ring((2100, 3150, 4200), 0.45, 0.12)
    place(out, flash, 0.0, 0.35)
    for k in range(RING_COUNT):
        place(out, whoosh(RING_FALL, 500, 2400, True), k * RING_STAGGER, 0.45)
    t = t_axis(rise)
    place(out, hum(rise, 140, 60) * (1 - t / rise) ** 1.5, 0.0, 0.5)
    place(out, clang(520), rise, 0.2)  # the last ring home in the ceiling
    return normalize(out, -3)


def rumble(dur, lo=40, hi=320):
    """Stone turning on stone: low band noise with a slow grinding wobble."""
    t = t_axis(dur)
    r = band(rng.standard_normal(len(t)), lo, hi) * (1 + 0.5 * np.sin(2 * np.pi * 9 * t))
    grit = band(rng.standard_normal(len(t)), 1500, 4000) * (rng.random(len(t)) < 0.02)
    return r + 0.25 * grit


def clunk(f0=90, gain=1.0):
    """A chevron locking: a heavy thud with a metal edge."""
    t = t_axis(0.35)
    body = np.sin(2 * np.pi * f0 * t * (1 - 0.3 * t)) * env_ad(t, 0.002, 0.07)
    edge = band(rng.standard_normal(len(t)), 800, 3500) * env_ad(t, 0.001, 0.02)
    return gain * (body + 0.5 * edge)


def chevron_lock():
    turn = CHEVRON_MS * 0.6
    out = np.zeros(int((CHEVRON_MS + 0.5) * SR))
    t = t_axis(turn)
    place(out, rumble(turn) * np.sin(np.pi * t / turn) ** 0.5, 0.0, 0.8)
    place(out, clunk(), turn, 1.0)
    swell, ts = chirp(110, 220, 0.4, 0.5)
    place(out, swell * env_ad(ts, 0.05, 0.15) * (1 + 0.3 * np.sin(2 * np.pi * 30 * ts)), turn + 0.02, 0.5)
    return normalize(out, -3)


def chevron_fail():
    dur = 2.2
    out = np.zeros(int(dur * SR))
    turn = CHEVRON_MS * 0.6
    t = t_axis(turn + 0.5)
    slow = np.clip(1 - (t - turn) / 0.5, 0, 1)
    place(out, rumble(turn + 0.5) * np.where(t < turn, np.sin(np.pi * np.minimum(t, turn) / (2 * turn)) ** 0.5, slow), 0.0, 0.8)
    place(out, clunk(70, 0.6), turn, 1.0)  # it does not catch
    place(out, clunk(60, 0.3), turn + 0.14, 1.0)
    whine, tw = chirp(420, 60, 1.4, 0.6)
    place(out, whine * np.exp(-tw / 0.6) * (1 + 0.4 * np.sin(2 * np.pi * 13 * tw)), turn + 0.2, 0.45)
    return normalize(out, -3)


def dial_press():
    out = np.zeros(int(0.8 * SR))
    place(out, clunk(130, 1.0), 0.0, 1.0)
    t = t_axis(0.4)
    crackle = band(rng.standard_normal(len(t)), 2000, 6000) * (rng.random(len(t)) < 0.03) * np.exp(-t / 0.15)
    place(out, crackle, 0.05, 0.6)
    buzz, tb = chirp(100, 80, 0.3, 1.0)
    place(out, buzz * np.exp(-tb / 0.08), 0.03, 0.3)
    return normalize(out, -4)


SOUNDS = {
    "mechanisms/rings_down": rings_down, "mechanisms/rings_up": rings_up,
    "mechanisms/chevron_lock": chevron_lock, "mechanisms/chevron_fail": chevron_fail,
    "mechanisms/dial_press": dial_press,
}


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds")
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
