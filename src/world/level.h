#ifndef LEVEL_H
#define LEVEL_H

// Level data without rendering: tile types, the cell grid and the file format.
// A cell has three layers: its structure (what it is made of), at most one fixture on its back wall (the wall layer)
// and at most one object in it. Two more say how it is dressed (plan placed-decor): the decor layer (its prop) and the
// decal layer (its wall decal and torch).
// Shared by the game (Dungeon) and the command line tools (levelcheck, levelgen), so no GL here.

#include <cstdint>
#include <iosfwd>
#include <string>

constexpr int LEVEL_WIDTH = 40;
constexpr int LEVEL_HEIGHT = 47;
constexpr int LEVEL_CELL_COUNT = LEVEL_WIDTH * LEVEL_HEIGHT + 1; // + 1 spare cell, from the v1 file format

// The structure layer. Wall and DeepWater block; objects stand in Empty, HalfWater, Sand or Quicksand cells.
enum class Structure : std::uint8_t {
	Wall = 0,
	Empty = 1,
	HalfWater = 2,
	DeepWater = 3,
	Sand = 4, // a drift on the floor: the player walks slower and cannot sprint (plan desert-chapter)
	Quicksand = 5, // looks like sand; standing in it sinks the player, a jump frees them
};
constexpr int STRUCTURE_COUNT = 6;
[[nodiscard]] constexpr bool isStructure(int s) { return s >= 0 && s < STRUCTURE_COUNT; }
// Wall and deep water: they block, and no object may stand there.
[[nodiscard]] constexpr bool isSolidStructure(Structure s) { return s == Structure::Wall || s == Structure::DeepWater; }

// The object and wall layers share one number space. The numbers are the v1 file's tile types: 0 was Wall (now a
// structure), 7 the unused Area3D. Lever, Inscription, WallSpikes, SunDisc and FireVent are wall types (isWallType):
// fixtures.
enum DungeonTileType : unsigned char {
	NoObject = 1,
	Door = 2,
	Death = 3,
	MonsterSpawn = 4,
	Spike = 5,
	Ladder = 6,
	Treasure = 8,
	Ankh = 9,
	Key = 10, // b = lock colour; picked up on touch, then the cell has no object
	Gate = 11, // b = lock colour or BOSS_LOCK, c: 0 closed, 2 opening, 1 open; only open gates let the player through
	Lever = 12, // b = lock colour, c = 1 when pulled; pulling opens every gate of that colour
	RockFall = 13, // loose ceiling, walkable; b: 0 rock, 1 sand (SAND_FALL); c: 0 armed, 2 falling, 1 fallen
	DartPlate = 14, // pressure plate in the floor: poison darts from the back wall above it; c: 0 armed, 1 pressed
	Inscription = 15, // hieroglyphs on the back wall, b = text id (inscriptions.h): interact to read them
	WallSpikes = 16, // spikes on a cycle out of the back wall (plan rhythm-traps); b = speed, c = phase | live state
	// Sunlight mirrors (plan sunlight-mirrors, sun_beam.h): a beam of sunlight from a light shaft, bronze mirrors turn
	// it, a sun disc it falls on opens the gates of its colour.
	LightShaft =
		17, // sunlight through a crack in the rock: b = the beam's way (BeamDir: 0 down, 1 left, 2 up, 3 right)
	Mirror = 18, // a bronze mirror on a stand: c = the way it sends the beam (BeamDir); interact turns it clockwise
	SunDisc =
		19, // on the back wall: b = lock colour, c = 1 once lit; lit by the beam, it opens the gates of its colour
	// A torn page of the earlier expedition's notebook on a skeleton (plan expedition-notes): b = its number (1 to
	// EXPEDITION_PAGE_COUNT), c = 1 once taken; picked up on touch, the skeleton stays.
	Page = 20,
	// A trapped treasure (plan trapped-chests): a Treasure (attr, value: the item) whose opening sets off a trap.
	TrappedTreasure = 21,
	// A vent low in the back wall that breathes fire up through its cell on the wall spikes' cycle (plan
	// fire-jet-trap); b = speed, c = phase | live state, as WallSpikes.
	FireVent = 22,
};
// The earlier expedition's notebook pages over the campaign (plan expedition-notes), numbered in order of depth.
constexpr int EXPEDITION_PAGE_COUNT = 24;
[[nodiscard]] constexpr bool isExpeditionPage(int number) { return number >= 1 && number <= EXPEDITION_PAGE_COUNT; }
// The types that hang on or come out of the back wall: they go in the wall layer (plan wall-layer), not the object's.
[[nodiscard]] constexpr bool isWallType(int type) {
	return type == Lever || type == Inscription || type == WallSpikes || type == SunDisc || type == FireVent;
}
// The rhythm traps (plans rhythm-traps, fire-jet-trap): wall spikes and fire vents, on the same cycle (spike_cycle.h).
[[nodiscard]] constexpr bool isRhythmTrap(int type) { return type == WallSpikes || type == FireVent; }

enum GateType : unsigned char {
	GateEntrance = 1,
	GateExit = 2,
	GateRiddle = 3,
	GateEmpty = 4,
	GateTeleport = 5, // value = pair id (| NAME_SEAL): interact to jump to the other teleporter with the same id
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
	MonsterVampireBat = 12, // boss: summons bats, heals by part of the damage it deals
	MonsterMummy = 13, // lies in its coffin until the player comes near
	MonsterAnubisBoss = 14, // boss: mummies climb out of the coffins round it
	MonsterCrocodile = 15, // lies under the water until the player comes near, swims fast
	MonsterScorpion = 16, // its sting poisons (weak)
	MonsterCobra = 17, // lies coiled until the player comes near; its bite and spit poison (medium)
	MonsterGiantCobra = 18, // the cobra's giant kin
	MonsterGiantScorpion = 19, // the scorpion's giant kin, its sting poisons (medium); the scorpion queen's minion
	MonsterEggCluster = 20, // rooted, harmless: the scorpion queen's minions hatch from it
	MonsterScorpionQueen = 21, // boss: giant scorpions hatch from the egg clusters; her sting poisons (strong)
	MonsterApep = 22, // boss: dives into the floor and comes up behind the player; cobras dig out
	MonsterSobek = 23, // boss: lies in the water, charges along the row; crocodiles come out
	MonsterLotus = 24, // rooted in half water, a floating flower until the player comes near; its bite drains stamina
	MonsterMummyPriest = 25, // the mummies' boss: lies in a large coffin, a staff, a purple bolt, many curses
	MonsterCamelSpider = 26, // fast, runs at the player in the dark, keeps out of the light of torches and braziers
	MonsterHippo = 27, // lies under the water, charges along the row; slow on land
	MonsterUshabti = 28, // stands among the decor ushabti as one of them until the player comes near, then walks
	MonsterCursedMimic = 29, // a mimic that hops after the player once woken; its bite may lay insanity
	MonsterSeth =
		30, // boss: the red hippo, lies in the water, charges, roars a sandstorm when stunned; hippos come out
	MonsterBaboon = 31, // keeps its distance (kites) and hurls burning coals; climbs, bites when cornered
	MonsterRerek = 32, // boss: the sand serpent, burrows in sand, comes up under or past the player, swallows; worms
	// boss: the camel spider's, puts out the torches and lamps she passes, spins webs on the floor; spiders drop
	MonsterMotherOfWebs = 33,
};
constexpr int MONSTER_TYPE_MAX = MonsterMotherOfWebs; // names, glyphs, threat, boss: monster_kinds.h

// Keys, gates and levers of one colour belong together. Colour ids run from 1 to LOCK_COLOUR_COUNT.
constexpr int LOCK_COLOUR_COUNT = 4;
inline bool isLockColour(int colour) { return colour >= 1 && colour <= LOCK_COLOUR_COUNT; }
// A gate's colour can also be the boss lock: no key or lever, the level's boss dying opens it.
constexpr int BOSS_LOCK = LOCK_COLOUR_COUNT + 1;
inline bool isGateColour(int colour) { return isLockColour(colour) || colour == BOSS_LOCK; }

struct LockColour {
	const char* name; // texture names (key_<name>.png, ...), messages
	const char* gem; // the key's gem, in the game's messages
	char keyGlyph; // levelcheck --map, ASCII level sources; the boss lock has no key
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

// What hangs on or comes out of a cell's back wall (the wall layer, plan wall-layer): a wall type (isWallType),
// attribute and value as for an object; NoObject for none.
struct Fixture {
	int type = NoObject;
	int attr = 0;
	int value = 0;
};

// The decor and decal layers of a cell (plan placed-decor): what a level designer put there over the scatter's roll
// (decor_scatter.h). A prop or decal is PLACED_UNSET (the scatter rolls as without the layers), PLACED_BARE (none) or
// its index (DECOR_NAMES, DECAL_DEFS in decor.h), with its mirror. The torch is a second field of the decal layer.
constexpr int PLACED_UNSET = -2;
constexpr int PLACED_BARE = -1;
enum class TorchPlacing : std::uint8_t { Unset = 0, None = 1, Torch = 2 };
constexpr int TORCH_PLACING_COUNT = 3;
struct Dressing {
	int prop = PLACED_UNSET;
	bool propMirror = false;
	int decal = PLACED_UNSET;
	bool decalMirror = false;
	TorchPlacing torch = TorchPlacing::Unset;
};
[[nodiscard]] inline bool hasPlacedProp(const Dressing& d) { return d.prop != PLACED_UNSET; }
[[nodiscard]] inline bool hasPlacedDecal(const Dressing& d) {
	return d.decal != PLACED_UNSET || d.torch != TorchPlacing::Unset;
}

// One level cell: the object (tile type, attribute and value), the structure it stands in, the fixture on its back
// wall and its dressing. Tile{Ladder, 0, 0} is a ladder in an empty cell; change the object of a cell already in a
// level with setObject, which keeps its structure and fixture.
struct Tile {
	int type = NoObject; // DungeonTileType
	int attr = 0; // attribute: meaning depends on the type (monster type, lock colour, ...)
	int value = 0; // value: meaning depends on the type (gate state, ...)
	Structure structure = Structure::Empty;
	Fixture fixture;
	Dressing dressing;
};
[[nodiscard]] inline Tile wallTile() { return Tile{NoObject, 0, 0, Structure::Wall}; }
[[nodiscard]] inline bool isWall(const Tile& t) { return t.structure == Structure::Wall; }
// Half water: open, the player and the walkers wade through it (crocodiles-and-flooded-cells).
[[nodiscard]] inline bool inHalfWater(const Tile& t) { return t.structure == Structure::HalfWater; }
// Sand or quicksand (plan desert-chapter): open, a drift on the floor; quicksand looks the same.
[[nodiscard]] inline bool inSand(const Tile& t) {
	return t.structure == Structure::Sand || t.structure == Structure::Quicksand;
}
[[nodiscard]] inline bool inQuicksand(const Tile& t) { return t.structure == Structure::Quicksand; }
[[nodiscard]] inline bool hasFixture(const Tile& t) { return t.fixture.type != NoObject; }
// Open, dry, no sand, and nothing in it or on its back wall: what a v1 Empty cell was.
[[nodiscard]] inline bool isEmptyCell(const Tile& t) {
	return t.structure == Structure::Empty && t.type == NoObject && !hasFixture(t);
}
// The object layer only.
[[nodiscard]] inline bool hasObject(const Tile& t) { return t.type != NoObject || t.attr != 0 || t.value != 0; }
// Puts object's type, attr and value in cell; the cell's structure and fixture stay.
inline void setObject(Tile& cell, const Tile& object) {
	cell.type = object.type;
	cell.attr = object.attr;
	cell.value = object.value;
}
inline void clearObject(Tile& cell) { setObject(cell, Tile{}); }
// A container with an item: a plain Treasure or a TrappedTreasure (plan trapped-chests).
[[nodiscard]] inline bool isTreasure(const Tile& t) { return t.type == Treasure || t.type == TrappedTreasure; }
inline void setFixture(Tile& cell, const Fixture& fixture) { cell.fixture = fixture; }
inline void clearFixture(Tile& cell) { cell.fixture = Fixture{}; }
// Puts thing's type, attr and value in its layer: a wall type as the fixture, anything else as the object. For the
// places a thing comes as a Tile (v2 files, the legend, the editor's brush).
inline void setThing(Tile& cell, const Tile& thing) {
	if (isWallType(thing.type))
		setFixture(cell, Fixture{thing.type, thing.attr, thing.value});
	else
		setObject(cell, thing);
}
// A cell of thing's structure with thing in its layer: cellWith({Lever, 1, 0}) is a red lever on an empty cell's back
// wall.
[[nodiscard]] inline Tile cellWith(const Tile& thing) {
	Tile cell;
	cell.structure = thing.structure;
	setThing(cell, thing);
	return cell;
}
// The fixture as a Tile in an empty cell, for code that reads either layer's things the same way (glyphs, hints).
[[nodiscard]] inline Tile fixtureAsTile(const Fixture& f) { return Tile{f.type, f.attr, f.value}; }

// A Gate's and a RockFall's value: their state, as stored in level files and save games.
enum class GateState : std::uint8_t { Closed = 0, Open = 1, Opening = 2 };
enum class RockState : std::uint8_t { Armed = 0, Fallen = 1, Falling = 2 };
// A RockFall's attribute: a rock, or a sand fall (plan desert-chapter) that pours from a ceiling crack and buries the
// cell under a drift (it turns to Sand): a lighter hit and a short slow, no stones.
constexpr int SAND_FALL = 1;
[[nodiscard]] inline bool isSandFall(const Tile& t) { return t.type == RockFall && t.attr == SAND_FALL; }
[[nodiscard]] inline GateState gateState(const Tile& t) { return static_cast<GateState>(t.value); }
[[nodiscard]] inline RockState rockState(const Tile& t) { return static_cast<RockState>(t.value); }
// A DartPlate's value: pressed from the volley until it re-arms (plan poison-dart-trap).
[[nodiscard]] inline bool platePressed(const Tile& t) { return t.type == DartPlate && t.value == 1; }
inline void setPlatePressed(Tile& t, bool pressed) { t.value = pressed ? 1 : 0; }
inline void setGateState(Tile& t, GateState s) { t.value = static_cast<int>(s); }
inline void setRockState(Tile& t, RockState s) { t.value = static_cast<int>(s); }
// A Lever's value: 0 up, 1 pulled.
[[nodiscard]] inline bool leverPulled(const Fixture& f) { return f.value != 0; }
inline void pullLever(Fixture& f) { f.value = 1; }
// A SunDisc's value: 0 dark, 1 lit (for good).
[[nodiscard]] inline bool sunDiscLit(const Fixture& f) { return f.value != 0; }
inline void lightSunDisc(Fixture& f) { f.value = 1; }
// A teleporter's value: its pair id, and NAME_SEAL on a name gate (plan secret-name-gates): it works only once the
// player has spoken the level's name, which takes the bit off for good (a plain teleporter then, both ways).
constexpr int NAME_SEAL = 256;
[[nodiscard]] inline int teleportPair(const Tile& t) { return t.value & (NAME_SEAL - 1); }
// A NoObject cell's attr: 0, or the monster type slain there (its spawn tile, plan no-respawn-on-load): it does not
// come back after a load, and the decorations stay as in its life (a mummy's coffin, a boss's coffins). Only save games
// have it.
[[nodiscard]] inline Tile slainObject(int monsterType) { return Tile{NoObject, monsterType, 0}; }
[[nodiscard]] inline int slainMonster(const Tile& t) { return t.type == NoObject ? t.attr : 0; }
// The monster type a cell spawns, or spawned before it was slain; 0 for none.
[[nodiscard]] inline int spawnType(const Tile& t) { return t.type == MonsterSpawn ? t.attr : slainMonster(t); }

// WallSpikes (plan rhythm-traps) and FireVent (plan fire-jet-trap): out of the back wall and back on a fixed cycle
// (spike_cycle.h), the vent's jet as the spikes. The attribute is the speed, the value's low bits the phase in eighths
// of the cycle; the game keeps the live state in the bits above it (level files leave them 0).
enum class SpikeSpeed : std::uint8_t { Normal = 0, Fast = 1, Slow = 2 };
constexpr int SPIKE_SPEED_COUNT = 3;
constexpr int SPIKE_PHASES = 8;
enum class SpikeState : std::uint8_t { Retracted = 0, Telegraph = 1, Extended = 2 };
[[nodiscard]] inline SpikeSpeed spikeSpeed(const Fixture& f) {
	return f.attr >= 0 && f.attr < SPIKE_SPEED_COUNT ? static_cast<SpikeSpeed>(f.attr) : SpikeSpeed::Normal;
}
[[nodiscard]] inline int spikePhase(const Fixture& f) { return f.value & (SPIKE_PHASES - 1); }
[[nodiscard]] inline SpikeState spikeState(const Fixture& f) { return static_cast<SpikeState>((f.value >> 3) & 3); }
inline void setSpikeState(Fixture& f, SpikeState s) { f.value = spikePhase(f) | (static_cast<int>(s) << 3); }
[[nodiscard]] inline bool spikesOut(const Fixture& f) {
	return f.type == WallSpikes && spikeState(f) == SpikeState::Extended;
}
[[nodiscard]] inline bool fireJetOn(const Fixture& f) {
	return f.type == FireVent && spikeState(f) == SpikeState::Extended;
}

// Blocks the player (walls, deep water, gates not fully open, wall spikes while out). Everything else is open space.
inline bool isSolidTile(const Tile& t) {
	return isSolidStructure(t.structure) || (t.type == Gate && gateState(t) != GateState::Open) || spikesOut(t.fixture);
}

// What a level says about itself, besides its cells (the file's lines before the layers).
constexpr int NO_ZONE = -1;
struct LevelMeta {
	int zone = NO_ZONE; // its campaign zone (ZoneId, campaign.h): its stone, decor, light and sound (zone_look.h)
	std::string title; // its name for the banner, the map and the save slots ("The Hall of Drifts"), empty: none
};

// Row 0 is the bottom of the level; index = row * LEVEL_WIDTH + column.
struct LevelGrid {
	Tile cells[LEVEL_CELL_COUNT];
	LevelMeta meta;

	LevelGrid(); // all wall, the rock a level is carved from

	[[nodiscard]] static bool inBounds(int col, int row) {
		return col >= 0 && col < LEVEL_WIDTH && row >= 0 && row < LEVEL_HEIGHT;
	}
	// Out of bounds reads as wall.
	[[nodiscard]] Tile at(int col, int row) const {
		// NOLINTNEXTLINE(clang-analyzer-security.ArrayBound): inBounds checks it; the analyzer loses that.
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
// A name gate not opened yet: it asks for the name instead of teleporting.
[[nodiscard]] inline bool isNameGate(const Tile& t) { return isTeleporter(t) && (t.value & NAME_SEAL) != 0; }
inline void unsealNameGate(Tile& t) { t.value &= ~NAME_SEAL; }
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

// The level format (docs/levels.md), in level files and save games. Version 5, text:
//   DCLEVEL 5 WIDTH HEIGHT
//   zone SLUG, the level's zone (ZoneInfo::slug, campaign.h), or "zone none"
//   title TEXT, optional: the level's title, the rest of the line (plan campaign-texts)
//   structure, then HEIGHT rows of WIDTH structure glyphs (STRUCTURE_GLYPHS), top row first
//   wall COUNT, then COUNT lines "COL ROW TYPE ATTR VALUE", the cells with a fixture (hasFixture)
//   objects COUNT, then COUNT lines "COL ROW TYPE ATTR VALUE", the cells with an object (hasObject)
//   decor COUNT, then COUNT lines "COL ROW PROP MIRROR", the cells with a placed prop (hasPlacedProp)
//   decals COUNT, then COUNT lines "COL ROW DECAL MIRROR TORCH", the cells with hasPlacedDecal (TORCH: TorchPlacing)
// PROP and DECAL as in Dressing: the index, -1 bare, -2 unset; MIRROR 0 or 1.
// Version 4 has no zone line: it loads with NO_ZONE.
// Version 3 has no decor and decal layers: every cell loads unset.
// Version 2 has no wall layer: its wall types stand in the object layer and move to the wall layer on load (setThing).
// Version 1, the old format: LEVEL_CELL_COUNT, then one "TYPE ATTR VALUE" per cell (type 0 a wall). Versions 1 to 4
// are read and converted, never written.
constexpr const char* LEVEL_MAGIC = "DCLEVEL";
constexpr int LEVEL_VERSION = 5;
constexpr char STRUCTURE_GLYPHS[STRUCTURE_COUNT + 1] = "#.~=,%"; // by Structure
[[nodiscard]] inline char structureGlyph(Structure s) { return STRUCTURE_GLYPHS[static_cast<int>(s)]; }

// A v1 cell: type 0 is a wall without an object, anything else a thing in an empty cell, in its layer (the unused
// Area3D, type 7, nothing).
[[nodiscard]] Tile convertV1Tile(int type, int attr, int value);

// Reads any version into cells (LEVEL_CELL_COUNT of them; the spare cell is left as it is). Error message, empty
// on success. version: the one read, meta: the level's, if not null.
std::string readLevel(std::istream& in, Tile* cells, int* version = nullptr, LevelMeta* meta = nullptr);
void writeLevel(std::ostream& out, const Tile* cells, const LevelMeta& meta = {});
// Error message, empty on success.
std::string loadLevelFile(const char* path, LevelGrid& grid, int* version = nullptr);
// The title line of a level file, empty for none or an unreadable file: reads the head only.
[[nodiscard]] std::string levelFileTitle(const char* path);
bool saveLevelFile(const char* path, const LevelGrid& grid);

#endif
