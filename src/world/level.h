#ifndef LEVEL_H
#define LEVEL_H

// Level data without rendering: tile types, the cell grid and the file format.
// Shared by the game (Dungeon) and the command line tools (levelcheck, levelgen), so no GL here.

#include <iosfwd>
#include <string>

constexpr int LEVEL_WIDTH = 40;
constexpr int LEVEL_HEIGHT = 47;
constexpr int LEVEL_CELL_COUNT = LEVEL_WIDTH * LEVEL_HEIGHT + 1; // + 1 spare cell, kept for the file format

enum DungeonTileType {
	Wall = 0,
	Empty = 1,
	Door = 2,
	Death = 3,
	Monster = 4,
	Spike = 5,
	Ladder = 6,
	Area3D = 7,
	Treasure = 8,
	Ankh = 9,
	Key = 10,	   // b = lock colour; picked up on touch, then the cell is Empty
	Gate = 11,	   // b = lock colour, c: 0 closed, 2 opening, 1 open; only open gates let the player through
	Lever = 12,	   // b = lock colour, c = 1 when pulled; pulling opens every gate of that colour
	RockFall = 13, // loose ceiling, walkable; c: 0 armed, 2 falling, 1 fallen
};

enum GateType {
	GateEntrance = 1,
	GateExit = 2,
	GateRiddle = 3,
	GateEmpty = 4,
};

enum MonsterType {
	MonsterScarab = 1,
	MonsterWorm = 2,
	MonsterPlant = 3,
	MonsterAnubis = 4,
	MonsterRat = 5,
	MonsterGiantRat = 6,
	MonsterBat = 7,
	MonsterGiantBat = 8,
};
constexpr int MONSTER_TYPE_MAX = MonsterGiantBat;

// Keys, gates and levers of one colour belong together. Colour ids run from 1 to LOCK_COLOUR_COUNT.
constexpr int LOCK_COLOUR_COUNT = 4;
constexpr const char* LOCK_COLOUR_NAMES[LOCK_COLOUR_COUNT] = {"red", "blue", "green", "gold"};
constexpr const char* LOCK_GEM_NAMES[LOCK_COLOUR_COUNT] = {"Carnelian", "Lapis", "Turquoise", "Amber"};
inline bool isLockColour(int colour) { return colour >= 1 && colour <= LOCK_COLOUR_COUNT; }

struct Tint {
	int a; // DungeonTileType
	int b; // attribute
	int c; // value
};

// Blocks the player (walls and closed gates). Everything else is open space.
inline bool isSolidTile(const Tint& t) { return t.a == Wall || (t.a == Gate && t.c != 1); }

// Row 0 is the bottom of the level; index = row * LEVEL_WIDTH + column.
struct LevelGrid {
	Tint cells[LEVEL_CELL_COUNT] = {};

	[[nodiscard]] static bool inBounds(int col, int row) {
		return col >= 0 && col < LEVEL_WIDTH && row >= 0 && row < LEVEL_HEIGHT;
	}
	// Out of bounds reads as Wall.
	[[nodiscard]] Tint at(int col, int row) const {
		return inBounds(col, row) ? cells[row * LEVEL_WIDTH + col] : Tint{Wall, 0, 0};
	}
	void set(int col, int row, Tint t) {
		if (inBounds(col, row))
			cells[row * LEVEL_WIDTH + col] = t;
	}
};

// Cell list after the header, as in the level files and save games. False on a short read.
bool readLevelCells(std::istream& in, Tint* cells, int cellCount);
// Error message, empty on success.
std::string loadLevelFile(const char* path, LevelGrid& grid);
bool saveLevelFile(const char* path, const LevelGrid& grid);

#endif
