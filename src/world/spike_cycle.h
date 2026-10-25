#ifndef SPIKE_CYCLE_H
#define SPIKE_CYCLE_H
#include "../core/gameplay_config.h"
#include "level.h"

// The rhythm traps' cycle (plans rhythm-traps, fire-jet-trap, poison-drip-trap: wall spikes, a fire vent's jet, a
// poison drip's falling venom): out first, then back in, the telegraph at the end of the safe part. The game sets each
// cell's live state from the clock (Dungeon::updateRhythmTraps), the checker walks through a run of them.

[[nodiscard]] inline int spikePeriodMs(const Fixture& f) { return SPIKES_PERIOD_MS[static_cast<int>(spikeSpeed(f))]; }
[[nodiscard]] inline int spikeOutMs(const Fixture& f) { return SPIKES_OUT_MS[static_cast<int>(spikeSpeed(f))]; }

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
	return at >= spikePeriodMs(f) - SPIKES_TELEGRAPH_MS ? SpikeState::Telegraph : SpikeState::Retracted;
}

// No moment in [fromMs, toMs] with the spikes out.
[[nodiscard]] inline bool spikesInBetween(const Fixture& f, int fromMs, int toMs) {
	const int at = spikeCycleMs(f, fromMs);
	if (at < spikeOutMs(f))
		return false;
	return fromMs + spikePeriodMs(f) - at > toMs; // the next time they come out
}

#endif
