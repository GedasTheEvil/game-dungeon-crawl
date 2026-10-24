// Flyers on the level (monster_flight.cpp, plan bats-high-ceiling): the way to another row and what a flyer sees.
#include "dungeon.h"
#include "../entities/player.h"
#include "../core/gameplay_config.h"
#include <algorithm>
#include <cmath>
#include <vector>

int Dungeon::flightShaft(int col, int row, int toCol, int toRow, bool boxed) const {
	const int left = boxed ? std::min(col, toCol) : 0, right = boxed ? std::max(col, toCol) : LEVEL_WIDTH - 1;
	const int bottom = boxed ? std::min(row, toRow) : 0, top = boxed ? std::max(row, toRow) : LEVEL_HEIGHT - 1;
	auto open = [&](int c, int r) {
		return c >= left && c <= right && r >= bottom && r <= top && IsInBounds(c, r) && !isSolidTile(MapAt(c, r));
	};
	auto vertical = [&](int c, int r1, int r2) {
		return open(c, r1) && open(c, r2) && !inHalfWater(MapAt(c, r1)) && !inHalfWater(MapAt(c, r2));
	};
	if (row == toRow || !open(col, row) || !open(toCol, toRow))
		return -1;
	// Steps to the goal from each cell, searched from the goal.
	std::vector<int> dist(LEVEL_CELL_COUNT, -1);
	std::vector<int> queue{MapIndex(toCol, toRow)};
	dist[queue[0]] = 0;
	for (size_t i = 0; i < queue.size(); i++) {
		const int c = queue[i] % LEVEL_WIDTH, r = queue[i] / LEVEL_WIDTH;
		auto visit = [&](int nc, int nr, bool ok) {
			if (ok && dist[MapIndex(nc, nr)] < 0) {
				dist[MapIndex(nc, nr)] = dist[queue[i]] + 1;
				queue.push_back(MapIndex(nc, nr));
			}
		};
		visit(c, r - 1, vertical(c, r, r - 1));
		visit(c, r + 1, vertical(c, r, r + 1));
		visit(c - 1, r, open(c, r) && open(c - 1, r));
		visit(c + 1, r, open(c, r) && open(c + 1, r));
	}
	if (dist[MapIndex(col, row)] < 0)
		return -1;
	// Along the row to the first step up or down; up or down at once where it can.
	for (int c = col;;) {
		const int d = dist[MapIndex(c, row)];
		for (int r : {row - 1, row + 1})
			if (vertical(c, row, r) && dist[MapIndex(c, r)] == d - 1)
				return c;
		c += open(c - 1, row) && dist[MapIndex(c - 1, row)] == d - 1 ? -1 : 1;
	}
}
//======================================================================================
FlightGoal Dungeon::flightGoal(const Monster& mon) const {
	FlightGoal goal;
	const auto col = static_cast<int>(std::floor(mon.CentreX()));
	const int row = mon.Row();
	goal.openBelow = IsInBounds(col, row - 1) && !isSolidTile(MapAt(col, row - 1));
	// The player's row: their feet's cell, on the floor, in a jump or up a ladder.
	const auto playerCol = static_cast<int>(std::floor(mapX));
	const auto playerRow = static_cast<int>(std::floor(mapY + 0.1f));
	if (mon.Hunts())
		goal.sees = std::abs(playerRow - row) <= BAT_HUNT_ROWS;
	else
		goal.sees = std::fabs(mapX - mon.CentreX()) <= BAT_SIGHT &&
					std::fabs(mapY - static_cast<float>(row)) <= BAT_SIGHT;
	goal.toRow = row;
	if (goal.sees && playerRow != row) { // in sight on another row: only with a way there
		const int shaft = flightShaft(col, row, playerCol, playerRow, true);
		goal.sees = shaft >= 0;
		goal.toRow = goal.sees ? playerRow : row;
		goal.shaftX = static_cast<float>(shaft - mon.Col());
	}
	if (!goal.sees && row != mon.SpawnRow()) { // back to the roost
		const int shaft = flightShaft(col, row, mon.Col(), mon.SpawnRow(), false);
		if (shaft >= 0) {
			goal.toRow = mon.SpawnRow();
			goal.shaftX = static_cast<float>(shaft - mon.Col());
		}
	}
	return goal;
}
//======================================================================================
void Dungeon::updateFlyer(Monster& mon) {
	if (won)
		return;
	const auto col = static_cast<int>(std::floor(mon.flightProbeX()));
	mon.Fly(!IsInBounds(col, mon.Row()) || isSolidTile(MapAt(col, mon.Row())), mapX, mapY, flightGoal(mon));
}
