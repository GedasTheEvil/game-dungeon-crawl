"""Synthesize the dung ball's sounds (plan scarab-dung-ball): the giant scarab's (and Khepri's) kick and the rolling
rumble of the ball over the stone (scarab_att_ranged.wav, played at the kick), and the wet thud as it breaks on a wall,
the floor or the player (scarab_ball_thud.wav, WorldSound::DungThud).

    python3 tools/audio/scarab_sounds.py [out_dir]      (default: media/sounds/monsters/)

Everything is generated (no samples), so this script is the source of these two; the scarab's bite and death are
older files.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from crocodile_sounds import REPO, SR, band, env, finish, lowpass, place, rms, t_axis, write  # noqa: E402

rng = np.random.default_rng(113)  # fixed seed: the same files on every run


def roll(dur=1.3, turns=3.2):
    """The kick (a scrape of legs, a soft push), then the ball rumbling over the stone: a low lumpy rumble that thumps
    once a turn, grit crunching under it, fading as it rolls away."""
    out = np.zeros(int((dur + 0.1) * SR))
    k = t_axis(0.12)
    place(out, rms(band(rng.standard_normal(len(k)), 1500, 6000) * env(k, 0.004, 0.03), 0.05), 0.0)
    push = t_axis(0.18)
    place(out, rms(lowpass(rng.standard_normal(len(push)), 300) * env(push, 0.01, 0.06), 0.12), 0.02)
    t = t_axis(dur)
    fade = np.clip(t / 0.08, 0, 1) * np.exp(-t / (0.55 * dur))
    lumps = 1 + 0.8 * np.maximum(np.sin(2 * np.pi * turns * t + 0.4 * np.sin(2 * np.pi * 1.3 * t)), 0) ** 6
    body = lowpass(rng.standard_normal(len(t)), 160) * lumps
    grit = band(rng.standard_normal(len(t)), 1800, 5500) * (0.4 + 0.6 * (rng.uniform(size=len(t)) < 0.02))
    place(out, rms(body * fade, 0.16) + rms(grit * fade, 0.025), 0.06)
    return finish(out)


def thud(dur=0.45):
    """A wet thud: a low soft knock, a squelch of the dung giving, a few clods pattering down."""
    out = np.zeros(int(dur * SR))
    t = t_axis(dur)
    knock = (np.sin(2 * np.pi * 70 * t) + 0.4 * np.sin(2 * np.pi * 140 * t)) * env(t, 0.003, 0.05)
    f = 900 * np.exp(-t / 0.05) + 250
    squelch = band(rng.standard_normal(len(t)), 250, 1800) * env(t, 0.006, 0.05) * (1 + 0.5 * np.sin(2 * np.pi * np.cumsum(f) / SR))
    place(out, rms(knock, 0.2) + rms(squelch, 0.1), 0.0)
    for _ in range(6):
        d = t_axis(0.03)
        clod = lowpass(rng.standard_normal(len(d)), 900) * env(d, 0.002, 0.008)
        place(out, rms(clod, rng.uniform(0.02, 0.05)), rng.uniform(0.08, 0.35))
    return finish(out)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds", "monsters")
    write(os.path.join(out_dir, "scarab_att_ranged.wav"), roll())
    write(os.path.join(out_dir, "scarab_ball_thud.wav"), thud())


if __name__ == "__main__":
    main()
