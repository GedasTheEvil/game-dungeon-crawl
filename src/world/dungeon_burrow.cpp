// Burrowers on the level (monster_burrow.cpp, Apep): where one comes up, the holes it leaves.
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
bool Dungeon::updateBurrower(Monster& mon) {
	if (mon.Burrowing()) {
		if (mon.UpdateBurrow())
			holes.push_back({mon.CentreX(), mon.Row(), GameClock::now()});
		return true;
	}
	if (won || !mon.canDive(mapY))
		return false;
	if (float to = 0.f; burrowTarget(mon, to)) {
		holes.push_back({mon.CentreX(), mon.Row(), GameClock::now()});
		mon.Dive(to - static_cast<float>(mon.Col()) - 0.5f);
	} else
		mon.DelayDive();
	return true;
}
