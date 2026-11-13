// The checker's mechanism checks (plan split-large-sources, moved from level_check.cpp): locks, dead gates,
// teleporters, name gates, sun beams, wall spikes, sand gusts.
#include "level_check_internal.h"
#include "climb_path.h"
#include "decor_scatter.h"
#include "movement.h"
#include "inscriptions.h"
#include "items.h"
#include "monster_kinds.h"
#include "spike_cycle.h"
#include "sun_beam.h"
#include "tile_defs.h"
#include "gameplay_config.h"
#include <algorithm>
#include <cstdio>
#include <map>
#include <numeric>
#include <optional>
#include <queue>
#include <set>
#include "level_check.h"

namespace levelcheck {
namespace {
// What a dragged thing or a body standing in the cell would weigh on a pressure plate (plan pressure-plate). A
// treasure counts as a chest: its look is rolled, a jar or a coffin does not drag (an over-estimate).
int dragWeight(const Tile& t) {
	switch (t.type) {
	case Stone:
		return STONE_WEIGHT;
	case Mirror:
		return MIRROR_WEIGHT;
	case Treasure:
	case TrappedTreasure:
		return CHEST_WEIGHT;
	case MonsterSpawn:
		if (const MonsterKind* kind = monsterKind(t.attr))
			return std::max(1, kind->weight); // its body, once slain
		return 0;
	default:
		return 0;
	}
}

// The flat floor the player can drag things along (Dungeon::dragGround): open and dry, no shape, not solid, on rock.
bool dragFloor(const LevelGrid& grid, int col, int row) {
	if (col < 0 || col >= LEVEL_WIDTH || row < 1 || row >= LEVEL_HEIGHT)
		return false;
	const Tile& t = grid.at(col, row);
	return t.structure == Structure::Empty && !hasShape(t) && !isSolidTile(t) && isWall(grid.at(col, row - 1));
}

// Pressure plates (plan pressure-plate): a lock colour and a weight; a plate's colour needs plate gates and the gates
// a plate; the things on the plate's floor run (the flat floor round it, up to rock, a closed gate or a step) weigh
// enough to hold it down without the player.
void checkPlates(const LevelGrid& grid, LevelReport& r) {
	int plateMask = 0, plateGateMask = 0;
	for (int cell = 0; cell < CELLS; cell++) {
		const Tile& t = grid.cells[cell];
		if (t.type == Gate && isPlateGateColour(t.attr))
			plateGateMask |= bitOf(t.attr);
		const Fixture& f = t.fixture;
		if (f.type != PressurePlate)
			continue;
		if (!isLockColour(f.attr)) {
			r.errors.push_back("bad lock colour " + std::to_string(f.attr) + " at " + at(cell));
			continue;
		}
		if (f.value < 1) {
			r.errors.push_back("a pressure plate needs a weight of 1 or more at " + at(cell));
			continue;
		}
		plateMask |= bitOf(f.attr);
		const int col = cell % LEVEL_WIDTH, row = cell / LEVEL_WIDTH;
		int weight = dragFloor(grid, col, row) ? dragWeight(t) : 0;
		for (int dir : {-1, 1})
			for (int c = col + dir; dragFloor(grid, c, row); c += dir)
				weight += dragWeight(grid.at(c, row));
		if (weight < f.value)
			r.warnings.push_back(
				"the " + std::string(lockColour(f.attr).name) + " pressure plate at " + at(cell) + " needs " +
				std::to_string(f.value) + ", its floor run holds " + std::to_string(weight) + " to drag onto it"
			);
	}
	for (int c = 1; c <= LOCK_COLOUR_COUNT; c++) {
		if ((plateMask & bitOf(c)) != 0 && (plateGateMask & bitOf(c)) == 0)
			r.warnings.push_back(std::string("a ") + lockColour(c).name + " pressure plate, but no plate gate of it");
		if ((plateGateMask & bitOf(c)) != 0 && (plateMask & bitOf(c)) == 0)
			r.warnings.push_back(std::string("no ") + lockColour(c).name + " pressure plate holds its plate gates");
	}
}
} // namespace

void checkLocks(const LevelGrid& grid, LevelReport& r) {
	int keyMask = 0, leverMask = 0, gateMask = 0;
	for (int cell = 0; cell < CELLS; cell++) {
		const Tile t = grid.cells[cell];
		const Fixture& f = t.fixture;
		if ((f.type == Lever || f.type == SunDisc) && !isLockColour(f.attr))
			r.errors.push_back("bad lock colour " + std::to_string(f.attr) + " at " + at(cell));
		else if (f.type == Lever || f.type == SunDisc)
			leverMask |= bitOf(f.attr);
		if ((t.type == Key && !isLockColour(t.attr)) || (t.type == Gate && !isGateColour(t.attr))) {
			r.errors.push_back("bad lock colour " + std::to_string(t.attr) + " at " + at(cell));
			continue;
		}
		if (t.type == Key)
			keyMask |= bitOf(t.attr);
		if (t.type == Gate && gateState(t) == GateState::Closed && !isPlateGateColour(t.attr))
			gateMask |= bitOf(t.attr);
	}
	for (int c = 1; c <= LOCK_COLOUR_COUNT; c++)
		if ((gateMask & bitOf(c)) != 0 && ((keyMask | leverMask) & bitOf(c)) == 0)
			r.warnings.push_back(std::string("no key, lever or sun disc opens the ") + lockColour(c).name + " gates");
	checkPlates(grid, r);

	bool bossGates = false;
	for (const Tile& t : grid.cells)
		bossGates = bossGates || (t.type == Gate && t.attr == BOSS_LOCK);
	if (bossGates && r.bosses == 0)
		r.warnings.emplace_back("boss gates, but no boss opens them");
	if (r.bosses > 0 && !bossGates)
		r.warnings.emplace_back("a boss without a boss gate");
	if (r.bosses > 1)
		r.warnings.push_back(std::to_string(r.bosses) + " bosses, a level has at most one");
}

// Only the game makes a dead gate, from an answered riddle gate (GateEmpty); in level data it has no purpose.
void checkDeadGates(const LevelGrid& grid, LevelReport& r) {
	for (int cell = 0; cell < CELLS; cell++) {
		Tile t = grid.cells[cell];
		if (t.type == Door && t.attr != GateEntrance && t.attr != GateExit && t.attr != GateRiddle &&
			t.attr != GateTeleport)
			r.warnings.push_back("gate without a purpose (attribute " + std::to_string(t.attr) + ") at " + at(cell));
	}
}

// Each teleporter pair id is used by exactly two teleporters, two ring platforms for a ring platform's.
void checkTeleporters(const LevelGrid& grid, LevelReport& r) {
	std::map<int, std::vector<int>> pairs;
	for (int cell = 0; cell < CELLS; cell++)
		if (isTeleporter(grid.cells[cell]))
			pairs[teleportPair(grid.cells[cell])].push_back(cell);
	for (const auto& [id, cells] : pairs)
		if (cells.size() != 2)
			r.warnings.push_back(
				std::to_string(cells.size()) + " teleporter(s) with pair id " + std::to_string(id) + ", e.g. at " +
				at(cells[0]) + ": a pair needs exactly two"
			);
		else if (isRingPlatform(grid.cells[cells[0]]) != isRingPlatform(grid.cells[cells[1]]))
			r.warnings.push_back(
				"the ring platform of pair id " + std::to_string(id) + " has a plain teleporter for a partner, at " +
				at(cells[0]) + " and " + at(cells[1])
			);
	// Plan ring-transporter: the rings hide in the rock over the platform.
	for (int cell = 0; cell < CELLS; cell++)
		if (isRingPlatform(grid.cells[cell]) && !isWall(grid.at(cell % LEVEL_WIDTH, cell / LEVEL_WIDTH + 1)))
			r.warnings.push_back("ring platform at " + at(cell) + " without rock above it for its rings");
}

// Plan name-gate-return: the player who has read the name walks back to the name gate (still sealed: it opens once
// they stand in it), from the cartouche with the colours they had there.
void checkWayBackToNameGate(
	const LevelGrid& grid,
	const Walker& sealedWalker,
	const std::vector<char>& sealedStates,
	int cartouche,
	LevelReport& r
) {
	std::vector<int> atCartouche;
	for (int node : sealedWalker.places().of(cartouche))
		for (int mask = 0; mask < MASKS; mask++)
			if (sealedStates[node * MASKS + mask] != 0)
				atCartouche.push_back(node * MASKS + mask);
	const std::vector<char> back = cellsOf(sealedWalker, reachableStates(sealedWalker, atCartouche));
	for (int gate = 0; gate < CELLS; gate++)
		if (isNameGate(grid.cells[gate]) && back[gate] == 0)
			r.warnings.push_back(
				"the name gate at " + at(gate) + " cannot be reached back from the name cartouche at " + at(cartouche)
			);
}

// Plan secret-name-gates: at most one name gate, with one name cartouche, and both are found with the name gate
// sealed, the cartouche not next to the gate; the exit, the keys and the levers too: nothing behind it is needed.
void checkNameGates(const LevelGrid& grid, int startState, LevelReport& r) {
	if (r.nameGates > 1)
		r.warnings.push_back(std::to_string(r.nameGates) + " name gates, a level has at most one");
	if (r.nameGates > 0 && r.nameCartouches == 0)
		r.warnings.emplace_back("a name gate without a name cartouche");
	if (r.nameGates == 0 && r.nameCartouches > 0)
		r.warnings.emplace_back("a name cartouche without a name gate");
	if (r.nameCartouches > 1)
		r.warnings.push_back(std::to_string(r.nameCartouches) + " name cartouches, a level has at most one");
	if (r.nameGates == 0)
		return;
	const Walker sealedWalker(grid, true, true);
	const std::vector<char> sealedStates = reachableStates(sealedWalker, {startState});
	const std::vector<char> sealed = cellsOf(sealedWalker, sealedStates);
	const std::vector<char> open = reachableCells(Walker(grid), startState);
	for (int cell = 0; cell < CELLS; cell++) {
		const Tile t = grid.cells[cell];
		if (t.fixture.type == Inscription && t.fixture.attr == NAME_CARTOUCHE_ID) {
			if (sealed[cell] == 0)
				r.warnings.push_back("the name cartouche at " + at(cell) + " is not found with the name gate sealed");
			else
				checkWayBackToNameGate(grid, sealedWalker, sealedStates, cell, r);
			for (int other = 0; other < CELLS; other++)
				if (isNameGate(grid.cells[other]) && std::abs(other % LEVEL_WIDTH - cell % LEVEL_WIDTH) <= 2 &&
					std::abs(other / LEVEL_WIDTH - cell / LEVEL_WIDTH) <= 1)
					r.warnings.push_back("the name cartouche at " + at(cell) + " is next to its name gate");
		}
		if (open[cell] == 0 || sealed[cell] != 0)
			continue;
		if (isGoal(t))
			r.warnings.push_back("the exit at " + at(cell) + " is behind a name gate");
		if (t.type == Key)
			r.warnings.push_back("the key at " + at(cell) + " is behind a name gate");
		if (t.fixture.type == Lever)
			r.warnings.push_back("the lever at " + at(cell) + " is behind a name gate");
	}
}

// Plan sunlight-mirrors: a light shaft has rock behind it (the crack the sun comes through), a mirror is not in front
// of an inscription (reading it would take the key), a sun disc stands where a beam can fall on it, with every mirror
// and gate as the player may want them. Returns the colours of the discs lit as the level starts (warned about: their
// gates open at once).
int checkSunBeams(const LevelGrid& grid, LevelReport& r) {
	for (int cell = 0; cell < CELLS; cell++) {
		const Tile& t = grid.cells[cell];
		const BeamDir back = opposite(shaftDir(t));
		if (t.type == LightShaft &&
			!isWall(grid.at(cell % LEVEL_WIDTH + beamDx(back), cell / LEVEL_WIDTH + beamDy(back))))
			r.warnings.push_back("the light shaft at " + at(cell) + " has no rock behind it");
		if (t.type == Mirror && t.fixture.type == Inscription) // the inscription takes the interact key first
			r.warnings.push_back(
				"the mirror at " + at(cell) + " stands in front of an inscription: it cannot be turned"
			);
	}
	if (r.sunDiscs == 0)
		return 0;
	const std::vector<char> anyWay = sunlitCells(
		grid.cells,
		[&](int cell, BeamDir way) {
			return isSolidStructure(grid.cells[cell].structure) || shapeStopsBeam(grid.cells[cell], way);
		},
		[](int) { return true; }
	);
	const std::vector<char> atStart = sunlitCells(
		grid.cells,
		[&](int cell, BeamDir way) { return blocksBeam(grid.cells[cell], way); },
		[](int) { return false; }
	);
	int lit = 0;
	for (int cell = 0; cell < CELLS; cell++) {
		const Fixture& f = grid.cells[cell].fixture;
		if (f.type != SunDisc || !isLockColour(f.attr))
			continue;
		if (anyWay[cell] == 0)
			r.warnings.push_back("no beam reaches the sun disc at " + at(cell));
		else if (atStart[cell] != 0) {
			r.warnings.push_back("the sun disc at " + at(cell) + " is lit as the level starts");
			lit |= bitOf(f.attr);
		}
	}
	return lit;
}

bool rhythmTrapAt(const LevelGrid& grid, int col, int row) { return isRhythmTrap(grid.at(col, row).fixture.type); }

// Wall spikes stand on a floor under a ceiling (out, they are solid: nothing may stand on them or fall into them). In
// front of them (plan wall-layer) only what the spikes do not trap: a key or a floor trap, no monster spawn (it would
// spawn inside them), ladder, gate or chest (the player could never stand at it safely). A fire vent (plan
// fire-jet-trap) stands on a floor, out of the water; its jet blocks nothing. A poison drip (plan poison-drip-trap)
// hangs from a ceiling over a floor, out of the water, its puddle free but for a key or a page. Each run of rhythm
// traps in a row, all kinds mixed, can be walked through both ways: the same safe windows; burning and the puddle's
// poison are a cost, not a block.
void checkWallSpikes(const LevelGrid& grid, LevelReport& r) {
	for (int row = 0; row < LEVEL_HEIGHT; row++)
		for (int col = 0; col < LEVEL_WIDTH; col++) {
			if (!rhythmTrapAt(grid, col, row))
				continue;
			const int cell = row * LEVEL_WIDTH + col;
			const Tile& t = grid.at(col, row);
			const int object = t.type;
			if (t.fixture.type == WallSpikes) {
				if (object == MonsterSpawn || object == Ladder || object == Gate || object == Door ||
					object == Treasure || object == TrappedTreasure || object == Mirror)
					r.warnings.push_back(std::string(tileDef(object).name) + " in front of wall spikes at " + at(cell));
				if (!isWall(grid.at(col, row - 1)))
					r.warnings.push_back("wall spikes without a floor under them at " + at(cell));
				if (!isWall(grid.at(col, row + 1)))
					r.warnings.push_back("wall spikes without a wall above them at " + at(cell));
			} else {
				const bool drip = t.fixture.type == PoisonDrip;
				const std::string name = drip ? "a poison drip" : "a fire vent";
				if (!isWall(grid.at(col, row - 1)))
					r.warnings.push_back(name + " without a floor under it at " + at(cell));
				if (t.structure != Structure::Empty && !inSand(t))
					r.warnings.push_back(name + " in water at " + at(cell));
				if (drip && !isWall(grid.at(col, row + 1)))
					r.warnings.push_back("a poison drip without a ceiling over it at " + at(cell));
				if (drip && object != NoObject && object != Key && object != Page)
					r.warnings.push_back(
						std::string(tileDef(object).name) + " in a poison drip's puddle at " + at(cell)
					);
			}
			if (rhythmTrapAt(grid, col - 1, row))
				continue; // not the first of its run
			int count = 1;
			while (rhythmTrapAt(grid, col + count, row))
				count++;
			if (!wallSpikesCrossable(grid, col, row, count, 1) || !wallSpikesCrossable(grid, col, row, count, -1))
				r.warnings.push_back(
					"the " + std::to_string(count) + " rhythm traps from " + at(cell) +
					" cannot be walked through: their safe windows do not line up"
				);
		}
}

bool gustAt(const LevelGrid& grid, int col, int row) { return grid.at(col, row).fixture.type == SandGust; }

// What a gust blows the player into at the downwind end of its run (col, row) is safe: rock (or a gate, as open) stops
// the push; open floor takes it, no drop, no trap, no quicksand. Empty: safe, else what is wrong.
std::string blownInto(const LevelGrid& grid, int col, int row) {
	const Tile t = grid.at(col, row);
	if (isSolidStructure(t.structure))
		return "";
	if (t.type == Spike || t.type == Death)
		return std::string(tileDef(t.type).name);
	if (inQuicksand(t))
		return "quicksand";
	if (isRhythmTrap(t.fixture.type))
		return std::string(tileDef(t.fixture.type).name);
	if (t.type != Ladder && !isWall(grid.at(col, row - 1)))
		return "a drop";
	return "";
}

// Sand gusts (plan sandstorm-halls) blow over a floor, out of the water, with nothing in their cells the push would
// drag the player through: no spikes, death trap or quicksand. Each run of them in a row blows one way (two winds
// meeting would hold the player between them), can be crossed against the wind in one calm (gustsCrossable), and
// blows the player at its downwind end into rock or safe floor (blownInto).
void checkSandGusts(const LevelGrid& grid, LevelReport& r) {
	for (int row = 0; row < LEVEL_HEIGHT; row++)
		for (int col = 0; col < LEVEL_WIDTH; col++) {
			if (!gustAt(grid, col, row))
				continue;
			const int cell = row * LEVEL_WIDTH + col;
			const Tile& t = grid.at(col, row);
			if (!isWall(grid.at(col, row - 1)))
				r.warnings.push_back("a sand gust without a floor under it at " + at(cell));
			if (t.structure != Structure::Empty && t.structure != Structure::Sand)
				r.warnings.push_back(
					"a sand gust over " + std::string(structureDef(t.structure).name) + " at " + at(cell)
				);
			if (t.type == Spike || t.type == Death)
				r.warnings.push_back(std::string(tileDef(t.type).name) + " in a sand gust at " + at(cell));
			if (gustAt(grid, col - 1, row))
				continue; // not the first of its run
			int count = 1;
			bool bothWays = false;
			while (gustAt(grid, col + count, row)) {
				bothWays = bothWays || gustDir(grid.at(col + count, row).fixture) != gustDir(t.fixture);
				count++;
			}
			const std::string run = "the " + std::to_string(count) + " sand gusts from " + at(cell);
			if (bothWays) {
				r.warnings.push_back(run + " blow both ways");
				continue;
			}
			if (!gustsCrossable(grid, col, row, count))
				r.warnings.push_back(run + " cannot be crossed against the wind in one calm");
			const int end = gustDir(t.fixture) > 0 ? col + count : col - 1;
			if (const std::string into = blownInto(grid, end, row); !into.empty())
				r.warnings.push_back(run + " blow the player into " + into + " at " + at(row * LEVEL_WIDTH + end));
		}
}

} // namespace levelcheck

bool wallSpikesCrossable(const LevelGrid& grid, int col, int row, int count, int dir) {
	int cycle = 1; // the run's pattern repeats after this long
	std::vector<Fixture> run;
	for (int k = 0; k < count; k++) {
		run.push_back(grid.at(dir > 0 ? col + k : col + count - 1 - k, row).fixture);
		cycle = std::lcm(cycle, spikePeriodMs(run.back()));
	}
	constexpr int START_STEP_MS = 10;
	for (int start = 0; start < cycle; start += START_STEP_MS)
		if (rhythmRunClear(run.data(), count, start))
			return true;
	return false;
}

bool gustsCrossable(const LevelGrid& grid, int col, int row, int count) {
	const int dir = gustDir(grid.at(col, row).fixture);
	int cycle = 1;
	bool sand = false;
	std::vector<Fixture> run; // in the order walked against the wind
	for (int k = 0; k < count; k++) {
		const Tile& t = grid.at(dir > 0 ? col + count - 1 - k : col + k, row);
		run.push_back(t.fixture);
		sand = sand || inSand(t);
		cycle = std::lcm(cycle, spikePeriodMs(t.fixture));
	}
	const float speed = WALK_SPEED * (sand ? SAND_SPEED_FACTOR : 1.f);
	constexpr int START_STEP_MS = 10;
	for (int start = 0; start < cycle; start += START_STEP_MS)
		if (rhythmRunClear(run.data(), count, start, GUST_CROSS_MARGIN_MS, speed))
			return true;
	return false;
}
