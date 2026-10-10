#ifndef CLIMB_PATH_H
#define CLIMB_PATH_H

// A climber's way to the player across floors (docs/plan/monster-climbers.md): the cells it can stand in, the ladders
// between them and, for a walk-jumper, its leaps over gaps. On the level's cells (LEVEL_WIDTH x LEVEL_HEIGHT, row 0
// the bottom), so the sim and the unit tests share it.

#include "level.h"
#include <array>
#include <cstdint>

// A walker cannot step into it: out of the level, solid (a wall, deep water, a closed gate), a trap unless it is
// reckless, or no floor under it.
[[nodiscard]] bool walkerBlockedAt(const Tile* cells, int col, int row, bool reckless = false);
// From blocked cell (col, row) on, going dir (-1 / +1): the first cell a leap lands on, past up to
// MONSTER_JUMP_MAX_GAP pits and traps it can walk on. -1: no such cell (a wall, or the gap is too wide).
[[nodiscard]] int leapLandingAt(const Tile* cells, int col, int row, int dir);

struct ClimbAbility {
	bool reckless = false; // walks through traps
	bool leaps = false;	   // a walk-jumper: leaps over gaps
};

// Steps from every cell to one goal cell (the player's), up to a cap. A step: to the next cell along a row, a rung up
// or down a ladder, or a leap over a gap. A climber stands in a cell with a floor under it or on a ladder.
class ClimbMap {
  public:
	// cells must outlive the map (Next reads them).
	void Build(const Tile* grid, int goalCol, int goalRow, ClimbAbility ability, int maxSteps);
	[[nodiscard]] int Steps(int col, int row) const; // -1: out of reach (or past the cap)
	// The next cell on a shortest way from (col, row) to the goal. False: out of reach, or at the goal.
	bool Next(int col, int row, int& outCol, int& outRow) const;
	// It can stand there: open, with a floor under it (no trap unless reckless) or a ladder.
	[[nodiscard]] bool Stands(int col, int row) const;

  private:
	std::array<int16_t, static_cast<size_t>(LEVEL_WIDTH) * LEVEL_HEIGHT> steps{};
	const Tile* cells = nullptr;
	ClimbAbility how;

	// The cells one step from (col, row), at most 4; returns how many.
	int neighbours(int col, int row, int (&outCol)[4], int (&outRow)[4]) const;
};

#endif
