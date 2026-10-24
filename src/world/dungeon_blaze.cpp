// A floor set burning by Babi's big coal (plan babi-boss, VolleyRules): the player standing in it burns. Drawn in
// dungeon_render_blaze.cpp.
#include "dungeon.h"
#include "../entities/player.h"
#include "world_events.h"
#include "../core/gameplay_config.h"
#include <algorithm>
#include <cmath>

bool Dungeon::blazeFloor(int col, int row) const {
	if (!IsInBounds(col, row) || !IsInBounds(col, row - 1))
		return false;
	const Tile t = MapAt(col, row);
	return !isSolidTile(t) && !inHalfWater(t) && isSolidTile(MapAt(col, row - 1));
}
//======================================================================================
void Dungeon::blazeAt(float x, float y, int ms) {
	const auto col = static_cast<int>(std::floor(x));
	auto row = static_cast<int>(std::floor(y));
	if (!IsInBounds(col, row) || isSolidTile(MapAt(col, row)))
		return;
	while (row > 0 && !isSolidTile(MapAt(col, row - 1)) && MapAt(col, row).type != Ladder) // thrown down a shaft
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
	const bool in = sim.player->Alive() && PlayerInBlaze();
	if (in && !playerBlazed)
		sim.events->Status("%s", "The floor is burning under your feet");
	playerBlazed = in;
	if (!in || now < nextBlazeHitMs)
		return;
	nextBlazeHitMs = now + BLAZE_TICK_MS;
	sim.player->TakeHit(BLAZE_DAMAGE, BLAZE_MIX, *sim.events);
}
