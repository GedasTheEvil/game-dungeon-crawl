"""Synthesize the tomb-curse containers' sounds (plan tomb-curse-containers): the coffin's lid and the mimic jar.

    python3 tools/audio/tomb_curse_sounds.py [out_dir]      (default: media/sounds/)

Writes items/lid_coffin.wav (WorldSound::LidCoffin: the lid rattles in its seat, scrapes off the case, a heavy wooden
thud as it lands on the floor; timed to the coffin's clip, CONTAINER_DEFS in src/world/containers.h) and the mimic
jar's monsters/mimic_jar_{wake,att,die}.wav: the mimic's voice smaller and higher, in pottery instead of wood (the clay
stopper grinding on the rim, clicking on the fangs; the death a shriek and the jar smashing as the container does).
Everything is generated (no samples), so this script is the source of the sounds. It uses the helpers of
mimic_sounds.py and container_sounds.py. Output: 16-bit PCM mono 22050 Hz.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import container_sounds  # noqa: E402
from mimic_sounds import REPO, SR, band, clack, env, finish, growl, knock, lowpass, place, rms, t_axis, write  # noqa: E402

rng = np.random.default_rng(31)  # fixed seed: the same files on every run


def noise(dur):
    return rng.standard_normal(int(dur * SR))


def scrape(dur, lo, hi, grain=60):
    """Wood or clay dragged over a rim: band noise in stick-slip bursts."""
    t = t_axis(dur)
    bursts = 0.5 + 0.5 * np.sign(np.sin(2 * np.pi * grain * t + 4 * np.sin(2 * np.pi * 3 * t)))
    return band(noise(dur), lo, hi) * (0.35 + 0.65 * bursts) * np.sin(np.pi * t / dur) ** 0.7


def tock(f, dur=0.08):
    """A fired clay knock: a short bright ring."""
    t = t_axis(dur)
    return sum(np.sin(2 * np.pi * f * m * t) / m for m in (1.0, 2.4, 3.9)) * env(t, 0.0005, 0.012)


def lid_coffin():
    out = np.zeros(int(1.2 * SR))
    for at in (0.0, 0.05, 0.1):  # the lid rattles in its seat
        place(out, rms(knock(0.08, rng.uniform(140, 180), 2500), 0.08), at)
    place(out, rms(scrape(0.5, 180, 1400, 45), 0.12), 0.14)  # pushed off over the case's edge
    place(out, rms(knock(0.3, 75, 1500), 0.3), 0.8)  # down on the floor behind
    place(out, rms(lowpass(noise(0.4), 250) * env(t_axis(0.4), 0.003, 0.09), 0.18), 0.8)
    place(out, rms(knock(0.12, 95, 1800), 0.08), 0.95)  # it settles
    return finish(out)


def wake():
    """The stopper grinds on the rim and lifts, a thin wet snarl comes out of the neck."""
    out = np.zeros(int(0.8 * SR))
    place(out, rms(scrape(0.3, 900, 4500, 80), 0.08), 0.0)
    place(out, rms(tock(1300), 0.06), 0.3)
    g = growl(0.45, 150, 200, rough=0.9, bright=3200) * env(t_axis(0.45), 0.12, 0.22)
    place(out, rms(g, 0.15), 0.25)
    return finish(out)


def attack():
    """A hiss as it gapes, the lunge, the stopper snaps down on the fangs: clay on bone."""
    out = np.zeros(int(0.6 * SR))
    hiss = band(noise(0.25), 2000, 6000) * env(t_axis(0.25), 0.04, 0.08)
    place(out, rms(hiss, 0.05), 0.0)
    g = growl(0.3, 170, 230, rough=1.0, bright=3500) * env(t_axis(0.3), 0.04, 0.14)
    place(out, rms(g, 0.15), 0.12)
    place(out, rms(tock(1100), 0.16), 0.36)
    for k, at in enumerate((0.365, 0.41, 0.45)):
        place(out, rms(clack(), 0.09 * (1 - 0.25 * k)), at)
    return finish(out)


def die():
    """A shriek as it shudders, then the jar smashes."""
    out = np.zeros(int(1.4 * SR))
    g = growl(0.45, 230, 110, rough=1.2, bright=4000) * env(t_axis(0.45), 0.02, 0.25)
    place(out, rms(g, 0.16), 0.0)
    for k in range(5):  # the stopper chattering as it shudders
        place(out, rms(tock(rng.uniform(1100, 1500)), 0.05), 0.04 + 0.06 * k)
    place(out, rms(container_sounds.jar_smash(), 0.2), 0.35)
    return finish(out)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds")
    write(os.path.join(out_dir, "items", "lid_coffin.wav"), lid_coffin())
    write(os.path.join(out_dir, "monsters", "mimic_jar_wake.wav"), wake())
    write(os.path.join(out_dir, "monsters", "mimic_jar_att.wav"), attack())
    write(os.path.join(out_dir, "monsters", "mimic_jar_die.wav"), die())


if __name__ == "__main__":
    main()
