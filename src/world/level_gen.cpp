#include "level_gen.h"
#include "campaign.h"
#include "decor.h"
#include "inscriptions.h"
#include "items.h"
#include "monster_kinds.h"
#include "rng.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <optional>
#include <vector>

namespace {
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

	bool build(LevelGrid& out) {
		if (!(zoneTarget > 0 ? buildHalls() : buildMainRoute()))
			return false;
		if (boss != 0) { // carved first: no pool digs into it
			bossRoom.row = BOSS_ROOM_ROW;
			bossRoom.height = bossRoomHeight(boss);
			bossRoom.x0 = rng.range(MIN_COL + 1, MAX_COL - BOSS_ROOM_LENGTH);
			bossRoom.x1 = bossRoom.x0 + BOSS_ROOM_LENGTH - 1;
			carveSegment(bossRoom);
		}
		if (zoneTarget > 0) {
			addWater();
			addSand();
			chooseDark();
		}
		placeLocks();
		if (boss != 0 && !placeBoss())
			return false;
		addGauntlets();
		addGateRooms();
		if (zoneTarget > 0) {
			placeHapi();
			if (forkSeg > 0)
				placeForkContent();
			addTexts();
		}
		int branches = rng.range(1, 2) + d / 4;
		for (int i = 0; i < branches * 3 && branches > 0; i++) {
			int s = rng.range(0, static_cast<int>(route.size()) - 1);
			if (tryBranch(s, route[s].x0, route[s].x1, Tile{Treasure, 0, 0}))
				branches--;
		}
		fillWings();
		for (Segment& s : route)
			fillSegment(s);
		if (zoneTarget > 0) {
			placeSupplies();
			dressFigures();
			addLight();
		}
		out = g;
		return true;
	}

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
	// A route fork (content.fork): the route's segment forkSeg is the left route's hall, forkRight the right one's,
	// side by side under the fork hall (forkSeg - 1), both dropping into the join hall (forkSeg + 1). 0: none.
	int forkSeg = 0;
	Segment forkRight;

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

	bool buildMainRoute() {
		int count = 3 + d / 2 + rng.range(0, 1);
		Segment start;
		start.row = rng.range(30, 42);
		int length = rng.range(8, 14);
		start.x0 = rng.range(MIN_COL, MAX_COL - length + 1);
		start.x1 = start.x0 + length - 1;
		start.entry = rng.chance(0.5f) ? start.x0 : start.x1;
		carveSegment(start);
		route.push_back(start);

		while (static_cast<int>(route.size()) < count) {
			if (!addNextSegment())
				break;
		}
		if (route.size() < 3)
			return false;
		placeDoors();
		return true;
	}

	// The entrance at the first segment's entry, the exit at the far end of the last one.
	void placeDoors() {
		Segment& first = route.front();
		g.set(first.entry, first.row, Tile{Door, GateEntrance, 0});
		markBusy(first, first.entry);
		busy[index(first.entry + (first.entry == first.x0 ? 2 : -2), first.row)] = 1; // a free step out of the door

		Segment& last = route.back();
		last.exit = farEnd(last, last.entry);
		g.set(last.exit, last.row, Tile{Door, GateExit, 0});
		markBusy(last, last.exit);
	}

	void fillRock(int col, int row) {
		g.set(col, row, wallTile());
		used[index(col, row)] = 0;
	}

	// A ladder or a drop shaft from cur to next at col.
	void carveLink(const Segment& cur, const Segment& next, int col, Link link) {
		const Segment& upper = cur.row > next.row ? cur : next;
		const Segment& lower = cur.row > next.row ? next : cur;
		if (link == Link::Drop)
			for (int y = upper.row - 1; y >= lower.row + lower.height; y--)
				carve(col, y, Tile{});
		else
			carveLadder(col, lower.row, upper.row);
		markBusy(cur, col);
		markBusy(next, col);
	}

	// Zone levels (plan campaign-levelgen): bands of halls from the top of the grid to the bottom, each across most of
	// its width, one to three rows tall, two rows of rock between them (one under a pillar, up to five over a shaft).
	// The route crosses each hall from where it comes in to a link far along, down to the next (or up, on a level
	// climbed from the bottom); now and then a hub hall the shaft runs straight through, or a second ladder that makes
	// a pillar loop. The hall's ends off the route are its wings.
	bool buildHalls() {
		for (int top = MAX_ROW;;) {
			Segment hall;
			int roll = rng.range(0, 99);
			hall.height = roll < 55 ? 1 : roll < 85 ? 2 : 3;
			hall.row = top - hall.height + 1;
			if (hall.row < hallsFloor)
				break;
			route.push_back(hall);
			roll = rng.range(0, 99);
			int rock = roll < 15 ? 1 : roll < 85 ? 2 : rng.range(3, 5);
			top = hall.row - 1 - rock;
		}
		if (route.size() < 6)
			return false;
		int full = 0;
		for (const Segment& hall : route)
			full += hall.height * HALL_FULL;
		float scale = std::min(1.f, static_cast<float>(zoneTarget) / static_cast<float>(full));
		for (Segment& hall : route) {
			float spread = 0.85f + 0.3f * static_cast<float>(rng.range(0, 100)) / 100.f;
			int length = std::clamp(
				static_cast<int>(static_cast<float>(HALL_FULL) * scale * spread),
				HALL_MIN,
				HALL_FULL
			);
			hall.x0 = rng.range(MIN_COL, MAX_COL - length + 1);
			hall.x1 = hall.x0 + length - 1;
			shape.halls++;
			shape.tallHalls += hall.height > 1 ? 1 : 0;
		}
		if (rng.chance(CLIMB_CHANCE))
			std::reverse(route.begin(), route.end());
		const bool down = route.front().row > route.back().row;

		Segment& first = route.front();
		first.entry = rng.chance(0.5f) ? first.x0 : first.x1;
		for (size_t i = 0; i + 1 < route.size(); i++) {
			Segment& cur = route[i];
			Segment& next = route[i + 1];
			int lo = std::max(cur.x0, next.x0) + 1;
			int hi = std::min(cur.x1, next.x1) - 1;
			bool hub = i > 0 && shape.hubs < MAX_HUBS && route[i - 1].link != Link::Drop && cur.entry >= lo &&
					   cur.entry <= hi && cur.entry - cur.x0 > WING_MIN + 2 && cur.x1 - cur.entry > WING_MIN + 2 &&
					   rng.chance(HUB_CHANCE);
			int col = hub ? cur.entry : crossingCol(cur, lo, hi);
			if (col < 0)
				return false;
			cur.exit = next.entry = col;
			cur.link = !down ? Link::LadderUp : !hub && rng.chance(DROP_CHANCE) ? Link::Drop : Link::LadderDown;
			shape.hubs += hub ? 1 : 0;
		}
		if (content.fork > 0 && down)
			chooseFork();
		for (const Segment& hall : route)
			carveSegment(hall);
		if (forkSeg > 0) { // the right route: its hall, the drop into it and the drop out of it
			carveSegment(forkRight);
			carveLink(route[static_cast<size_t>(forkSeg - 1)], forkRight, forkRight.entry, Link::Drop);
			carveLink(forkRight, route[static_cast<size_t>(forkSeg) + 1], forkRight.exit, Link::Drop);
		}
		for (size_t i = 0; i + 1 < route.size(); i++) {
			const Segment& cur = route[i];
			const Segment& next = route[i + 1];
			carveLink(cur, next, cur.exit, cur.link);
			const int rock = std::abs(cur.row - next.row) - (cur.row > next.row ? next.height : cur.height);
			shape.shafts += rock >= 3 ? 1 : 0;
		}
		placeDoors();
		addLoops();
		for (int s = 0; s < static_cast<int>(route.size()); s++) {
			const Segment& hall = route[static_cast<size_t>(s)];
			int lo = std::min(hall.entry, hall.exit);
			int hi = std::max(hall.entry, hall.exit);
			addWing(s, lo, hall.x0, hall.entry == lo);
			addWing(s, hi, hall.x1, hall.entry == hi);
		}
		return true;
	}

	// Where the route leaves a hall: far from where it came in, inside the next hall too. -1 if there is no room.
	int crossingCol(const Segment& hall, int lo, int hi) {
		int cross = std::max(8, (hall.x1 - hall.x0) / 2 - 2);
		int leftHi = hall.entry - cross;
		int rightLo = hall.entry + cross;
		int left = leftHi - lo + 1;
		int right = hi - rightLo + 1;
		if (left <= 0 && right <= 0)
			return -1;
		if (right <= 0 || (left > 0 && rng.range(1, left + right) <= left))
			return rng.range(lo, leftHi);
		return rng.range(rightLo, hi);
	}

	// A second ladder between two halls the route crosses one way and then back: two ways down (or up) round the rock
	// between them, the pillar. No gate goes on the loop (lockSpan).
	void addLoops() {
		for (size_t i = 0; i + 1 < route.size() && shape.loops < MAX_LOOPS; i++) {
			if (forkSeg > 0 && static_cast<int>(i) >= forkSeg - 1 && static_cast<int>(i) <= forkSeg)
				continue; // a loop would join the two routes
			Segment& cur = route[i];
			const Segment& next = route[i + 1];
			int dir = cur.exit > cur.entry ? 1 : -1;
			if (cur.exit == cur.entry || next.exit == next.entry || (next.exit > next.entry ? 1 : -1) == dir)
				continue;
			int lo = dir > 0 ? std::max(cur.entry, next.exit) + 4 : cur.exit + 5;
			int hi = dir > 0 ? cur.exit - 5 : std::min(cur.entry, next.exit) - 4;
			if (lo > hi || !rng.chance(LOOP_CHANCE))
				continue;
			cur.loopCol = rng.range(lo, hi);
			carveLink(cur, next, cur.loopCol, cur.row > next.row ? Link::LadderDown : Link::LadderUp);
			shape.loops++;
		}
	}

	// The wing from inner to end, if it is long enough; in a tall hall, now and then ending in a ledge: the floor
	// raised a row over its last cells, up a ladder.
	void addWing(int s, int inner, int end, bool entrySide) {
		const int length = std::abs(end - inner);
		if (length < WING_MIN)
			return;
		const Segment& hall = route[static_cast<size_t>(s)];
		Wing wing{s, inner, end, end, hall.row, entrySide};
		if (hall.height > 1 && length >= LEDGE_WING && shape.ledges < MAX_LEDGES && rng.chance(LEDGE_CHANCE)) {
			const int dir = end > inner ? 1 : -1;
			const int width = rng.range(3, std::min(5, length - 4));
			for (int k = 0; k < width; k++)
				fillRock(end - k * dir, hall.row);
			wing.fillEnd = end - width * dir;
			carveLadder(wing.fillEnd, hall.row, hall.row + 1);
			wing.itemRow = hall.row + 1;
			shape.ledges++;
		}
		busy[index(end, wing.itemRow)] = 1; // its item's, whatever it gets: no branch's ladder
		wings.push_back(wing);
	}

	// The part of segment s a gate can go on, from to to: the whole crossing, but off a pillar loop.
	void lockSpan(size_t s, int& from, int& to) const {
		const Segment& seg = route[s];
		from = s > 0 && route[s - 1].loopCol >= 0 ? route[s - 1].loopCol : seg.entry;
		to = seg.loopCol >= 0 ? seg.loopCol : seg.exit;
	}

	// The wing lies past the gate on segment seg: on a later segment, or on seg's far side (the route is a line, the
	// gate off any pillar loop).
	[[nodiscard]] static bool pastLock(const Wing& wing, int seg) {
		return wing.seg > seg || (wing.seg == seg && !wing.entrySide);
	}

	// A free wing's end for a door or a chest: dry floor or a drift, nothing in it; -1 if none.
	int freeWing() {
		std::vector<size_t> free;
		for (size_t w = 0; w < wings.size(); w++) {
			const Wing& wing = wings[w];
			const Structure s = g.at(wing.end, wing.itemRow).structure;
			if (!wing.taken && freeCell(wing.end, wing.itemRow) && (s == Structure::Empty || s == Structure::Sand))
				free.push_back(w);
		}
		return free.empty()
				   ? -1
				   : static_cast<int>(free[static_cast<size_t>(rng.range(0, static_cast<int>(free.size()) - 1))]);
	}

	// A room sealed in the rock, one row tall and length cells long, rock all round it: only a teleporter leads in.
	bool sealedRoom(int length, Segment& room) {
		for (int tries = 0; tries < 300; tries++) {
			room = Segment{};
			room.row = rng.range(MIN_ROW + 1, MAX_ROW - 1);
			room.x0 = rng.range(MIN_COL + 1, MAX_COL - length);
			room.x1 = room.x0 + length - 1;
			if (segmentFits(room)) {
				carveSegment(room);
				return true;
			}
		}
		return false;
	}

	// A rare chest for a secret room or a tomb: an amulet of the depth's tier, or a rare potion.
	Tile rareChest() {
		constexpr ItemKind AMULETS[] = {
			ItemKind::StrengthLesser,
			ItemKind::ArmorLesser,
			ItemKind::HealthLesser,
			ItemKind::PoisonWardLesser,
			ItemKind::TrapWardLesser,
			ItemKind::BluntWardLesser,
			ItemKind::SlashWardLesser,
			ItemKind::PierceWardLesser,
			ItemKind::VenomLesser,
			ItemKind::FireWardLesser,
			ItemKind::CrushingLesser,
			ItemKind::CleavingLesser,
			ItemKind::PiercingLesser
		};
		constexpr ItemKind POTIONS[] =
			{ItemKind::LargeHealth, ItemKind::Life, ItemKind::Might, ItemKind::Armor, ItemKind::GreaterResistance};
		if (rng.chance(0.5f)) {
			const ItemKind lesser = AMULETS[rng.range(0, static_cast<int>(std::size(AMULETS)) - 1)];
			return chestOf(itemAt(itemIndex(lesser) + std::min(3, (d - 1) / 3))); // lesser, minor, normal, grand
		}
		return chestOf(POTIONS[rng.range(0, static_cast<int>(std::size(POTIONS)) - 1)]);
	}

	// A boss level (GenOptions::boss): the last gate on the last hall's crossing, a teleporter at a late wing's end
	// before it to the boss room sealed under the halls. There, from the teleporter: the boss in the middle, then the
	// boss gate and behind it the key of the last gate and a rare chest; the room shaped for the boss (bossRoomShape).
	bool placeBoss() {
		const int bossColour = colours[LOCK_COLOUR_COUNT - 1];
		const auto last = route.size() - 1;
		int from = 0, to = 0;
		lockSpan(last, from, to);
		const Segment& exitHall = route[last];
		const int dir = to > from ? 1 : -1;
		int gate = -1;
		for (int tries = 0; tries < 30 && gate < 0 && std::abs(to - from) > 4; tries++) {
			const int x = from + dir * rng.range(2, std::abs(to - from) - 2);
			if (busy[index(x, exitHall.row)] == 0 && isEmptyCell(g.at(x, exitHall.row)) &&
				isWall(g.at(x, exitHall.row - 1)))
				gate = x;
		}
		std::vector<size_t> late; // the free wings before it, the latest first
		for (size_t w = 0; w < wings.size(); w++) {
			const Wing& wing = wings[w];
			const Structure s = g.at(wing.end, wing.itemRow).structure;
			if (!wing.taken && freeCell(wing.end, wing.itemRow) && (s == Structure::Empty || s == Structure::Sand) &&
				(wing.seg < static_cast<int>(last) || wing.entrySide) && (forkSeg == 0 || wing.seg != forkSeg))
				late.push_back(w);
		}
		if (gate < 0 || late.empty())
			return false;
		std::stable_sort(late.begin(), late.end(), [this](size_t a, size_t b) { return wings[a].seg > wings[b].seg; });
		Wing& wing = wings[late[static_cast<size_t>(rng.range(0, std::min(3, static_cast<int>(late.size())) - 1))]];
		g.set(gate, exitHall.row, Tile{Gate, bossColour, 0});
		markBusy(exitHall, gate);
		locks.push_back(Lock{static_cast<int>(last), gate, bossColour});
		const int pair = ++teleportPairs;
		put(wing.end, wing.itemRow, Tile{Door, GateTeleport, pair});
		wing.taken = true;

		const Segment& room = bossRoom;
		const bool east = rng.chance(0.5f); // the teleporter's end
		const int step = east ? -1 : 1;
		auto at = [&room, east](int k) { return east ? room.x1 - k : room.x0 + k; };
		const int n = BOSS_ROOM_LENGTH;
		const int mid = n / 2;
		int floorRow = room.row; // the ends' floor: a ledge's in Apep's room
		if (boss == MonsterApep) {
			for (int k = 0; k < BOSS_LEDGE; k++) {
				fillRock(at(k), room.row);
				fillRock(at(n - 1 - k), room.row);
			}
			carveLadder(at(BOSS_LEDGE), room.row, room.row + 1);
			carveLadder(at(n - 1 - BOSS_LEDGE), room.row, room.row + 1);
			floorRow = room.row + 1;
		} else
			for (int k = n - 4; k < n; k++) // the boss gate's alcove, one row tall
				for (int y = room.row + 1; y < room.row + room.height; y++)
					fillRock(at(k), y);
		put(at(0), floorRow, Tile{Door, GateTeleport, pair});
		put(at(n - (boss == MonsterApep ? BOSS_LEDGE : 4)), floorRow, Tile{Gate, BOSS_LOCK, 0});
		put(at(n - 2), floorRow, Tile{Key, bossColour, 0});
		put(at(n - 1), floorRow, rareChest());
		for (int k = 0; k < n; k++)
			busy[index(at(k), floorRow)] = busy[index(at(k), room.row)] = 1;
		shapeBossRoom(room, at(mid), step);
		put(at(mid), room.row, Tile{MonsterSpawn, boss, 0});
		shape.bossRoom = true;
		return true;
	}

	// What the boss needs in its room, round its cell mid: the scorpion queen's egg clusters, water for Sobek, a pool
	// between drifts for Seth, a run of sand for Rerek, gaps for Babi to leap, braziers in the dark for the Mother of
	// Webs. The High Priest's coffins need the floor round him free, as it is.
	void shapeBossRoom(const Segment& room, int mid, int step) {
		auto structure = [&](int from, int to, Structure s) {
			for (int k = from; k <= to; k++)
				setStructure(mid + k * step, room.row, s);
		};
		switch (boss) {
		case MonsterScorpionQueen:
			for (int k : {-3, 3})
				put(mid + k * step, room.row, Tile{MonsterSpawn, MonsterEggCluster, 0});
			break;
		case MonsterSobek:
			structure(-6, 4, Structure::HalfWater);
			break;
		case MonsterSeth:
			structure(-6, -4, Structure::Sand);
			structure(-3, 2, Structure::HalfWater);
			structure(3, 4, Structure::Sand);
			break;
		case MonsterRerek:
			structure(-7, 4, Structure::Sand);
			break;
		case MonsterBabi:
			for (int k : {-3, 3}) {
				busy[index(mid + k * step, room.row)] = 0;
				busy[index(mid + (k - 1) * step, room.row)] = busy[index(mid + (k + 1) * step, room.row)] = 0;
				placePit(room, mid + k * step);
			}
			break;
		case MonsterMotherOfWebs:
			for (int x = room.x0; x <= room.x1; x++)
				for (int y = room.row; y < room.row + room.height; y++) {
					Tile cell = g.at(x, y);
					if (!isWall(cell)) {
						cell.dressing.torch = TorchPlacing::None;
						g.set(x, y, cell);
					}
				}
			for (int k : {-5, 4}) {
				Tile cell = g.at(mid + k * step, room.row);
				cell.dressing.prop = DECOR_BRAZIER;
				g.set(mid + k * step, room.row, cell);
			}
			break;
		default:
			break;
		}
	}

	// A route fork (plan denser-levels, Route fork): a hall B the route crosses leftwards, under a hall A, over a hall
	// C, is split by a pillar of rock under A's link: two drops side by side in A, one each side of it. The left
	// route crosses B's left part (long) and drops into C where the route did; the right one drops from B's right
	// part (short) a few cells along, its end a tomb. Both drops are one-way, so no way leads from one route to the
	// other. Before the halls are carved.
	void chooseFork() {
		std::vector<int> candidates;
		for (int b = 2; b + 1 < static_cast<int>(route.size()); b++) {
			const Segment& a = route[static_cast<size_t>(b - 1)];
			const Segment& hall = route[static_cast<size_t>(b)];
			const int e = hall.entry;
			if (hall.exit <= e - 8 && std::abs(a.entry - e) > 4 && a.x0 < e - 3 && e + 3 < a.x1 && hall.x0 < e - 3 &&
				hall.x1 >= e + 6 && forkDrop(b) >= 0)
				candidates.push_back(b);
		}
		if (candidates.empty())
			return;
		forkSeg = candidates[static_cast<size_t>(rng.range(0, static_cast<int>(candidates.size()) - 1))];
		Segment& a = route[static_cast<size_t>(forkSeg - 1)];
		Segment& hall = route[static_cast<size_t>(forkSeg)];
		const int e = hall.entry;
		forkRight = Segment{};
		forkRight.row = hall.row;
		forkRight.height = hall.height;
		forkRight.x0 = e + 1;
		forkRight.x1 = hall.x1;
		forkRight.entry = e + 2;
		forkRight.exit = forkDrop(forkSeg);
		forkRight.link = Link::Drop;
		hall.x1 = e - 1;
		a.exit = hall.entry = e - 2;
		a.link = hall.link = Link::Drop;
		busy[index(e, a.row)] = 1; // the warning's, between the drops
		shape.forks++;
	}

	// Where the right route of a fork at segment b drops into the hall under it: 2 to 5 cells along, over that hall,
	// 2 cells short of its end (the tomb); -1 if nowhere.
	[[nodiscard]] int forkDrop(int b) const {
		const Segment& hall = route[static_cast<size_t>(b)];
		const Segment& join = route[static_cast<size_t>(b) + 1];
		for (int x = hall.entry + 4; x <= std::min(hall.entry + 7, hall.x1 - 2); x++)
			if (x > join.x0 && x < join.x1 && std::abs(x - hall.exit) > 2)
				return x;
		return -1;
	}

	// The fork's content: the warning between the drops on the last fork (FORK_TEXT) and the expedition page on the
	// way to it, on the floor; traps on every other cell of the right route, at its end the remains of the earlier
	// expedition (a skeleton) and their gear (a weapon of the depth or a rare chest); the left route gets two monsters
	// more.
	void placeForkContent() {
		const Segment& a = route[static_cast<size_t>(forkSeg - 1)];
		const int between = a.exit + 2; // under it, the pillar between the two routes
		if (content.fork >= 2 && freeCell(between, a.row))
			put(between, a.row, Tile{Inscription, FORK_TEXT, 0});
		if (isExpeditionPage(content.page)) {
			const int dir = a.exit > a.entry ? 1 : -1;
			for (int k = 3; k <= 9; k++) {
				const int x = a.exit - k * dir;
				if ((x - a.entry) * dir > 1 && busy[index(x, a.row)] == 0 && isEmptyCell(g.at(x, a.row)) &&
					isWall(g.at(x, a.row - 1))) {
					put(x, a.row, Tile{Page, content.page, 0});
					busy[index(x, a.row)] = 1;
					shape.pages++;
					break;
				}
			}
		}
		const Segment& right = forkRight;
		constexpr Feature KINDS[] = {Feature::Spike, Feature::RockFall, Feature::Dart, Feature::FireVent};
		for (int x = right.entry + 1; x < right.exit; x += 2)
			for (int k = 0; k < 4; k++) {
				const Feature kind = KINDS[(x + k) % 4];
				if (trapFits(right, x, kind)) {
					put(x, right.row, trapTile(kind));
					busy[index(x, right.row)] = 1;
					break;
				}
			}
		const int end = right.x1;
		put(end, right.row, rng.chance(0.5f) ? chestOf(randomWeapon(true)) : rareChest());
		busy[index(end, right.row)] = 1;
		Tile bones = g.at(end - 1, right.row);
		if (freeCell(end - 1, right.row) && !hasPlacedProp(bones.dressing)) {
			bones.dressing.prop = DECOR_SKELETON;
			g.set(end - 1, right.row, bones);
			busy[index(end - 1, right.row)] = 1;
		}
		const Segment& left = route[static_cast<size_t>(forkSeg)];
		for (int n = 0, tries = 0; n < 2 && tries < 30; tries++)
			n += placeMonster(rng.range(left.x0 + 1, left.x1 - 1), left.row, left.dark) ? 1 : 0;
	}

	[[nodiscard]] bool hasBoon(int id) const {
		const InscriptionText* text = content.texts != nullptr ? content.texts->Find(id) : nullptr;
		return text != nullptr && text->boon.Any();
	}

	// A secret room's lore (SECRET_ROOM_TEXTS): with the texts, none with a boon if the level has one already.
	int pickLore() {
		std::vector<int> pool;
		for (int id : SECRET_ROOM_TEXTS)
			if (content.texts == nullptr || (content.texts->Find(id) != nullptr && (boons == 0 || !hasBoon(id))))
				pool.push_back(id);
		return pool.empty() ? SECRET_ROOM_TEXTS[0]
							: pool[static_cast<size_t>(rng.range(0, static_cast<int>(pool.size()) - 1))];
	}

	// The carried content (step 8): the inscriptions on the halls of the route, on the way (GenOptions::inscriptions,
	// else 2 or 3 of the zone's texts, 1 or 2 in the first zone, at most one boon a level; the name gate's hint only
	// with a name gate), and the expedition page on a skeleton at a wing's end.
	void addTexts() {
		std::vector<int> ids = content.inscriptions;
		if (ids.empty() && content.texts != nullptr && isZone(zoneId)) {
			std::vector<int> pool;
			for (int id = ZONE_TEXT_FIRST[zoneId]; id < ZONE_TEXT_FIRST[zoneId + 1]; id++)
				if (isInscriptionId(id) && id != RING_TEXT && id != FORK_TEXT && id != STARGATE_TEXT &&
					(id != NAME_GATE_TEXT || shape.nameGates > 0) && content.texts->Find(id) != nullptr)
					pool.push_back(id);
			for (int i = static_cast<int>(pool.size()) - 1; i > 0; i--)
				std::swap(pool[static_cast<size_t>(i)], pool[static_cast<size_t>(rng.range(0, i))]);
			const int wanted = zoneId == static_cast<int>(ZoneId::TombEntrance) ? rng.range(1, 2) : rng.range(2, 3);
			for (int id : pool) {
				if (static_cast<int>(ids.size()) >= wanted)
					break;
				if (hasBoon(id) && boons > 0)
					continue;
				boons += hasBoon(id) ? 1 : 0;
				ids.push_back(id);
			}
		}
		for (int id : ids)
			for (int tries = 0; tries < 60; tries++) {
				const Segment& hall = route[static_cast<size_t>(rng.range(0, static_cast<int>(route.size()) - 1))];
				const int x = rng.range(std::min(hall.entry, hall.exit), std::max(hall.entry, hall.exit));
				const Structure s = g.at(x, hall.row).structure;
				if (busy[index(x, hall.row)] == 0 && freeCell(x, hall.row) && isWall(g.at(x, hall.row - 1)) &&
					(s == Structure::Empty || s == Structure::Sand)) {
					put(x, hall.row, Tile{Inscription, id, 0});
					busy[index(x, hall.row)] = 1;
					shape.inscriptions++;
					break;
				}
			}
		if (isExpeditionPage(content.page) && shape.pages == 0) {
			const int w = freeWing();
			if (w >= 0) {
				Wing& wing = wings[static_cast<size_t>(w)];
				put(wing.end, wing.itemRow, Tile{Page, content.page, 0});
				wing.taken = true;
				shape.pages++;
			}
		}
	}

	// The zone's gates besides the locks (ZoneGates): riddle gates, a name gate, hidden rooms, treasure tombs.
	void addGateRooms() {
		for (int n = gates.riddles; n > 0; n--)
			placeRiddle();
		if (content.stargate || (gates.nameGatePercent > 0 && rng.percent(gates.nameGatePercent)))
			placeTeleporterRoom(true);
		for (int n = gates.hiddenRooms; n > 0; n--)
			placeTeleporterRoom(false);
		for (int n = gates.treasureTombs; n > 0; n--)
			placeTomb();
	}

	// A riddle gate: in a riddle room up a ladder off a hall, or at a wing's end.
	void placeRiddle() {
		const Tile riddle{Door, GateRiddle, 0};
		for (int tries = 0; tries < 3; tries++) {
			const int s = rng.range(0, static_cast<int>(route.size()) - 1);
			if (rng.chance(0.5f) && tryBranch(s, route[s].x0, route[s].x1, riddle)) {
				shape.riddleGates++;
				return;
			}
		}
		const int w = freeWing();
		if (w < 0)
			return;
		Wing& wing = wings[static_cast<size_t>(w)];
		put(wing.end, wing.itemRow, riddle);
		wing.taken = true;
		shape.riddleGates++;
	}

	// A teleporter at a wing's end to a sealed room with a chest at its far end (or two): a name gate (plan
	// secret-name-gates) with its name cartouche on a hall of the route, or a plain teleporter to a hidden room, now
	// and then with a guard from difficulty 6.
	void placeTeleporterRoom(bool named) {
		const int w = freeWing();
		if (w < 0)
			return;
		Wing& wing = wings[static_cast<size_t>(w)];
		int cartoucheCol = -1, cartoucheRow = -1;
		for (int tries = 0; named && tries < 60 && cartoucheCol < 0; tries++) {
			const Segment& hall = route[static_cast<size_t>(rng.range(0, static_cast<int>(route.size()) - 1))];
			const int x = rng.range(hall.x0, hall.x1);
			const Structure s = g.at(x, hall.row).structure;
			if (busy[index(x, hall.row)] == 0 && freeCell(x, hall.row) && isWall(g.at(x, hall.row - 1)) &&
				(s == Structure::Empty || s == Structure::Sand) &&
				(std::abs(x - wing.end) > 2 || std::abs(hall.row - wing.itemRow) > 1))
				cartoucheCol = x, cartoucheRow = hall.row;
		}
		Segment room;
		if ((named && cartoucheCol < 0) || !sealedRoom(rng.range(5, 7), room))
			return;
		const int pair = ++teleportPairs;
		const bool west = rng.chance(0.5f);
		put(wing.end, wing.itemRow, Tile{Door, GateTeleport, pair + (named ? NAME_SEAL : 0)});
		wing.taken = true;
		put(west ? room.x0 : room.x1, room.row, Tile{Door, GateTeleport, pair});
		const int far = west ? room.x1 : room.x0;
		put(far, room.row, named ? rareChest() : randomTreasure());
		if (!named && rng.chance(0.5f))
			put(far + (west ? -2 : 2), room.row, randomTreasure());
		else if (!named && d >= 6 && rng.chance(0.5f))
			placeMonster((room.x0 + room.x1) / 2, room.row);
		if (named) {
			// Its lore, or the ring's text in the Stargate chamber's stand-in.
			const int lore = content.stargate ? STARGATE_TEXT : pickLore();
			put((room.x0 + room.x1) / 2, room.row, Tile{Inscription, lore, 0});
			boons += hasBoon(lore) ? 1 : 0;
			put(cartoucheCol, cartoucheRow, Tile{Inscription, NAME_CARTOUCHE_ID, 0});
			busy[index(cartoucheCol, cartoucheRow)] = 1;
			shape.nameGates++;
		} else
			shape.hiddenRooms++;
	}

	// A treasure tomb: a wing past a gate of the route closed by a second gate of its colour, a rare chest at its end.
	void placeTomb() {
		std::vector<std::pair<size_t, int>> spots; // a wing and a lock colour
		for (size_t w = 0; w < wings.size(); w++)
			for (const Lock& lock : locks)
				if (!wings[w].taken && pastLock(wings[w], lock.seg) && std::abs(wings[w].fillEnd - wings[w].inner) >= 5)
					spots.emplace_back(w, lock.colour);
		if (spots.empty())
			return;
		const auto [w, colour] = spots[static_cast<size_t>(rng.range(0, static_cast<int>(spots.size()) - 1))];
		Wing& wing = wings[w];
		const Segment& hall = route[static_cast<size_t>(wing.seg)];
		const int dir = wing.end > wing.inner ? 1 : -1;
		const int gate = wing.inner + 2 * dir;
		const Tile t = g.at(gate, hall.row);
		if (busy[index(gate, hall.row)] != 0 || !freeCell(gate, hall.row) || inQuicksand(t) ||
			!freeCell(wing.end, wing.itemRow))
			return;
		g.set(gate, hall.row, Tile{Gate, colour, 0});
		markBusy(hall, gate);
		put(wing.end, wing.itemRow, rareChest());
		wing.taken = true;
		shape.treasureTombs++;
	}

	// A wing for the key or lever of a gate on segment s: one on s before the gate, or on a segment before it with no
	// drop on the way (the player could drop past it and be stuck at the gate).
	bool wingOpener(int s, Tile opener) {
		std::vector<size_t> free;
		for (size_t w = 0; w < wings.size(); w++) {
			const Wing& wing = wings[w];
			bool before = wing.seg < s || (wing.seg == s && wing.entrySide);
			for (int k = wing.seg; k < s && before; k++)
				before = route[static_cast<size_t>(k)].link != Link::Drop;
			if (!wing.taken && before && wing.seg != forkSeg) // a left route's wing: not on the right one
				free.push_back(w);
		}
		if (free.empty())
			return false;
		// A chain: the opener behind the nearest gate before this one, if a wing there is free.
		int last = -1;
		for (const Lock& lock : locks)
			if (lock.seg < s)
				last = std::max(last, lock.seg);
		std::vector<size_t> behind;
		for (size_t w : free)
			if (last >= 0 && pastLock(wings[w], last))
				behind.push_back(w);
		if (!behind.empty()) {
			free = behind;
			shape.chains++;
		}
		if (terrain.mirrors && rng.chance(MIRROR_CHANCE)) {
			std::vector<size_t> sunny;
			for (size_t w : free)
				if (mirrorCol(wings[w]) >= 0)
					sunny.push_back(w);
			if (!sunny.empty()) {
				placeSunDisc(
					wings[sunny[static_cast<size_t>(rng.range(0, static_cast<int>(sunny.size()) - 1))]],
					opener.attr
				);
				return true;
			}
		}
		Wing& wing = wings[free[static_cast<size_t>(rng.range(0, static_cast<int>(free.size()) - 1))]];
		put(wing.end, wing.itemRow, opener);
		busy[index(wing.end, wing.itemRow)] = 1;
		wing.taken = true;
		return true;
	}

	// Where a sun disc puzzle's mirror goes in the wing, -1 if it takes none: a tall hall's wing without a ledge, long
	// enough, a light shaft over the mirror in the hall's top row, nothing between the mirror and the wing's end.
	[[nodiscard]] int mirrorCol(const Wing& wing) const {
		const Segment& hall = route[static_cast<size_t>(wing.seg)];
		const int dir = wing.end > wing.inner ? 1 : -1;
		const int col = wing.inner + 2 * dir;
		if (hall.height < 2 || wing.itemRow != hall.row || std::abs(wing.end - wing.inner) < 5 ||
			busy[index(col, hall.row)] != 0)
			return -1;
		for (int y = hall.row; y < hall.row + hall.height; y++)
			if (!freeCell(col, y))
				return -1;
		for (int x = col + dir; x != wing.end + dir; x += dir)
			if (blocksBeam(g.at(x, hall.row)))
				return -1;
		return col;
	}

	// The sun disc of colour at the wing's end, a mirror by its link with daylight falling on it: the mirror catches
	// the beam until the player turns it to the disc (plan sunlight-mirrors).
	void placeSunDisc(Wing& wing, int colour) {
		const Segment& hall = route[static_cast<size_t>(wing.seg)];
		const int col = mirrorCol(wing);
		put(col, hall.row + hall.height - 1, Tile{LightShaft, static_cast<int>(BeamDir::Down), 0});
		put(col, hall.row, Tile{Mirror, 0, static_cast<int>(BeamDir::Up)});
		put(wing.end, wing.itemRow, Tile{SunDisc, colour, 0});
		busy[index(col, hall.row)] = 1;
		wing.taken = true;
		shape.sunDiscs++;
	}

	// The wings no key or lever took: treasure at the end, now and then; hazards and monsters along all of them.
	void fillWings() {
		for (Wing& wing : wings) {
			if (!wing.taken && rng.chance(WING_TREASURE)) {
				put(wing.end, wing.itemRow, randomTreasure());
				busy[index(wing.end, wing.itemRow)] = 1;
			}
			fillRun(route[static_cast<size_t>(wing.seg)], wing.inner, wing.fillEnd);
		}
	}

	// Wades and pools (ZoneTerrain::water): runs of half water on a hall's floor, over rows of deep water dug into the
	// rock under it (a pool), or over the rock; with much water, whole halls flooded. Never next to a drop (the jump
	// over its hole starts on dry floor) and never where the floor has a hole.
	void addWater() {
		int runs = 0, longest = 0, pools = 0, flooded = 0;
		switch (terrain.water) {
		case TerrainAmount::None:
			return;
		case TerrainAmount::Few:
			runs = 1, longest = 6;
			break;
		case TerrainAmount::Some:
			runs = rng.range(2, 3), longest = 10, pools = 1;
			break;
		case TerrainAmount::Much:
			runs = rng.range(5, 8), longest = 20, pools = 3, flooded = rng.range(2, 3);
			break;
		}
		for (int tries = 0; tries < (runs + flooded) * 6 && runs + flooded > 0; tries++) {
			const Segment& hall = route[static_cast<size_t>(rng.range(0, static_cast<int>(route.size()) - 1))];
			const int length = flooded > 0 ? hall.x1 - hall.x0 + 1
										   : rng.range(3, std::min(longest, hall.x1 - hall.x0 + 1));
			const int x0 = rng.range(hall.x0, hall.x1 - length + 1);
			const bool pool = flooded > 0 || (pools > 0 && rng.chance(0.6f));
			if (flood(hall, x0, x0 + length - 1, pool) < 3)
				continue;
			(flooded > 0 ? flooded : runs)--;
			pools -= pool && pools > 0 ? 1 : 0;
			(pool ? shape.pools : shape.wades)++;
		}
	}

	// Half water on the wettable cells of the hall's floor from x0 to x1, a pool's deep water under them; the cells
	// it wetted.
	int flood(const Segment& hall, int x0, int x1, bool pool) {
		int wet = 0;
		for (int x = x0; x <= x1; x++) {
			if (!wettable(hall, x))
				continue;
			setStructure(x, hall.row, Structure::HalfWater);
			wet++;
			for (int y = hall.row - 1; pool && y >= hall.row - POOL_DEPTH && poolRock(x, y); y--) {
				setStructure(x, y, Structure::DeepWater);
				used[index(x, y)] = 1; // no branch digs through it
			}
		}
		return wet;
	}

	// A floor cell that can take half water: dry, on rock, nothing but a ladder's foot in it, no drop near.
	[[nodiscard]] bool wettable(const Segment& hall, int x) const {
		const Tile t = g.at(x, hall.row);
		if (t.structure != Structure::Empty || (t.type != NoObject && t.type != Ladder) ||
			!isWall(g.at(x, hall.row - 1)))
			return false;
		for (int k = -WATER_CLEAR; k <= WATER_CLEAR; k++)
			if (!isSolidStructure(g.at(x + k, hall.row - 1).structure) && g.at(x + k, hall.row - 1).type != Ladder)
				return false; // a hole in the floor: a drop shaft
		return true;
	}

	// Rock under a pool that can be water: uncarved, with uncarved rock under it (the pool's floor).
	[[nodiscard]] bool poolRock(int x, int y) const {
		return y - 1 >= MIN_ROW && used[index(x, y)] == 0 && used[index(x, y - 1)] == 0 && isWall(g.at(x, y)) &&
			   isWall(g.at(x, y - 1)) && (isWall(g.at(x - 1, y)) || g.at(x - 1, y).structure == Structure::DeepWater) &&
			   isWall(g.at(x + 1, y));
	}

	// Drifts of sand on the floor (ZoneTerrain::sand), with quicksand in them (Some and Much): one or two cells, never
	// by a link.
	void addSand() {
		int drifts = 0, longest = 0, quicksand = 0;
		switch (terrain.sand) {
		case TerrainAmount::None:
			return;
		case TerrainAmount::Few:
			drifts = rng.range(2, 3), longest = 8;
			break;
		case TerrainAmount::Some:
			drifts = static_cast<int>(route.size()) / 2 + 1, longest = 16, quicksand = rng.range(2, 4);
			break;
		case TerrainAmount::Much:
			drifts = static_cast<int>(route.size()), longest = 24, quicksand = rng.range(4, 8);
			break;
		}
		for (int tries = 0; tries < drifts * 6 && drifts > 0; tries++) {
			const Segment& hall = route[static_cast<size_t>(rng.range(0, static_cast<int>(route.size()) - 1))];
			const int length = rng.range(3, std::min(longest, hall.x1 - hall.x0 + 1));
			const int x0 = rng.range(hall.x0, hall.x1 - length + 1);
			int laid = 0;
			for (int x = x0; x < x0 + length; x++) {
				const Tile t = g.at(x, hall.row);
				if (t.structure == Structure::Empty && t.type == NoObject && isWall(g.at(x, hall.row - 1))) {
					setStructure(x, hall.row, Structure::Sand);
					laid++;
				}
			}
			drifts -= laid > 0 ? 1 : 0;
			for (int q = 0; q < 3 && quicksand > 0 && length >= 5; q++) {
				const int x = rng.range(x0 + 1, x0 + length - 2);
				const int run = rng.range(1, QUICKSAND_RUN);
				bool fits = true;
				for (int k = -1; k <= run && fits; k++)
					fits = busy[index(x + k, hall.row)] == 0 &&
						   (k < 0 || k == run ? !inQuicksand(g.at(x + k, hall.row)) : inSand(g.at(x + k, hall.row)));
				if (!fits)
					continue;
				for (int k = 0; k < run; k++)
					setStructure(x + k, hall.row, Structure::Quicksand);
				quicksand--;
			}
		}
	}

	// The light (ZoneTerrain): dark halls with no wall torch but a torch or two, braziers on the floor, daylight
	// through cracks in the ceiling. Last, so nothing else takes their cells.
	void addLight() {
		for (const Segment& hall : route)
			if (hall.dark)
				darken(hall);
		if (terrain.braziers)
			for (int n = rng.range(2, 5), tries = 0; n > 0 && tries < 40; tries++) {
				const Segment& hall = route[static_cast<size_t>(rng.range(0, static_cast<int>(route.size()) - 1))];
				const int x = rng.range(hall.x0, hall.x1);
				const Tile t = g.at(x, hall.row);
				if (t.structure != Structure::Empty || !freeCell(x, hall.row) || busy[index(x, hall.row)] != 0 ||
					!isWall(g.at(x, hall.row - 1)) || t.dressing.torch == TorchPlacing::Torch ||
					hasPlacedProp(g.at(x - 1, hall.row).dressing) || hasPlacedProp(g.at(x + 1, hall.row).dressing))
					continue;
				Tile cell = t;
				cell.dressing.prop = DECOR_BRAZIER;
				g.set(x, hall.row, cell);
				busy[index(x, hall.row)] = 1;
				shape.braziers++;
				n--;
			}
		if (terrain.lightShafts)
			for (int n = rng.range(2, 4), tries = 0; n > 0 && tries < 40; tries++) {
				const Segment& hall = route[static_cast<size_t>(rng.range(0, static_cast<int>(route.size()) - 1))];
				const int x = rng.range(hall.x0, hall.x1);
				if (crack(hall, x)) {
					put(x, hall.row + hall.height - 1, Tile{LightShaft, static_cast<int>(BeamDir::Down), 0});
					n--;
				}
			}
	}

	// The figures the zone's monsters hide among: a mummy's coffin (its own, on its spawn) between decoy coffins with
	// a mummy asleep in them, a living ushabti (its decor ushabti on its spawn) among decor ushabti.
	void dressFigures() {
		for (int cell = 0; cell < CELLS; cell++) {
			const Tile t = g.cells[cell];
			if (t.type != MonsterSpawn || (t.attr != MonsterMummy && t.attr != MonsterUshabti))
				continue;
			const int prop = t.attr == MonsterMummy ? DECOR_COFFIN_MUMMY : DECOR_USHABTI;
			if (prop == DECOR_COFFIN_MUMMY && !rng.chance(0.5f))
				continue;
			const int col = cell % LEVEL_WIDTH;
			const int row = cell / LEVEL_WIDTH;
			for (int k : {-1, 1, -2, 2})
				if (figureCell(col + k, row)) {
					Tile beside = g.at(col + k, row);
					beside.dressing.prop = prop;
					beside.dressing.propMirror = k > 0;
					g.set(col + k, row, beside);
					busy[index(col + k, row)] = 1;
				}
		}
	}

	// Dry floor with nothing in it or on its back wall, no prop yet.
	[[nodiscard]] bool figureCell(int col, int row) const {
		const Tile t = g.at(col, row);
		return t.structure == Structure::Empty && freeCell(col, row) && isWall(g.at(col, row - 1)) &&
			   !hasPlacedProp(t.dressing);
	}

	// The dark halls (ZoneTerrain::dark), chosen before the monsters: the camel spiders run in them.
	void chooseDark() {
		if (terrain.dark == TerrainAmount::Few)
			for (int n = rng.range(1, 2); n > 0; n--)
				route[static_cast<size_t>(rng.range(0, static_cast<int>(route.size()) - 1))].dark = true;
		else if (terrain.dark != TerrainAmount::None)
			for (Segment& hall : route)
				hall.dark = rng.percent(terrain.dark == TerrainAmount::Much ? 75 : 40);
	}

	// No wall torch in the hall, but one every TORCH_GAP cells or so in a few places, on free floor.
	void darken(const Segment& hall) {
		for (int y = hall.row; y < hall.row + hall.height; y++)
			for (int x = hall.x0; x <= hall.x1; x++)
				if (!isSolidStructure(g.at(x, y).structure)) {
					Tile cell = g.at(x, y);
					cell.dressing.torch = TorchPlacing::None;
					g.set(x, y, cell);
				}
		for (int x = hall.x0 + rng.range(2, TORCH_GAP); x <= hall.x1 - 2; x += rng.range(TORCH_GAP, 2 * TORCH_GAP)) {
			if (!freeCell(x, hall.row))
				continue;
			Tile cell = g.at(x, hall.row);
			cell.dressing.torch = TorchPlacing::Torch;
			g.set(x, hall.row, cell);
		}
		shape.darkHalls++;
	}

	// A crack for daylight over column x of the hall: rock above its top row, the beam down to the floor through
	// free cells, onto rock (no hole, no mirror, no sun disc it would light).
	[[nodiscard]] bool crack(const Segment& hall, int x) const {
		if (!isWall(g.at(x, hall.row + hall.height)) || !isWall(g.at(x, hall.row - 1)) || busy[index(x, hall.row)] != 0)
			return false;
		for (int y = hall.row; y < hall.row + hall.height; y++)
			if (!freeCell(x, y) || hasPlacedProp(g.at(x, y).dressing))
				return false;
		return true;
	}

	// A plain potion's chest (health, stamina, might, armour): one a trapped chest or a resistance potion may take.
	[[nodiscard]] bool plainPotionChest(const Tile& t) const {
		if (t.type != Treasure)
			return false;
		const std::optional<ItemKind> kind = itemFromFile(t.attr, t.value);
		return kind == ItemKind::SmallHealth || kind == ItemKind::LargeHealth || kind == ItemKind::SmallStamina ||
			   kind == ItemKind::LargeStamina || kind == ItemKind::Might || kind == ItemKind::Armor;
	}

	// Treasure and supplies by the level's dangers (plan campaign-levelgen, step 7), once everything else is placed.
	// With a poisoner, a dart plate or a poison drip: an antidote in a wing on the way before the first of them, a
	// lesser resistance potion, a greater one too from GREATER_RESISTANCE_FROM on. Then the zone's trapped chests
	// (ZONE_TRAPPED_CHESTS), each a chest that held a plain potion: never what the level needs.
	void placeSupplies() {
		int firstPoison = -1; // the first segment of the route with poison on its rows
		bool poison = false;
		for (int cell = 0; cell < CELLS; cell++) {
			const Tile& t = g.cells[cell];
			poison = poison || (t.type == MonsterSpawn && isPoisoner(t.attr)) || t.type == DartPlate ||
					 t.fixture.type == PoisonDrip;
		}
		for (int si = 0; si < static_cast<int>(route.size()) && poison && firstPoison < 0; si++) {
			const Segment& hall = route[static_cast<size_t>(si)];
			for (int y = hall.row; y < hall.row + hall.height && firstPoison < 0; y++)
				for (int x = hall.x0; x <= hall.x1; x++) {
					const Tile t = g.at(x, y);
					if ((t.type == MonsterSpawn && isPoisoner(t.attr)) || t.type == DartPlate ||
						t.fixture.type == PoisonDrip) {
						firstPoison = si;
						break;
					}
				}
		}
		if (poison) {
			placeAntidote(firstPoison < 0 ? static_cast<int>(route.size()) - 1 : firstPoison);
			int resistance = zoneId >= static_cast<int>(GREATER_RESISTANCE_FROM) ? 2 : 1;
			for (ItemKind kind : {ItemKind::LesserResistance, ItemKind::GreaterResistance})
				if (resistance-- > 0 && swapPlainChest(chestOf(kind)))
					shape.resistancePotions++;
		}
		for (int n = isZone(zoneId) ? ZONE_TRAPPED_CHESTS[zoneId] : 0; n > 0; n--) {
			std::vector<int> plain;
			for (int cell = 0; cell < CELLS; cell++)
				if (plainPotionChest(g.cells[cell]))
					plain.push_back(cell);
			if (plain.empty())
				break;
			g.cells[plain[static_cast<size_t>(rng.range(0, static_cast<int>(plain.size()) - 1))]]
				.type = TrappedTreasure;
			shape.trappedChests++;
		}
	}

	// One of the level's plain potion chests, by chance, now holds chest's item instead; false if there is none.
	bool swapPlainChest(const Tile& chest) {
		std::vector<int> plain;
		for (int cell = 0; cell < CELLS; cell++)
			if (plainPotionChest(g.cells[cell]))
				plain.push_back(cell);
		if (plain.empty())
			return false;
		const int cell = plain[static_cast<size_t>(rng.range(0, static_cast<int>(plain.size()) - 1))];
		put(cell % LEVEL_WIDTH, cell / LEVEL_WIDTH, chest);
		return true;
	}

	// The antidote: in the chest (or else at the free end) of the wing nearest before the segment the poison starts
	// on, else of any wing, else in the first chest on the route.
	void placeAntidote(int poisonSeg) {
		auto chest = [this](const Wing& w) { return !w.taken && g.at(w.end, w.itemRow).type == Treasure; };
		auto open = [this](const Wing& w) { return !w.taken && freeCell(w.end, w.itemRow); };
		const Wing* spot = nullptr;
		for (int pass = 0; pass < 2 && spot == nullptr; pass++)
			for (const Wing& w : wings)
				if ((pass == 0 ? chest(w) : open(w)) && w.seg <= poisonSeg && (spot == nullptr || w.seg > spot->seg))
					spot = &w;
		if (spot == nullptr) {
			auto any = std::find_if(wings.begin(), wings.end(), chest);
			if (any == wings.end())
				any = std::find_if(wings.begin(), wings.end(), open);
			spot = any == wings.end() ? nullptr : &*any;
		}
		if (spot != nullptr) {
			put(spot->end, spot->itemRow, chestOf(ItemKind::Antidote));
			return;
		}
		for (const Segment& s : route)
			for (int x = s.x0; x <= s.x1; x++)
				if (g.at(x, s.row).type == Treasure) {
					put(x, s.row, chestOf(ItemKind::Antidote));
					return;
				}
	}

	// The Blessing of Hapi (plan water-blessing-potion): a chest a few cells before the first half water the route
	// wades through, on dry floor of its hall or, if there is none, of the hall before it.
	void placeHapi() {
		for (size_t si = 0; si < route.size(); si++) {
			const Segment& hall = route[si];
			const int dir = hall.exit >= hall.entry ? 1 : -1;
			int wet = -1;
			for (int x = hall.entry; x != hall.exit + dir && wet < 0; x += dir)
				wet = inHalfWater(g.at(x, hall.row)) ? x : -1;
			if (wet < 0)
				continue;
			auto fits = [this](const Segment& s, int x) {
				return x >= s.x0 && x <= s.x1 && busy[index(x, s.row)] == 0 && isEmptyCell(g.at(x, s.row)) &&
					   isWall(g.at(x, s.row - 1));
			};
			for (int k = 2; k <= 6; k++)
				if (fits(hall, wet - k * dir)) {
					putHapi(wet - k * dir, hall.row);
					return;
				}
			if (si == 0)
				return;
			const Segment& before = route[si - 1];
			const int back = before.exit >= before.entry ? -1 : 1;
			for (int k = 2; k <= 6; k++)
				if (fits(before, before.exit + k * back)) {
					putHapi(before.exit + k * back, before.row);
					return;
				}
			return;
		}
	}

	void putHapi(int col, int row) {
		put(col, row, chestOf(ItemKind::WaterBlessing));
		busy[index(col, row)] = 1;
		shape.hapiPotions++;
	}

	bool addNextSegment() {
		Segment& cur = route.back();
		for (int attempt = 0; attempt < SEGMENT_TRIES; attempt++) {
			Link link = Link::LadderDown;
			int roll = rng.range(0, 99);
			if (roll < 20)
				link = Link::LadderUp;
			else if (roll < 55)
				link = Link::Drop;
			if (cur.row < 10)
				link = Link::LadderUp;

			// Leave by a column well away from the arrival, so every segment has to be crossed.
			int exitCol = rng.range(cur.x0, cur.x1);
			if (std::abs(exitCol - cur.entry) < 4)
				continue;

			Segment next;
			next.height = rng.chance(0.2f) ? 2 : 1;
			int wall = rng.range(1, 3);
			if (link == Link::LadderUp)
				next.row = cur.row + cur.height + wall;
			else
				next.row = cur.row - 1 - wall - next.height + 1;
			int length = rng.range(6, 15);
			next.x0 = exitCol - rng.range(0, length - 1);
			next.x0 = std::clamp(next.x0, MIN_COL, MAX_COL - length + 1);
			next.x1 = next.x0 + length - 1;
			next.entry = exitCol;
			if (exitCol < next.x0 || exitCol > next.x1 || std::max(exitCol - next.x0, next.x1 - exitCol) < 4)
				continue;
			if (!segmentFits(next))
				continue;
			bool shaft = link == Link::LadderUp ? shaftFits(exitCol, cur.row + cur.height - 1, next.row)
												: shaftFits(exitCol, next.row + next.height - 1, cur.row);
			if (!shaft)
				continue;

			carveSegment(next);
			if (link == Link::LadderUp)
				carveLadder(exitCol, cur.row, next.row);
			else if (link == Link::LadderDown)
				carveLadder(exitCol, next.row, cur.row);
			else
				for (int y = cur.row - 1; y >= next.row + next.height; y--)
					carve(exitCol, y, Tile{});
			cur.exit = exitCol;
			cur.link = link;
			markBusy(cur, exitCol);
			markBusy(next, exitCol);
			route.push_back(next);
			return true;
		}
		return false;
	}

	// A dead-end side corridor off segment s, joined by a ladder at a column in [lo, hi], with `item` at its end.
	bool tryBranch(int s, int lo, int hi, Tile item) {
		const Segment& seg = route[static_cast<size_t>(s)];
		if (lo > hi)
			std::swap(lo, hi);
		for (int attempt = 0; attempt < SEGMENT_TRIES; attempt++) {
			int col = rng.range(lo, hi);
			if (col < seg.x0 || col > seg.x1 || busy[index(col, seg.row)] != 0 || !isEmptyCell(g.at(col, seg.row)))
				continue;
			bool up = rng.chance(0.5f);
			Segment b;
			b.height = 1;
			int wall = rng.range(1, 2);
			b.row = up ? seg.row + seg.height + wall : seg.row - 1 - wall;
			int length = rng.range(3, 7);
			b.x0 = rng.chance(0.5f) ? col : col - length + 1;
			b.x1 = b.x0 + length - 1;
			if (!segmentFits(b))
				continue;
			bool shaft = up ? shaftFits(col, seg.row + seg.height - 1, b.row) : shaftFits(col, b.row, seg.row);
			if (!shaft)
				continue;
			carveSegment(b);
			if (up)
				carveLadder(col, seg.row, b.row);
			else
				carveLadder(col, b.row, seg.row);
			markBusy(seg, col);
			int end = farEnd(b, col);
			if (item.type == Treasure)
				item = randomTreasure();
			put(end, b.row, item); // a lever goes on the back wall
			busy[index(end, b.row)] = 1;
			if (length >= 5 && rng.chance(0.3f + 0.05f * static_cast<float>(d)))
				placeMonster((col + end) / 2, b.row);
			return true;
		}
		return false;
	}

	// Gates on the route, each with its key or lever on the near side of the gate: in a branch off its segment, or on
	// a zone level in a wing before it.
	void placeLocks() {
		const bool zone = zoneTarget > 0;
		int wanted = d >= 9 ? 3 : d >= 6 ? 2 : d >= 3 ? 1 : 0;
		if (zone) // a boss level keeps the last colour for the key behind the boss gate
			wanted = std::min(wanted + 1, LOCK_COLOUR_COUNT - (boss != 0 ? 1 : 0));
		for (int i = LOCK_COLOUR_COUNT - 1; i > 0; i--)
			std::swap(colours[i], colours[rng.range(0, i)]);
		for (int attempt = 0; attempt < (zone ? 40 : 20) && locksPlaced < wanted; attempt++) {
			int s = rng.range(1, static_cast<int>(route.size()) - 1);
			Segment& seg = route[static_cast<size_t>(s)];
			if (seg.exit < 0)
				continue;
			int from = 0, to = 0;
			lockSpan(static_cast<size_t>(s), from, to);
			int dir = to > from ? 1 : -1;
			int span = std::abs(to - from);
			if (span < 5)
				continue;
			int gate = from + dir * rng.range(3, span - 1);
			if (busy[index(gate, seg.row)] != 0 || !isEmptyCell(g.at(gate, seg.row)))
				continue;
			int colour = colours[locksPlaced];
			g.set(gate, seg.row, Tile{Gate, colour, 0});
			busy[index(gate, seg.row)] = 1;
			Tile opener = rng.chance(0.35f) ? Tile{Lever, colour, 0} : Tile{Key, colour, 0};
			if ((zone && wingOpener(s, opener)) || tryBranch(s, seg.entry + dir, gate - 2 * dir, opener)) {
				markBusy(seg, gate);
				locksPlaced++;
				locks.push_back(Lock{s, gate, colour});
			} else {
				g.set(gate, seg.row, Tile{});
				busy[index(gate, seg.row)] = 0;
			}
		}
	}

	// A treasure chest, or now and then a mimic in its place.
	Tile randomTreasure() {
		Tile chest = randomChest();
		if (fauna != nullptr) {
			if (!mimicRng.percent(fauna->mimicPercent))
				return chest;
			return Tile{MonsterSpawn, mimicRng.percent(fauna->cursedPercent) ? MonsterCursedMimic : MonsterMimic, 0};
		}
		if (d >= MIMIC_MIN_DIFFICULTY && mimicRng.chance(MIMIC_CHANCE)) {
			const bool cursed = d >= CURSED_MIMIC_MIN_DIFFICULTY && mimicRng.chance(CURSED_MIMIC_SHARE);
			return Tile{MonsterSpawn, cursed ? MonsterCursedMimic : MonsterMimic, 0};
		}
		return chest;
	}

	Tile randomChest() {
		int roll = rng.range(0, 99);
		if (roll < 62) { // a potion by its weight (PotionDef::generatedWeight)
			int pick = rng.range(0, 99);
			int i = WEAPON_KIND_COUNT;
			while (i < FIRST_AMULET - 1 && pick >= potionDef(itemAt(i)).generatedWeight) {
				pick -= potionDef(itemAt(i)).generatedWeight;
				i++;
			}
			return chestOf(itemAt(i));
		}
		// The weapons come in by depth, like in the campaign (WeaponDef::generatedFrom): the deeper, the more kinds.
		return chestOf(randomWeapon(roll < 88));
	}

	[[nodiscard]] ItemKind randomWeapon(bool melee) {
		auto allowed = [this, melee](int i) {
			const ItemKind kind = itemAt(i);
			return isRanged(kind) != melee && weaponDef(kind).generatedFrom <= d;
		};
		int kinds = 0;
		for (int i = 0; i < WEAPON_KIND_COUNT; i++)
			kinds += allowed(i) ? 1 : 0;
		int pick = rng.range(0, kinds - 1);
		for (int i = 0; i < WEAPON_KIND_COUNT; i++)
			if (allowed(i) && pick-- == 0)
				return itemAt(i);
		return ItemKind::Club;
	}

	static Tile chestOf(ItemKind kind) {
		const ItemFileId file = fileIdOf(kind);
		return Tile{Treasure, file.type, file.id};
	}

	// Weak monsters give way to their giant kin deeper down (MonsterKind::generated).
	int randomMonster() {
		auto allowed = [this](const GenPick& p) {
			return p.weight > 0 && d >= p.minDifficulty && d <= p.maxDifficulty;
		};
		int total = 0;
		for (int type = 1; type <= MONSTER_TYPE_MAX; type++)
			if (const GenPick& p = monsterKind(type)->generated; allowed(p))
				total += p.weight;
		int roll = rng.range(0, total - 1);
		for (int type = 1; type <= MONSTER_TYPE_MAX; type++) {
			const GenPick& p = monsterKind(type)->generated;
			if (!allowed(p))
				continue;
			if (roll < p.weight)
				return type;
			roll -= p.weight;
		}
		return MonsterRat;
	}

	// A zone level's monster for the cell, from the zone's set (ZoneFauna): swimmers and lotuses only in half water and
	// likelier there, sand burrowers likelier in sand, light shunners likelier in a dark hall and rarer in a lit one.
	// 0 if none of the set fits.
	int zoneMonster(int col, int row, bool dark) {
		const Tile t = g.at(col, row);
		int weights[ZONE_MONSTER_MAX] = {};
		int total = 0;
		for (int i = 0; i < ZONE_MONSTER_MAX; i++) {
			const ZoneMonster& m = fauna->monsters[i];
			const MonsterKind* kind = monsterKind(m.type);
			if (kind == nullptr)
				break;
			const bool water = kind->locomotion == Locomotion::Submerged || kind->locomotion == Locomotion::Floating;
			int w = m.weight * 2;
			if (water)
				w = inHalfWater(t) ? w * 4 : 0;
			if (kind->sandBurrow.has_value() && inSand(t))
				w *= 3;
			if (kind->shunsLight && terrain.dark != TerrainAmount::None)
				w = dark ? w * 3 : w / 4;
			weights[i] = w;
			total += w;
		}
		if (total == 0)
			return 0;
		int roll = rng.range(0, total - 1);
		for (int i = 0; i < ZONE_MONSTER_MAX; i++) {
			if (roll < weights[i])
				return fauna->monsters[i].type;
			roll -= weights[i];
		}
		return 0;
	}

	bool placeMonster(int col, int row, bool dark = false) {
		if (monsters >= monsterCap || !freeCell(col, row) || !isSolidStructure(g.at(col, row - 1).structure))
			return false;
		for (int x = col - MONSTER_GAP; x <= col + MONSTER_GAP; x++)
			if (g.at(x, row).type == MonsterSpawn)
				return false;
		const int type = fauna != nullptr ? zoneMonster(col, row, dark) : randomMonster();
		if (type == 0)
			return false;
		put(col, row, Tile{MonsterSpawn, type, 0});
		busy[index(col, row)] = 1;
		monsters++;
		return true;
	}

	[[nodiscard]] bool floorCell(const Segment& s, int col) const {
		return col >= s.x0 && col <= s.x1 && isEmptyCell(g.at(col, s.row)) && isWall(g.at(col, s.row - 1));
	}

	// A drift on rock (not quicksand) with nothing in it.
	[[nodiscard]] bool sandFloor(const Segment& s, int col) const {
		const Tile t = g.at(col, s.row);
		return col >= s.x0 && col <= s.x1 && t.structure == Structure::Sand && freeCell(col, s.row) &&
			   isWall(g.at(col, s.row - 1));
	}

	// A trap at col the player jumps: from either side, never out of half water.
	[[nodiscard]] bool jumpable(const Segment& s, int col) const {
		return !inHalfWater(g.at(col - 1, s.row)) && !inHalfWater(g.at(col + 1, s.row));
	}

	// A one-cell hole in the floor with a death trap in it: jump it or die.
	bool placePit(const Segment& s, int col) {
		if (!floorCell(s, col - 1) || !floorCell(s, col) || !floorCell(s, col + 1))
			return false;
		int below = s.row - 1;
		if (used[index(col, below - 1)] != 0 || !isWall(g.at(col, below - 1)) || !isWall(g.at(col - 1, below)) ||
			!isWall(g.at(col + 1, below)))
			return false;
		carve(col, below, Tile{Death, 0, 0});
		busy[index(col - 1, s.row)] = busy[index(col, s.row)] = busy[index(col + 1, s.row)] = 1;
		return true;
	}

	Feature pickFeature(const float (&weights)[FEATURE_COUNT]) {
		float total = 0.f;
		for (float w : weights)
			total += w;
		float roll = static_cast<float>(rng.range(0, 9999)) / 10000.f * total;
		for (int i = 0; i < FEATURE_COUNT; i++) {
			roll -= weights[i];
			if (roll < 0)
				return static_cast<Feature>(i);
		}
		return Feature::Treasure;
	}

	// Hazards, monsters and loot along the part of the segment the player has to cross.
	void fillSegment(const Segment& s) { fillRun(s, s.entry, s.exit >= 0 ? s.exit : farEnd(s, s.entry)); }

	static float share(int percent) { return static_cast<float>(percent) / 100.f; }

	// A cell of s's floor a trap of kind can go in: free, dry or a drift (no quicksand), on rock; a rock fall, wall
	// spikes and a poison drip under a one-row ceiling; spikes and a dart plate on dry floor, spikes never next to half
	// water (they are jumped).
	[[nodiscard]] bool trapFits(const Segment& s, int col, Feature kind) const {
		const Tile t = g.at(col, s.row);
		if (col < s.x0 || col > s.x1 || busy[index(col, s.row)] != 0 || !freeCell(col, s.row) || inHalfWater(t) ||
			inQuicksand(t) || !isWall(g.at(col, s.row - 1)))
			return false;
		const bool ceiling = isWall(g.at(col, s.row + 1));
		switch (kind) {
		case Feature::Spike:
			return t.structure == Structure::Empty && jumpable(s, col);
		case Feature::Dart:
			return t.structure == Structure::Empty;
		case Feature::RockFall:
		case Feature::WallSpikes:
		case Feature::Drip:
			return ceiling;
		case Feature::FireVent:
			return true;
		default:
			return false;
		}
	}

	// A rhythm trap (plan rhythm-traps) of type: a phase of its own, now and then slow, or fast deeper down.
	Tile rhythmTrap(int type) {
		const int roll = rng.range(0, 99);
		const SpikeSpeed speed = d >= FAST_TRAP_MIN_DIFFICULTY && roll < 30 ? SpikeSpeed::Fast
								 : roll >= 80								? SpikeSpeed::Slow
																			: SpikeSpeed::Normal;
		return Tile{type, static_cast<int>(speed), rng.range(0, SPIKE_PHASES - 1)};
	}

	Tile trapTile(Feature kind) {
		switch (kind) {
		case Feature::Spike:
			return Tile{Spike, 0, 0};
		case Feature::RockFall:
			return Tile{
				RockFall,
				terrain.sand == TerrainAmount::Much && rng.chance(SAND_FALL_SHARE) ? SAND_FALL : 0,
				0
			};
		case Feature::Dart:
			return Tile{DartPlate, 0, 0};
		case Feature::WallSpikes:
			return rhythmTrap(WallSpikes);
		case Feature::FireVent:
			return rhythmTrap(FireVent);
		default:
			return rhythmTrap(PoisonDrip);
		}
	}

	// Trap gauntlets (ZoneTraps::gauntlets): a stretch of a hall the route has to cross (off a pillar loop, clear of
	// its ends) with a trap on every other cell, of the zone's kinds by their weights, and a free cell between to wait
	// in; from PAIR_MIN_DIFFICULTY now and then two rhythm traps side by side, in step. Before the rest of the
	// content, so the stretch is the traps' alone.
	void addGauntlets() {
		constexpr Feature KINDS[] =
			{Feature::Spike, Feature::RockFall, Feature::Dart, Feature::WallSpikes, Feature::FireVent, Feature::Drip};
		const int weights[] =
			{traps.spikes, traps.rockFalls, traps.darts, traps.wallSpikes, traps.fireVents, traps.poisonDrips};
		for (int n = traps.gauntlets, tries = 0; n > 0 && tries < 30; tries++) {
			const auto s = static_cast<size_t>(rng.range(0, static_cast<int>(route.size()) - 1));
			const Segment& hall = route[s];
			if (hall.height > 1 && tries < 15)
				continue; // a one-row ceiling first: the rock falls, wall spikes and drips need it
			int from = 0, to = 0;
			lockSpan(s, from, to);
			const int dir = to >= from ? 1 : -1;
			const int room = std::abs(to - from) - 1 - 2 * GAUNTLET_CLEAR;
			if (room < GAUNTLET_MIN)
				continue;
			int length = rng.range(GAUNTLET_MIN, std::min(GAUNTLET_MAX, room));
			length -= length % 2 == 0 ? 1 : 0;
			const int start = from + dir * (GAUNTLET_CLEAR + 1 + rng.range(0, room - length));
			bool clear = true;
			for (int k = -1; k <= length && clear; k++) {
				const int x = start + k * dir;
				const Tile t = g.at(x, hall.row);
				clear = busy[index(x, hall.row)] == 0 && freeCell(x, hall.row) && !inHalfWater(t) && !inQuicksand(t) &&
						isWall(g.at(x, hall.row - 1));
			}
			if (!clear)
				continue;
			std::vector<std::pair<int, Feature>> picks;
			for (int k = 0; k < length; k += 2) {
				const int x = start + k * dir;
				int total = 0;
				for (size_t i = 0; i < std::size(KINDS); i++)
					total += trapFits(hall, x, KINDS[i]) ? weights[i] : 0;
				if (total == 0)
					continue;
				int roll = rng.range(0, total - 1);
				for (size_t i = 0; i < std::size(KINDS); i++) {
					if (!trapFits(hall, x, KINDS[i]))
						continue;
					if (roll < weights[i]) {
						picks.emplace_back(x, KINDS[i]);
						break;
					}
					roll -= weights[i];
				}
			}
			if (static_cast<int>(picks.size()) < GAUNTLET_TRAPS)
				continue;
			for (size_t i = 0; i < picks.size(); i++) {
				const auto [x, kind] = picks[i];
				const Tile trap = trapTile(kind);
				put(x, hall.row, trap);
				const bool rhythm = isRhythmTrap(trap.type);
				const int next = x + dir;
				const bool lastPick = i + 1 == picks.size();
				// The pair takes the wait cell after it; the next pick goes, so no run of three forms.
				if (rhythm && d >= PAIR_MIN_DIFFICULTY && (next - start) * dir < length && rng.chance(PAIR_CHANCE) &&
					trapFits(hall, next, kind)) {
					const Tile waitCell = g.at(next, hall.row);
					put(next, hall.row, trap);
					const int left = std::min(x, next);
					if (wallSpikesCrossable(g, left, hall.row, 2, 1) && wallSpikesCrossable(g, left, hall.row, 2, -1)) {
						if (!lastPick && picks[i + 1].first == x + 2 * dir)
							i++;
					} else
						g.set(next, hall.row, waitCell);
				}
			}
			for (int k = -1; k <= length; k++)
				busy[index(start + k * dir, hall.row)] = 1;
			shape.gauntlets++;
			n--;
		}
	}

	// The same along s's floor between from and to, not on the cells at either end.
	void fillRun(const Segment& s, int from, int to) {
		int dir = to >= from ? 1 : -1;
		float fd = static_cast<float>(d);
		const float newTrap = 0.04f + 0.01f * fd; // dart plates and each rhythm trap, by the zone's share
		for (int x = from + 2 * dir; (to - x) * dir > 1; x += dir) {
			if (busy[index(x, s.row)] != 0 || !freeCell(x, s.row))
				continue;
			// Weights in Feature order: none, pit, spike, rock fall, monster, treasure, dart plate, wall spikes, fire
			// vent, poison drip.
			float weights[FEATURE_COUNT] = {
				zoneTarget > 0 ? ZONE_CALM : 2.5f,
				(0.2f + 0.08f * fd) * share(traps.pits),
				(0.35f + 0.05f * fd) * share(traps.spikes),
				(s.height == 1 && d >= 2 ? 0.15f + 0.07f * fd : 0.f) * share(traps.rockFalls),
				0.6f + 0.25f * fd,
				0.2f,
				newTrap * share(traps.darts),
				newTrap * share(traps.wallSpikes),
				newTrap * share(traps.fireVents),
				newTrap * share(traps.poisonDrips)
			};
			int size = 1;
			const Feature feature = pickFeature(weights);
			switch (feature) {
			case Feature::None:
				continue;
			case Feature::Pit:
				if (!placePit(s, x + dir))
					continue;
				size = 3;
				break;
			case Feature::Spike:
				// A zone level's halls are many: a pair to walk over would turn up on almost every one.
				size = zoneTarget == 0 && d >= 5 && rng.chance(0.4f) ? 2 : 1;
				for (int k = 0; k < size; k++)
					if (floorCell(s, x + k * dir) && busy[index(x + k * dir, s.row)] == 0 && jumpable(s, x + k * dir))
						put(x + k * dir, s.row, Tile{Spike, 0, 0});
				break;
			case Feature::RockFall:
				size = rng.range(1, std::min(3, 1 + d / 3));
				for (int k = 0; k < size; k++) {
					int cx = x + k * dir;
					if ((floorCell(s, cx) || sandFloor(s, cx)) && busy[index(cx, s.row)] == 0 &&
						isWall(g.at(cx, s.row + 1))) {
						const bool sand = terrain.sand == TerrainAmount::Much && rng.chance(SAND_FALL_SHARE);
						put(cx, s.row, Tile{RockFall, sand ? SAND_FALL : 0, 0});
					}
				}
				break;
			case Feature::Monster:
				if (!placeMonster(x, s.row, s.dark))
					continue;
				break;
			case Feature::Treasure:
				put(x, s.row, randomTreasure());
				break;
			case Feature::Dart:
			case Feature::WallSpikes:
			case Feature::FireVent:
			case Feature::Drip:
				if (!trapFits(s, x, feature))
					continue;
				put(x, s.row, trapTile(feature));
				busy[index(x, s.row)] = 1;
				break;
			}
			x += dir * (size - 1 + rng.range(1, 2)); // a free cell after every feature
		}
	}
};
} // namespace

float targetScore(int difficulty, int openCells) {
	float score = 2.5f + 2.2f * static_cast<float>(difficulty);
	return openCells > 0 ? score * static_cast<float>(openCells) / static_cast<float>(ZONE_SCORE_CELLS) : score;
}

GenResult generateLevel(const GenOptions& options) {
	GenResult best;
	int difficulty = std::clamp(options.difficulty, GEN_MIN_DIFFICULTY, GEN_MAX_DIFFICULTY);
	const bool zone = isZone(options.zone);
	const int openTarget = !zone ? 0 : options.openCells > 0 ? options.openCells : ZONE_OPEN_CELLS;
	best.target = targetScore(difficulty, openTarget);
	const int boss = zone && options.boss ? zoneBoss(static_cast<ZoneId>(options.zone)) : 0;
	float bestError = 0.f;
	for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
		best.attempts = attempt + 1;
		LevelBuilder builder(
			options.seed * 7919U + static_cast<uint32_t>(attempt) * 104729U,
			difficulty,
			openTarget,
			options.zone,
			boss,
			options
		);
		LevelGrid grid;
		if (!builder.build(grid))
			continue;
		LevelReport report = checkLevel(grid);
		if (!report.valid || !report.warnings.empty())
			continue;
		float error = std::fabs(report.difficulty - best.target) / best.target;
		if (zone)
			error += std::fabs(static_cast<float>(report.openCells - openTarget)) / static_cast<float>(openTarget);
		// What was asked for and did not fit: a candidate with it wins over any without.
		const GenShape& shape = builder.Shape();
		error += (options.fork > 0 && shape.forks == 0 ? 10.f : 0.f) +
				 (options.stargate && shape.nameGates == 0 ? 10.f : 0.f) +
				 (isExpeditionPage(options.page) && shape.pages == 0 ? 10.f : 0.f);
		if (!best.ok || error < bestError) {
			best.ok = true;
			best.grid = grid;
			best.report = report;
			best.shape = builder.Shape();
			bestError = error;
		}
		if (bestError <= GOOD_ENOUGH)
			break;
	}
	if (zone) {
		best.grid.meta.zone = options.zone;
		best.grid.meta.title = options.title;
	}
	return best;
}

bool parseGenSpec(const std::string& spec, GenOptions& options) {
	unsigned int seed = 0;
	int difficulty = 0;
	int read = 0;
	if (sscanf(spec.c_str(), "gen:%u:%d%n", &seed, &difficulty, &read) != 2)
		return false;
	GenOptions parsed;
	parsed.seed = seed;
	parsed.difficulty = difficulty;
	const std::string rest = spec.substr(static_cast<size_t>(read));
	if (!rest.empty()) {
		std::string slug = rest.substr(1);
		const size_t colon = slug.find(':');
		if (colon != std::string::npos) {
			if (slug.substr(colon + 1) != "boss")
				return false;
			parsed.boss = true;
			slug.resize(colon);
		}
		parsed.zone = rest[0] == ':' ? zoneBySlug(slug) : NO_ZONE;
		if (parsed.zone == NO_ZONE || (parsed.boss && zoneBoss(static_cast<ZoneId>(parsed.zone)) == 0))
			return false;
	}
	options = parsed;
	return true;
}
