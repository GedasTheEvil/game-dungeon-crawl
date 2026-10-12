"""Synthesize the Anubis's ranged attack (plan anubis-ranged-attack): its grunt, then the bolt of light leaving the
was-sceptre: a low hum rising in pitch with a faint glassy shimmer as the sceptre swings, a short dry crack at the
release, a falling hiss and buzz as the bolt flies off. Short and dry, not the teleporter's smooth swell.

    python3 tools/audio/anubis_sounds.py [out_dir]      (default: sounds/monsters/, writes anubis_att_ranged.wav)

The magic is generated (no samples), so this script is its source. The grunt is the sample sounds/monsters/anubis_att.wav
(IMA ADPCM, which Python's wave cannot read: ffmpeg decodes it), mixed in at 70% from the start.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import subprocess
import sys
import wave

import numpy as np
from scipy.signal import butter, sosfilt

SR = 22050
REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
rng = np.random.default_rng(29)  # fixed seed: the same file on every run

# The sound starts with the swing (Monster::AttackRanged); the bolt leaves at frame 12 of 26, at 14 frames a second
# (ModelInfo::CLIP_SPEED): the crack lands there.
RELEASE_S = 12 / 14
CHARGE_S = 0.3
GRUNT_GAIN = 0.7


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


def sweep(f0, f1, dur, curve=1.0):
    """A sine gliding from f0 to f1 Hz; curve > 1 holds low and rises late."""
    t = t_axis(dur)
    f = f0 + (f1 - f0) * (t / dur) ** curve
    return np.sin(2 * np.pi * np.cumsum(f) / SR), t


def charge(dur):
    """The sceptre gathers light: a low hum rising in pitch (a buzzy fifth on top), a faint glassy shimmer."""
    hum, t = sweep(70, 260, dur, 1.6)
    fifth, _ = sweep(105, 390, dur, 1.6)
    buzz = np.tanh(2.5 * (hum + 0.5 * fifth))  # a little grit: the hum is not a clean sine
    swell = (t / dur) ** 1.5
    glass = np.zeros(len(t))
    for f in (2350, 3130, 4170):
        glass += np.sin(2 * np.pi * f * t + rng.uniform(0, 2 * np.pi)) * (1 + 0.5 * np.sin(2 * np.pi * rng.uniform(9, 14) * t))
    return buzz * swell + 0.12 * glass * swell ** 2


def crack(dur=0.06):
    """The release: a short dry crack, a spark or a snapped reed."""
    t = t_axis(dur)
    snap = band(rng.standard_normal(len(t)), 1800, 9000) * env(t, 0.0003, 0.006)
    knock = np.sin(2 * np.pi * 900 * t * (1 - 3 * t)) * env(t, 0.0004, 0.01)
    return snap + 0.5 * knock


def tail(dur):
    """The bolt flies off: a hiss and a buzz falling in pitch and fading."""
    t = t_axis(dur)
    k = t / dur
    noise = rng.standard_normal(len(t))
    hiss = band(noise, 2500, 7500) * (1 - k) ** 2 + band(noise, 1200, 3000) * k * (1 - k)
    buzz, _ = sweep(260, 90, dur, 0.7)
    return hiss + 0.5 * np.tanh(3 * buzz) * (1 - k) ** 2.5


def grunt():
    """The Anubis's attack grunt (anubis_att.wav), decoded by ffmpeg."""
    pcm = subprocess.run(["ffmpeg", "-v", "error", "-i", os.path.join(REPO, "sounds", "monsters", "anubis_att.wav"),
                          "-f", "s16le", "-ac", "1", "-ar", str(SR), "-"], check=True, capture_output=True).stdout
    return np.frombuffer(pcm, dtype="<i2").astype(np.float64) / 32768


def att_ranged():
    """The grunt at 70%, the charge rising into the release, the crack, the bolt's tail."""
    voice = grunt()
    tail_s = 0.32
    out = np.zeros(max(len(voice), int((RELEASE_S + tail_s + 0.05) * SR)))
    magic = np.zeros(len(out))
    place(magic, rms(charge(CHARGE_S), 0.2), RELEASE_S - CHARGE_S)
    place(magic, rms(crack(), 0.2), RELEASE_S)
    place(magic, rms(tail(tail_s), 0.15), RELEASE_S + 0.01)
    magic = lowpass(magic, 10000)
    magic *= 10 ** (-3 / 20) / np.max(np.abs(magic))
    out += magic
    place(out, voice * GRUNT_GAIN / np.max(np.abs(voice)) * 10 ** (-3 / 20), 0.0)
    fade = int(0.03 * SR)
    out[-fade:] *= np.linspace(1, 0, fade)
    peak = np.max(np.abs(out))
    return out / peak * 10 ** (-3 / 20) if peak > 10 ** (-3 / 20) else out


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
    write(os.path.join(out_dir, "anubis_att_ranged.wav"), att_ranged())


if __name__ == "__main__":
    main()
