// The Stargate chamber (plan stargate-chamber): the ring dialled (its chevrons and track: stargate_dial.cpp), the
// reward the first time; its dial device. Drawn by DungeonView::drawStargate (dungeon_render_decor.cpp).
#include "dungeon.h"
#include "player.h"
#include "../level/decor.h"
#include "dungeon_rules.h"
#include "../level/journal.h"
#include "../level/world_events.h"
#include "../level/gameplay_world.h"
#include <algorithm>
#include <cmath>

namespace {
// What the first dial, and the first press on the dial device, pay.
int stargateXP(const PlayerStats& stats) { return std::max(stats.RiddleXP(), STARGATE_MIN_XP); }
} // namespace

void Dungeon::startDial() {
	if (Dialling())
		return;
	dial.Start(MapIndex(static_cast<int>(mapX), static_cast<int>(mapY)));
	updateDial(); // the first chevron's turn starts now
}
void Dungeon::pressDialDevice() {
	sim.events->Play(WorldSound::DialPress);
	const auto& notes = sim.journal->Notes();
	if (!std::ranges::contains(notes, FieldNote::DialDevice)) {
		const int xp = stargateXP(sim.player->stats); // the first press
		sim.events->Status("The red dome sinks with a dull clunk. Nothing happens. +%d XP", xp);
		sim.player->stats.AddXP(xp, *sim.events);
	} else
		sim.events->Status("The red dome sinks with a dull clunk. Nothing happens.");
	sim.events->Note(FieldNote::DialDevice);
}
void Dungeon::updateDial() {
	for (StargateDial::Step step : dial.Update())
		switch (step) {
		case StargateDial::Step::ChevronLock:
			sim.events->Play(WorldSound::ChevronLock);
			break;
		case StargateDial::Step::ChevronFail:
			sim.events->Play(WorldSound::ChevronFail);
			break;
		case StargateDial::Step::Failed:
			if (!std::ranges::contains(sim.journal->Notes(), FieldNote::Stargate)) {
				const int xp = stargateXP(sim.player->stats); // the first dial
				sim.events->Status("Chevron seven... will not lock. +%d XP", xp);
				sim.player->stats.AddXP(xp, *sim.events);
			} else
				sim.events->Status("Chevron seven... will not lock.");
			sim.events->Note(FieldNote::Stargate);
			break;
		}
}
