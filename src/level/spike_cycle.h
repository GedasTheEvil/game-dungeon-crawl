#ifndef SPIKE_CYCLE_H
#define SPIKE_CYCLE_H
#include "gameplay_config.h"
#include "level.h"
#include <algorithm>

// The rhythm traps' cycle (plans rhythm-traps, fire-jet-trap, poison-drip-trap: wall spikes, a fire vent's jet, a
// poison drip's falling venom): out first, then back in, the telegraph at the end of the safe part. The sand gusts
// (plan sandstorm-halls) blow on a slower one of the same shape. The game sets each cell's live state from the clock
// (Dungeon::updateRhythmTraps), the checker walks through a run of them.

[[nodiscard]] inline int spikePeriodMs(const Fixture& f) {
	return f.type == SandGust ? GUST_PERIOD_MS[static_cast<int>(gustSpeed(f))]
							  : SPIKES_PERIOD_MS[static_cast<int>(spikeSpeed(f))];
}
[[nodiscard]] inline int spikeOutMs(const Fixture& f) {
	return f.type == SandGust ? GUST_OUT_MS[static_cast<int>(gustSpeed(f))]
							  : SPIKES_OUT_MS[static_cast<int>(spikeSpeed(f))];
}
[[nodiscard]] inline int spikeTelegraphMs(const Fixture& f) {
	return f.type == SandGust ? GUST_TELEGRAPH_MS : SPIKES_TELEGRAPH_MS;
}

// Where in its cycle the trap is at nowMs, 0 to the period: the phase shifts it by eighths.
[[nodiscard]] inline int spikeCycleMs(const Fixture& f, int nowMs) {
	const int period = spikePeriodMs(f);
	const int shifted = nowMs + spikePhase(f) * period / SPIKE_PHASES;
	return ((shifted % period) + period) % period;
}

[[nodiscard]] inline SpikeState spikeStateAt(const Fixture& f, int nowMs) {
	const int at = spikeCycleMs(f, nowMs);
	if (at < spikeOutMs(f))
		return SpikeState::Extended;
	return at >= spikePeriodMs(f) - spikeTelegraphMs(f) ? SpikeState::Telegraph : SpikeState::Retracted;
}

// No moment in [fromMs, toMs] with the spikes out.
[[nodiscard]] inline bool spikesInBetween(const Fixture& f, int fromMs, int toMs) {
	const int at = spikeCycleMs(f, fromMs);
	if (at < spikeOutMs(f))
		return false;
	return fromMs + spikePeriodMs(f) - at > toMs; // the next time they come out
}

// A run of count rhythm traps in a row (run: in the order they are walked through) walked at speed (tiles a second)
// without stopping, the body stopped by the first one from startMs on: none out while the body could be in its cell
// (from the moment it could enter it until it is out of it), each window widened by marginMs either side. A run of sand
// gusts the same, against the wind.
[[nodiscard]] inline bool
rhythmRunClear(const Fixture* run, int count, int startMs, int marginMs = 0, float speed = WALK_SPEED) {
	const auto cellMs = static_cast<int>(1000.f / speed);
	const auto crossMs = static_cast<int>(SPIKES_CROSS_MS * WALK_SPEED / speed);
	for (int k = 0; k < count; k++)
		if (!spikesInBetween(run[k], startMs + k * cellMs - marginMs, startMs + k * cellMs + crossMs + marginMs))
			return false;
	return true;
}

// How hard a gust fixture blows now, 0..1: GUST_HAZE_CALM between gusts, rising through the telegraph, 1 in a gust.
[[nodiscard]] inline float gustStrength(const Fixture& f, int now) {
	const SpikeState s = spikeStateAt(f, now);
	if (s == SpikeState::Extended)
		return 1.f;
	if (s == SpikeState::Retracted)
		return GUST_HAZE_CALM;
	const int left = spikePeriodMs(f) - spikeCycleMs(f, now); // until the gust
	const float k = 1.f - static_cast<float>(left) / static_cast<float>(spikeTelegraphMs(f));
	return GUST_HAZE_CALM + (0.6f - GUST_HAZE_CALM) * std::clamp(k, 0.f, 1.f);
}

#endif
