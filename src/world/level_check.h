#ifndef LEVEL_CHECK_H
#define LEVEL_CHECK_H

// Level validator: can the player get from the entrance to the exit, how big is the level, how hard is it.
// Models the player's movement from Dungeon (dungeon_base.cpp) on whole cells:
//   - walk left / right into any open cell (not a wall, not a closed gate); without a floor below, fall straight down
//     until there is one or a ladder catches the player;
//   - climb up / down between vertically adjacent ladder cells;
//   - jump over one cell, a gap in the floor, a spike / death trap on it or quicksand, from a floor (a ladder's foot,
//     sand and quicksand too; the jump peaks 0.4 tiles high, Jump::ARC in movement.h, so no step up onto a ledge),
//     never out of half water (walking through it is slower, which the checker does not count);
//   - sand slows the walk, quicksand sinks the player (plan desert-chapter): both cost a little more on the path, a run
//     of quicksand is at most QUICKSAND_MAX_RUN cells (checkSand);
//   - touching a key, or pulling a lever not pulled yet, opens every gate of its colour from then on (in level data a
//     gate is closed only with value 0);
//   - interacting with a teleporter moves the player to its partner (both ways); a name gate too (the name spoken),
//     but the exit, the keys, the levers and the name cartouche must be found with it sealed;
//   - reaching the boss's cell counts as killing it: the boss gates (BOSS_LOCK) open;
//   - turning a mirror (plan sunlight-mirrors) lights every sun disc a beam can reach with the mirrors the player
//     reaches from there set as needed (sun_beam.h): the gates of its colour open; a disc lit as the level starts
//     opens them from the start (and is warned about);
//   - wall spikes (plan rhythm-traps) go back in, so they never block for good; a run of them in a row must be
//     crossable at walking speed, which is checked on its own (checkWallSpikes), not on the path.
// A Death cell with no way out (a spike pit) kills the player; any other dead end is a softlock.

#include "level.h"
#include "sun_beam.h"
#include <string>
#include <vector>

struct CellPos {
	int col = -1;
	int row = -1;
};

// How the player gets into a path cell from the one before it.
enum class PathMove : unsigned char { Start, Walk, Drop, Climb, Jump, Pull, Teleport, Turn };

struct LevelReport {
	bool valid = false;
	std::vector<std::string> errors; // the level cannot be finished (or has no start / goal)
	std::vector<std::string> warnings; // it can be finished, but something is off

	CellPos start;
	int entrances = 0;
	int exits = 0; // exit gates + ankhs
	bool finale = false; // the goal is the ankh (wins the game) rather than an exit gate

	// Size
	int openCells = 0; // every non-wall cell
	int reachableCells = 0; // cells the player can stand in
	int boundsWidth = 0; // bounding box of the open cells
	int boundsHeight = 0;

	// Content, whole level
	int monsters[MONSTER_TYPE_MAX + 1] = {}; // by MonsterTypeId (index 0 = unknown type)
	int monsterCount = 0;
	int spikes = 0, deathTraps = 0, rockFalls = 0, dartPlates = 0, wallSpikes = 0, treasures = 0, keys = 0, gates = 0,
		levers = 0, riddles = 0;
	int sandFalls = 0; // among the rock falls (plan desert-chapter)
	int sandCells = 0, quicksandCells = 0; // sand counts the quicksand too
	int teleporters = 0;
	int lightShafts = 0, mirrors = 0, sunDiscs = 0; // plan sunlight-mirrors
	int nameGates = 0; // teleporters sealed by the level's name (plan secret-name-gates)
	int inscriptions = 0;
	int pages = 0, reachablePages = 0; // the earlier expedition's (plan expedition-notes)
	int nameCartouches = 0; // among the inscriptions
	int bosses = 0;
	int reachableTreasures = 0;
	int softlockCells = 0; // reachable cells from which the goal cannot be reached (spike pits not counted)

	// Cheapest path from the entrance to the goal (hazards cost extra, see level_check.cpp)
	std::vector<CellPos> path;
	std::vector<PathMove> pathMoves; // one per path cell (Start for the first; Pull and Turn stay in the cell)
	// Per Turn on the path, in order: how the mirrors the player reaches are set for the sun disc it lights.
	std::vector<std::vector<MirrorSetting>> pathTurnWays;
	int pathLength = 0; // moves
	int pathJumps = 0, pathDrops = 0, pathClimb = 0, pathTeleports = 0, pathTurns = 0;
	int pathSpikes = 0, pathDeathTraps = 0, pathRockFalls = 0, pathDartPlates = 0, pathWallSpikes = 0, pathGates = 0,
		pathMonsters = 0;
	int pathSandFalls = 0, pathSand = 0, pathQuicksand = 0; // pathRockFalls counts no sand falls
	int keysNeeded = 0; // colours the path has to collect before a gate
	bool pathBoss = false; // the path goes through a boss gate: the boss has to die

	float difficulty = 0.f; // see difficultyScore(); ~0 trivial, 10 hard, 20+ brutal
};

[[nodiscard]] LevelReport checkLevel(const LevelGrid& grid);

// Wall spikes (plan rhythm-traps): can the player walk through the run of them from col to col + count - 1 on row,
// without stopping, in the direction dir (+1 / -1), at some start time? Out, a cell blocks; coming out on the player,
// it stabs.
[[nodiscard]] bool wallSpikesCrossable(const LevelGrid& grid, int col, int row, int count, int dir);

// Threat of one monster of this MonsterTypeId in the difficulty score.
[[nodiscard]] float monsterThreat(int type);

// The level as text, top row first (wall rows above and below the level are skipped), one tileGlyph per cell
// (legend: glyphLegend in tile_defs.h, `levelcheck --legend`; m, q, Q: a monster type, key or gate colour the game
// does not know); a cell without an object shows its fixture. With a report, the path is drawn as '*' over open
// cells. A level with water or sand gets a second drawing after a "structure" line, the same rows in structureGlyph
// (level.h); a level with fixtures one after a "wall" line, the same rows, the fixtures alone ('#' / '.': none).
[[nodiscard]] std::string renderLevel(const LevelGrid& grid, const LevelReport* report);

#endif
