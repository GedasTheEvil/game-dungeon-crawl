"""Synthesize Rerek's sounds (plan worm-boss): the worm's bite and death pitched down with a deep rumble under them
(the same mouth, a far bigger body), and his wake, played as he dives and as he bursts up: the ground rumbling, a
gust of sand thrown up, grains ticking down after it.

    python3 tools/audio/rerek_sounds.py [out_dir]     (default: media/sounds/monsters/, writes rerek_{wake,att,die}.wav)

The worm's samples (media/sounds/monsters/worm_{att,die}.wav, IMA ADPCM) are read through ffmpeg; the rest is
generated, so this script is the source of Rerek's sounds.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import subprocess
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from crocodile_sounds import REPO, SR, env, finish, place, rms, rumble, t_axis, write  # noqa: E402
from seth_sounds import grains, gust, pitched  # noqa: E402

MONSTERS = os.path.join(REPO, "media", "sounds", "monsters")
PITCH = 0.72  # of the worm's: slower and deeper


def sample(name):
    """A sound file as floats at SR, mono."""
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", os.path.join(MONSTERS, name), "-f", "s16le", "-ac", "1",
                          "-ar", str(SR), "-"], check=True, capture_output=True).stdout
    return np.frombuffer(raw, "<i2").astype(float) / 32768


def deeper(name, growl):
    """The worm's sound pitched down, a rumble of growl seconds under its start."""
    x = pitched(sample(name), PITCH)
    out = np.zeros(len(x) + int(0.2 * SR))
    place(out, rms(x, 0.2), 0.0)
    place(out, rms(rumble(growl, 52, 38, 0.6) * env(t_axis(growl), 0.05, 0.4 * growl), 0.08), 0.0)
    return finish(out)


def wake():
    """Bursting through the sand: a rumble from below, the gust of sand thrown up, grains raining down."""
    out = np.zeros(int(1.8 * SR))
    place(out, rms(rumble(0.7, 40, 58, 0.5) * env(t_axis(0.7), 0.25, 0.3), 0.14), 0.0)
    place(out, rms(gust(1.1, 200, 1600), 0.12), 0.2)
    place(out, rms(grains(1.3, 120), 0.03), 0.45)
    return finish(out)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else MONSTERS
    write(os.path.join(out_dir, "rerek_wake.wav"), wake())
    write(os.path.join(out_dir, "rerek_att.wav"), deeper("worm_att.wav", 0.5))
    write(os.path.join(out_dir, "rerek_die.wav"), deeper("worm_die.wav", 1.0))


if __name__ == "__main__":
    main()
