#ifndef LEVEL_H
#define LEVEL_H

// Level data without rendering: tile types, the cell grid and the file format.
// A cell has two layers: its structure (what it is made of) and at most one object in it.
// Shared by the game (Dungeon) and the command line tools (levelcheck, levelgen), so no GL here.

#include <cstdint>
#include <iosfwd>
#include <string>

constexpr int LEVEL_WIDTH = 40;
constexpr int LEVEL_HEIGHT = 47;
constexpr int LEVEL_CELL_COUNT = LEVEL_WIDTH * LEVEL_HEIGHT + 1; // + 1 spare cell, from the v1 file format

// The structure layer. Wall and DeepWater block; objects stand in Empty or HalfWater cells.
enum class Structure : std::uint8_t {
	Wall = 0,
	Empty = 1,
	HalfWater = 2,
	DeepWater = 3,
};
constexpr int STRUCTURE_COUNT = 4;
[[nodiscard]] constexpr bool isStructure(int s) { return s >= 0 && s < STRUCTURE_COUNT; }
// Wall and deep water: they block, and no object may stand there.
[[nodiscard]] constexpr bool isSolidStructure(Structure s) { return s == Structure::Wall || s == Structure::DeepWater; }

// The object layer. The numbers are the v1 file's tile types: 0 was Wall (now a structure), 7 the unused Area3D.
enum DungeonTileType : unsigned char {
	NoObject = 1,
	Door = 2,
	Death = 3,
	MonsterSpawn = 4,
	Spike = 5,
	Ladder = 6,
	Treasure = 8,
	Ankh = 9,
	Key = 10,	// b = lock colour; picked up on touch, then the cell has no object
	Gate = 11,	// b = lock colour or BOSS_LOCK, c: 0 closed, 2 opening, 1 open; only open gates let the player through
	Lever = 12, // b = lock colour, c = 1 when pulled; pulling opens every gate of that colour
	RockFall = 13,	// loose ceiling, walkable; c: 0 armed, 2 falling, 1 fallen
	DartPlate = 14, // pressure plate in the floor: poison darts from the back wall above it; c: 0 armed, 1 pressed
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
	MonsterBossScarab = 11,	   // boss: summons scarabs; its death opens the boss gates
	MonsterVampireBat = 12,	   // boss: summons bats, heals by part of the damage it deals
	MonsterMummy = 13,		   // lies in its coffin until the player comes near
	MonsterAnubisBoss = 14,	   // boss: mummies climb out of the coffins round it
	MonsterCrocodile = 15,	   // lies under the water until the player comes near, swims fast
	MonsterScorpion = 16,	   // its sting poisons (weak)
	MonsterCobra = 17,		   // lies coiled until the player comes near; its bite and spit poison (medium)
	MonsterGiantCobra = 18,	   // the cobra's giant kin
	MonsterGiantScorpion = 19, // the scorpion's giant kin, its sting poisons (medium); the scorpion queen's minion
	MonsterEggCluster = 20,	   // rooted, harmless: the scorpion queen's minions hatch from it
	MonsterScorpionQueen = 21, // boss: giant scorpions hatch from the egg clusters; her sting poisons (strong)
	MonsterApep = 22,		   // boss: dives into the floor and comes up behind the player; cobras dig out
	MonsterSobek = 23,		   // boss: lies in the water, charges along the row; crocodiles come out
};
constexpr int MONSTER_TYPE_MAX = MonsterSobek; // names, glyphs, threat, boss: monster_kinds.h

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

// One level cell: the object (tile type, attribute and value) and the structure it stands in.
// Tile{Ladder, 0, 0} is a ladder in an empty cell; change the object of a cell already in a level with setObject,
// which keeps its structure.
struct Tile {
	int type = NoObject; // DungeonTileType
	int attr = 0;		 // attribute: meaning depends on the type (monster type, lock colour, ...)
	int value = 0;		 // value: meaning depends on the type (gate state, ...)
	Structure structure = Structure::Empty;
};
[[nodiscard]] inline Tile wallTile() { return Tile{NoObject, 0, 0, Structure::Wall}; }
[[nodiscard]] inline bool isWall(const Tile& t) { return t.structure == Structure::Wall; }
// Half water: open, the player and the walkers wade through it (crocodiles-and-flooded-cells).
[[nodiscard]] inline bool inHalfWater(const Tile& t) { return t.structure == Structure::HalfWater; }
// Open, dry and nothing in it: what a v1 Empty cell was.
[[nodiscard]] inline bool isEmptyCell(const Tile& t) { return t.structure == Structure::Empty && t.type == NoObject; }
[[nodiscard]] inline bool hasObject(const Tile& t) { return t.type != NoObject || t.attr != 0 || t.value != 0; }
// Puts object's type, attr and value in cell; the cell's structure stays.
inline void setObject(Tile& cell, const Tile& object) {
	cell.type = object.type;
	cell.attr = object.attr;
	cell.value = object.value;
}
inline void clearObject(Tile& cell) { setObject(cell, Tile{}); }

// A Gate's and a RockFall's value: their state, as stored in level files and save games.
enum class GateState : std::uint8_t { Closed = 0, Open = 1, Opening = 2 };
enum class RockState : std::uint8_t { Armed = 0, Fallen = 1, Falling = 2 };
[[nodiscard]] inline GateState gateState(const Tile& t) { return static_cast<GateState>(t.value); }
[[nodiscard]] inline RockState rockState(const Tile& t) { return static_cast<RockState>(t.value); }
// A DartPlate's value: pressed from the volley until it re-arms (docs/plan/poison-dart-trap.md).
[[nodiscard]] inline bool platePressed(const Tile& t) { return t.type == DartPlate && t.value == 1; }
inline void setPlatePressed(Tile& t, bool pressed) { t.value = pressed ? 1 : 0; }
inline void setGateState(Tile& t, GateState s) { t.value = static_cast<int>(s); }
inline void setRockState(Tile& t, RockState s) { t.value = static_cast<int>(s); }
// A Lever's value: 0 up, 1 pulled.
[[nodiscard]] inline bool leverPulled(const Tile& t) { return t.value != 0; }
inline void pullLever(Tile& t) { t.value = 1; }
// A teleporter's value: its pair id.
[[nodiscard]] inline int teleportPair(const Tile& t) { return t.value; }
// A NoObject cell's attr: 0, or the monster type of the boss slain there (its spawn tile), so its chamber keeps its
// coffins after a load. Only save games have it.
[[nodiscard]] inline Tile slainBossObject(int bossType) { return Tile{NoObject, bossType, 0}; }
[[nodiscard]] inline int slainBoss(const Tile& t) { return t.type == NoObject ? t.attr : 0; }

// Blocks the player (walls, deep water and gates not fully open). Everything else is open space.
inline bool isSolidTile(const Tile& t) {
	return isSolidStructure(t.structure) || (t.type == Gate && gateState(t) != GateState::Open);
}

// Row 0 is the bottom of the level; index = row * LEVEL_WIDTH + column.
struct LevelGrid {
	Tile cells[LEVEL_CELL_COUNT];

	LevelGrid(); // all wall, the rock a level is carved from

	[[nodiscard]] static bool inBounds(int col, int row) {
		return col >= 0 && col < LEVEL_WIDTH && row >= 0 && row < LEVEL_HEIGHT;
	}
	// Out of bounds reads as wall.
	[[nodiscard]] Tile at(int col, int row) const {
		return inBounds(col, row) ? cells[row * LEVEL_WIDTH + col] : wallTile();
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

// The level format (docs/levels.md), in level files and save games. Version 2, text:
//   DCLEVEL 2 WIDTH HEIGHT
//   structure, then HEIGHT rows of WIDTH structure glyphs (STRUCTURE_GLYPHS), top row first
//   objects COUNT, then COUNT lines "COL ROW TYPE ATTR VALUE", the cells with an object (hasObject)
// Version 1, the old format: LEVEL_CELL_COUNT, then one "TYPE ATTR VALUE" per cell (type 0 a wall). It is read and
// converted (convertV1Tile), never written.
constexpr const char* LEVEL_MAGIC = "DCLEVEL";
constexpr int LEVEL_VERSION = 2;
constexpr char STRUCTURE_GLYPHS[STRUCTURE_COUNT + 1] = "#.~="; // by Structure
[[nodiscard]] inline char structureGlyph(Structure s) { return STRUCTURE_GLYPHS[static_cast<int>(s)]; }

// A v1 cell in v2: type 0 is a wall without an object, anything else an object in an empty cell (the unused
// Area3D, type 7, no object).
[[nodiscard]] Tile convertV1Tile(int type, int attr, int value);

// Reads either version into cells (LEVEL_CELL_COUNT of them; the spare cell is left as it is). Error message, empty
// on success. version: the one read, if not null.
std::string readLevel(std::istream& in, Tile* cells, int* version = nullptr);
void writeLevel(std::ostream& out, const Tile* cells);
// Error message, empty on success.
std::string loadLevelFile(const char* path, LevelGrid& grid, int* version = nullptr);
bool saveLevelFile(const char* path, const LevelGrid& grid);

#endif
