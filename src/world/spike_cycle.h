#ifndef SPIKE_CYCLE_H
#define SPIKE_CYCLE_H
#include "../core/gameplay_config.h"
#include "level.h"

// Wall spikes' cycle (plan rhythm-traps): out first, then back in, the telegraph at the end of the safe part. The game
// sets each cell's live state from the clock (Dungeon::updateWallSpikes), the checker walks through a run of them.

[[nodiscard]] inline int spikePeriodMs(const Tile& t) { return SPIKES_PERIOD_MS[static_cast<int>(spikeSpeed(t))]; }
[[nodiscard]] inline int spikeOutMs(const Tile& t) { return SPIKES_OUT_MS[static_cast<int>(spikeSpeed(t))]; }

// Where in its cycle the trap is at nowMs, 0 to the period: the phase shifts it by eighths.
[[nodiscard]] inline int spikeCycleMs(const Tile& t, int nowMs) {
	const int period = spikePeriodMs(t);
	const int shifted = nowMs + spikePhase(t) * period / SPIKE_PHASES;
	return ((shifted % period) + period) % period;
}

[[nodiscard]] inline SpikeState spikeStateAt(const Tile& t, int nowMs) {
	const int at = spikeCycleMs(t, nowMs);
	if (at < spikeOutMs(t))
		return SpikeState::Extended;
	return at >= spikePeriodMs(t) - SPIKES_TELEGRAPH_MS ? SpikeState::Telegraph : SpikeState::Retracted;
}

// No moment in [fromMs, toMs] with the spikes out.
[[nodiscard]] inline bool spikesInBetween(const Tile& t, int fromMs, int toMs) {
	const int at = spikeCycleMs(t, fromMs);
	if (at < spikeOutMs(t))
		return false;
	return fromMs + spikePeriodMs(t) - at > toMs; // the next time they come out
}

#endif
