#include "level_gen.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace {
constexpr int MIN_COL = 1; // the outer ring stays wall
constexpr int MAX_COL = LEVEL_WIDTH - 2;
constexpr int MIN_ROW = 1;
constexpr int MAX_ROW = LEVEL_HEIGHT - 2;
constexpr int CELLS = LEVEL_WIDTH * LEVEL_HEIGHT;
constexpr int MAX_ATTEMPTS = 80;
constexpr int SEGMENT_TRIES = 60;
constexpr int MONSTER_GAP = 3;		// cells between monsters in a row
constexpr float GOOD_ENOUGH = 0.1f; // stop searching within 10 % of the target score

// Item types and ids as in ui/inventory.h (not included: it pulls in GL).
constexpr int ITEM_MELEE = 1;
constexpr int ITEM_RANGED = 2;
constexpr int ITEM_POTION = 3;

class Rng { // splitmix64: the same sequence on every platform
  public:
	explicit Rng(uint64_t seed) : state(seed * 0x9e3779b97f4a7c15ULL + 0x632be59bd9b4e019ULL) {}
	uint32_t next() {
		uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
		z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
		z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
		return static_cast<uint32_t>((z ^ (z >> 31)) >> 32);
	}
	int range(int lo, int hi) { // inclusive
		return hi <= lo ? lo : lo + static_cast<int>(next() % static_cast<uint32_t>(hi - lo + 1));
	}
	bool chance(float p) { return static_cast<float>(next() % 10000U) < p * 10000.f; }

  private:
	uint64_t state;
};

enum class Link : unsigned char { None, LadderDown, LadderUp, Drop };

// What fillSegment() puts at a corridor cell.
enum class Feature : unsigned char { None, Pit, Spike, RockFall, Monster, Treasure };
constexpr int FEATURE_COUNT = 6;

struct Segment {
	int row = 0;	// floor row (the player's row)
	int height = 1; // 1 = corridor, 2 = hall
	int x0 = 0, x1 = 0;
	int entry = 0; // column the player arrives at
	int exit = -1; // column the player leaves by
	Link link = Link::None;
};

class LevelBuilder {
  public:
	LevelBuilder(uint32_t seed, int difficulty) : rng(seed), d(difficulty) {
		for (Tint& t : g.cells)
			t = Tint{Wall, 0, 0};
		used.assign(CELLS, 0);
		busy.assign(CELLS, 0);
	}

	bool build(LevelGrid& out) {
		if (!buildMainRoute())
			return false;
		placeLocks();
		int branches = rng.range(1, 2) + d / 4;
		for (int i = 0; i < branches * 3 && branches > 0; i++) {
			int s = rng.range(0, static_cast<int>(route.size()) - 1);
			if (tryBranch(s, route[s].x0, route[s].x1, Tint{Treasure, 0, 0}))
				branches--;
		}
		for (Segment& s : route)
			fillSegment(s);
		out = g;
		return true;
	}

  private:
	Rng rng;
	int d;
	LevelGrid g;
	std::vector<char> used; // carved (anything but wall)
	std::vector<char> busy; // corridor cells taken by a connector or a feature
	std::vector<Segment> route;
	int monsters = 0;
	int colours[LOCK_COLOUR_COUNT] = {1, 2, 3, 4};
	int locksPlaced = 0;

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

	void carve(int col, int row, Tint t) {
		g.set(col, row, t);
		used[index(col, row)] = 1;
	}

	void carveSegment(const Segment& s) {
		for (int y = s.row; y < s.row + s.height; y++)
			for (int x = s.x0; x <= s.x1; x++)
				carve(x, y, Tint{Empty, 0, 0});
	}

	void carveLadder(int col, int yLow, int yHigh) {
		for (int y = yLow; y <= yHigh; y++)
			carve(col, y, Tint{Ladder, 0, 0});
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

		Segment& first = route.front();
		g.set(first.entry, first.row, Tint{Door, GateEntrance, 0});
		markBusy(first, first.entry);
		busy[index(first.entry + (first.entry == first.x0 ? 2 : -2), first.row)] = 1; // a free step out of the door

		Segment& last = route.back();
		last.exit = farEnd(last, last.entry);
		g.set(last.exit, last.row, Tint{Door, GateExit, 0});
		markBusy(last, last.exit);
		return true;
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
					carve(exitCol, y, Tint{Empty, 0, 0});
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
	bool tryBranch(int s, int lo, int hi, Tint item) {
		const Segment& seg = route[static_cast<size_t>(s)];
		if (lo > hi)
			std::swap(lo, hi);
		for (int attempt = 0; attempt < SEGMENT_TRIES; attempt++) {
			int col = rng.range(lo, hi);
			if (col < seg.x0 || col > seg.x1 || busy[index(col, seg.row)] != 0 || g.at(col, seg.row).a != Empty)
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
			if (item.a == Treasure)
				item = randomTreasure();
			g.set(end, b.row, item);
			busy[index(end, b.row)] = 1;
			if (length >= 5 && rng.chance(0.3f + 0.05f * static_cast<float>(d)))
				placeMonster((col + end) / 2, b.row);
			return true;
		}
		return false;
	}

	// Gates on the route, each with its key or lever in a branch on the near side of the gate.
	void placeLocks() {
		int wanted = d >= 9 ? 3 : d >= 6 ? 2 : d >= 3 ? 1 : 0;
		for (int i = LOCK_COLOUR_COUNT - 1; i > 0; i--)
			std::swap(colours[i], colours[rng.range(0, i)]);
		for (int attempt = 0; attempt < 20 && locksPlaced < wanted; attempt++) {
			int s = rng.range(1, static_cast<int>(route.size()) - 1);
			Segment& seg = route[static_cast<size_t>(s)];
			if (seg.exit < 0)
				continue;
			int dir = seg.exit > seg.entry ? 1 : -1;
			int span = std::abs(seg.exit - seg.entry);
			if (span < 5)
				continue;
			int gate = seg.entry + dir * rng.range(3, span - 1);
			if (busy[index(gate, seg.row)] != 0 || g.at(gate, seg.row).a != Empty)
				continue;
			int colour = colours[locksPlaced];
			g.set(gate, seg.row, Tint{Gate, colour, 0});
			busy[index(gate, seg.row)] = 1;
			Tint opener = rng.chance(0.35f) ? Tint{Lever, colour, 0} : Tint{Key, colour, 0};
			if (tryBranch(s, seg.entry + dir, gate - 2 * dir, opener)) {
				markBusy(seg, gate);
				locksPlaced++;
			} else {
				g.set(gate, seg.row, Tint{Empty, 0, 0});
				busy[index(gate, seg.row)] = 0;
			}
		}
	}

	Tint randomTreasure() {
		int roll = rng.range(0, 99);
		if (roll < 62) {
			// small health, large health, might, armour, life, small stamina, large stamina
			static const int WEIGHTS[7] = {30, 15, 10, 10, 5, 20, 10};
			int pick = rng.range(0, 99);
			int id = 0;
			while (pick >= WEIGHTS[id]) {
				pick -= WEIGHTS[id];
				id++;
			}
			return Tint{Treasure, ITEM_POTION, id};
		}
		if (roll < 88) {
			int id = rng.range(0, 99) < 20 + 6 * d ? rng.range(1, 2) : 0; // sword / spear more often later
			return Tint{Treasure, ITEM_MELEE, id};
		}
		return Tint{Treasure, ITEM_RANGED, 0};
	}

	int randomMonster() {
		struct Pick {
			int type, minDifficulty, weight;
		};
		static const Pick PICKS[] = {{MonsterRat, 1, 6},  {MonsterScarab, 1, 4},   {MonsterPlant, 3, 2},
									 {MonsterWorm, 3, 2}, {MonsterGiantRat, 4, 3}, {MonsterAnubis, 8, 1}};
		int total = 0;
		for (const Pick& p : PICKS)
			if (d >= p.minDifficulty)
				total += p.weight;
		int roll = rng.range(0, total - 1);
		for (const Pick& p : PICKS) {
			if (d < p.minDifficulty)
				continue;
			if (roll < p.weight)
				return p.type;
			roll -= p.weight;
		}
		return MonsterRat;
	}

	bool placeMonster(int col, int row) {
		if (monsters >= 4 + 2 * d || g.at(col, row).a != Empty || g.at(col, row - 1).a != Wall)
			return false;
		for (int x = col - MONSTER_GAP; x <= col + MONSTER_GAP; x++)
			if (g.at(x, row).a == Monster)
				return false;
		g.set(col, row, Tint{Monster, randomMonster(), 0});
		busy[index(col, row)] = 1;
		monsters++;
		return true;
	}

	[[nodiscard]] bool floorCell(const Segment& s, int col) const {
		return col >= s.x0 && col <= s.x1 && g.at(col, s.row).a == Empty && g.at(col, s.row - 1).a == Wall;
	}

	// A one-cell hole in the floor with a death trap in it: jump it or die.
	bool placePit(const Segment& s, int col) {
		if (!floorCell(s, col - 1) || !floorCell(s, col) || !floorCell(s, col + 1))
			return false;
		int below = s.row - 1;
		if (used[index(col, below - 1)] != 0 || g.at(col, below - 1).a != Wall || g.at(col - 1, below).a != Wall ||
			g.at(col + 1, below).a != Wall)
			return false;
		carve(col, below, Tint{Death, 0, 0});
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
	void fillSegment(const Segment& s) {
		int from = s.entry;
		int to = s.exit >= 0 ? s.exit : farEnd(s, s.entry);
		int dir = to >= from ? 1 : -1;
		float fd = static_cast<float>(d);
		for (int x = from + 2 * dir; (to - x) * dir > 1; x += dir) {
			if (busy[index(x, s.row)] != 0 || g.at(x, s.row).a != Empty)
				continue;
			// Weights in Feature order: none, pit, spike, rock fall, monster, treasure.
			float weights[FEATURE_COUNT] = {2.5f,
											0.2f + 0.08f * fd,
											0.35f + 0.05f * fd,
											s.height == 1 && d >= 2 ? 0.15f + 0.07f * fd : 0.f,
											0.6f + 0.25f * fd,
											0.2f};
			int size = 1;
			switch (pickFeature(weights)) {
			case Feature::None:
				continue;
			case Feature::Pit:
				if (!placePit(s, x + dir))
					continue;
				size = 3;
				break;
			case Feature::Spike:
				size = d >= 5 && rng.chance(0.4f) ? 2 : 1;
				for (int k = 0; k < size; k++)
					if (floorCell(s, x + k * dir) && busy[index(x + k * dir, s.row)] == 0)
						g.set(x + k * dir, s.row, Tint{Spike, 0, 0});
				break;
			case Feature::RockFall:
				size = rng.range(1, std::min(3, 1 + d / 3));
				for (int k = 0; k < size; k++) {
					int cx = x + k * dir;
					if (floorCell(s, cx) && busy[index(cx, s.row)] == 0 && g.at(cx, s.row + 1).a == Wall)
						g.set(cx, s.row, Tint{RockFall, 0, 0});
				}
				break;
			case Feature::Monster:
				if (!placeMonster(x, s.row))
					continue;
				break;
			case Feature::Treasure:
				g.set(x, s.row, randomTreasure());
				break;
			}
			x += dir * (size - 1 + rng.range(1, 2)); // a free cell after every feature
		}
	}
};
} // namespace

float targetScore(int difficulty) { return 2.5f + 2.2f * static_cast<float>(difficulty); }

GenResult generateLevel(const GenOptions& options) {
	GenResult best;
	int difficulty = std::clamp(options.difficulty, GEN_MIN_DIFFICULTY, GEN_MAX_DIFFICULTY);
	best.target = targetScore(difficulty);
	float bestError = 0.f;
	for (int attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
		best.attempts = attempt + 1;
		LevelBuilder builder(options.seed * 7919U + static_cast<uint32_t>(attempt) * 104729U, difficulty);
		LevelGrid grid;
		if (!builder.build(grid))
			continue;
		LevelReport report = checkLevel(grid);
		if (!report.valid || !report.warnings.empty())
			continue;
		float error = std::fabs(report.difficulty - best.target) / best.target;
		if (!best.ok || error < bestError) {
			best.ok = true;
			best.grid = grid;
			best.report = report;
			bestError = error;
		}
		if (bestError <= GOOD_ENOUGH)
			break;
	}
	return best;
}
