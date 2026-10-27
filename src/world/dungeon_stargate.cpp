// The Stargate chamber (plan stargate-chamber): the ring dialled, its chevrons locking one by one, the seventh that
// will not, the reward the first time; its dial device. Drawn by Dungeon::drawStargate (dungeon_render_decor.cpp).
#include "dungeon.h"
#include "../entities/player.h"
#include "decor.h"
#include "dungeon_rules.h"
#include "journal.h"
#include "world_events.h"
#include <algorithm>
#include <cmath>

namespace {
// The dial's steps, in order: a chevron's turn each (the seventh's too), the seventh failing, the end.
constexpr int STEP_FAIL = STARGATE_CHEVRONS;
constexpr int STEP_END = STARGATE_CHEVRONS + 1;

int stepMs(int step) {
	if (step < STARGATE_CHEVRONS)
		return step * STARGATE_CHEVRON_MS;
	return step == STEP_FAIL ? STARGATE_FAIL_AT_MS : STARGATE_DIAL_MS;
}

// The track's turn for chevron k: back and forth, a little further each time.
float turnDeg(int chevron) { return (chevron % 2 == 0 ? 1.f : -1.f) * (90.f + 23.f * static_cast<float>(chevron)); }
} // namespace
//======================================================================================
void Dungeon::startDial() {
	if (Dialling())
		return;
	dial = {MapIndex(static_cast<int>(mapX), static_cast<int>(mapY)), GameClock::now(), 0};
	updateDial(); // the first chevron's turn starts now
}
//======================================================================================
void Dungeon::pressDialDevice() {
	sim.events->Play(WorldSound::DialPress);
	sim.events->Status("The red dome sinks with a dull clunk. Nothing happens.");
	sim.events->Note(FieldNote::DialDevice);
}
//======================================================================================
void Dungeon::updateDial() {
	if (!Dialling())
		return;
	const int elapsed = GameClock::now() - dial.startMs;
	while (dial.step <= STEP_END && elapsed >= stepMs(dial.step)) {
		const int step = dial.step++;
		if (step < STARGATE_CHEVRONS - 1)
			sim.events->Play(WorldSound::ChevronLock);
		else if (step == STARGATE_CHEVRONS - 1)
			sim.events->Play(WorldSound::ChevronFail);
		else if (step == STEP_FAIL) {
			const auto& notes = sim.journal->Notes();
			if (std::find(notes.begin(), notes.end(), FieldNote::Stargate) == notes.end()) {
				const int xp = sim.player->stats.RiddleXP(); // the first dial: a riddle's worth
				sim.events->Status("Chevron seven... will not lock. +%d XP", xp);
				sim.player->stats.AddXP(xp, *sim.events);
			} else
				sim.events->Status("Chevron seven... will not lock.");
			sim.events->Note(FieldNote::Stargate);
		} else {
			stargateRestDeg = std::fmod(stargateRestDeg + dialTrackDeg(STARGATE_DIAL_MS), 360.f);
			dial = {};
			return;
		}
	}
}
//======================================================================================
float Dungeon::dialTrackDeg(int elapsedMs) const {
	float deg = 0.f;
	for (int k = 0; k < STARGATE_CHEVRONS; k++) {
		const float t = std::clamp(
			static_cast<float>(elapsedMs - k * STARGATE_CHEVRON_MS) / STARGATE_TURN_MS,
			0.f,
			1.f
		);
		deg += turnDeg(k) * t * t * (3.f - 2.f * t); // eased in and out
	}
	return deg;
}
//======================================================================================
float Dungeon::StargateTrackDeg(int col, int row) const {
	if (dial.cell != MapIndex(col, row))
		return stargateRestDeg;
	return stargateRestDeg + dialTrackDeg(GameClock::now() - dial.startMs);
}
//======================================================================================
float Dungeon::ChevronGlow(int col, int row, int chevron) const {
	if (dial.cell != MapIndex(col, row) || chevron < 0 || chevron >= STARGATE_CHEVRONS)
		return 0.f;
	const int elapsed = GameClock::now() - dial.startMs;
	const int lockAt = chevron * STARGATE_CHEVRON_MS + STARGATE_TURN_MS;
	if (elapsed < lockAt)
		return 0.f;
	if (chevron == STARGATE_CHEVRONS - 1) { // flickers and dies
		const int since = elapsed - lockAt;
		if (since >= STARGATE_FAIL_MS)
			return 0.f;
		const float left = 1.f - static_cast<float>(since) / STARGATE_FAIL_MS;
		return left * (0.5f + 0.5f * std::sin(static_cast<float>(since) * 0.045f));
	}
	const int fadeFrom = STARGATE_FAIL_AT_MS + STARGATE_FAIL_MS;
	return elapsed < fadeFrom ? 1.f : std::max(0.f, 1.f - static_cast<float>(elapsed - fadeFrom) / STARGATE_FADE_MS);
}
