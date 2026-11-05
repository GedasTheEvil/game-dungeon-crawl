"""Synthesize the prayers at the gods' shrines (plans shrine-of-isis, shrines-of-the-gods): a sistrum shaken, then a
soft chime.

    python3 tools/audio/shrine_sounds.py [out_dir]      (default: media/sounds/)

Writes mechanisms/shrine_pray.wav (WorldSound::ShrinePray, Isis): the sistrum's loose bronze discs jingle on their rods
in three shakes, each a cluster of small metal rings, then a low bell-like chime swells and fades under them. Then
shrine_pray_<god>.wav for Montu, Ptah, Amun and Serket (WorldSound::ShrinePrayMontu, ...): the same at another pitch,
interval and pace (GODS). Everything is generated (no samples), so this script is the source of the sound. Output:
16-bit PCM mono 22050 Hz.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mimic_sounds import REPO, SR, band, env, finish, place, rms, t_axis, write  # noqa: E402

rng = np.random.default_rng(47)  # fixed seed: the same file on every run


# Per god: the chime and the note over it, the discs' pitch range, the shakes' starts.
GODS = {
    "": (523.25, 784.0, (2600, 4800), (0.0, 0.28, 0.56)),  # Isis: C5 and a fifth over it
    "_montu": (392.0, 587.33, (1800, 3400), (0.0, 0.18, 0.36, 0.54)),  # G4, quick martial shakes, low discs
    "_ptah": (440.0, 587.33, (2200, 4000), (0.0, 0.32, 0.64)),  # A4 and a fourth, measured
    "_amun": (261.63, 523.25, (2000, 3600), (0.0, 0.4, 0.8)),  # C4 and its octave, slow and deep
    "_serket": (659.26, 784.0, (3500, 6000), (0.0, 0.2, 0.4)),  # E5 and a minor third, high dry discs
}


def disc(f, dur=0.18):
    """One small bronze disc on its rod: inharmonic partials, a quick decay."""
    t = t_axis(dur)
    return sum(np.sin(2 * np.pi * f * m * t + rng.uniform(0, 6)) * np.exp(-t / (0.05 / m)) / m for m in (1.0, 1.59, 2.31, 3.17))


def shake(dur=0.22, discs=(2600, 4800)):
    """A shake: the discs slide along the rods and strike, a dense cluster with a rattle of noise."""
    out = np.zeros(int((dur + 0.2) * SR))
    for _ in range(26):
        at = dur * rng.random() ** 1.6
        place(out, rms(disc(rng.uniform(*discs)), 0.04 * rng.uniform(0.5, 1.0)), at)
    rattle = band(rng.standard_normal(int(dur * SR)), 3000, 8000) * env(t_axis(dur), 0.01, 0.06)
    place(out, rms(rattle, 0.03), 0.0)
    return out


def chime(f=523.25, dur=1.6):
    """A soft bell: a slow swell, harmonic partials with a slight shimmer."""
    t = t_axis(dur)
    tone = sum(np.sin(2 * np.pi * f * m * t * (1 + 0.002 * np.sin(2 * np.pi * 5 * t))) / m ** 1.5 for m in (1.0, 2.0, 3.0, 4.2))
    return tone * env(t, 0.08, 0.55)


def pray(f, over, discs, shakes):
    out = np.zeros(int(2.0 * SR))
    for k, at in enumerate(shakes):
        place(out, rms(shake(discs=discs), 0.12 * (1 - 0.15 * k)), at)
    place(out, rms(chime(f), 0.09), 0.3)
    place(out, rms(chime(over, 1.3), 0.04), 0.45)  # the note over it
    return finish(out)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds")
    for suffix, args in GODS.items():  # Isis first: her file as before the other gods
        write(os.path.join(out_dir, "mechanisms", "shrine_pray" + suffix + ".wav"), pray(*args))


if __name__ == "__main__":
    main()
