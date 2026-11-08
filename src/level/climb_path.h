#ifndef CLIMB_PATH_H
#define CLIMB_PATH_H

// A climber's way to the player across floors (plan monster-climbers): the cells it can stand in, the ladders between
// them and, for a walk-jumper, its leaps over gaps. On the level's cells (LEVEL_WIDTH x LEVEL_HEIGHT, row 0 the
// bottom), so the sim and the unit tests share it.

#include "level.h"
#include "gameplay_config.h"
#include <array>
#include <cstdint>
#include <functional>

// A monster's body against a cell's shape (plan subcell-monsters), from its kind (MonsterKind::heightQuarters,
// stepQuarters): an opening lower than its height stops it, a rise over its step stops it.
struct BodyFit {
	int heightQuarters = 0; // 0: no body, shapes do not stop it (the old cell rules)
	int stepQuarters = 0;
};
// The body at feet height h (tiles over the cell's bottom) steps into the sub-cell column `column` of cell t: no
// higher a rise than its step, an opening at least its height (a ceiling under the cell's top counts).
[[nodiscard]] bool columnFits(const Tile& t, int column, float h, BodyFit body);
// The rise into that column over h, in tiles; <= 0 a level walk or a drop.
[[nodiscard]] float columnRise(const Tile& t, int column, float h);
// Walking across shaped cell (col, row) going dir (-1 / +1), from the floor of the cell it comes from at their border:
// every sub-cell column fits the body. True for a cell with no shape, dir 0 or no body.
[[nodiscard]] bool shapePasses(const Tile* cells, int col, int row, int dir, BodyFit body);

// A trap a coward walker stops at (Courage): spikes, a death trap, a dart plate, a rock fall not yet fallen, rhythm
// spikes about to come out, a fire vent about to burn or burning, a poison drip's puddle.
[[nodiscard]] bool walkerTrapAt(const Tile& cell);
// A walker cannot step into it: out of the level, solid (a wall, deep water, a closed gate), a trap unless it is
// reckless, or no floor under it; going dir with a body, a shape it does not pass (shapePasses).
[[nodiscard]] bool
walkerBlockedAt(const Tile* cells, int col, int row, bool reckless = false, int dir = 0, BodyFit body = {});
// From blocked cell (col, row) on, going dir (-1 / +1): the first cell a leap lands on, past up to
// MONSTER_JUMP_MAX_GAP pits and traps it can walk on. -1: no such cell (a wall, or the gap is too wide).
// With a body: a shaped cell on the way that it does not pass going dir ends the leap (-1).
[[nodiscard]] int leapLandingAt(const Tile* cells, int col, int row, int dir, BodyFit body = {});
// From blocked cell (col, row) on, going dir: a ladder cell over the gap, up to reach cells on, whose rungs go on up
// or down, before any cell a leap would land on (plan jump-onto-ladder): a leaper catches its rungs in the air. -1:
// none (a wall, a shape it does not pass, or floor first).
[[nodiscard]] int leapCatchAt(const Tile* cells, int col, int row, int dir, int reach, BodyFit body = {});

struct ClimbAbility {
	bool reckless = false; // walks through traps
	bool leaps = false; // a walk-jumper: leaps over gaps
	BodyFit body{}; // its steps and the openings it fits (plan subcell-monsters)
	// A leaper's leap catches the rungs of a ladder over the gap up to this many cells out (plan jump-onto-ladder).
	int catchReach = MONSTER_JUMP_MAX_GAP;
	// Cells it will not stand in, on top of the level's own rules: a light shunner's lit ones (plan spider-climbers).
	// Empty: none.
	std::function<bool(int col, int row)> avoids;
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
	// It can stand there: open, with a floor under it (no trap unless reckless) or a ladder, and not a cell it avoids.
	[[nodiscard]] bool Stands(int col, int row) const;

  private:
	std::array<int16_t, static_cast<size_t>(LEVEL_WIDTH) * LEVEL_HEIGHT> steps{};
	const Tile* cells = nullptr;
	ClimbAbility how;

	// Along the row and leaps either way (2 each), up and down a ladder, the leaps that catch its rungs (one a side).
	static constexpr int NEIGHBOURS_MAX = 6;
	// The cells one step from (col, row), at most NEIGHBOURS_MAX; returns how many.
	int neighbours(int col, int row, int (&outCol)[NEIGHBOURS_MAX], int (&outRow)[NEIGHBOURS_MAX]) const;
};

#endif
