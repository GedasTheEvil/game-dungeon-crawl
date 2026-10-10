#include "climb_path.h"
#include "../core/gameplay_config.h"
#include <deque>

namespace {
bool inBounds(int col, int row) { return col >= 0 && col < LEVEL_WIDTH && row >= 0 && row < LEVEL_HEIGHT; }
const Tile& at(const Tile* cells, int col, int row) { return cells[row * LEVEL_WIDTH + col]; }
size_t indexOf(int col, int row) { return static_cast<size_t>(row) * LEVEL_WIDTH + static_cast<size_t>(col); }
bool ladderAt(const Tile* cells, int col, int row) { return inBounds(col, row) && at(cells, col, row).type == Ladder; }
} // namespace

bool walkerBlockedAt(const Tile* cells, int col, int row, bool reckless) {
	if (!inBounds(col, row))
		return true;
	const Tile& cell = at(cells, col, row);
	if (isSolidTile(cell))
		return true;
	const bool trap = cell.type == Spike || cell.type == Death || cell.type == DartPlate ||
					  (cell.type == RockFall && rockState(cell) != RockState::Fallen);
	if (trap && !reckless)
		return true;
	return !inBounds(col, row - 1) || !isSolidTile(at(cells, col, row - 1));
}

int leapLandingAt(const Tile* cells, int col, int row, int dir) {
	if (!inBounds(col, row) || isSolidTile(at(cells, col, row)))
		return -1; // a wall, not a gap
	for (int k = 1; k <= MONSTER_JUMP_MAX_GAP; k++) {
		const int c = col + dir * k;
		if (!inBounds(c, row) || isSolidTile(at(cells, c, row)))
			return -1;
		if (!walkerBlockedAt(cells, c, row))
			return c;
	}
	return -1;
}

bool ClimbMap::Stands(int col, int row) const {
	if (!inBounds(col, row) || isSolidTile(at(cells, col, row)))
		return false;
	return ladderAt(cells, col, row) || !walkerBlockedAt(cells, col, row, how.reckless);
}

int ClimbMap::neighbours(int col, int row, int (&outCol)[4], int (&outRow)[4]) const {
	int n = 0;
	for (int dir : {-1, 1}) {
		if (Stands(col + dir, row)) {
			outCol[n] = col + dir;
			outRow[n++] = row;
		} else if (how.leaps && !ladderAt(cells, col, row) && walkerBlockedAt(cells, col + dir, row, how.reckless)) {
			const int land = leapLandingAt(cells, col + dir, row, dir);
			if (land >= 0 && Stands(land, row)) {
				outCol[n] = land;
				outRow[n++] = row;
			}
		}
	}
	if (ladderAt(cells, col, row))
		for (int dir : {-1, 1})
			if (ladderAt(cells, col, row + dir)) {
				outCol[n] = col;
				outRow[n++] = row + dir;
			}
	return n;
}

void ClimbMap::Build(const Tile* grid, int goalCol, int goalRow, ClimbAbility ability, int maxSteps) {
	cells = grid;
	how = ability;
	steps.fill(-1);
	if (!Stands(goalCol, goalRow))
		return;
	std::deque<int> open{goalRow * LEVEL_WIDTH + goalCol};
	steps[static_cast<size_t>(open.front())] = 0;
	while (!open.empty()) {
		const int cell = open.front();
		open.pop_front();
		const int step = steps[static_cast<size_t>(cell)];
		if (step >= maxSteps)
			continue;
		int cols[4], rows[4];
		const int n = neighbours(cell % LEVEL_WIDTH, cell / LEVEL_WIDTH, cols, rows);
		for (int k = 0; k < n; k++) {
			const size_t next = indexOf(cols[k], rows[k]);
			if (steps[next] >= 0)
				continue;
			steps[next] = static_cast<int16_t>(step + 1);
			open.push_back(static_cast<int>(next));
		}
	}
}

int ClimbMap::Steps(int col, int row) const { return inBounds(col, row) ? steps[indexOf(col, row)] : -1; }

bool ClimbMap::Next(int col, int row, int& outCol, int& outRow) const {
	const int here = Steps(col, row);
	if (here <= 0)
		return false;
	int cols[4], rows[4];
	const int n = neighbours(col, row, cols, rows);
	for (int k = 0; k < n; k++)
		if (Steps(cols[k], rows[k]) == here - 1) {
			outCol = cols[k];
			outRow = rows[k];
			return true;
		}
	return false;
}
