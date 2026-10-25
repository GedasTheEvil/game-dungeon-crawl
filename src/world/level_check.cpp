#include "level_check.h"
#include "decor_scatter.h"
#include "movement.h"
#include "inscriptions.h"
#include "items.h"
#include "monster_kinds.h"
#include "spike_cycle.h"
#include "sun_beam.h"
#include "tile_defs.h"
#include <algorithm>
#include <cstdio>
#include <map>
#include <numeric>
#include <queue>
#include <set>

namespace {
constexpr int MASKS = 1 << BOSS_LOCK; // lock colours opened so far, the boss lock too
constexpr int CELLS = LEVEL_WIDTH * LEVEL_HEIGHT;
constexpr int STATES = CELLS * MASKS;
constexpr int NONE = -1;

// Path costs: the cheapest path avoids hazards when there is a way around them.
constexpr int COST_MOVE = 2;
constexpr int COST_JUMP = 4;
constexpr int COST_SPIKE = 30; // a walk over it costs SPIKES_CROSS_HP: deadly early on, jump it
constexpr int COST_DEATH = 30;
constexpr int COST_ROCK_FALL = 15;
constexpr int COST_DART_PLATE = 8;
constexpr int COST_WALL_SPIKES = 6; // the wait for the safe window, the risk of misjudging it
constexpr int COST_DRIP_PUDDLE = 10; // a poison drip's puddle (plan poison-drip-trap): weak poison, jump it
constexpr int COST_MONSTER = 2;
constexpr int COST_TELEPORT = 4;
constexpr int COST_SAND = 1; // the slower walk (plan desert-chapter)
constexpr int COST_QUICKSAND = 8; // sinking, and the jump out of it
constexpr int COST_TURN = 4; // turning mirrors, here and elsewhere (plan sunlight-mirrors)
constexpr int MONSTER_REACH = 3; // cells along a row a monster covers (they walk towards the player)

using Move = PathMove;

struct Edge {
	int to;
	int cost;
	Move move;
};

int stateOf(int col, int row, int mask) { return (row * LEVEL_WIDTH + col) * MASKS + mask; }
int cellOf(int state) { return state / MASKS; }
int maskOf(int state) { return state % MASKS; }
int bitOf(int colour) { return isGateColour(colour) ? 1 << (colour - 1) : 0; }

class Walker {
  public:
	// sealed: the name gates (plan secret-name-gates) do not teleport, as before the name is spoken.
	explicit Walker(const LevelGrid& grid, bool teleports = true, bool sealed = false)
		: grid(grid), pairs(grid.cells), teleports(teleports), sealed(sealed) {}

	// Turning the mirror in the player's cell (plan sunlight-mirrors): a sun disc of a colour not open yet that a beam
	// can light, the player setting the mirrors they reach from here with the colours they have (sunlitCells), and one
	// way of setting them for it. The checker does not follow which way each mirror points: any mirror the player
	// reaches points where the disc needs it.
	struct Turn {
		int colour;
		std::vector<MirrorSetting> way;
	};
	[[nodiscard]] const std::vector<Turn>& turns(int state) const {
		auto found = turnCache.find(state);
		if (found != turnCache.end())
			return found->second;
		const int mask = maskOf(state);
		std::vector<char> reach(CELLS, 0); // with the colours kept: what a key there opens does not count
		std::vector<int> stack = {cellOf(state)};
		reach[cellOf(state)] = 1;
		std::vector<Edge> edges;
		while (!stack.empty()) {
			const int cell = stack.back();
			stack.pop_back();
			successors(cell * MASKS + mask, edges, false);
			for (const Edge& e : edges)
				if (reach[cellOf(e.to)] == 0) {
					reach[cellOf(e.to)] = 1;
					stack.push_back(cellOf(e.to));
				}
		}
		std::vector<std::vector<MirrorSetting>> ways;
		const std::vector<char> lit = sunlitCells(
			grid.cells,
			[&](int cell) { return blocksBeam(cell, mask); },
			[&](int cell) { return reach[cell] != 0; },
			&ways
		);
		std::vector<Turn>& out = turnCache[state];
		int gained = mask;
		for (int cell = 0; cell < CELLS; cell++) {
			const Fixture& f = grid.cells[cell].fixture;
			if (lit[cell] != 0 && f.type == SunDisc && isLockColour(f.attr) && (gained & bitOf(f.attr)) == 0) {
				gained |= bitOf(f.attr);
				out.push_back({f.attr, ways[cell]});
			}
		}
		return out;
	}

	// The beam stops in the cell with these colours open (blocksBeam).
	[[nodiscard]] bool blocksBeam(int cell, int mask) const {
		const Tile& t = grid.cells[cell];
		if (t.type == Gate)
			return gateState(t) == GateState::Closed && (mask & bitOf(t.attr)) == 0;
		return isSolidStructure(t.structure);
	}

	// In level data a gate is closed (0) or open: the game opens one saved mid-motion (2) on load.
	[[nodiscard]] bool solid(int col, int row, int mask) const {
		Tile t = grid.at(col, row);
		if (t.type == Gate)
			return gateState(t) == GateState::Closed && (mask & bitOf(t.attr)) == 0;
		return isSolidTile(t);
	}

	[[nodiscard]] bool standable(int col, int row, int mask) const {
		if (!LevelGrid::inBounds(col, row) || solid(col, row, mask))
			return false;
		return grid.at(col, row).type == Ladder || solid(col, row - 1, mask);
	}

	// Picks up what the player touches in this cell. Reaching the boss counts as killing it: the boss gates open.
	[[nodiscard]] int touch(int col, int row, int mask) const {
		Tile t = grid.at(col, row);
		if (t.type == MonsterSpawn && isBossMonster(t.attr))
			return mask | bitOf(BOSS_LOCK);
		return t.type == Key && isLockColour(t.attr) ? mask | bitOf(t.attr) : mask; // no key opens the boss lock
	}

	// Falls from an open cell until a floor or a ladder stops the player. NONE if the fall leaves the level.
	[[nodiscard]] int settle(int col, int row, int mask, int& rowsFallen) const {
		rowsFallen = 0;
		mask = touch(col, row, mask);
		while (!standable(col, row, mask)) {
			row--;
			rowsFallen++;
			if (row < 0 || solid(col, row, mask))
				return NONE;
			mask = touch(col, row, mask);
		}
		return stateOf(col, row, mask);
	}

	[[nodiscard]] int hazardCost(int cell) const {
		Tile t = grid.cells[cell];
		const int sand = inQuicksand(t) ? COST_QUICKSAND : (inSand(t) ? COST_SAND : 0);
		return sand + objectCost(t);
	}

	[[nodiscard]] static int objectCost(const Tile& t) {
		if (t.type == Spike)
			return COST_SPIKE;
		if (t.type == Death)
			return COST_DEATH;
		if (t.type == RockFall && rockState(t) == RockState::Armed)
			return COST_ROCK_FALL;
		// On the back wall, over the object's; a poison drip's puddle on top.
		const int spikes = isRhythmTrap(t.fixture.type)
							   ? COST_WALL_SPIKES + (t.fixture.type == PoisonDrip ? COST_DRIP_PUDDLE : 0)
							   : 0;
		if (t.type == DartPlate)
			return COST_DART_PLATE + spikes;
		if (t.type == MonsterSpawn)
			return COST_MONSTER + spikes;
		return spikes;
	}

	void successors(int state, std::vector<Edge>& out, bool turning = true) const {
		out.clear();
		int cell = cellOf(state);
		int mask = maskOf(state);
		int col = cell % LEVEL_WIDTH;
		int row = cell / LEVEL_WIDTH;
		Tile here = grid.at(col, row);

		auto add = [&](int to, int base, Move move) {
			if (to != NONE && to != state)
				out.push_back({to, base + hazardCost(cellOf(to)), move});
		};

		const Fixture& lever = here.fixture;
		if (lever.type == Lever && isLockColour(lever.attr) && !leverPulled(lever)) // a pulled lever opens nothing
			add(stateOf(col, row, mask | bitOf(lever.attr)), COST_MOVE, Move::Pull);
		if (turning && here.type == Mirror)
			for (const Turn& turn : turns(state))
				add(stateOf(col, row, mask | bitOf(turn.colour)), COST_TURN, Move::Turn);

		if (teleports && isTeleporter(here) && !(sealed && isNameGate(here))) {
			int to = pairs.Partner(cell);
			if (to != NONE) {
				int fallen = 0;
				add(settle(to % LEVEL_WIDTH, to / LEVEL_WIDTH, mask, fallen), COST_TELEPORT, Move::Teleport);
			}
		}

		for (int dir : {-1, 1}) {
			int next = col + dir;
			if (!LevelGrid::inBounds(next, row) || solid(next, row, mask))
				continue;
			int fallen = 0;
			int to = settle(next, row, mask, fallen);
			add(to, COST_MOVE + fallen, fallen > 0 ? Move::Drop : Move::Walk);

			// Over one cell, a gap in the floor, a trap on it (a poison drip's puddle too) or quicksand, from a floor
			// (a ladder's foot too, sand and quicksand), not out of half water. A jump from mid-ladder is not modelled:
			// its reach depends on the walk key held during the jump. A rock fall is not jumped: the player passes
			// through its cell either way, and it drops all the same.
			const Tile nextTile = grid.at(next, row);
			bool overHazard = fallen == 0 && (nextTile.type == Spike || nextTile.type == Death ||
											  nextTile.fixture.type == PoisonDrip || inQuicksand(nextTile));
			bool overGap = fallen > 0;
			int far = col + 2 * dir;
			if ((overGap || overHazard) && !inHalfWater(here) && solid(col, row - 1, mask) &&
				LevelGrid::inBounds(far, row) && standable(far, row, mask))
				add(stateOf(far, row, touch(far, row, touch(next, row, mask))), COST_JUMP, Move::Jump);
		}

		if (here.type == Ladder) {
			for (int dy : {-1, 1})
				if (grid.at(col, row + dy).type == Ladder)
					add(stateOf(col, row + dy, touch(col, row + dy, mask)), COST_MOVE, Move::Climb);
		}
	}

  private:
	const LevelGrid& grid;
	TeleportPairs pairs;
	bool teleports;
	bool sealed;
	mutable std::map<int, std::vector<Turn>> turnCache; // by state
};

// Cells the walker reaches from the start state (any opened colours).
std::vector<char> reachableCells(const Walker& walker, int startState) {
	std::vector<char> seen(STATES, 0);
	std::vector<char> cells(CELLS, 0);
	std::vector<int> stack = {startState};
	seen[startState] = 1;
	std::vector<Edge> edges;
	while (!stack.empty()) {
		int state = stack.back();
		stack.pop_back();
		cells[cellOf(state)] = 1;
		walker.successors(state, edges);
		for (const Edge& e : edges)
			if (seen[e.to] == 0) {
				seen[e.to] = 1;
				stack.push_back(e.to);
			}
	}
	return cells;
}

bool isGoal(const Tile& t) { return t.type == Ankh || (t.type == Door && t.attr == GateExit); }

std::string at(int cell) {
	char buf[32];
	snprintf(buf, sizeof(buf), "(col %d, row %d)", cell % LEVEL_WIDTH, cell / LEVEL_WIDTH);
	return buf;
}

void countContent(const LevelGrid& grid, LevelReport& r) {
	int minCol = LEVEL_WIDTH, maxCol = -1, minRow = LEVEL_HEIGHT, maxRow = -1;
	for (int row = 0; row < LEVEL_HEIGHT; row++)
		for (int col = 0; col < LEVEL_WIDTH; col++) {
			Tile t = grid.at(col, row);
			if (isSolidStructure(t.structure))
				continue;
			r.openCells++;
			minCol = std::min(minCol, col);
			maxCol = std::max(maxCol, col);
			minRow = std::min(minRow, row);
			maxRow = std::max(maxRow, row);
			switch (t.type) {
			case Door:
				if (t.attr == GateEntrance) {
					r.entrances++;
					r.start = {col, row}; // the game takes the last one in file order
				} else if (t.attr == GateExit)
					r.exits++;
				else if (t.attr == GateRiddle)
					r.riddles++;
				else if (t.attr == GateTeleport) {
					r.teleporters++;
					r.nameGates += isNameGate(t) ? 1 : 0;
				}
				break;
			case Ankh:
				r.exits++;
				r.finale = true;
				break;
			case MonsterSpawn:
				r.monsters[t.attr >= 1 && t.attr <= MONSTER_TYPE_MAX ? t.attr : 0]++;
				r.monsterCount++;
				r.bosses += isBossMonster(t.attr) ? 1 : 0;
				break;
			case Spike:
				r.spikes++;
				break;
			case Death:
				r.deathTraps++;
				break;
			case RockFall:
				r.rockFalls++;
				r.sandFalls += isSandFall(t) ? 1 : 0;
				break;
			case DartPlate:
				r.dartPlates++;
				break;
			case Treasure:
				r.treasures++;
				break;
			case TrappedTreasure:
				r.treasures++;
				r.trappedTreasures++;
				break;
			case Key:
				r.keys++;
				break;
			case Gate:
				r.gates++;
				break;
			case LightShaft:
				r.lightShafts++;
				break;
			case Mirror:
				r.mirrors++;
				break;
			case Page:
				r.pages++;
				if (!isExpeditionPage(t.attr))
					r.warnings.push_back(
						"expedition page without a number (1 to " + std::to_string(EXPEDITION_PAGE_COUNT) + ") at " +
						at(row * LEVEL_WIDTH + col)
					);
				break;
			default:
				break;
			}
			const Fixture& f = t.fixture;
			r.wallSpikes += f.type == WallSpikes ? 1 : 0;
			r.fireVents += f.type == FireVent ? 1 : 0;
			r.poisonDrips += f.type == PoisonDrip ? 1 : 0;
			r.levers += f.type == Lever ? 1 : 0;
			r.sunDiscs += f.type == SunDisc ? 1 : 0;
			if (f.type == Inscription) {
				r.inscriptions++;
				r.nameCartouches += f.attr == NAME_CARTOUCHE_ID ? 1 : 0;
				if (!isInscriptionId(f.attr) && f.attr != NAME_CARTOUCHE_ID)
					r.warnings.push_back(
						"inscription without a text id (1 to " + std::to_string(INSCRIPTION_MAX) + ") at " +
						at(row * LEVEL_WIDTH + col)
					);
			}
		}
	if (maxCol >= 0) {
		r.boundsWidth = maxCol - minCol + 1;
		r.boundsHeight = maxRow - minRow + 1;
	}
}

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
		if (t.type == Gate && gateState(t) == GateState::Closed)
			gateMask |= bitOf(t.attr);
	}
	for (int c = 1; c <= LOCK_COLOUR_COUNT; c++)
		if ((gateMask & bitOf(c)) != 0 && ((keyMask | leverMask) & bitOf(c)) == 0)
			r.warnings.push_back(std::string("no key, lever or sun disc opens the ") + lockColour(c).name + " gates");

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

// The boss room is only reached by teleporter: walking there skips the arrival.
void checkBossRoom(const LevelGrid& grid, int startState, LevelReport& r) {
	if (r.bosses == 0)
		return;
	std::vector<char> walked = reachableCells(Walker(grid, false), startState);
	for (int cell = 0; cell < CELLS; cell++) {
		Tile t = grid.cells[cell];
		if (t.type == MonsterSpawn && isBossMonster(t.attr) && walked[cell] != 0)
			r.warnings.push_back("the boss at " + at(cell) + " can be reached without a teleporter");
	}
}

void checkObjectsInRock(const LevelGrid& grid, LevelReport& r) {
	for (int cell = 0; cell < CELLS; cell++) {
		Tile t = grid.cells[cell];
		if (isSolidStructure(t.structure) && hasObject(t))
			r.warnings.push_back(
				std::string(tileDef(t.type).name) + " in " + structureDef(t.structure).name + " at " + at(cell)
			);
		if (isSolidStructure(t.structure) && hasFixture(t))
			r.warnings.push_back(
				std::string(tileDef(t.fixture.type).name) + " in " + structureDef(t.structure).name + " at " + at(cell)
			);
	}
}

// Things a placed prop would stand in front of or in the way of, on screen: a web hangs in the corner over them all.
bool hidesBehindProp(const Tile& t) {
	switch (t.type) {
	case Treasure:
	case TrappedTreasure:
	case Door:
	case Gate:
	case Ankh:
	case Ladder:
	case Mirror:
	case MonsterSpawn:
		return true;
	default:
		break;
	}
	return hasFixture(t);
}

// The scatter's own prop for the cell, which beats a placed one: a coffin, the skeleton under a page, an ushabti.
bool scatterOwnsProp(const LevelGrid& grid, int col, int row) {
	const Tile t = grid.at(col, row);
	const MonsterKind* kind = monsterKind(spawnType(t));
	return t.type == Page || bossCoffinCell(grid.cells, col, row) ||
		   (kind != nullptr && (kind->locomotion == Locomotion::Entombed || kind->locomotion == Locomotion::Figurine));
}

void checkPlacedProp(const LevelGrid& grid, int cell, LevelReport& r) {
	const int col = cell % LEVEL_WIDTH;
	const int row = cell / LEVEL_WIDTH;
	const Tile t = grid.cells[cell];
	const int prop = t.dressing.prop;
	if (prop < 0)
		return;
	const std::string what = std::string("the ") + DECOR_NAMES[prop] + " at " + at(cell);
	if (isWall(t)) {
		r.warnings.push_back(what + " is in rock");
		return;
	}
	if (scatterOwnsProp(grid, col, row))
		r.warnings.push_back(what + " is not shown: the cell has its own (a coffin, a skeleton, an ushabti)");
	if (isWaterProp(prop) && !inHalfWater(t))
		r.warnings.push_back(what + " is a water prop out of half water");
	if (!isWaterProp(prop) && (t.structure == Structure::HalfWater || t.structure == Structure::DeepWater))
		r.warnings.push_back(what + " stands in water");
	if (!isWaterProp(prop) && prop != DECOR_WEB && !isWall(grid.at(col, row - 1)))
		r.warnings.push_back(what + " has no floor under it");
	if (prop == DECOR_WEB && !isWall(grid.at(col, row + 1)))
		r.warnings.push_back(what + " has no ceiling");
	if (prop != DECOR_WEB && hidesBehindProp(t))
		r.warnings.push_back(
			what + " stands in front of the " + (hasObject(t) ? tileDef(t.type).name : tileDef(t.fixture.type).name)
		);
}

void checkPlacedDecal(const LevelGrid& grid, int cell, LevelReport& r) {
	const int col = cell % LEVEL_WIDTH;
	const int row = cell / LEVEL_WIDTH;
	const Tile t = grid.cells[cell];
	const Dressing& d = t.dressing;
	const bool torch = d.torch == TorchPlacing::Torch;
	if (d.decal >= 0) {
		const DecalAnchor anchor = DECAL_DEFS[d.decal].anchor;
		const std::string what = "decal " + std::to_string(d.decal) + " at " + at(cell);
		if (isWall(t))
			r.warnings.push_back(what + " is in rock");
		else if (!tileDef(t.type).decals || !tileDef(t.fixture.type).decals)
			r.warnings.push_back(what + " is on a cell that takes no decals");
		if (anchor == DecalAnchor::Ceiling && !isWall(grid.at(col, row + 1)))
			r.warnings.push_back(what + " hangs from the ceiling, there is none");
		if (anchor == DecalAnchor::Floor &&
			(!isWall(grid.at(col, row - 1)) || t.structure != Structure::Empty || d.prop >= 0))
			r.warnings.push_back(what + " grows from the floor: no floor, in water or sand, or behind a prop");
		if (anchor == DecalAnchor::Free && torch)
			r.warnings.push_back(what + " is behind the torch");
	}
	if (!torch)
		return;
	const std::string what = "the torch at " + at(cell);
	if (isWall(t))
		r.warnings.push_back(what + " is in rock");
	else if (t.structure == Structure::DeepWater)
		r.warnings.push_back(what + " is in deep water");
	else if (!tileDef(t.type).torch || !tileDef(t.fixture.type).torch)
		r.warnings.push_back(what + " is on a cell that takes no torch");
	if (d.prop == DECOR_BRAZIER || d.prop == DECOR_LAMP || isThoth(d.prop))
		r.warnings.push_back(what + " is over a " + DECOR_NAMES[d.prop]);
}

// The decor and decal layers (plan placed-decor): a placed prop, decal or torch where it fits.
void checkDressing(const LevelGrid& grid, LevelReport& r) {
	for (int cell = 0; cell < CELLS; cell++) {
		checkPlacedProp(grid, cell, r);
		checkPlacedDecal(grid, cell, r);
	}
}

// Half water stands on deep water or a wall, deep water only under water; a ladder starts in the water and goes up,
// none goes down into it; a crocodile lives in or next to the water, a lotus in it.
void checkWater(const LevelGrid& grid, LevelReport& r) {
	for (int cell = 0; cell < CELLS; cell++) {
		const int col = cell % LEVEL_WIDTH;
		const int row = cell / LEVEL_WIDTH;
		const Tile t = grid.cells[cell];
		const Tile below = grid.at(col, row - 1);
		const Tile above = grid.at(col, row + 1);
		if (inHalfWater(t) && !isSolidStructure(below.structure))
			r.warnings.push_back("half water at " + at(cell) + " is not on deep water or a wall");
		if (t.structure == Structure::DeepWater && above.structure != Structure::HalfWater &&
			above.structure != Structure::DeepWater)
			r.warnings.push_back("deep water at " + at(cell) + " is not under water");
		if (t.type == Ladder && !inHalfWater(t) && inHalfWater(below) && below.type != Ladder)
			r.warnings.push_back("the ladder at " + at(cell) + " goes down into the water");
		const MonsterKind* kind = t.type == MonsterSpawn ? monsterKind(t.attr) : nullptr;
		if (kind != nullptr && kind->locomotion == Locomotion::Submerged && !inHalfWater(t) &&
			!inHalfWater(grid.at(col - 1, row)) && !inHalfWater(grid.at(col + 1, row)))
			r.warnings.push_back(std::string("the ") + kind->label + " at " + at(cell) + " is not in or next to water");
		if (kind != nullptr && kind->locomotion == Locomotion::Floating && !inHalfWater(t))
			r.warnings.push_back(std::string("the ") + kind->label + " at " + at(cell) + " is not in half water");
	}
}

// Sand and quicksand (plan desert-chapter) lie on a wall; a run of quicksand is at most QUICKSAND_MAX_RUN cells (a jump
// out of it lands past it).
void checkSand(const LevelGrid& grid, LevelReport& r) {
	for (int cell = 0; cell < CELLS; cell++) {
		const int col = cell % LEVEL_WIDTH;
		const int row = cell / LEVEL_WIDTH;
		const Tile t = grid.cells[cell];
		if (!inSand(t))
			continue;
		r.sandCells++;
		r.quicksandCells += inQuicksand(t) ? 1 : 0;
		if (grid.at(col, row - 1).structure != Structure::Wall)
			r.warnings.push_back(std::string(structureDef(t.structure).name) + " at " + at(cell) + " is not on a wall");
		if (inQuicksand(t) && !inQuicksand(grid.at(col - 1, row))) {
			int run = 0;
			while (inQuicksand(grid.at(col + run, row)))
				run++;
			if (run > QUICKSAND_MAX_RUN)
				r.warnings.push_back(
					"quicksand at " + at(cell) + " runs " + std::to_string(run) + " cells, at most " +
					std::to_string(QUICKSAND_MAX_RUN)
				);
		}
	}
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

// Each teleporter pair id is used by exactly two teleporters.
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
	const std::vector<char> sealed = reachableCells(Walker(grid, true, true), startState);
	const std::vector<char> open = reachableCells(Walker(grid), startState);
	for (int cell = 0; cell < CELLS; cell++) {
		const Tile t = grid.cells[cell];
		if (t.fixture.type == Inscription && t.fixture.attr == NAME_CARTOUCHE_ID) {
			if (sealed[cell] == 0)
				r.warnings.push_back("the name cartouche at " + at(cell) + " is not found with the name gate sealed");
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
		[&](int cell) { return isSolidStructure(grid.cells[cell].structure); },
		[](int) { return true; }
	);
	const std::vector<char> atStart = sunlitCells(
		grid.cells,
		[&](int cell) { return blocksBeam(grid.cells[cell]); },
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

float difficultyScore(const LevelReport& r, const LevelGrid& grid) {
	float score = 0.04f * static_cast<float>(r.pathLength);
	score += 1.0f * static_cast<float>(r.pathSpikes) + 4.f * static_cast<float>(r.pathDeathTraps);
	score += 3.f * static_cast<float>(r.pathRockFalls) + 1.2f * static_cast<float>(r.pathJumps);
	score += 1.f * static_cast<float>(r.pathSandFalls) + 1.5f * static_cast<float>(r.pathQuicksand);
	score += 0.02f * static_cast<float>(r.pathSand);
	score += 2.f * static_cast<float>(r.pathDartPlates);
	score += 1.5f * static_cast<float>(r.pathWallSpikes + r.pathFireVents + r.pathPoisonDrips);
	score += 0.8f * static_cast<float>(r.pathGates);
	score += 0.8f * static_cast<float>(r.pathTurns); // a light puzzle on the way

	// Monsters near the path count fully, the rest of the level a little (the player may go looking for loot).
	std::set<int> near;
	for (const CellPos& p : r.path)
		for (int dx = -MONSTER_REACH; dx <= MONSTER_REACH; dx++) {
			Tile t = grid.at(p.col + dx, p.row);
			if (t.type == MonsterSpawn)
				near.insert(p.row * LEVEL_WIDTH + p.col + dx);
		}
	for (int cell = 0; cell < CELLS; cell++) {
		Tile t = grid.cells[cell];
		if (t.type == MonsterSpawn)
			score += monsterThreat(t.attr) * (near.count(cell) != 0 ? 1.f : 0.25f);
	}

	// Jumps over spike pits hurt when missed.
	for (size_t i = 1; i < r.path.size(); i++) {
		const CellPos& a = r.path[i - 1];
		const CellPos& b = r.path[i];
		if (std::abs(b.col - a.col) == 2 && grid.at((a.col + b.col) / 2, a.row - 1).type == Death)
			score += 1.f;
	}

	// Treasure on the way is a little help.
	score -= 0.2f * static_cast<float>(r.reachableTreasures);
	return std::max(0.f, score);
}
} // namespace

float monsterThreat(int type) {
	const MonsterKind* kind = monsterKind(type);
	if (kind == nullptr)
		return UNKNOWN_MONSTER_THREAT;
	return kind->threat * (kind->climbs ? CLIMBER_THREAT_FACTOR : 1.f) * (kind->kite ? KITER_THREAT_FACTOR : 1.f);
}

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

LevelReport checkLevel(const LevelGrid& grid) {
	LevelReport r;
	countContent(grid, r);
	checkLocks(grid, r);
	checkObjectsInRock(grid, r);
	checkWater(grid, r);
	checkSand(grid, r);
	checkDeadGates(grid, r);
	checkTeleporters(grid, r);
	checkWallSpikes(grid, r);
	checkDressing(grid, r);
	const int litAtStart = checkSunBeams(grid, r);

	if (r.entrances == 0)
		r.errors.emplace_back("no entrance (Door with attribute 1)");
	if (r.entrances > 1)
		r.warnings.push_back(
			std::to_string(r.entrances) + " entrances, the game starts at the last one " +
			at(r.start.row * LEVEL_WIDTH + r.start.col)
		);
	if (r.exits == 0)
		r.errors.emplace_back("no exit (Door with attribute 2) and no ankh");
	if (r.entrances == 0) {
		r.valid = false;
		return r;
	}

	Walker walker(grid);
	int fallen = 0;
	int startState = walker.settle(r.start.col, r.start.row, litAtStart, fallen);
	if (startState == NONE) {
		r.errors.emplace_back("the entrance has no floor below it");
		return r;
	}

	// Dijkstra over (cell, opened colours), keeping the edges for the backward pass.
	checkBossRoom(grid, startState, r);
	checkNameGates(grid, startState, r);

	std::vector<int> dist(STATES, NONE);
	std::vector<int> prev(STATES, NONE);
	std::vector<Move> prevMove(STATES, Move::Start);
	std::vector<std::vector<int>> incoming(STATES);
	using Item = std::pair<int, int>; // cost, state
	std::priority_queue<Item, std::vector<Item>, std::greater<>> open;
	dist[startState] = 0;
	open.push({0, startState});
	std::vector<Edge> edges;
	std::vector<char> expanded(STATES, 0);
	while (!open.empty()) {
		auto [cost, state] = open.top();
		open.pop();
		if (expanded[state] != 0)
			continue;
		expanded[state] = 1;
		walker.successors(state, edges);
		for (const Edge& e : edges) {
			incoming[e.to].push_back(state);
			int nd = cost + e.cost;
			if (dist[e.to] == NONE || nd < dist[e.to]) {
				dist[e.to] = nd;
				prev[e.to] = state;
				prevMove[e.to] = e.move;
				open.push({nd, e.to});
			}
		}
	}

	// Reachable cells and the cheapest goal state.
	std::vector<char> cellReached(CELLS, 0);
	int goal = NONE;
	for (int s = 0; s < STATES; s++) {
		if (expanded[s] == 0)
			continue;
		cellReached[cellOf(s)] = 1;
		if (isGoal(grid.cells[cellOf(s)]) && (goal == NONE || dist[s] < dist[goal]))
			goal = s;
	}
	bool poisoners = false;
	bool antidote = false; // in reach
	for (int cell = 0; cell < CELLS; cell++) {
		const Tile t = grid.cells[cell];
		poisoners = poisoners || (t.type == MonsterSpawn && isPoisoner(t.attr));
		if (cellReached[cell] == 0)
			continue;
		r.reachableCells++;
		r.reachablePages += t.type == Page ? 1 : 0;
		r.reachableTreasures += isTreasure(t) ? 1 : 0;
		// A trapped chest's antidote does not count: half of them are empty (plan trapped-chests).
		antidote = antidote || (t.type == Treasure && itemFromFile(t.attr, t.value) == ItemKind::Antidote);
	}
	if (poisoners && !antidote)
		r.warnings.emplace_back("poisoners but no antidote in reach");
	if (r.poisonDrips > 0 && !antidote) // plan poison-drip-trap
		r.warnings.emplace_back("poison drips but no antidote in reach");
	for (int cell = 0; cell < CELLS; cell++) {
		if (cellReached[cell] != 0)
			continue;
		const Tile t = grid.cells[cell];
		if (t.type == Key)
			r.warnings.push_back("key out of reach at " + at(cell));
		if (t.type == Page)
			r.warnings.push_back("expedition page out of reach at " + at(cell));
		if (t.fixture.type == Lever || t.fixture.type == Inscription)
			r.warnings.push_back(
				std::string(t.fixture.type == Lever ? "lever" : "inscription") + " out of reach at " + at(cell)
			);
	}
	if (r.treasures > r.reachableTreasures)
		r.warnings.push_back(std::to_string(r.treasures - r.reachableTreasures) + " treasure(s) out of reach");

	if (goal == NONE) {
		if (r.exits > 0)
			r.errors.emplace_back("the exit cannot be reached from the entrance");
		r.valid = false;
		return r;
	}

	// Backwards from every goal state: reachable states that cannot get there are softlocks.
	std::vector<char> canFinish(STATES, 0);
	std::vector<int> stack;
	for (int s = 0; s < STATES; s++)
		if (expanded[s] != 0 && isGoal(grid.cells[cellOf(s)])) {
			canFinish[s] = 1;
			stack.push_back(s);
		}
	while (!stack.empty()) {
		int s = stack.back();
		stack.pop_back();
		for (int from : incoming[s])
			if (canFinish[from] == 0) {
				canFinish[from] = 1;
				stack.push_back(from);
			}
	}
	std::vector<char> softCell(CELLS, 0);
	int example = NONE;
	for (int s = 0; s < STATES; s++)
		if (expanded[s] != 0 && canFinish[s] == 0 && grid.cells[cellOf(s)].type != Death && softCell[cellOf(s)] == 0) {
			softCell[cellOf(s)] = 1;
			r.softlockCells++;
			if (example == NONE)
				example = cellOf(s);
		}
	if (r.softlockCells > 0)
		r.warnings.push_back(
			std::to_string(r.softlockCells) + " cell(s) the player can reach but not leave for the exit, e.g. " +
			at(example)
		);

	// The path, entrance first.
	std::vector<int> states;
	for (int s = goal; s != NONE; s = prev[s])
		states.push_back(s);
	std::reverse(states.begin(), states.end());
	int gateColours = 0;
	for (size_t i = 0; i < states.size(); i++) {
		int cell = cellOf(states[i]);
		r.path.push_back({cell % LEVEL_WIDTH, cell / LEVEL_WIDTH});
		r.pathMoves.push_back(prevMove[states[i]]);
		Tile t = grid.cells[cell];
		if (i > 0) {
			r.pathLength++;
			Move m = prevMove[states[i]];
			r.pathJumps += m == Move::Jump ? 1 : 0;
			r.pathDrops += m == Move::Drop ? 1 : 0;
			r.pathClimb += m == Move::Climb ? 1 : 0;
			r.pathTeleports += m == Move::Teleport ? 1 : 0;
			if (m == Move::Turn) {
				r.pathTurns++;
				const int gained = maskOf(states[i]) & ~maskOf(states[i - 1]);
				for (const Walker::Turn& turn : walker.turns(states[i - 1]))
					if (bitOf(turn.colour) == gained)
						r.pathTurnWays.push_back(turn.way);
			}
		}
		r.pathSpikes += t.type == Spike ? 1 : 0;
		r.pathDeathTraps += t.type == Death ? 1 : 0;
		r.pathRockFalls += t.type == RockFall && rockState(t) == RockState::Armed && !isSandFall(t) ? 1 : 0;
		r.pathSandFalls += isSandFall(t) && rockState(t) == RockState::Armed ? 1 : 0;
		r.pathSand += inSand(t) && !inQuicksand(t) ? 1 : 0;
		r.pathQuicksand += inQuicksand(t) ? 1 : 0;
		r.pathDartPlates += t.type == DartPlate ? 1 : 0;
		r.pathWallSpikes += t.fixture.type == WallSpikes ? 1 : 0;
		r.pathFireVents += t.fixture.type == FireVent ? 1 : 0;
		r.pathPoisonDrips += t.fixture.type == PoisonDrip ? 1 : 0;
		if (t.type == Gate && gateState(t) == GateState::Closed) {
			r.pathGates++;
			gateColours |= bitOf(t.attr);
		}
	}
	for (int c = 0; c < LOCK_COLOUR_COUNT; c++)
		r.keysNeeded += (gateColours >> c) & 1;
	r.pathBoss = (gateColours & bitOf(BOSS_LOCK)) != 0;

	std::set<int> monstersNear;
	for (const CellPos& p : r.path)
		for (int dx = -MONSTER_REACH; dx <= MONSTER_REACH; dx++)
			if (grid.at(p.col + dx, p.row).type == MonsterSpawn)
				monstersNear.insert(p.row * LEVEL_WIDTH + p.col + dx);
	r.pathMonsters = static_cast<int>(monstersNear.size());

	if (r.pathDeathTraps > 0)
		r.warnings.emplace_back("the only way to the exit walks over a death trap");
	if (r.pathSpikes > 0) // a crossing costs SPIKES_CROSS_HP: deadly early on
		r.warnings.emplace_back("the only way to the exit walks over spikes");
	if (r.pathDartPlates > 0 && !antidote)
		r.warnings.emplace_back("a dart trap on the path but no antidote in reach");

	r.difficulty = difficultyScore(r, grid);
	r.valid = r.errors.empty();
	return r;
}

char propGlyph(const Tile& t) {
	const int prop = t.dressing.prop;
	if (prop >= 0)
		return static_cast<char>('a' + prop);
	if (prop == PLACED_BARE)
		return '-';
	return isSolidStructure(t.structure) ? '#' : '.';
}

char decalGlyph(const Tile& t) {
	const Dressing& d = t.dressing;
	const bool torch = d.torch == TorchPlacing::Torch;
	if (d.decal >= 0)
		return static_cast<char>((torch ? 'A' : 'a') + d.decal);
	if (torch)
		return 't';
	if (hasPlacedDecal(d))
		return '-';
	return isSolidStructure(t.structure) ? '#' : '.';
}

std::string renderLevel(const LevelGrid& grid, const LevelReport* report) {
	std::vector<char> onPath(CELLS, 0);
	if (report != nullptr)
		for (const CellPos& p : report->path)
			onPath[p.row * LEVEL_WIDTH + p.col] = 1;

	std::string objects, structure, wall, decor, decals;
	bool layered = false, fixtures = false; // water or sand: the structure drawing too
	bool props = false, wallDecals = false;
	for (int row = LEVEL_HEIGHT - 1; row >= 0; row--) {
		std::string line, structureLine, wallLine, decorLine, decalLine;
		bool any = false;
		for (int col = 0; col < LEVEL_WIDTH; col++) {
			Tile t = grid.at(col, row);
			char c = tileGlyph(t);
			any = any || !isWall(t) || hasObject(t) || hasFixture(t);
			layered = layered || (t.structure != Structure::Wall && t.structure != Structure::Empty);
			fixtures = fixtures || hasFixture(t);
			props = props || hasPlacedProp(t.dressing);
			wallDecals = wallDecals || hasPlacedDecal(t.dressing);
			if (c == '.' && onPath[row * LEVEL_WIDTH + col] != 0)
				c = '*';
			line += c;
			structureLine += structureGlyph(t.structure);
			wallLine += hasFixture(t) ? tileGlyph(fixtureAsTile(t.fixture)) : isSolidStructure(t.structure) ? '#' : '.';
			decorLine += propGlyph(t);
			decalLine += decalGlyph(t);
		}
		if (any) {
			char head[8];
			snprintf(head, sizeof(head), "%2d ", row);
			objects += head + line + '\n';
			structure += head + structureLine + '\n';
			wall += head + wallLine + '\n';
			decor += head + decorLine + '\n';
			decals += head + decalLine + '\n';
		}
	}
	return objects + (layered ? "structure\n" + structure : "") + (fixtures ? "wall\n" + wall : "") +
		   (props ? "decor\n" + decor : "") + (wallDecals ? "decals\n" + decals : "");
}
