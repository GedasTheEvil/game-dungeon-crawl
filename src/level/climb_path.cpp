#include "climb_path.h"
#include "../core/gameplay_config.h"
#include <algorithm>
#include <deque>
#include <utility>

namespace {
bool inBounds(int col, int row) { return col >= 0 && col < LEVEL_WIDTH && row >= 0 && row < LEVEL_HEIGHT; }
const Tile& at(const Tile* cells, int col, int row) { return cells[row * LEVEL_WIDTH + col]; }
size_t indexOf(int col, int row) { return static_cast<size_t>(row) * LEVEL_WIDTH + static_cast<size_t>(col); }
bool ladderAt(const Tile* cells, int col, int row) { return inBounds(col, row) && at(cells, col, row).type == Ladder; }
constexpr float FIT_EPSILON = 1e-3f;
float columnX(int column) { return (static_cast<float>(column) + 0.5f) / SUBCELLS; }
} // namespace

float columnRise(const Tile& t, int column, float h) { return floorHeight(t, columnX(column)) - h; }

bool columnFits(const Tile& t, int column, float h, BodyFit body) {
	if (body.heightQuarters <= 0 || !hasShape(t))
		return true;
	const float floor = floorHeight(t, columnX(column));
	if (floor - h > static_cast<float>(body.stepQuarters) * SUBCELL_SIZE + FIT_EPSILON)
		return false; // too high a rise
	const float ceiling = ceilingHeight(t, columnX(column));
	return ceiling >= 1.f - FIT_EPSILON ||
		   ceiling - std::max(floor, h) >= static_cast<float>(body.heightQuarters) * SUBCELL_SIZE - FIT_EPSILON;
}

bool shapePasses(const Tile* cells, int col, int row, int dir, BodyFit body) {
	if (dir == 0 || body.heightQuarters <= 0 || !inBounds(col, row) || !hasShape(at(cells, col, row)))
		return true;
	float h = 0.f; // the floor at the border: the edge column of a shaped cell it comes from
	if (inBounds(col - dir, row) && hasShape(at(cells, col - dir, row)))
		h = floorHeight(at(cells, col - dir, row), columnX(dir > 0 ? SUBCELLS - 1 : 0));
	const Tile& t = at(cells, col, row);
	for (int k = 0; k < SUBCELLS; k++) {
		const int column = dir > 0 ? k : SUBCELLS - 1 - k;
		if (!columnFits(t, column, h, body))
			return false;
		h = floorHeight(t, columnX(column));
	}
	return true;
}

bool walkerBlockedAt(const Tile* cells, int col, int row, bool reckless, int dir, BodyFit body) {
	if (!inBounds(col, row))
		return true;
	const Tile& cell = at(cells, col, row);
	if (isSolidTile(cell))
		return true;
	const bool trap = cell.type == Spike || cell.type == Death || cell.type == DartPlate ||
					  (cell.type == RockFall && rockState(cell) != RockState::Fallen) ||
					  (cell.fixture.type == WallSpikes &&
					   spikeState(cell.fixture) == SpikeState::Telegraph) || // out: solid
					  (cell.fixture.type == FireVent &&
					   spikeState(cell.fixture) != SpikeState::Retracted) || // about to burn, burning
					  cell.fixture.type == PoisonDrip; // its puddle always poisons
	if (trap && !reckless)
		return true;
	return !inBounds(col, row - 1) || !isFloor(at(cells, col, row - 1)) || !shapePasses(cells, col, row, dir, body);
}

int leapLandingAt(const Tile* cells, int col, int row, int dir, BodyFit body) {
	if (!inBounds(col, row) || isSolidTile(at(cells, col, row)))
		return -1; // a wall, not a gap
	for (int k = 1; k <= MONSTER_JUMP_MAX_GAP; k++) {
		const int c = col + dir * k;
		if (!inBounds(c, row) || isSolidTile(at(cells, c, row)) || !shapePasses(cells, c, row, dir, body))
			return -1; // a wall, or a shape it does not pass: no leap over it
		if (!walkerBlockedAt(cells, c, row, false, dir, body))
			return c;
	}
	return -1;
}

int leapCatchAt(const Tile* cells, int col, int row, int dir, int reach, BodyFit body) {
	if (!inBounds(col, row) || isSolidTile(at(cells, col, row)))
		return -1;
	for (int k = 1; k <= reach; k++) {
		const int c = col + dir * k;
		if (!inBounds(c, row) || isSolidTile(at(cells, c, row)) || !shapePasses(cells, c, row, dir, body))
			return -1;
		if (ladderAt(cells, c, row) && (ladderAt(cells, c, row + 1) || ladderAt(cells, c, row - 1)))
			return c;
		if (!walkerBlockedAt(cells, c, row, false, dir, body))
			return -1; // a floor first: the leap lands there
	}
	return -1;
}

bool ClimbMap::Stands(int col, int row) const {
	if (!inBounds(col, row) || isSolidTile(at(cells, col, row)) || (how.avoids && how.avoids(col, row)))
		return false;
	return ladderAt(cells, col, row) || !walkerBlockedAt(cells, col, row, how.reckless);
}

int ClimbMap::neighbours(int col, int row, int (&outCol)[NEIGHBOURS_MAX], int (&outRow)[NEIGHBOURS_MAX]) const {
	int n = 0;
	for (int dir : {-1, 1}) {
		if (Stands(col + dir, row) && shapePasses(cells, col + dir, row, dir, how.body)) {
			outCol[n] = col + dir;
			outRow[n++] = row;
		} else if (how.leaps && !ladderAt(cells, col, row) && walkerBlockedAt(cells, col + dir, row, how.reckless)) {
			const int land = leapLandingAt(cells, col + dir, row, dir, how.body);
			if (land >= 0 && Stands(land, row)) {
				outCol[n] = land;
				outRow[n++] = row;
			}
			// A ladder over the gap: the leap catches its rungs (plan jump-onto-ladder).
			if (const int grab = leapCatchAt(cells, col + dir, row, dir, how.catchReach, how.body);
				grab >= 0 && grab != land) {
				outCol[n] = grab;
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
	// The way back of a leap that catches these rungs: the path is built from the goal, its steps must go both ways.
	if (how.leaps && ladderAt(cells, col, row))
		for (int dir : {-1, 1})
			for (int k = 2; k <= how.catchReach + 1; k++) {
				const int from = col - dir * k; // the take-off cell, the leap going dir
				if (inBounds(from, row) && !ladderAt(cells, from, row) && Stands(from, row) &&
					leapCatchAt(cells, from + dir, row, dir, how.catchReach, how.body) == col) {
					outCol[n] = from;
					outRow[n++] = row;
					break;
				}
			}
	return n;
}

void ClimbMap::Build(const Tile* grid, int goalCol, int goalRow, ClimbAbility ability, int maxSteps) {
	cells = grid;
	how = std::move(ability);
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
		int cols[NEIGHBOURS_MAX], rows[NEIGHBOURS_MAX];
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
	int cols[NEIGHBOURS_MAX], rows[NEIGHBOURS_MAX];
	const int n = neighbours(col, row, cols, rows);
	for (int k = 0; k < n; k++)
		if (Steps(cols[k], rows[k]) == here - 1) {
			outCol = cols[k];
			outRow = rows[k];
			return true;
		}
	return false;
}
