"""Synthesize the living ushabti sounds: the wake (glaze crackling as the folded arms come free, a gritty scrape off
its place, a clay knock as it lands), the attack (the hoe's swish, a hollow faience knock) and the death (the figure
shattering: a crack, a burst of shards ringing like pottery, pieces knocking down and tinkling to rest). The clicks and
scrapes come from scorpion_sounds.py.

    python3 tools/audio/ushabti_sounds.py [out_dir]  (default: media/sounds/monsters/, writes ushabti_{wake,att,die}.wav)

Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from scorpion_sounds import REPO, SR, band, clack, env, finish, lowpass, place, rms, scrape, t_axis, whip, write  # noqa: E402

rng = np.random.default_rng(28)  # fixed seed: the same files on every run


def knock(dur=0.12, pitch=700, ring=0.04):
    """Glazed clay struck: a dull thud with the hollow ring of a fired pot (two damped partials)."""
    t = t_axis(dur)
    body = sum(a * np.sin(2 * np.pi * pitch * k * t) for k, a in ((1.0, 1.0), (2.37, 0.5), (3.9, 0.25)))
    thud = lowpass(rng.standard_normal(len(t)), 900) * env(t, 0.001, 0.015)
    return body * env(t, 0.0008, ring) + 1.5 * thud


def tink(pitch):
    """A small shard landing: a high glassy ping."""
    t = t_axis(0.12)
    return np.sin(2 * np.pi * pitch * t) * env(t, 0.0003, 0.025) + 0.3 * np.sin(2 * np.pi * pitch * 2.7 * t) * env(t, 0.0003, 0.01)


def crackle(dur, count):
    """Glaze crazing: tiny sharp crack ticks scattered over dur."""
    out = np.zeros(int(dur * SR))
    for at in np.sort(rng.uniform(0, dur * 0.9, count)):
        place(out, clack(0.02, rng.uniform(3500, 6000), 0.4) * rng.uniform(0.3, 1.0), at)
    return out


def wake():
    out = np.zeros(int(0.9 * SR))
    place(out, rms(crackle(0.35, 18), 0.06), 0.0)
    place(out, rms(scrape(0.3, 22) * env(t_axis(0.3), 0.03, 0.12), 0.05), 0.3)
    place(out, rms(knock(0.15, 520, 0.05), 0.12), 0.62)
    return finish(out)


def attack():
    out = np.zeros(int(0.7 * SR))
    place(out, rms(whip(0.2, 500, 1800), 0.04), 0.08)
    place(out, rms(knock(0.16, 620, 0.06), 0.14), 0.3)
    place(out, rms(crackle(0.12, 4), 0.02), 0.32)
    return finish(out)


def die():
    out = np.zeros(int(1.6 * SR))
    place(out, rms(clack(0.1, 2400, 1.5) + 0.6 * knock(0.1, 800, 0.03), 0.16), 0.0)  # the crack
    burst = band(rng.standard_normal(int(0.25 * SR)), 1500, 8000) * env(t_axis(0.25), 0.002, 0.06)
    place(out, rms(burst, 0.07), 0.02)
    for at, pitch, level in ((0.3, 560, 0.10), (0.42, 640, 0.08), (0.55, 480, 0.09), (0.75, 600, 0.05)):  # pieces down
        place(out, rms(knock(0.12, pitch, 0.03), level), at)
    for at in np.sort(rng.uniform(0.15, 1.3, 14)):  # the shards
        place(out, rms(tink(rng.uniform(2800, 5200)), rng.uniform(0.01, 0.035)), at)
    return finish(out)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds", "monsters")
    write(os.path.join(out_dir, "ushabti_wake.wav"), wake())
    write(os.path.join(out_dir, "ushabti_att.wav"), attack())
    write(os.path.join(out_dir, "ushabti_die.wav"), die())


if __name__ == "__main__":
    main()
