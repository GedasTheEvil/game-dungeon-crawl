#ifndef LEVEL_BUILDER_H
#define LEVEL_BUILDER_H

// The level generator's builder (level_gen.h): one candidate level, built in phases. Its members by phase:
// level_gen.cpp (build), level_gen_layout.cpp (the route, halls, wings, branches, water and sand),
// level_gen_gates.cpp (locks, gates, gate rooms, the boss room), level_gen_content.cpp (monsters, traps, treasure,
// supplies) and level_gen_dressing.cpp (texts, light, figures, props).

#include "campaign.h"
#include "decor.h"
#include "items.h"
#include "level_gen.h"
#include "monster_kinds.h"
#include "rng.h"
#include "tile_defs.h"
#include <vector>

namespace levelgen {
constexpr int MIN_COL = 1; // the outer ring stays wall
constexpr int MAX_COL = LEVEL_WIDTH - 2;
constexpr int MIN_ROW = 1;
constexpr int MAX_ROW = LEVEL_HEIGHT - 2;
constexpr int CELLS = LEVEL_WIDTH * LEVEL_HEIGHT;
constexpr int MAX_ATTEMPTS = 80;
constexpr int SEGMENT_TRIES = 60;
constexpr int MONSTER_GAP = 3; // cells between monsters in a row
constexpr float GOOD_ENOUGH = 0.1f; // stop searching within 10 % of the target score
constexpr int MIMIC_MIN_DIFFICULTY = 2;
constexpr float MIMIC_CHANCE = 0.15f; // of the treasure chests from MIMIC_MIN_DIFFICULTY on
// Of those mimics from CURSED_MIMIC_MIN_DIFFICULTY on, the share that is cursed (plan cursed-mimic).
constexpr int CURSED_MIMIC_MIN_DIFFICULTY = 8;
constexpr float CURSED_MIMIC_SHARE = 0.3f;
// Of the other mimics from MIMIC_JAR_MIN_DIFFICULTY on, the share that is a mimic jar (plan tomb-curse-containers):
// with the tombs' decorations (decorTier).
constexpr int MIMIC_JAR_MIN_DIFFICULTY = 6;
constexpr float MIMIC_JAR_SHARE = 0.3f;

// Zone levels (plan campaign-levelgen): the campaign's shape.
constexpr int HALL_FULL = MAX_COL - MIN_COL + 1; // a hall across the whole grid
constexpr int HALL_MIN = 16;
constexpr int WING_MIN = 2; // cells of a hall's end off the route that make a wing
constexpr int LEDGE_WING = 7; // a wing this long in a tall hall can end in a ledge
constexpr int MAX_HUBS = 2;
constexpr int MAX_LOOPS = 3;
constexpr int MAX_LEDGES = 3;
constexpr float CLIMB_CHANCE = 0.25f; // a level climbed from the bottom to the top
constexpr float DROP_CHANCE = 0.25f; // of the links down
constexpr float HUB_CHANCE = 0.2f;
constexpr float LOOP_CHANCE = 0.4f;
constexpr float LEDGE_CHANCE = 0.5f;
constexpr float WING_TREASURE = 0.6f; // a wing no key or lever took
// A small zone level (the first zones' ramp, plan denser-levels): only as many halls as it needs, each about this
// share (percent) of the width, and at least this many.
constexpr int SMALL_LEVEL_CELLS = 450;
constexpr int SMALL_LEVEL_FILL = 70;
constexpr size_t SMALL_LEVEL_HALLS = 4;
constexpr float ZONE_CALM = 2.5f; // the no-feature weight along a zone level's halls (fillRun)
constexpr int ZONE_SCORE_CELLS = 50; // a zone level aims at targetScore(d) per this many open cells
// The zone's terrain (ZoneTerrain, campaign.h).
constexpr int WATER_CLEAR = 2; // cells of dry floor between the water and a drop: the jump over the hole
constexpr int POOL_DEPTH = 2; // rows of deep water under a pool, at most
constexpr int QUICKSAND_RUN = 2; // QUICKSAND_MAX_RUN in the checker
constexpr float SAND_FALL_SHARE = 0.5f; // of the rock falls in a zone with much sand
constexpr int TORCH_GAP = 8; // between the torches in a dark hall
constexpr float MIRROR_CHANCE = 0.6f; // of the gates in a zone with mirrors: a sun disc opens it
// The zone's traps (ZoneTraps, campaign.h).
constexpr ZoneTraps TEST_LEVEL_TRAPS = {100, 100, 100, 0, 0, 0, 0, 0};
constexpr int GAUNTLET_MIN = 7; // cells, a trap at either end: odd
constexpr int GAUNTLET_MAX = 13;
constexpr int GAUNTLET_TRAPS = 3; // at least, else none
constexpr int GAUNTLET_CLEAR = 2; // free cells between a gauntlet and the ends of its crossing
constexpr int PAIR_MIN_DIFFICULTY = 6; // two rhythm traps side by side, in step
constexpr float PAIR_CHANCE = 0.3f;
constexpr int FAST_TRAP_MIN_DIFFICULTY = 6;

// The statue before a zone's boss gate (step 9), by ZoneId; -1 for none: the cave zones have none.
constexpr int ZONE_GUARDIAN_PROP[ZONE_ID_COUNT] = {
	-1, // tomb entrance
	-1, // bat galleries
	DECOR_CAT, // web halls
	DECOR_JACKAL, // priests' quarter
	DECOR_OSIRIS, // scorpion nest
	DECOR_OSIRIS, // serpent temple
	DECOR_OSIRIS, // cisterns of Sobek
	DECOR_JACKAL, // Seth's desert
	DECOR_OSIRIS, // land of Sokar
	DECOR_OSIRIS, // lake of fire
	DECOR_JACKAL, // necropolis
	DECOR_JACKAL, // hall of judgment
};

// A boss level's room (plan campaign-levelgen, step 6): sealed under the halls, a row of rock under it (and the
// level's outer wall), BOSS_ROOM_LENGTH cells long.
constexpr int BOSS_ROOM_ROW = MIN_ROW + 1;
constexpr int BOSS_ROOM_LENGTH = 18;
constexpr int BOSS_LEDGE = 3; // Apep's room: the ledges either side of his pit

// Its rows, by the boss: the flyers and the Anubis boss's guards get a tall room, Apep a pit between ledges.
constexpr int bossRoomHeight(int boss) {
	switch (boss) {
	case MonsterVampireBat:
	case MonsterAnubisBoss:
		return 3;
	case MonsterBossScarab:
	case MonsterMummyPriest:
	case MonsterRerek:
		return 1;
	default:
		return 2;
	}
}

enum class Link : unsigned char { None, LadderDown, LadderUp, Drop };

// What fillSegment() puts at a corridor cell; the kinds after Treasure only on zone levels (ZoneTraps).
enum class Feature : unsigned char { None, Pit, Spike, RockFall, Monster, Treasure, Dart, WallSpikes, FireVent, Drip };
constexpr int FEATURE_COUNT = 10;

struct Segment {
	int row = 0; // floor row (the player's row)
	int height = 1; // 1 = corridor, 2 or 3 = hall
	int x0 = 0, x1 = 0;
	int entry = 0; // column the player arrives at
	int exit = -1; // column the player leaves by
	Link link = Link::None; // to the next segment
	int loopCol = -1; // a second ladder to the next segment: a pillar loop (zone levels)
	bool dark = false; // no wall torch but a few (ZoneTerrain::dark)
};

// A hall's end off the route, from the link at inner to the rock at end (zone levels): a dead end for a key, a lever
// or treasure.
struct Wing {
	int seg = 0;
	int inner = 0;
	int end = 0;
	int fillEnd = 0; // where its hazards stop: end, or the ladder up to its ledge
	int itemRow = 0; // the floor at end: the hall's, or its ledge's
	bool entrySide = false; // before the segment's gate
	bool taken = false; // holds a key or a lever
};

// A gate on the route, on segment seg at col.
struct Lock {
	int seg = 0;
	int col = 0;
	int colour = 0;
};

class LevelBuilder {
  public:
	// openTarget: a zone level's target size, 0 for a test level; zone: its ZoneId; boss: the boss of a boss level, 0
	// for none.
	// content: the carried content (GenOptions, step 8).
	LevelBuilder(uint32_t seed, int difficulty, int openTarget, int zone, int boss, const GenOptions& content)
		: content(content), rng(seed), mimicRng(seed ^ 0x5eedc0deU), d(difficulty), zoneTarget(openTarget), boss(boss),
		  hallsFloor(boss != 0 ? BOSS_ROOM_ROW + bossRoomHeight(boss) + 1 : MIN_ROW),
		  terrain(isZone(zone) ? zoneTerrain(static_cast<ZoneId>(zone)) : ZoneTerrain{}),
		  fauna(isZone(zone) ? &zoneFauna(static_cast<ZoneId>(zone)) : nullptr),
		  traps(isZone(zone) ? zoneTraps(static_cast<ZoneId>(zone)) : TEST_LEVEL_TRAPS),
		  gates(isZone(zone) ? zoneGates(static_cast<ZoneId>(zone)) : ZoneGates{}), zoneId(zone) {
		for (Tile& t : g.cells)
			t = wallTile();
		used.assign(CELLS, 0);
		busy.assign(CELLS, 0);
		monsterCap = 4 + 2 * d;
		if (zoneTarget > 0)
			monsterCap = monsterCap * zoneTarget / 100;
	}

	bool build(LevelGrid& out);

	[[nodiscard]] const GenShape& Shape() const { return shape; }

  private:
	const GenOptions& content;
	Rng rng;
	Rng mimicRng; // its own stream, so mimics leave the layout rolls of a seed alone
	int d;
	int zoneTarget;
	int boss;
	int hallsFloor; // the lowest row a hall may stand on: over the boss room on a boss level
	Segment bossRoom;
	ZoneTerrain terrain; // all None on a test level
	const ZoneFauna* fauna; // the zone's monsters, nullptr on a test level (monsters by difficulty)
	ZoneTraps traps; // TEST_LEVEL_TRAPS on a test level
	ZoneGates gates; // none on a test level
	int zoneId; // NO_ZONE on a test level
	int monsterCap;
	LevelGrid g;
	std::vector<char> used; // carved (anything but wall)
	std::vector<char> busy; // corridor cells taken by a connector or a feature
	std::vector<Segment> route;
	std::vector<Wing> wings;
	GenShape shape;
	int monsters = 0;
	int colours[LOCK_COLOUR_COUNT] = {1, 2, 3, 4};
	int locksPlaced = 0;
	std::vector<Lock> locks; // the route's gates, as placed
	int teleportPairs = 0;
	int boons = 0; // inscriptions with a boon placed (content.texts)
	size_t lorePlaced = 0; // of content.lore
	// A route fork (content.fork): the route's segment forkSeg is the left route's hall, forkRight the right one's,
	// side by side under the fork hall (forkSeg - 1), both dropping into the join hall (forkSeg + 1). 0: none.
	int forkSeg = 0;
	Segment forkRight;
	std::vector<Segment> tombRooms; // dressed as tombs at the end (step 9): secret and hidden rooms, tombs, the fork's
	int bossGate = -1; // the boss gate's column, the room's floor row and the way from the teleporter to it
	int bossGateRow = 0;
	int bossStep = 0;

	static int index(int col, int row) { return row * LEVEL_WIDTH + col; }
	static bool interior(int col, int row) {
		return col >= MIN_COL && col <= MAX_COL && row >= MIN_ROW && row <= MAX_ROW;
	}

	[[nodiscard]] bool rectFree(int x0, int x1, int y0, int y1) const {
		for (int y = y0; y <= y1; y++)
			for (int x = x0; x <= x1; x++)
				if (!LevelGrid::inBounds(x, y) || used[index(x, y)] != 0)
					return false;
		return true;
	}

	// The corridor and the wall ring around it must be uncarved and inside the level, so the new corridor touches
	// nothing but what is carved into it later on purpose (its connectors).
	[[nodiscard]] bool segmentFits(const Segment& s) const {
		if (!interior(s.x0, s.row - 1) || !interior(s.x1, s.row + s.height))
			return false;
		return rectFree(s.x0 - 1, s.x1 + 1, s.row - 1, s.row + s.height);
	}

	// Wall rows strictly between yLow and yHigh at col, with walls either side.
	[[nodiscard]] bool shaftFits(int col, int yLow, int yHigh) const {
		for (int y = yLow + 1; y < yHigh; y++)
			if (!interior(col, y) || !rectFree(col - 1, col + 1, y, y))
				return false;
		return true;
	}

	void carve(int col, int row, Tile t) {
		g.set(col, row, t);
		used[index(col, row)] = 1;
	}

	// Puts thing in its layer of the cell, which keeps its water or sand (setThing).
	void put(int col, int row, const Tile& thing) {
		Tile cell = g.at(col, row);
		setThing(cell, thing);
		g.set(col, row, cell);
	}

	// Open, with nothing in it or on its back wall: dry, wet or sandy.
	[[nodiscard]] bool freeCell(int col, int row) const {
		const Tile t = g.at(col, row);
		return !isSolidStructure(t.structure) && t.type == NoObject && !hasFixture(t);
	}

	void setStructure(int col, int row, Structure s) {
		Tile cell = g.at(col, row);
		cell.structure = s;
		g.set(col, row, cell);
	}

	void carveSegment(const Segment& s) {
		for (int y = s.row; y < s.row + s.height; y++)
			for (int x = s.x0; x <= s.x1; x++)
				carve(x, y, Tile{});
	}

	void carveLadder(int col, int yLow, int yHigh) {
		for (int y = yLow; y <= yHigh; y++)
			carve(col, y, Tile{Ladder, 0, 0});
		busy[index(col, yLow)] = busy[index(col, yHigh)] = 1;
	}

	void markBusy(const Segment& s, int col) {
		for (int x = col - 1; x <= col + 1; x++)
			if (x >= s.x0 && x <= s.x1)
				busy[index(x, s.row)] = 1;
	}

	[[nodiscard]] static int farEnd(const Segment& s, int from) { return from - s.x0 > s.x1 - from ? s.x0 : s.x1; }

	// level_gen_layout.cpp
	bool buildMainRoute();
	void placeDoors();
	void fillRock(int col, int row);
	void carveLink(const Segment& cur, const Segment& next, int col, Link link);
	bool buildHalls();
	int crossingCol(const Segment& hall, int lo, int hi);
	void addLoops();
	void addWing(int s, int inner, int end, bool entrySide);
	void lockSpan(size_t s, int& from, int& to) const;
	[[nodiscard]] static bool pastLock(const Wing& wing, int seg);
	int freeWing();
	bool sealedRoom(int length, Segment& room);
	void chooseFork();
	[[nodiscard]] int forkDrop(int b) const;
	void addWater();
	int flood(const Segment& hall, int x0, int x1, bool pool);
	[[nodiscard]] bool wettable(const Segment& hall, int x) const;
	[[nodiscard]] bool poolRock(int x, int y) const;
	void addSand();
	bool addNextSegment();
	bool tryBranch(int s, int lo, int hi, Tile item);

	// level_gen_gates.cpp
	bool placeBoss();
	void shapeBossRoom(const Segment& room, int mid, int step);
	void addGateRooms();
	void placeRiddle();
	void placeTeleporterRoom(bool named);
	void placeTomb();
	bool wingOpener(int s, Tile opener);
	[[nodiscard]] int mirrorCol(const Wing& wing) const;
	void placeSunDisc(Wing& wing, int colour);
	void placeLocks();

	// level_gen_content.cpp
	Tile rareChest();
	void placeForkContent();
	void fillWings();
	[[nodiscard]] bool plainPotionChest(const Tile& t) const;
	void placeSupplies();
	bool swapPlainChest(const Tile& chest);
	void placeAntidote(int poisonSeg);
	void placeHapi();
	void putHapi(int col, int row);
	Tile randomTreasure();
	Tile randomChest();
	[[nodiscard]] ItemKind randomWeapon(bool melee);
	static Tile chestOf(ItemKind kind);
	int randomMonster();
	int zoneMonster(int col, int row, bool dark);
	bool placeMonster(int col, int row, bool dark = false);
	[[nodiscard]] bool groupCell(int col, int row) const;
	void placeGroup(int col, int row, int type);
	[[nodiscard]] bool floorCell(const Segment& s, int col) const;
	[[nodiscard]] bool sandFloor(const Segment& s, int col) const;
	[[nodiscard]] bool jumpable(const Segment& s, int col) const;
	bool placePit(const Segment& s, int col);
	Feature pickFeature(const float (&weights)[FEATURE_COUNT]);
	void fillSegment(const Segment& s);
	static float share(int percent);
	[[nodiscard]] bool trapFits(const Segment& s, int col, Feature kind) const;
	Tile rhythmTrap(int type);
	Tile trapTile(Feature kind);
	void addGauntlets();
	void fillRun(const Segment& s, int from, int to);

	// level_gen_dressing.cpp
	void dressRooms();
	[[nodiscard]] bool scatterOwns(int col, int row) const;
	void setProp(int col, int row, int prop);
	[[nodiscard]] bool hasBoon(int id) const;
	int pickLore();
	void addTexts();
	void addLight();
	void dressFigures();
	[[nodiscard]] bool figureCell(int col, int row) const;
	void chooseDark();
	void darken(const Segment& hall);
	[[nodiscard]] bool crack(const Segment& hall, int x) const;
};
} // namespace levelgen

#endif
