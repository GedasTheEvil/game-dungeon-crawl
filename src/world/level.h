#ifndef LEVEL_H
#define LEVEL_H

// Level data without rendering: tile types, the cell grid and the file format.
// Shared by the game (Dungeon) and the command line tools (levelcheck, levelgen), so no GL here.

#include <cstdint>
#include <iosfwd>
#include <string>

constexpr int LEVEL_WIDTH = 40;
constexpr int LEVEL_HEIGHT = 47;
constexpr int LEVEL_CELL_COUNT = LEVEL_WIDTH * LEVEL_HEIGHT + 1; // + 1 spare cell, kept for the file format

enum DungeonTileType : unsigned char {
	Wall = 0,
	Empty = 1,
	Door = 2,
	Death = 3,
	MonsterSpawn = 4,
	Spike = 5,
	Ladder = 6,
	Area3D = 7,
	Treasure = 8,
	Ankh = 9,
	Key = 10,	// b = lock colour; picked up on touch, then the cell is Empty
	Gate = 11,	// b = lock colour or BOSS_LOCK, c: 0 closed, 2 opening, 1 open; only open gates let the player through
	Lever = 12, // b = lock colour, c = 1 when pulled; pulling opens every gate of that colour
	RockFall = 13, // loose ceiling, walkable; c: 0 armed, 2 falling, 1 fallen
};

enum GateType : unsigned char {
	GateEntrance = 1,
	GateExit = 2,
	GateRiddle = 3,
	GateEmpty = 4,
	GateTeleport = 5, // value = pair id: interact to jump to the other teleporter with the same id
};

enum MonsterTypeId : unsigned char {
	MonsterScarab = 1,
	MonsterWorm = 2,
	MonsterPlant = 3,
	MonsterAnubis = 4,
	MonsterRat = 5,
	MonsterGiantRat = 6,
	MonsterBat = 7,
	MonsterGiantBat = 8,
	MonsterMimic = 9,
	MonsterGiantScarab = 10,
	MonsterBossScarab = 11, // boss: summons scarabs; its death opens the boss gates
};
constexpr int MONSTER_TYPE_MAX = MonsterBossScarab; // names, glyphs, threat, boss: monster_kinds.h

// Keys, gates and levers of one colour belong together. Colour ids run from 1 to LOCK_COLOUR_COUNT.
constexpr int LOCK_COLOUR_COUNT = 4;
inline bool isLockColour(int colour) { return colour >= 1 && colour <= LOCK_COLOUR_COUNT; }
// A gate's colour can also be the boss lock: no key or lever, the level's boss dying opens it.
constexpr int BOSS_LOCK = LOCK_COLOUR_COUNT + 1;
inline bool isGateColour(int colour) { return isLockColour(colour) || colour == BOSS_LOCK; }

struct LockColour {
	const char* name; // texture names (key_<name>.png, ...), messages
	const char* gem;  // the key's gem, in the game's messages
	char keyGlyph;	  // levelcheck --map, ASCII level sources; the boss lock has no key
	char gateGlyph;
	const char* choice; // the editor's list
};
// By colour id - 1, the boss lock last.
constexpr LockColour LOCK_COLOURS[BOSS_LOCK] = {
	{"red", "Carnelian", 'r', 'R', "Red (Carnelian)"},
	{"blue", "Lapis", 'b', 'B', "Blue (Lapis)"},
	{"green", "Turquoise", 'g', 'G', "Green (Turquoise)"},
	{"gold", "Amber", 'y', 'Y', "Gold (Amber)"},
	{"boss", "Obsidian", '\0', 'Z', "Boss gate, opens when the level's boss dies"},
};
// colour must be isGateColour.
inline const LockColour& lockColour(int colour) { return LOCK_COLOURS[colour - 1]; }

// One level cell, as in the level files and the editor: tile type, attribute and value.
struct Tile {
	int type;  // DungeonTileType
	int attr;  // attribute: meaning depends on the type (monster type, lock colour, ...)
	int value; // value: meaning depends on the type (gate state, ...)
};

// A Gate's and a RockFall's value: their state, as stored in level files and save games.
enum class GateState : std::uint8_t { Closed = 0, Open = 1, Opening = 2 };
enum class RockState : std::uint8_t { Armed = 0, Fallen = 1, Falling = 2 };
[[nodiscard]] inline GateState gateState(const Tile& t) { return static_cast<GateState>(t.value); }
[[nodiscard]] inline RockState rockState(const Tile& t) { return static_cast<RockState>(t.value); }
inline void setGateState(Tile& t, GateState s) { t.value = static_cast<int>(s); }
inline void setRockState(Tile& t, RockState s) { t.value = static_cast<int>(s); }
// A Lever's value: 0 up, 1 pulled.
[[nodiscard]] inline bool leverPulled(const Tile& t) { return t.value != 0; }
inline void pullLever(Tile& t) { t.value = 1; }
// A teleporter's value: its pair id.
[[nodiscard]] inline int teleportPair(const Tile& t) { return t.value; }

// Blocks the player (walls and gates not fully open). Everything else is open space.
inline bool isSolidTile(const Tile& t) { return t.type == Wall || (t.type == Gate && gateState(t) != GateState::Open); }

// Row 0 is the bottom of the level; index = row * LEVEL_WIDTH + column.
struct LevelGrid {
	Tile cells[LEVEL_CELL_COUNT] = {};

	[[nodiscard]] static bool inBounds(int col, int row) {
		return col >= 0 && col < LEVEL_WIDTH && row >= 0 && row < LEVEL_HEIGHT;
	}
	// Out of bounds reads as Wall.
	[[nodiscard]] Tile at(int col, int row) const {
		return inBounds(col, row) ? cells[row * LEVEL_WIDTH + col] : Tile{Wall, 0, 0};
	}
	void set(int col, int row, Tile t) {
		if (inBounds(col, row))
			cells[row * LEVEL_WIDTH + col] = t;
	}
};

// Where in the partner's cell the player arrives (Dungeon::Teleport), for the checker's path scripts too.
constexpr float TELEPORT_ARRIVAL_X = 0.5f;
inline bool isTeleporter(const Tile& t) { return t.type == Door && t.attr == GateTeleport; }
// Cell index of the other teleporter with the same pair id, -1 if there is none. cells: a whole level.
[[nodiscard]] int teleportPartner(const Tile* cells, int index);

// teleportPartner for every cell, found once (it scans the whole level): for code that asks often, like the checker.
class TeleportPairs {
  public:
	explicit TeleportPairs(const Tile* cells);
	[[nodiscard]] int Partner(int index) const { return partners[index]; }

  private:
	int partners[LEVEL_WIDTH * LEVEL_HEIGHT];
};

// Cell list after the header, as in the level files and save games. False on a short read.
bool readLevelCells(std::istream& in, Tile* cells, int cellCount);
// Error message, empty on success.
std::string loadLevelFile(const char* path, LevelGrid& grid);
bool saveLevelFile(const char* path, const LevelGrid& grid);

#endif
