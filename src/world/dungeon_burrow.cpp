// Burrowers on the level (monster_burrow.cpp, Apep, the worm in sand): where one comes up, the holes it leaves.
#include "dungeon.h"
#include "../core/gameplay_config.h"
#include <cmath>

bool Dungeon::burrowTarget(const Monster& mon, float& outX) const {
	const int side = mon.CentreX() < mapX ? 1 : -1;
	for (int s : {side, -side}) {
		const float x = mapX + static_cast<float>(s) * BURROW_BEHIND;
		if (!walkerBlocked(static_cast<int>(std::floor(x)), mon.Row(), true)) {
			outX = x;
			return true;
		}
	}
	return false;
}
//======================================================================================
bool Dungeon::sandTarget(const Monster& mon, float range, bool edge, float& outX) const {
	const auto col = static_cast<int>(std::floor(mon.CentreX()));
	if (static_cast<int>(mapY) != mon.Row() || std::fabs(mapX - mon.CentreX()) > range ||
		!inSand(MapAt(col, mon.Row())))
		return false;
	const auto playerCol = static_cast<int>(std::floor(mapX));
	const int dir = playerCol < col ? -1 : 1;
	int c = col;
	while (c != playerCol && inSand(MapAt(c + dir, mon.Row())))
		c += dir;
	if (c == playerCol && PlayerInSand()) {
		outX = mapX;
		return true;
	}
	if (!edge || c == playerCol) // in the air over its sand: it waits
		return false;
	outX = static_cast<float>(c) + 0.5f;
	return true;
}
//======================================================================================
bool Dungeon::updateBurrower(Monster& mon) {
	if (mon.UnderSand()) { // lying in wait under the sand since its spawn
		if (float to = 0.f; !won && sandTarget(mon, SAND_WORM_WAKE_RANGE, true, to))
			mon.Surge(to - static_cast<float>(mon.Col()) - 0.5f);
		return true;
	}
	if (mon.Burrowing()) {
		if (mon.UpdateBurrow())
			holes.push_back({mon.CentreX(), mon.Row(), GameClock::now()});
		return true;
	}
	if (won || !mon.canDive(mapY))
		return false;
	if (mon.Type()->sandBurrow) { // only through sand, from a little way off: near, it bites
		float to = 0.f;
		if (std::fabs(mapX - mon.CentreX()) < SAND_WORM_DIVE_MIN || !sandTarget(mon, SAND_WORM_DIVE_RANGE, false, to)) {
			mon.DelayDive();
			return false;
		}
		holes.push_back({mon.CentreX(), mon.Row(), GameClock::now()});
		mon.Dive(to - static_cast<float>(mon.Col()) - 0.5f);
		return true;
	}
	if (float to = 0.f; burrowTarget(mon, to)) {
		holes.push_back({mon.CentreX(), mon.Row(), GameClock::now()});
		mon.Dive(to - static_cast<float>(mon.Col()) - 0.5f);
	} else
		mon.DelayDive();
	return true;
}
