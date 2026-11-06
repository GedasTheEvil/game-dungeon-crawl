#include "level_builder.h"
#include "inscriptions.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace levelgen {
// A boss level (GenOptions::boss): the last gate on the last hall's crossing, a teleporter at a late wing's end
// before it to the boss room sealed under the halls. There, from the teleporter: the boss in the middle, then the
// boss gate and behind it the key of the last gate and a rare chest; the room shaped for the boss (bossRoomShape).
bool LevelBuilder::placeBoss() {
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
	if (!content.finale) {
		g.set(gate, exitHall.row, Tile{Gate, bossColour, 0});
		markBusy(exitHall, gate);
		locks.push_back(Lock{static_cast<int>(last), gate, bossColour});
	}
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
	bossGate = at(n - (boss == MonsterApep ? BOSS_LEDGE : 4));
	bossGateRow = floorRow;
	bossStep = step;
	put(bossGate, floorRow, Tile{Gate, BOSS_LOCK, 0});
	if (content.finale) // the ankh behind the boss gate wins the game
		put(at(n - 1), floorRow, Tile{Ankh, 0, 0});
	else {
		put(at(n - 2), floorRow, Tile{Key, bossColour, 0});
		put(at(n - 1), floorRow, rareChest());
	}
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
void LevelBuilder::shapeBossRoom(const Segment& room, int mid, int step) {
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
		for (int k : {-5, 2}) { // clear of the boss gate's guardian
			Tile cell = g.at(mid + k * step, room.row);
			cell.dressing.prop = DECOR_BRAZIER;
			g.set(mid + k * step, room.row, cell);
		}
		break;
	default:
		break;
	}
}

// The zone's gates besides the locks (ZoneGates): riddle gates, a name gate, hidden rooms, treasure tombs.
void LevelBuilder::addGateRooms() {
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
void LevelBuilder::placeRiddle() {
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
// secret-name-gates) with its name cartouche on a hall of the route up to the wing's, or a plain teleporter to a
// hidden room, now and then with a guard from difficulty 6.
void LevelBuilder::placeTeleporterRoom(bool named) {
	const int w = freeWing();
	if (w < 0)
		return;
	Wing& wing = wings[static_cast<size_t>(w)];
	// On the wing's hall or one before it on the route (plan name-gate-return): the player who read the name walks
	// on to the gate, no drop on the way cuts it off.
	int cartoucheCol = -1, cartoucheRow = -1;
	for (int tries = 0; named && tries < 60 && cartoucheCol < 0; tries++) {
		const Segment& hall = route[static_cast<size_t>(rng.range(0, wing.seg))];
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
	tombRooms.push_back(room);
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
		// Its lore, or the ring's text in the Stargate chamber (plan stargate-chamber): the ring between it and
		// the chest, its dial device on the teleporter's side.
		const int lore = content.stargate ? STARGATE_TEXT : pickLore();
		const int mid = (room.x0 + room.x1) / 2;
		put(mid, room.row, Tile{Inscription, lore, 0});
		if (content.stargate) {
			const int toChest = west ? 1 : -1;
			g.cells[index(mid + toChest, room.row)].dressing.prop = DECOR_STARGATE;
			g.cells[index(mid - toChest, room.row)].dressing.prop = DECOR_DIAL;
		}
		boons += hasBoon(lore) ? 1 : 0;
		put(cartoucheCol, cartoucheRow, Tile{Inscription, NAME_CARTOUCHE_ID, 0});
		busy[index(cartoucheCol, cartoucheRow)] = 1;
		shape.nameGates++;
	} else
		shape.hiddenRooms++;
}

// A treasure tomb: a wing past a gate of the route closed by a second gate of its colour, a rare chest at its end.
void LevelBuilder::placeTomb() {
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
	Segment tomb = hall;
	tomb.height = 1;
	tomb.x0 = std::min(gate + dir, wing.fillEnd);
	tomb.x1 = std::max(gate + dir, wing.fillEnd);
	tombRooms.push_back(tomb);
	put(wing.end, wing.itemRow, rareChest());
	wing.taken = true;
	shape.treasureTombs++;
}

// A wing for the key or lever of a gate on segment s: one on s before the gate, or on a segment before it with no
// drop on the way (the player could drop past it and be stuck at the gate).
bool LevelBuilder::wingOpener(int s, Tile opener) {
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
int LevelBuilder::mirrorCol(const Wing& wing) const {
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
		if (blocksBeam(g.at(x, hall.row), dir > 0 ? BeamDir::Right : BeamDir::Left))
			return -1;
	return col;
}

// The sun disc of colour at the wing's end, a mirror by its link with daylight falling on it: the mirror catches
// the beam until the player turns it to the disc (plan sunlight-mirrors).
void LevelBuilder::placeSunDisc(Wing& wing, int colour) {
	const Segment& hall = route[static_cast<size_t>(wing.seg)];
	const int col = mirrorCol(wing);
	put(col, hall.row + hall.height - 1, Tile{LightShaft, static_cast<int>(BeamDir::Down), 0});
	put(col, hall.row, Tile{Mirror, 0, static_cast<int>(BeamDir::Up)});
	put(wing.end, wing.itemRow, Tile{SunDisc, colour, 0});
	busy[index(col, hall.row)] = 1;
	wing.taken = true;
	shape.sunDiscs++;
}

// Gates on the route, each with its key or lever on the near side of the gate: in a branch off its segment, or on
// a zone level in a wing before it.
void LevelBuilder::placeLocks() {
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
		const bool lever = rng.chance(0.35f) && (!isZone(zoneId) || zoneId >= static_cast<int>(LEVERS_FROM));
		Tile opener = lever ? Tile{Lever, colour, 0} : Tile{Key, colour, 0};
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
} // namespace levelgen
