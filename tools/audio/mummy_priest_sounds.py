"""Synthesize the mummy priest's sounds (the mummy boss, "High Priest of Anubis"): deeper and more voiced than the
mummy's (tools/audio/mummy_sounds.py, whose creaks, knocks, rustles and thuds this script reuses), with a formant
voice for his words.

    python3 tools/audio/mummy_priest_sounds.py [out_dir]
        (default: media/sounds/monsters/, writes mummy_priest_{wake,att,att_ranged,die}.wav)

  wake        the heavy coffin creaks, knocks on the rim, linen, then a deep groan that rises into a spoken word,
  att         the staff swung (a long low whoosh), a deep voiced "hah", the staff's wooden crack and a thud,
  att_ranged  a curse spoken over a rising hum (four syllables, chanted on one low note), a dark crack at the release
              (frame 12 of the 26-frame attack clip at 14 frames a second, as the Anubis's), a falling purple hiss,
  die         a long voiced moan sinking and breaking up, the staff clattering on the stone, dust, linen, a thud.
Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, which SDL_mixer's Mix_LoadWAV reads directly.
"""

import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mummy_sounds as ms  # noqa: E402
from mummy_sounds import SR, band, creak, dust, env, finish, knock, lowpass, place, rms, rustle, t_axis, thud, whoosh, write  # noqa: E402

REPO = ms.REPO
ms.rng = np.random.default_rng(73)  # the shared helpers draw from this one; fixed seed: the same files on every run
rng = ms.rng

RELEASE_S = 12 / 14  # the bolt leaves at frame 12 of the attack clip (mummy_priest.py --measure), 14 frames a second

# Vowel formants (Hz, bandwidth Hz, gain) of a deep male voice.
VOWELS = {
    "a": ((700, 110, 1.0), (1100, 130, 0.5), (2450, 180, 0.18)),
    "o": ((480, 90, 1.0), (820, 110, 0.45), (2600, 200, 0.12)),
    "u": ((320, 80, 1.0), (760, 110, 0.3), (2300, 200, 0.08)),
    "e": ((520, 90, 1.0), (1750, 150, 0.35), (2500, 200, 0.15)),
    "i": ((300, 70, 1.0), (2100, 160, 0.3), (2900, 220, 0.15)),
}
CONSONANTS = {"k": (1400, 4200, 0.035), "t": (2600, 6500, 0.025), "s": (4000, 9000, 0.09), "h": (500, 3000, 0.08),
              "m": None, "n": None, "r": None}


def glottal(f):
    """Voice source for the pitch contour f (Hz per sample): a buzzy pulse train with jitter and shimmer, plus breath."""
    n = len(f)
    jitter = 1 + 0.02 * lowpass(rng.standard_normal(n), 25) * 4
    ph = 2 * np.pi * np.cumsum(f * jitter) / SR
    src = sum(np.sin(k * ph) / k ** 1.1 for k in range(1, 40) if k * np.max(f) < SR / 2.2)
    shimmer = 1 + 0.15 * lowpass(rng.standard_normal(n), 12) * 4
    breath = band(rng.standard_normal(n), 300, 5000) * 0.25
    return src * shimmer + breath


def formant(src, vowel):
    return sum(g * band(src, max(40, f - bw / 2), f + bw / 2) for f, bw, g in VOWELS[vowel])


def speak(syllables, f0, f1, dur, growl=0.0):
    """Spoken / chanted words: syllables = [(consonant or "", vowel, share of dur)], pitch gliding f0 -> f1. The voice
    source runs through each vowel's formants under a smooth syllable envelope; consonants are noise bursts (m, n, r:
    a hummed, darker start). growl adds a slow amplitude rasp (the dry throat)."""
    t = t_axis(dur)
    f = f0 * (f1 / f0) ** (t / dur)
    src = glottal(f)
    if growl:
        src *= 1 + growl * np.sin(2 * np.pi * 31 * t + 3 * lowpass(rng.standard_normal(len(t)), 7))
    out = np.zeros(len(t))
    at = 0.0
    total = sum(s for _c, _v, s in syllables)
    for cons, vowel, share in syllables:
        d = dur * share / total
        i0, i1 = int(at * SR), min(len(t), int((at + d) * SR))
        w = np.zeros(len(t))
        tt = np.arange(i1 - i0) / SR
        w[i0:i1] = np.minimum(tt / 0.04, 1.0) * np.minimum((d - tt) / 0.06, 1.0).clip(0, 1) ** 0.7
        hum_start = cons in ("m", "n", "r")
        if hum_start:  # nasal: the vowel's first part muffled
            w2 = w * (1 - np.clip(tt_full(len(t), i0, 0.07), 0, 1))
            out += lowpass(src, 400) * w2 * 0.8
            w = w * np.clip(tt_full(len(t), i0, 0.07), 0, 1)
        out += formant(src, vowel) * w
        if CONSONANTS.get(cons):
            lo, hi, cd = CONSONANTS[cons]
            burst = band(rng.standard_normal(int(cd * SR)), lo, hi) * env(t_axis(cd), 0.003, cd / 3)
            place(out, burst * 0.6 * np.sqrt(np.mean(formant(src[i0:i1], vowel) ** 2) + 1e-9) / (np.sqrt(np.mean(burst ** 2)) + 1e-9),
                  max(0.0, at - cd * 0.6))
        at += d
    return out


def tt_full(n, i0, ramp):
    """0 before sample i0, then rising linearly to 1 over ramp seconds."""
    x = (np.arange(n) - i0) / (ramp * SR)
    return np.where(np.arange(n) < i0, 0.0, x)


def hum(dur, f0, f1, curve=1.6):
    """The curse gathering in the staff: a low drone rising in pitch, a buzzy fifth and a dark glassy shimmer."""
    t = t_axis(dur)
    f = f0 + (f1 - f0) * (t / dur) ** curve
    ph = 2 * np.pi * np.cumsum(f) / SR
    drone = np.tanh(2.2 * (np.sin(ph) + 0.6 * np.sin(1.5 * ph) + 0.3 * np.sin(0.5 * ph)))
    glass = sum(np.sin(2 * np.pi * g * t + rng.uniform(0, 2 * np.pi)) * (1 + 0.5 * np.sin(2 * np.pi * rng.uniform(5, 9) * t))
                for g in (1650, 2210, 2930))
    swell = (t / dur) ** 1.2
    return lowpass(drone, 2500) * swell + 0.08 * glass * swell ** 2


def dark_crack(dur=0.09):
    """The release: a low, dry crack with a short body (heavier than the Anubis's spark)."""
    t = t_axis(dur)
    snap = band(rng.standard_normal(len(t)), 1200, 7000) * env(t, 0.0004, 0.01)
    body = np.sin(2 * np.pi * 420 * t * (1 - 2.5 * t)) * env(t, 0.0005, 0.02)
    return snap + 0.8 * body


def bolt_tail(dur):
    """The purple bolt flies off: a hiss and a deep buzz falling and fading."""
    t = t_axis(dur)
    k = t / dur
    noise = rng.standard_normal(len(t))
    hiss = band(noise, 1800, 6000) * (1 - k) ** 2 + band(noise, 700, 2000) * k * (1 - k)
    f = 200 + (60 - 200) * k ** 0.7
    buzz = np.tanh(3 * np.sin(2 * np.pi * np.cumsum(f) / SR))
    return hiss + 0.6 * buzz * (1 - k) ** 2


def clatter(dur=0.5):
    """The staff falls on stone: a wooden crack, two bounces, a short roll."""
    out = np.zeros(int(dur * SR))
    place(out, knock(0.12, 210, 4000), 0.0)
    place(out, 0.55 * knock(0.1, 240, 4000), 0.13)
    place(out, 0.3 * knock(0.08, 260, 4000), 0.22)
    place(out, 0.25 * rustle(0.2, 1500, 5000, 60), 0.26)
    return out


def wake():
    """The heavy coffin creaks and is knocked, linen rustles, a deep groan that rises into a word ("a-MUN")."""
    out = np.zeros(int(2.4 * SR))
    place(out, rms(creak(0.6, 260, 170, 40, 20), 0.07), 0.0)
    place(out, rms(knock(0.14, 110, 2200), 0.13), 0.42)
    place(out, rms(knock(0.12, 130, 2200), 0.1), 0.62)
    place(out, rms(rustle(1.0, 1200, 6000), 0.06), 0.35)
    place(out, rms(creak(0.35, 330, 240, 50, 30), 0.05), 1.05)
    d = 1.4
    groan = speak([("h", "o", 1.4), ("", "a", 1.0), ("m", "u", 1.6)], 58, 72, d, growl=0.35) * env(t_axis(d), 0.3, 0.9)
    place(out, rms(groan, 0.17), 0.75)
    return finish(out)


def attack():
    """The staff swung: a long low whoosh, a deep "hah", the wooden crack of the staff and a thud."""
    out = np.zeros(int(0.85 * SR))
    place(out, rms(rustle(0.2, 1000, 4500), 0.035), 0.0)
    place(out, rms(whoosh(0.45, 350, 2500), 0.12), 0.15)
    d = 0.3
    place(out, rms(speak([("h", "a", 1.0)], 82, 64, d, growl=0.3) * env(t_axis(d), 0.03, 0.12), 0.17), 0.18)
    place(out, rms(knock(0.12, 160, 3500), 0.16), 0.52)
    place(out, rms(thud(0.25, 60), 0.2), 0.53)
    return finish(out)


def att_ranged():
    """A curse spoken on one low note over the rising hum ("ka-ru ne-SET"), the dark crack at the release, the bolt's
    tail."""
    tail_s = 0.4
    out = np.zeros(int((RELEASE_S + tail_s + 0.08) * SR))
    words = speak([("k", "a", 0.9), ("r", "u", 1.0), ("n", "e", 0.9), ("s", "e", 1.5)], 68, 62, 0.78, growl=0.2)
    words *= env(t_axis(0.78), 0.03, 0.6)
    place(out, rms(words, 0.17), 0.02)
    place(out, rms(hum(RELEASE_S, 45, 150), 0.09), 0.0)
    place(out, rms(dark_crack(), 0.2), RELEASE_S)
    place(out, rms(bolt_tail(tail_s), 0.13), RELEASE_S + 0.01)
    return finish(out)


def die():
    """A long voiced moan sinking and breaking up, the staff clattering down, dust, linen flopping, a heavy thud."""
    out = np.zeros(int(1.9 * SR))
    d = 1.2
    moan = speak([("", "o", 1.0), ("", "u", 1.5)], 70, 34, d, growl=0.6) * env(t_axis(d), 0.06, 0.45)
    place(out, rms(moan, 0.16), 0.0)
    place(out, rms(clatter(), 0.1), 0.86)  # the staff hits the floor at die frame 12
    place(out, rms(dust(1.1, 240), 0.05), 0.3)
    place(out, rms(rustle(0.45, 700, 4000, 25), 0.06), 0.45)
    place(out, rms(thud(0.3, 50), 0.2), 0.8)
    place(out, rms(lowpass(rustle(0.4, 600, 3500, 20), 3000), 0.07), 1.1)
    place(out, rms(thud(0.25, 42), 0.12), 1.4)
    return finish(out)


def main():
    out_dir = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds", "monsters")
    for name, fn in (("wake", wake), ("att", attack), ("att_ranged", att_ranged), ("die", die)):
        write(os.path.join(out_dir, "mummy_priest_{}.wav".format(name)), fn())


if __name__ == "__main__":
    main()
