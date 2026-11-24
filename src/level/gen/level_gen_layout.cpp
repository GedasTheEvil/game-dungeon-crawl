#include "level_builder.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace levelgen {
bool LevelBuilder::buildMainRoute() {
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
void LevelBuilder::placeDoors() {
	Segment& first = route.front();
	g.set(first.entry, first.row, Tile{Door, GateEntrance, 0});
	markBusy(first, first.entry);
	busy[index(first.entry + (first.entry == first.x0 ? 2 : -2), first.row)] = 1; // a free step out of the door

	Segment& last = route.back();
	last.exit = farEnd(last, last.entry);
	g.set(last.exit, last.row, Tile{Door, GateExit, 0});
	markBusy(last, last.exit);
}

void LevelBuilder::fillRock(int col, int row) {
	g.set(col, row, wallTile());
	used[index(col, row)] = 0;
}

// A ladder or a drop shaft from cur to next at col.
void LevelBuilder::carveLink(const Segment& cur, const Segment& next, int col, Link link) {
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
bool LevelBuilder::buildHalls() {
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
	if (zoneTarget < SMALL_LEVEL_CELLS) { // fewer halls, still most of the width across, from the top
		size_t keep = 0;
		for (int rows = 0; keep < route.size() &&
						   (keep < SMALL_LEVEL_HALLS || rows * HALL_FULL * SMALL_LEVEL_FILL / 100 < zoneTarget);
			 keep++)
			rows += route[keep].height;
		route.resize(keep);
	}
	if (route.size() < (zoneTarget < SMALL_LEVEL_CELLS ? SMALL_LEVEL_HALLS : 6))
		return false;
	int full = 0;
	for (const Segment& hall : route)
		full += hall.height * HALL_FULL;
	float scale = std::min(1.f, static_cast<float>(zoneTarget) / static_cast<float>(full));
	for (Segment& hall : route) {
		float spread = 0.85f + 0.3f * static_cast<float>(rng.range(0, 100)) / 100.f;
		int length = std::clamp(static_cast<int>(static_cast<float>(HALL_FULL) * scale * spread), HALL_MIN, HALL_FULL);
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
int LevelBuilder::crossingCol(const Segment& hall, int lo, int hi) {
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
void LevelBuilder::addLoops() {
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
void LevelBuilder::addWing(int s, int inner, int end, bool entrySide) {
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
void LevelBuilder::lockSpan(size_t s, int& from, int& to) const {
	const Segment& seg = route[s];
	from = s > 0 && route[s - 1].loopCol >= 0 ? route[s - 1].loopCol : seg.entry;
	to = seg.loopCol >= 0 ? seg.loopCol : seg.exit;
}

// The wing lies past the gate on segment seg: on a later segment, or on seg's far side (the route is a line, the
// gate off any pillar loop).
bool LevelBuilder::pastLock(const Wing& wing, int seg) {
	return wing.seg > seg || (wing.seg == seg && !wing.entrySide);
}

// A free wing's end for a door or a chest: dry floor or a drift, nothing in it; -1 if none.
int LevelBuilder::freeWing() {
	std::vector<size_t> free;
	for (size_t w = 0; w < wings.size(); w++) {
		const Wing& wing = wings[w];
		const Structure s = g.at(wing.end, wing.itemRow).structure;
		if (!wing.taken && freeCell(wing.end, wing.itemRow) && (s == Structure::Empty || s == Structure::Sand))
			free.push_back(w);
	}
	return free.empty() ? -1
						: static_cast<int>(free[static_cast<size_t>(rng.range(0, static_cast<int>(free.size()) - 1))]);
}

// A room sealed in the rock, one row tall and length cells long, rock all round it: only a teleporter leads in.
bool LevelBuilder::sealedRoom(int length, Segment& room) {
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

// A route fork (plan denser-levels, Route fork): a hall B the route crosses leftwards, under a hall A, over a hall
// C, is split by a pillar of rock under A's link: two drops side by side in A, one each side of it. The left
// route crosses B's left part (long) and drops into C where the route did; the right one drops from B's right
// part (short) a few cells along, its end a tomb. Both drops are one-way, so no way leads from one route to the
// other. Before the halls are carved.
void LevelBuilder::chooseFork() {
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
int LevelBuilder::forkDrop(int b) const {
	const Segment& hall = route[static_cast<size_t>(b)];
	const Segment& join = route[static_cast<size_t>(b) + 1];
	for (int x = hall.entry + 4; x <= std::min(hall.entry + 7, hall.x1 - 2); x++)
		if (x > join.x0 && x < join.x1 && std::abs(x - hall.exit) > 2)
			return x;
	return -1;
}

// Wades and pools (ZoneTerrain::water): runs of half water on a hall's floor, over rows of deep water dug into the
// rock under it (a pool), or over the rock; with much water, whole halls flooded. Never next to a drop (the jump
// over its hole starts on dry floor) and never where the floor has a hole.
void LevelBuilder::addWater() {
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
		const int length = flooded > 0 ? hall.x1 - hall.x0 + 1 : rng.range(3, std::min(longest, hall.x1 - hall.x0 + 1));
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
int LevelBuilder::flood(const Segment& hall, int x0, int x1, bool pool) {
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
bool LevelBuilder::wettable(const Segment& hall, int x) const {
	const Tile t = g.at(x, hall.row);
	if (t.structure != Structure::Empty || (t.type != NoObject && t.type != Ladder) || !isWall(g.at(x, hall.row - 1)))
		return false;
	for (int k = -WATER_CLEAR; k <= WATER_CLEAR; k++)
		if (!isSolidStructure(g.at(x + k, hall.row - 1).structure) && g.at(x + k, hall.row - 1).type != Ladder)
			return false; // a hole in the floor: a drop shaft
	return true;
}

// Rock under a pool that can be water: uncarved, with uncarved rock under it (the pool's floor).
bool LevelBuilder::poolRock(int x, int y) const {
	return y - 1 >= MIN_ROW && used[index(x, y)] == 0 && used[index(x, y - 1)] == 0 && isWall(g.at(x, y)) &&
		   isWall(g.at(x, y - 1)) && (isWall(g.at(x - 1, y)) || g.at(x - 1, y).structure == Structure::DeepWater) &&
		   isWall(g.at(x + 1, y));
}

// Drifts of sand on the floor (ZoneTerrain::sand), with quicksand in them (Some and Much): one or two cells, never
// by a link.
void LevelBuilder::addSand() {
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

bool LevelBuilder::addNextSegment() {
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
bool LevelBuilder::tryBranch(int s, int lo, int hi, Tile item) {
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
} // namespace levelgen
