// Chargers on the level (monster_charge.cpp, Sobek): the rush starts with a clear row, ends at what blocks it.
#include "dungeon.h"
#include <cmath>

bool Dungeon::updateCharger(Monster& mon) {
	if (mon.Charging()) {
		const auto col = static_cast<int>(std::floor(mon.HeadX() + 0.05f * static_cast<float>(mon.ChargeDir())));
		mon.UpdateCharge(walkerBlocked(col, mon.Row(), true), mapX, mapY);
		return true;
	}
	if (won || !mon.canCharge(mapX, mapY) || !clearRow(mon.HeadX(), mapX, mon.Row()))
		return false;
	mon.StartCharge(mapX);
	return true;
}
