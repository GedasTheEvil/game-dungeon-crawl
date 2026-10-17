"""Synthesize the treasure containers' opening sounds (plan treasure-containers, WorldSound::Lid* and JarSmash).

    python3 tools/audio/container_sounds.py [out_dir]      (default: media/sounds/)

Writes items/lid_wood.wav (the chest's and the coffer's lid: a latch click, a creaking hinge, a soft knock as it comes
to rest), lid_reed.wav (the basket's lid and the offering table's linen cover: a dry rustle and a light tap),
lid_stone.wav (the canopic jars' stone heads: four gritty scrapes and clinks, one per head), jar_smash.wav (a sealed
pottery jar breaks: a hollow crack, the shards' clatter, a puff of dust). Timed to the clips (CONTAINER_DEFS in
src/world/containers.h). Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, like mechanism_sounds.py, whose helpers it uses.
"""

import os
import sys
import wave

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mechanism_sounds import REPO, SR, band, chirp, env_ad, lowpass, normalize, place, ring, t_axis  # noqa: E402

rng = np.random.default_rng(23)  # fixed seed: the same files on every run


def noise(dur):
    return rng.standard_normal(int(dur * SR))


def creak(f0, f1, dur, grit=0.35):
    """A wooden hinge: a rough, wavering tone sliding from f0 to f1."""
    s, t = chirp(f0, f1, dur, 1.0)
    s *= 1 + 0.5 * np.sign(np.sin(2 * np.pi * 38 * t + 3 * np.sin(2 * np.pi * 5 * t)))
    s = band(s * (1 + rng.standard_normal(len(t)) * grit), 200, 2200)
    return s * np.sin(np.pi * t / dur) ** 0.8


def lid_wood():
    out = np.zeros(int(0.85 * SR))
    click, _ = ring((2100, 3300), 0.06, 0.008)
    place(out, click, 0.0, 0.5)
    place(out, creak(520, 300, 0.45), 0.06, 0.55)
    t = t_axis(0.25)
    knock = lowpass(noise(0.25), 300) * env_ad(t, 0.002, 0.04)
    place(out, knock, 0.6, 2.0)
    return normalize(out, -5)


def lid_reed():
    out = np.zeros(int(0.75 * SR))
    t = t_axis(0.5)
    rustle = band(noise(0.5), 2500, 8000) * (0.4 + 0.6 * (rng.random(len(t)) > 0.6)) * np.sin(np.pi * t / 0.5) ** 1.5
    place(out, rustle, 0.02, 0.5)
    t = t_axis(0.15)
    tap = band(noise(0.15), 300, 1800) * env_ad(t, 0.002, 0.025)
    place(out, tap, 0.5, 1.2)
    return normalize(out, -7)


def lid_stone():
    out = np.zeros(int(1.05 * SR))
    for k in range(4):  # the heads one after the other (canopic: 0.12 of the clip apart)
        at = 0.02 + 0.11 * k
        t = t_axis(0.12)
        scrape = band(noise(0.12), 900, 5000) * np.sin(np.pi * t / 0.12) ** 2
        place(out, scrape, at, 0.35)
        clink, _ = ring((rng.uniform(1700, 2100), rng.uniform(2900, 3400)), 0.18, 0.03)
        place(out, clink, at + 0.5, 0.3)
        t = t_axis(0.12)
        place(out, lowpass(noise(0.12), 350) * env_ad(t, 0.002, 0.02), at + 0.5, 0.8)
    return normalize(out, -6)


def jar_smash():
    dur = 1.0
    out = np.zeros(int(dur * SR))
    t = t_axis(0.4)
    crack = band(noise(0.4), 500, 4500) * env_ad(t, 0.0008, 0.03)
    hollow = np.sin(2 * np.pi * 180 * t) * env_ad(t, 0.002, 0.06)
    place(out, crack * 1.5 + hollow, 0.0, 1.0)
    place(out, lowpass(noise(0.4), 200, 4) * env_ad(t, 0.002, 0.08) * 6, 0.0, 1.0)
    tt = 0.04
    while tt < 0.7:  # the shards clatter down
        shard, _ = ring((rng.uniform(1400, 3200), rng.uniform(3500, 6000)), 0.06, 0.012)
        place(out, shard, tt, rng.uniform(0.15, 0.5) * (1 - tt))
        tt += rng.exponential(0.03)
    t = t_axis(0.6)
    dust = band(noise(0.6), 1200, 6000) * env_ad(t, 0.05, 0.18)
    place(out, dust, 0.05, 0.25)
    return normalize(out, -3)


SOUNDS = {"items/lid_wood": lid_wood, "items/lid_reed": lid_reed, "items/lid_stone": lid_stone, "items/jar_smash": jar_smash}


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
