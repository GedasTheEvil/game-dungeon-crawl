// Burrowers on the level (monster_burrow.cpp, Apep, the worm and Rerek in sand): where one comes up, the holes it
// leaves, Rerek's spray of sand.
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
bool Dungeon::sandTarget(const Monster& mon, float range, bool edge, float past, float& outX) const {
	const auto col = static_cast<int>(std::floor(mon.CentreX()));
	if (static_cast<int>(mapY) != mon.Row() || std::fabs(mapX - mon.CentreX()) > range ||
		!inSand(MapAt(col, mon.Row())))
		return false;
	const auto playerCol = static_cast<int>(std::floor(mapX));
	const int dir = playerCol < col ? -1 : 1;
	auto sandTo = [&](int from, int to) { // the last sand cell from from towards to, through sand all the way
		while (from != to && inSand(MapAt(from + dir, mon.Row())))
			from += dir;
		return from;
	};
	const int c = sandTo(col, playerCol);
	if (c == playerCol && PlayerInSand()) {
		const float beyond = mapX + static_cast<float>(dir) * past;
		const auto beyondCol = static_cast<int>(std::floor(beyond));
		outX = past > 0.f && sandTo(c, beyondCol) == beyondCol ? beyond : mapX;
		return true;
	}
	if (!edge || c == playerCol) // in the air over its sand: it waits
		return false;
	outX = static_cast<float>(c) + 0.5f;
	return true;
}
//======================================================================================
bool Dungeon::updateBurrower(Monster& mon) {
	const std::optional<SandBurrowRules>& sand = mon.Type()->sandBurrow;
	if (sand && mon.UnderSand()) { // lying in wait under the sand since its spawn
		if (float to = 0.f; !won && sandTarget(mon, sand->wakeRange, true, 0.f, to))
			mon.Surge(to - static_cast<float>(mon.Col()) - 0.5f);
		return true;
	}
	if (mon.Burrowing()) {
		if (mon.UpdateBurrow()) {
			holes.push_back({mon.CentreX(), mon.Row(), GameClock::now()});
			if (mon.Type()->spray) {
				sprays.push_back({mon.CentreX(), mon.Row(), GameClock::now()});
				sandBurst(mon, *mon.Type()->spray);
			}
		}
		return true;
	}
	if (won || !mon.canDive(mapY))
		return false;
	if (sand) { // only through sand, from a little way off: near, it bites
		const float past = sand->behind && mon.Dives() % 2 == 0 ? BURROW_BEHIND : 0.f; // every other dive, behind
		float to = 0.f;
		if (std::fabs(mapX - mon.CentreX()) < sand->diveMin || !sandTarget(mon, sand->diveRange, false, past, to)) {
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
