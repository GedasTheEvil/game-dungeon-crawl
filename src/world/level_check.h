#ifndef LEVEL_CHECK_H
#define LEVEL_CHECK_H

// Level validator: can the player get from the entrance to the exit, how big is the level, how hard is it.
// Models the player's movement from Dungeon (dungeon_base.cpp) on whole cells:
//   - walk left / right into any open cell (not a wall, not a closed gate); without a floor below, fall straight down
//     until there is one or a ladder catches the player;
//   - climb up / down between vertically adjacent ladder cells;
//   - jump over a one-cell gap in the floor (the jump is ~0.45 tiles high, so no step up onto a ledge);
//   - touching a key, or pulling a lever, opens every gate of its colour from then on.
// A Death cell with no way out (a spike pit) kills the player; any other dead end is a softlock.

#include "level.h"
#include <string>
#include <vector>

struct CellPos {
	int col = -1;
	int row = -1;
};

// How the player gets into a path cell from the one before it.
enum class PathMove : unsigned char { Start, Walk, Drop, Climb, Jump, Pull };

struct LevelReport {
	bool valid = false;
	std::vector<std::string> errors;   // the level cannot be finished (or has no start / goal)
	std::vector<std::string> warnings; // it can be finished, but something is off

	CellPos start;
	int entrances = 0;
	int exits = 0;		 // exit gates + ankhs
	bool finale = false; // the goal is the ankh (wins the game) rather than an exit gate

	// Size
	int openCells = 0;		// every non-wall cell
	int reachableCells = 0; // cells the player can stand in
	int boundsWidth = 0;	// bounding box of the open cells
	int boundsHeight = 0;

	// Content, whole level
	int monsters[MONSTER_TYPE_MAX + 1] = {}; // by MonsterType (index 0 = unknown type)
	int monsterCount = 0;
	int spikes = 0, deathTraps = 0, rockFalls = 0, treasures = 0, keys = 0, gates = 0, levers = 0, riddles = 0;
	int reachableTreasures = 0;
	int softlockCells = 0; // reachable cells from which the goal cannot be reached (spike pits not counted)

	// Cheapest path from the entrance to the goal (hazards cost extra, see level_check.cpp)
	std::vector<CellPos> path;
	std::vector<PathMove> pathMoves; // one per path cell (Start for the first; Pull stays in the cell)
	int pathLength = 0;				 // moves
	int pathJumps = 0, pathDrops = 0, pathClimb = 0;
	int pathSpikes = 0, pathDeathTraps = 0, pathRockFalls = 0, pathGates = 0, pathMonsters = 0;
	int keysNeeded = 0; // colours the path has to collect before a gate

	float difficulty = 0.f; // see difficultyScore(); ~0 trivial, 10 hard, 20+ brutal
};

[[nodiscard]] LevelReport checkLevel(const LevelGrid& grid);

// Threat of one monster of this MonsterType in the difficulty score.
[[nodiscard]] float monsterThreat(int type);

// The level as text, top row first (wall rows above and below the level are skipped). Legend:
//   # wall  . open  S entrance  E exit  A ankh  ? riddle gate  D other gate  H ladder  $ treasure
//   ^ spikes  X death trap  v rock fall  monsters: s scarab w worm p plant n anubis t rat T giant rat
//   f bat F giant bat m other
//   keys r b g y, gates R B G Y (red, blue, green, gold), / lever
// With a report, the path is drawn as '*' over open cells.
[[nodiscard]] std::string renderLevel(const LevelGrid& grid, const LevelReport* report);

#endif
