"""Synthesize the ambient loop of each campaign zone (zone-looks).

    python3 tools/audio/zone_ambience.py [out_dir]      (default: media/sounds/)

Writes ambience/<name>.wav, one seamless 10 s loop per zone, in the order of the zone table: tomb, bats, webs,
temple, nest, serpent, cisterns, desert, sokar, fire, necropolis, judgment. They play quietly under the effects all
the time, so the beds are steady and the events sparse.
Seamless: the whole loop is built on a circle. Noise beds are shaped in the frequency domain of the full loop
(periodic by construction), drones and LFOs make a whole number of cycles per loop, events wrap round the end and
echoes are circular convolutions. Everything is generated (no samples), so this script is the source of the sounds.
Output: 16-bit PCM mono 22050 Hz, like jump_sound.py.
"""

import os
import sys
import wave

import numpy as np

SR = 22050
LOOP = 10.0
N = int(LOOP * SR)
PEAK = 10 ** (-12 / 20)  # -12 dBFS: a soft limit on the clicks and drips
RMS = -32  # dBFS: the level of a loop, the same in every zone (but the near-silent webs)
REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
rng = np.random.default_rng(31)  # fixed seed: the same files on every run
T = np.arange(N) / SR
FREQS = np.fft.rfftfreq(N, 1 / SR)


# ---------------------------------------------------------------- circular helpers


def shaped_noise(lo, hi, tilt=0.0, order=2):
    """Periodic noise with a smooth band-pass between lo and hi Hz, amplitude ~ f^-tilt. RMS 1."""
    spec = np.fft.rfft(rng.standard_normal(N))
    f = np.maximum(FREQS, 1.0)
    gain = 1 / np.sqrt(1 + (lo / f) ** (2 * order)) / np.sqrt(1 + (f / hi) ** (2 * order)) * f ** -tilt
    x = np.fft.irfft(spec * gain, N)
    return x / (np.sqrt(np.mean(x**2)) + 1e-12)


def circ_filter(x, lo, hi, order=2):
    """Band-pass x on the circle (no edge at the loop point)."""
    f = np.maximum(FREQS, 1.0)
    gain = 1 / np.sqrt(1 + (lo / f) ** (2 * order)) / np.sqrt(1 + (f / hi) ** (2 * order))
    return np.fft.irfft(np.fft.rfft(x) * gain, N)


def lfo(cycles, phase=0.0):
    """0..1, a whole number of cycles per loop."""
    return 0.5 + 0.5 * np.sin(2 * np.pi * cycles * T / LOOP + phase)


def slow_env(points, depth):
    """A smooth periodic envelope from a few random harmonics: 1 +- depth."""
    e = np.zeros(N)
    for k in range(1, points + 1):
        e += rng.uniform(0.3, 1.0) / k * np.sin(2 * np.pi * k * T / LOOP + rng.uniform(0, 2 * np.pi))
    e /= np.max(np.abs(e)) + 1e-12
    return 1 + depth * e


def tone(freq, amp=1.0):
    """A sine whose frequency is rounded to a whole number of cycles per loop."""
    cycles = round(freq * LOOP)
    return amp * np.sin(2 * np.pi * cycles * T / LOOP + rng.uniform(0, 2 * np.pi))


def place(out, clip, at, gain=1.0):
    """Add clip at time `at`, wrapping round the end of the loop."""
    idx = (int(at * SR) + np.arange(len(clip))) % N
    np.add.at(out, idx, gain * clip)


def echo(x, delays, gains):
    """Circular echoes (a cheap stone-room reverb that wraps over the loop point)."""
    ir = np.zeros(N)
    ir[0] = 1
    for d, g in zip(delays, gains):
        ir[int(d * SR) % N] += g
    return np.fft.irfft(np.fft.rfft(x) * np.fft.rfft(ir), N)


def reverb(x, length=1.2, amount=0.5, lo=200, hi=4000):
    """Circular convolution with a decaying noise tail."""
    n = int(length * SR)
    tail = rng.standard_normal(n) * np.exp(-np.arange(n) / SR / (length / 5))
    ir = np.zeros(N)
    ir[:n] = tail * amount / np.sqrt(np.sum(tail**2))
    ir = circ_filter(ir, lo, hi)
    ir[0] += 1
    return np.fft.irfft(np.fft.rfft(x) * np.fft.rfft(ir), N)


def t_axis(duration):
    return np.arange(int(duration * SR)) / SR


def env_ad(t, attack, decay):
    return (1 - np.exp(-t / attack)) * np.exp(-t / decay)


def burst(duration, lo, hi, attack, decay):
    """A short filtered noise event (not circular: a clip to place)."""
    t = t_axis(duration)
    x = rng.standard_normal(len(t))
    spec = np.fft.rfft(x)
    f = np.maximum(np.fft.rfftfreq(len(t), 1 / SR), 1.0)
    spec *= 1 / np.sqrt(1 + (lo / f) ** 4) / np.sqrt(1 + (f / hi) ** 4)
    return np.fft.irfft(spec, len(t)) * env_ad(t, attack, decay)


def chirp(duration, f0, f1, decay):
    t = t_axis(duration)
    f = f0 * (f1 / f0) ** (t / duration)
    return np.sin(2 * np.pi * np.cumsum(f) / SR) * env_ad(t, 0.002, decay)


def times(count, gap=0.4):
    """count event times spread over the loop, at least `gap` s apart (circularly)."""
    while True:
        ts = np.sort(rng.uniform(0, LOOP, count))
        d = np.diff(np.concatenate([ts, [ts[0] + LOOP]]))
        if count < 2 or d.min() > gap:
            return ts


def norm(x, rms):
    return x * rms / (np.sqrt(np.mean(x**2)) + 1e-12)


# ---------------------------------------------------------------- the zones


def tomb():
    room = norm(shaped_noise(30, 400, tilt=0.5), 0.012) * slow_env(3, 0.25)
    room += tone(55, 0.004) + tone(82.5, 0.002)
    ev = np.zeros(N)
    at = rng.uniform(1, 9)
    for k in range(rng.integers(5, 9)):  # a small pebble trickle
        place(ev, burst(0.04, 1500, 6000, 0.001, 0.008), at + k * rng.uniform(0.05, 0.14), rng.uniform(0.2, 0.5))
    return room + norm(reverb(ev, 1.4, 0.8), 0.006)


def bats():
    air = norm(shaped_noise(40, 600, tilt=0.5), 0.011) * slow_env(3, 0.3)
    ev = np.zeros(N)
    for at in times(4, 1.5):
        for k in range(rng.integers(1, 4)):
            f0 = rng.uniform(5000, 7500)
            place(ev, chirp(0.05, f0, f0 * rng.uniform(0.7, 0.85), 0.015), at + k * 0.09, rng.uniform(0.3, 0.6))
    for at in times(2, 3.0):  # wing flutters
        for k in range(rng.integers(6, 12)):
            place(ev, burst(0.03, 300, 1800, 0.003, 0.01), at + k * 0.045, 0.6)
    return air + norm(reverb(ev, 1.6, 0.9, 300, 6000), 0.005)


def webs():
    air = norm(shaped_noise(80, 2500, tilt=0.3), 0.005) * slow_env(2, 0.2)
    ev = np.zeros(N)
    for at in times(3, 2.0):  # dry rustle
        n = burst(rng.uniform(0.3, 0.6), 2000, 7000, 0.08, 0.15)
        place(ev, n, at, 0.4)
    at = rng.uniform(1, 9)  # a tiny creak: a scraped resonance
    t = t_axis(0.35)
    creak = np.sin(2 * np.pi * 420 * t + 3 * np.sin(2 * np.pi * 7 * t)) * (np.sin(2 * np.pi * 38 * t) > 0.3)
    place(ev, creak * env_ad(t, 0.05, 0.12), at, 0.15)
    return air + norm(ev, 0.004)


def temple():
    hum = sum(tone(f, a) for f, a in [(65, 1.0), (98, 0.5), (130, 0.35), (196, 0.15), (261, 0.07)])
    hum = norm(hum, 0.012) * (0.85 + 0.3 * lfo(1) * lfo(2, 1.0))
    air = norm(shaped_noise(60, 900, tilt=0.5), 0.005)
    return reverb(hum + air, 2.0, 0.6, 60, 1500)


def nest():
    air = norm(shaped_noise(150, 3000, tilt=0.4), 0.007) * slow_env(3, 0.25)
    ev = np.zeros(N)
    for at in times(7, 0.8):  # chitter: a run of fast clicks
        for k in range(rng.integers(3, 9)):
            place(ev, burst(0.012, 2500, 8000, 0.0005, 0.002), at + k * rng.uniform(0.018, 0.035), rng.uniform(0.4, 1))
    return air + norm(reverb(ev, 0.9, 0.6, 1000, 8000), 0.006)


def serpent():
    hiss = norm(shaped_noise(2500, 7000, tilt=0.2), 0.008) * (0.6 + 0.8 * lfo(2) * lfo(3, 0.7))
    drone = norm(tone(49) + tone(73.5, 0.5) + tone(98, 0.2), 0.008) * (0.9 + 0.2 * lfo(1, 2.0))
    air = norm(shaped_noise(40, 500, tilt=0.5), 0.006)
    return hiss + drone + air


def cisterns():
    lap = norm(shaped_noise(150, 1200, tilt=0.6), 0.008) * slow_env(5, 0.5)
    ev = np.zeros(N)
    for at in times(5, 1.2):  # a drip: a short rising plink
        f0 = rng.uniform(900, 1800)
        t = t_axis(0.12)
        drop = np.sin(2 * np.pi * np.cumsum(f0 * (1 + 0.8 * t / 0.12)) / SR) * env_ad(t, 0.001, 0.025)
        place(ev, drop, at, rng.uniform(0.5, 1.0))
    ev = echo(ev, [0.21, 0.43, 0.67], [0.35, 0.2, 0.1])
    return lap + norm(reverb(ev, 1.8, 1.0, 300, 6000), 0.012)


def desert():
    gust = slow_env(4, 0.6)
    wind = norm(shaped_noise(150, 1200, tilt=0.8), 0.015) * gust
    whistle = norm(circ_filter(shaped_noise(600, 900), 650, 850, order=4), 0.003) * gust
    sand = norm(shaped_noise(3500, 9000, tilt=0.1), 0.005) * gust**1.5
    return wind + whistle + sand


def sokar():
    gust = slow_env(3, 0.5)
    wind = norm(shaped_noise(50, 600, tilt=0.9), 0.018) * gust
    ev = np.zeros(N)
    at = rng.uniform(1, 9)
    pour = burst(2.5, 1500, 6000, 0.4, 0.9) * (1 + 0.5 * rng.standard_normal(int(2.5 * SR)).clip(-1, 1))
    place(ev, pour, at, 1.0)
    trickle_at = (at + 5) % LOOP
    for k in range(8):
        place(ev, burst(0.05, 2000, 7000, 0.002, 0.015), trickle_at + k * 0.08, 0.5)
    return wind + norm(ev, 0.006)


def fire():
    rumble = norm(shaped_noise(20, 150, tilt=0.7), 0.014) * slow_env(4, 0.3)
    roar = norm(shaped_noise(200, 2000, tilt=0.5), 0.008) * slow_env(6, 0.35)
    ev = np.zeros(N)
    for at in rng.uniform(0, LOOP, 70):  # crackle
        place(ev, burst(0.01, 1500, 8000, 0.0003, 0.002), at, rng.uniform(0.2, 1.0) ** 2)
    return rumble + roar + norm(ev, 0.007)


def necropolis():
    drone = norm(tone(41) + tone(61.5, 0.4) + tone(82, 0.25), 0.012) * (0.85 + 0.3 * lfo(1, 0.4))
    cold = norm(shaped_noise(100, 800, tilt=0.6), 0.005) * slow_env(2, 0.3)
    ev = np.zeros(N)
    t = t_axis(1.8)  # distant stone grinding once
    grind = burst(1.8, 80, 600, 0.3, 0.6) * (0.6 + 0.4 * np.sin(2 * np.pi * 11 * t))
    place(ev, grind, rng.uniform(2, 7), 1.0)
    return drone + cold + norm(reverb(ev, 2.0, 1.0, 60, 1200), 0.006)


def judgment():
    drone = norm(tone(36.7) + tone(55, 0.6) + tone(73.4, 0.3), 0.011)
    # A choir-like "ah": a saw-ish harmonic stack on A2 shaped by vowel formants, slowly breathing.
    f0 = 110.0
    voice = np.zeros(N)
    formants = [(700, 110), (1220, 120), (2600, 170)]
    for h in range(1, 30):
        f = f0 * h
        g = sum(np.exp(-0.5 * ((f - c) / bw) ** 2) * (1 / (i + 1)) for i, (c, bw) in enumerate(formants))
        voice += tone(f, (g + 0.02) / h)
        voice += tone(f * 1.004, 0.6 * (g + 0.02) / h)  # a second voice, slightly detuned
    voice = norm(voice, 0.005) * (0.7 + 0.6 * lfo(1, 1.3))
    return reverb(drone + voice, 2.5, 0.7, 60, 3000)


ZONES = [
    ("tomb", tomb, RMS),
    ("bats", bats, RMS),
    ("webs", webs, RMS - 6),
    ("temple", temple, RMS),
    ("nest", nest, RMS),
    ("serpent", serpent, RMS),
    ("cisterns", cisterns, RMS),
    ("desert", desert, RMS),
    ("sokar", sokar, RMS),
    ("fire", fire, RMS),
    ("necropolis", necropolis, RMS),
    ("judgment", judgment, RMS),
]


def write(path, x, rms):
    x = x - np.mean(x)
    x = PEAK * np.tanh(norm(x, 10 ** (rms / 20)) / PEAK)
    data = np.round(x * 32767).astype("<i2")
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(data.tobytes())
    return x


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "media", "sounds")
    os.makedirs(os.path.join(out, "ambience"), exist_ok=True)
    for name, make, rms in ZONES:
        x = write(os.path.join(out, "ambience", name + ".wav"), make(), rms)
        level = 20 * np.log10(np.sqrt(np.mean(x**2)))
        peak = 20 * np.log10(np.max(np.abs(x)))
        # The seam against the typical step between neighbouring samples: about 1 or less is seamless.
        seam = abs(x[0] - x[-1]) / np.mean(np.abs(np.diff(x)))
        print(f"ambience/{name}.wav  rms {level:.1f} dBFS  peak {peak:.1f} dBFS  seam {seam:.2f}")


if __name__ == "__main__":
    main()
