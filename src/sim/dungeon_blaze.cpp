// A floor set burning by Babi's big coal (plan babi-boss, VolleyRules): whoever stands in it burns, the player and the
// monsters alike (plan same-hazards-for-all); a monster native to fire (Babi) walks in it unharmed. Drawn in
// dungeon_render_blaze.cpp.
#include "dungeon.h"
#include "player.h"
#include "monster.h"
#include "../level/world_events.h"
#include "../level/gameplay_config.h"
#include <algorithm>
#include <cmath>

bool Dungeon::blazeFloor(int col, int row) const {
	if (!IsInBounds(col, row) || !IsInBounds(col, row - 1))
		return false;
	const Tile t = MapAt(col, row);
	return !isSolidTile(t) && !inHalfWater(t) && isFloor(MapAt(col, row - 1));
}
//======================================================================================
void Dungeon::blazeAt(float x, float y, int ms) {
	const auto col = static_cast<int>(std::floor(x));
	auto row = static_cast<int>(std::floor(y));
	if (!IsInBounds(col, row) || isSolidTile(MapAt(col, row)))
		return;
	while (row > 0 && !isFloor(MapAt(col, row - 1)) && MapAt(col, row).type != Ladder) // thrown down a shaft
		row--;
	if (!blazeFloor(col, row))
		return;
	const int now = GameClock::now(), cell = MapIndex(col, row);
	for (Blaze& b : blazes)
		if (b.cell == cell) { // burns on, from now
			b.endMs = std::max(b.endMs, now + ms);
			return;
		}
	blazes.push_back({cell, now, now + ms});
	sim.events->Play(WorldSound::FireRoar);
}
//======================================================================================
bool Dungeon::BlazeAt(int col, int row) const {
	if (!IsInBounds(col, row))
		return false;
	const int cell = MapIndex(col, row);
	return std::any_of(blazes.begin(), blazes.end(), [cell](const Blaze& b) { return b.cell == cell; });
}
//======================================================================================
bool Dungeon::PlaceBlaze(int col, int row, int ms) {
	if (!blazeFloor(col, row))
		return false;
	blazeAt(static_cast<float>(col) + 0.5f, static_cast<float>(row), ms);
	return true;
}
//======================================================================================
bool Dungeon::PlayerInBlaze() const {
	const JumpState& jump = sim.player->jump;
	return !jump.jumping && !jump.falling &&
		   BlazeAt(static_cast<int>(std::floor(mapX)), static_cast<int>(std::floor(mapY + STANDING_EPSILON)));
}
//======================================================================================
void Dungeon::updateBlazes() {
	const int now = GameClock::now();
	blazes.erase(
		std::remove_if(blazes.begin(), blazes.end(), [now](const Blaze& b) { return now >= b.endMs; }),
		blazes.end()
	);
	// One rule for every body: a hit every BLAZE_TICK_MS while it stands in it, the first on stepping in.
	const auto due = [now](Vitals& vitals, bool in) {
		if (!in || now < vitals.blazeReadyMs)
			return false;
		vitals.blazeReadyMs = now + BLAZE_TICK_MS;
		return true;
	};
	const bool in = sim.player->Alive() && PlayerInBlaze();
	if (in && !playerBlazed)
		sim.events->Status("%s", "The floor is burning under your feet");
	playerBlazed = in;
	if (due(sim.player->stats.vitals, in))
		sim.player->TakeHit(BLAZE_DAMAGE, BLAZE_MIX, *sim.events);
	if (blazes.empty())
		return;
	for (Monster& mon : monsters) {
		if (!mon.Active() || !mon.Alive() || mon.flies() || mon.jumping() || mon.Hidden() ||
			mon.Type()->native.has(Terrain::Fire))
			continue;
		const bool monIn = BlazeAt(static_cast<int>(std::floor(mon.CentreX())), mon.Row());
		if (due(mon.MonsterVitals(), monIn))
			mon.TakeHarm(BLAZE_DAMAGE, BLAZE_MIX, HarmSource::Trap); // no XP: the coal is Babi's, not the player's
	}
}
