#include "level_check.h"
#include "climb_path.h"
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
constexpr int NONE = -1;
// The level in sub-cells (plan subcell-checker): X across, Y up, SUBCELLS of each per cell.
constexpr int SUB_WIDTH = LEVEL_WIDTH * SUBCELLS;
constexpr int SUB_HEIGHT = LEVEL_HEIGHT * SUBCELLS;
constexpr int SUBS_PER_CELL = SUBCELLS * SUBCELLS;
// A jump over a gap a cell wide: from a cell's last sub-cell column over the next cell into the first one of the cell
// after (Jump::ARC's drift and the walk carry the player past 1.5 tiles, movement.h).
constexpr int JUMP_SUBCELLS = SUBCELLS + 1;
// Head room in sub-cells over the feet: a jump over a gap needs the whole arc (a bump cuts it short), a jump onto a
// quarter step the body over a quarter's rise.
constexpr int JUMP_ROOM = 4;
constexpr int STEP_ROOM = 3;
static_assert(
	Jump::ARC.peak + PLAYER_BODY_HEIGHT <= static_cast<float>(JUMP_ROOM) * SUBCELL_SIZE,
	"a jump's head stays in JUMP_ROOM sub-cells"
);
static_assert(
	SUBCELL_SIZE + PLAYER_BODY_HEIGHT <= static_cast<float>(STEP_ROOM) * SUBCELL_SIZE,
	"onto a quarter step the head stays in STEP_ROOM sub-cells"
);
static_assert(
	PLAYER_BODY_HEIGHT <= 2.f * SUBCELL_SIZE,
	"the player stands in two sub-cells, crouched in one (movement.h)"
);

// Path costs: the cheapest path avoids hazards when there is a way around them.
constexpr int COST_MOVE = 2;
constexpr int COST_JUMP = 4;
constexpr int COST_STEP = 3; // a jump onto a quarter step: short, no gap under it
constexpr int COST_CRAWL = 1; // per sub-cell crawled: the crouched walk is a third of the walk (plan crouch)
constexpr int COST_SPIKE = 30; // a walk over it costs SPIKES_CROSS_HP: deadly early on, jump it
constexpr int COST_DEATH = 30;
constexpr int COST_ROCK_FALL = 15;
constexpr int COST_DART_PLATE = 8;
constexpr int COST_WALL_SPIKES = 6; // the wait for the safe window, the risk of misjudging it
constexpr int COST_DRIP_PUDDLE = 10; // a poison drip's puddle (plan poison-drip-trap): weak poison, jump it
constexpr int COST_GUST = 3; // a sand gust (plan sandstorm-halls): the wait for the calm, the ground lost to a gust
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

int nodeOf(int state) { return state / MASKS; }
int maskOf(int state) { return state % MASKS; }
int bitOf(int colour) { return isGateColour(colour) ? 1 << (colour - 1) : 0; }
int subCell(int x, int y) { return (y / SUBCELLS) * LEVEL_WIDTH + x / SUBCELLS; }
bool subInBounds(int x, int y) { return x >= 0 && x < SUB_WIDTH && y >= 0 && y < SUB_HEIGHT; }

// A fine cell (plan subcell-checker) is walked by its sub-cells: a shaped one, or one over a shaped cell whose top row
// is solid in part (the player stands on the solid sub-cells and falls by the open ones). A ladder's cell is not (a
// ladder in a shape is warned about, checkShapes).
bool fineCell(const LevelGrid& grid, int cell) {
	const Tile t = grid.cells[cell];
	if (t.type == Ladder || isSolidStructure(t.structure))
		return false;
	if (hasShape(t))
		return true;
	constexpr std::uint16_t TOP_ROW = 0xf000;
	const Tile below = grid.at(cell % LEVEL_WIDTH, cell / LEVEL_WIDTH - 1);
	const int top = below.shape & TOP_ROW;
	return hasShape(below) && top != 0 && top != TOP_ROW;
}

// The walker's places. A cell is one node, as before shapes; a fine cell has one node per standing place: a sub-cell
// open over a solid one, or on the cell's bottom row (whether the cell below carries it depends on the gates). The
// cell's own node goes unused then.
class Nodes {
  public:
	explicit Nodes(const LevelGrid& grid) : sub(static_cast<size_t>(CELLS) * SUBS_PER_CELL, NONE), fineCells(CELLS, 0) {
		for (int cell = 0; cell < CELLS; cell++) {
			nodeCell.push_back(cell);
			place.push_back(0);
		}
		for (int cell = 0; cell < CELLS; cell++) {
			if (!fineCell(grid, cell))
				continue;
			fineCells[cell] = 1;
			const Tile t = grid.cells[cell];
			for (int q = 0; q < SUBCELLS; q++)
				for (int s = 0; s < SUBCELLS; s++)
					if (!subcellSolid(t, s, q) && (q == 0 || subcellSolid(t, s, q - 1))) {
						sub[subIndex(cell, s, q)] = static_cast<int>(nodeCell.size());
						nodeCell.push_back(cell);
						place.push_back(s + SUBCELLS * q);
					}
		}
	}

	[[nodiscard]] int count() const { return static_cast<int>(nodeCell.size()); }
	[[nodiscard]] int cell(int node) const { return nodeCell[node]; }
	[[nodiscard]] bool fine(int node) const { return node >= CELLS; }
	// The node of the place at sub-cell (x, y): its cell's for a cell that is not fine, NONE for no standing place.
	[[nodiscard]] int at(int x, int y) const {
		if (!subInBounds(x, y))
			return NONE;
		const int cell = subCell(x, y);
		const int fineNode = sub[subIndex(cell, x % SUBCELLS, y % SUBCELLS)];
		return fineCells[cell] != 0 ? fineNode : cell;
	}
	// Where the player stands at the node, leaving it going dir: a cell's node stands at its side.
	[[nodiscard]] int x(int node, int dir) const {
		const int col = nodeCell[node] % LEVEL_WIDTH;
		return col * SUBCELLS + (fine(node) ? place[node] % SUBCELLS : (dir > 0 ? SUBCELLS - 1 : 0));
	}
	[[nodiscard]] int y(int node) const {
		return nodeCell[node] / LEVEL_WIDTH * SUBCELLS + (fine(node) ? place[node] / SUBCELLS : 0);
	}
	// The nodes standing in the cell.
	[[nodiscard]] std::vector<int> of(int cell) const {
		if (fineCells[cell] == 0)
			return {cell};
		std::vector<int> out;
		for (int k = 0; k < SUBS_PER_CELL; k++)
			if (const int node = sub[static_cast<size_t>(cell) * SUBS_PER_CELL + k]; node != NONE)
				out.push_back(node);
		return out;
	}

  private:
	std::vector<int> nodeCell; // by node
	std::vector<int> place; // by node: a fine one's sub-cell, s + SUBCELLS * q
	std::vector<int> sub; // by cell and sub-cell: the fine node, or NONE
	std::vector<char> fineCells; // by cell

	[[nodiscard]] static size_t subIndex(int cell, int s, int q) {
		return static_cast<size_t>(cell) * SUBS_PER_CELL + static_cast<size_t>(s + SUBCELLS * q);
	}
};

class Walker {
  public:
	// sealed: the name gates (plan secret-name-gates) do not teleport, as before the name is spoken.
	explicit Walker(const LevelGrid& grid, bool teleports = true, bool sealed = false)
		: grid(grid), nodes(grid), pairs(grid.cells), teleports(teleports), sealed(sealed) {}

	[[nodiscard]] const Nodes& places() const { return nodes; }
	[[nodiscard]] int states() const { return nodes.count() * MASKS; }
	[[nodiscard]] int cellOf(int state) const { return nodes.cell(nodeOf(state)); }

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
		std::vector<char> seen(nodes.count(), 0);
		std::vector<int> stack = {nodeOf(state)};
		seen[nodeOf(state)] = 1;
		std::vector<Edge> edges;
		while (!stack.empty()) {
			const int node = stack.back();
			stack.pop_back();
			reach[nodes.cell(node)] = 1;
			successors(node * MASKS + mask, edges, false);
			for (const Edge& e : edges)
				if (seen[nodeOf(e.to)] == 0) {
					seen[nodeOf(e.to)] = 1;
					stack.push_back(nodeOf(e.to));
				}
		}
		std::vector<std::vector<MirrorSetting>> ways;
		const std::vector<char> lit = sunlitCells(
			grid.cells,
			[&](int cell, BeamDir way) { return blocksBeam(cell, mask, way); },
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

	// The beam going way stops in the cell with these colours open (blocksBeam).
	[[nodiscard]] bool blocksBeam(int cell, int mask, BeamDir way) const {
		const Tile& t = grid.cells[cell];
		if (t.type == Gate)
			return gateState(t) == GateState::Closed && (mask & bitOf(t.attr)) == 0;
		return isSolidStructure(t.structure) || shapeStopsBeam(t, way);
	}

	// In level data a gate is closed (0) or open: the game opens one saved mid-motion (2) on load.
	[[nodiscard]] bool solid(int col, int row, int mask) const {
		Tile t = grid.at(col, row);
		if (t.type == Gate)
			return gateState(t) == GateState::Closed && (mask & bitOf(t.attr)) == 0;
		return isSolidTile(t);
	}

	// Sub-cell (x, y) is open: in the level, its cell not solid, the sub-cell not in the cell's shape.
	[[nodiscard]] bool open(int x, int y, int mask) const {
		if (!subInBounds(x, y) || solid(x / SUBCELLS, y / SUBCELLS, mask))
			return false;
		return !subcellSolid(grid.cells[subCell(x, y)], x % SUBCELLS, y % SUBCELLS);
	}

	// The player stands at sub-cell (x, y): open, over a solid sub-cell (a whole solid cell, isFloor, is solid in
	// every one) or in a ladder's cell.
	[[nodiscard]] bool stands(int x, int y, int mask) const {
		return open(x, y, mask) && (grid.cells[subCell(x, y)].type == Ladder || !open(x, y - 1, mask));
	}

	// Standing there, the player has head room only crouched: a crawl space (plan crouch).
	[[nodiscard]] bool crawl(int x, int y, int mask) const { return !open(x, y + 1, mask); }

	// On the cell's bottom: in its water or sand, not on a step (Dungeon::playerOnStep).
	[[nodiscard]] static bool atBottom(int y) { return y % SUBCELLS == 0; }
	[[nodiscard]] bool inQuicksandAt(int x, int y) const {
		return atBottom(y) && inQuicksand(grid.cells[subCell(x, y)]);
	}

	// A floor at or under sub-cell (x, y) down to its cell's bottom: else a gap, the fall goes under the row.
	[[nodiscard]] bool floorInRow(int x, int y, int mask) const {
		for (int down = y; down >= y - y % SUBCELLS; down--)
			if (!open(x, down, mask))
				return false;
			else if (stands(x, down, mask))
				return true;
		return false;
	}

	// A jump takes off here: from a floor (a ladder's foot too), with room sub-cells of head room, not out of half
	// water. A jump from mid-ladder is not modelled: its reach depends on the walk key held during the jump.
	[[nodiscard]] bool takesOff(int x, int y, int mask, int room) const {
		if (open(x, y - 1, mask) || (atBottom(y) && inHalfWater(grid.cells[subCell(x, y)])))
			return false;
		for (int k = 1; k < room; k++)
			if (!open(x, y + k, mask))
				return false;
		return true;
	}

	// Picks up what the player touches in this cell. Reaching the boss counts as killing it: the boss gates open.
	[[nodiscard]] int touch(int col, int row, int mask) const {
		Tile t = grid.at(col, row);
		if (t.type == MonsterSpawn && isBossMonster(t.attr))
			return mask | bitOf(BOSS_LOCK);
		return t.type == Key && isLockColour(t.attr) ? mask | bitOf(t.attr) : mask; // no key opens the boss lock
	}

	// Falls from open sub-cell (x, y) until a floor or a ladder stops the player. NONE if the fall goes under minY (out
	// of the level for 0). fallen: sub-cells.
	[[nodiscard]] int settle(int x, int y, int mask, int& fallen, int minY = 0) const {
		fallen = 0;
		if (!open(x, y, mask))
			return NONE;
		mask = touch(x / SUBCELLS, y / SUBCELLS, mask);
		while (!stands(x, y, mask)) {
			y--;
			fallen++;
			if (y < minY || !open(x, y, mask))
				return NONE;
			if (y % SUBCELLS == SUBCELLS - 1)
				mask = touch(x / SUBCELLS, y / SUBCELLS, mask);
		}
		const int node = nodes.at(x, y);
		return node == NONE ? NONE : node * MASKS + mask;
	}

	// Into a cell at its bottom row, in its middle (the entrance, a teleporter's partner).
	[[nodiscard]] int settleCell(int col, int row, int mask) const {
		int fallen = 0;
		return settle(col * SUBCELLS + SUBCELLS / 2, row * SUBCELLS, mask, fallen);
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
							   : (t.fixture.type == SandGust ? COST_GUST : 0);
		if (t.type == DartPlate)
			return COST_DART_PLATE + spikes;
		if (t.type == MonsterSpawn)
			return COST_MONSTER + spikes;
		return spikes;
	}

	// A trap jumped over rather than walked into: spikes, a death trap, a poison drip's puddle, quicksand.
	[[nodiscard]] static bool jumpedOver(const Tile& t) {
		return t.type == Spike || t.type == Death || t.fixture.type == PoisonDrip || inQuicksand(t);
	}

	void successors(int state, std::vector<Edge>& out, bool turning = true) const {
		out.clear();
		const int node = nodeOf(state);
		const int cell = nodes.cell(node);
		const int mask = maskOf(state);
		const int col = cell % LEVEL_WIDTH;
		const int row = cell / LEVEL_WIDTH;
		const Tile here = grid.at(col, row);

		// A move inside the cell costs no walk and no hazard (the player is in it already); the walk's extras stay.
		auto add = [&](int to, int base, Move move) {
			if (to == NONE || to == state)
				return;
			const bool along = move == Move::Walk || move == Move::Drop || move == Move::Jump || move == Move::Crawl ||
							   move == Move::Step;
			const bool inCell = along && cellOf(to) == cell;
			const int walk = inCell && move != Move::Jump && move != Move::Step ? COST_MOVE : 0;
			out.push_back({to, base - walk + (inCell ? 0 : hazardCost(cellOf(to))), move});
		};

		const Fixture& lever = here.fixture;
		if (lever.type == Lever && isLockColour(lever.attr) && !leverPulled(lever)) // a pulled lever opens nothing
			add(node * MASKS + (mask | bitOf(lever.attr)), COST_MOVE, Move::Pull);
		if (turning && here.type == Mirror)
			for (const Turn& turn : turns(state))
				add(node * MASKS + (mask | bitOf(turn.colour)), COST_TURN, Move::Turn);
		if (teleports && isTeleporter(here) && !(sealed && isNameGate(here))) {
			int to = pairs.Partner(cell);
			if (to != NONE)
				add(settleCell(to % LEVEL_WIDTH, to / LEVEL_WIDTH, mask), COST_TELEPORT, Move::Teleport);
		}

		for (int dir : {-1, 1}) {
			const int x = nodes.x(node, dir);
			const int y = nodes.y(node);
			walk(x, y, dir, mask, add);
			if (here.type == Ladder) // off a ladder onto a step beside it: up to the step's height, then over
				for (int q = 1; q < SUBCELLS; q++)
					if (stands(x + dir, y + q, mask)) {
						int fallen = 0;
						add(settle(x + dir, y + q, mask, fallen),
							COST_MOVE + (crawl(x + dir, y + q, mask) ? COST_CRAWL : 0),
							crawl(x + dir, y + q, mask) ? Move::Crawl : Move::Walk);
					}
			jump(x, y, dir, mask, add);
		}

		if (here.type == Ladder) {
			for (int dy : {-1, 1})
				if (grid.at(col, row + dy).type == Ladder)
					add((cell + dy * LEVEL_WIDTH) * MASKS + touch(col, row + dy, mask), COST_MOVE, Move::Climb);
		}
	}

  private:
	const LevelGrid& grid;
	Nodes nodes;
	TeleportPairs pairs;
	bool teleports;
	bool sealed;
	mutable std::map<int, std::vector<Turn>> turnCache; // by state

	// One sub-cell along the row from (x, y): a walk (a crawl into a crawl space, a drop off an edge), or a jump up
	// onto a quarter step (no higher: Jump::ARC, movement.h).
	template <typename Add> void walk(int x, int y, int dir, int mask, Add& add) const {
		const int next = x + dir;
		if (open(next, y, mask)) {
			int fallen = 0;
			const int to = settle(next, y, mask, fallen);
			// Quicksand slows the walk to nothing (QUICKSAND_STUCK_MS) before the player has crossed a cell of it: no
			// walk on from quicksand into more of it, only the jump out (plan paths-failures).
			if (to == NONE || (fallen == 0 && inQuicksandAt(x, y) && inQuicksandAt(next, y)))
				return;
			const bool crawling = fallen == 0 && crawl(next, y, mask);
			add(to,
				COST_MOVE + (fallen + SUBCELLS - 1) / SUBCELLS + (crawling ? COST_CRAWL : 0),
				fallen > 0 ? Move::Drop : (crawling ? Move::Crawl : Move::Walk));
			return;
		}
		if (!takesOff(x, y, mask, STEP_ROOM))
			return;
		for (int k = 1; k < STEP_ROOM; k++)
			if (!open(next, y + k, mask))
				return;
		int fallen = 0;
		add(settle(next, y + 1, mask, fallen), COST_STEP, Move::Step);
	}

	// Up to JUMP_SUBCELLS along the row, from a floor: over a gap or a trap (jumpedOver), landing on a floor no higher
	// than the take-off and no lower than its cell's bottom (a jump off a step lands on the floor beside it); or onto
	// a quarter step past the next sub-cell (over water, say: the arc's feet are over a quarter from about a third of a
	// tile out to past a cell, Jump::ARC). A rock fall is not jumped: the player passes through its cell either way,
	// and it drops all the same. Never into quicksand: the player would land deep in it, too far in to walk out (plan
	// paths-failures).
	template <typename Add> void jump(int x, int y, int dir, int mask, Add& add) const {
		if (!takesOff(x, y, mask, JUMP_ROOM))
			return;
		const int start = subCell(x, y);
		bool gap = false;
		int trapCell = NONE; // the last cell with a trap jumped over
		int passed = mask; // with the keys touched on the way
		for (int k = 1; k <= JUMP_SUBCELLS; k++) {
			const int at = x + dir * k;
			if (!open(at, y, mask)) {
				bool room = k >= 2;
				for (int h = 1; h < JUMP_ROOM; h++)
					room = room && open(at, y + h, mask);
				int fallen = 0;
				if (room)
					add(settle(at, y + 1, passed, fallen), COST_JUMP, Move::Step);
				return;
			}
			if (!open(at, y + 1, mask))
				return;
			const int cell = subCell(at, y);
			if (k >= 2 && (gap || (trapCell != NONE && trapCell != cell)) && !inQuicksandAt(at, y)) {
				int fallen = 0;
				const int to = settle(at, y, passed, fallen, y - y % SUBCELLS);
				if (to != NONE && !inQuicksandAt(at, y - fallen))
					add(to, COST_JUMP, Move::Jump);
			}
			for (int h = 2; h < JUMP_ROOM; h++)
				if (!open(at, y + h, mask))
					return;
			if (!floorInRow(at, y, mask))
				gap = true;
			else if (cell != start && jumpedOver(grid.cells[cell]))
				trapCell = cell;
			passed = touch(at / SUBCELLS, y / SUBCELLS, passed);
		}
	}
};

// States the walker reaches from the start states.
std::vector<char> reachableStates(const Walker& walker, const std::vector<int>& startStates) {
	std::vector<char> seen(walker.states(), 0);
	std::vector<int> stack = startStates;
	for (int state : startStates)
		seen[state] = 1;
	std::vector<Edge> edges;
	while (!stack.empty()) {
		int state = stack.back();
		stack.pop_back();
		walker.successors(state, edges);
		for (const Edge& e : edges)
			if (seen[e.to] == 0) {
				seen[e.to] = 1;
				stack.push_back(e.to);
			}
	}
	return seen;
}

std::vector<char> cellsOf(const Walker& walker, const std::vector<char>& states) {
	std::vector<char> cells(CELLS, 0);
	for (int state = 0; state < walker.states(); state++)
		if (states[state] != 0)
			cells[walker.cellOf(state)] = 1;
	return cells;
}

// Cells the walker reaches from the start state (any opened colours).
std::vector<char> reachableCells(const Walker& walker, int startState) {
	return cellsOf(walker, reachableStates(walker, {startState}));
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
			r.sandGusts += f.type == SandGust ? 1 : 0;
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

// A shaped cell's floor for a placed prop (plan subcell-checker): the same height in every sub-cell column, in
// quarters, with nothing solid over it; -1 when it is not flat. A cell with no shape: 0.
int flatFloor(const Tile& t) {
	const int floor = nextSubcellRow(t, 0, 0, false);
	for (int s = 0; s < SUBCELLS; s++)
		if (nextSubcellRow(t, s, 0, false) != floor || nextSubcellRow(t, s, floor, true) != SUBCELLS)
			return -1;
	return floor;
}

// The sub-cells of the back wall a decal or a torch is drawn on, by its anchor; a solid sub-cell there hides it.
std::uint16_t decalSubcells(DecalAnchor anchor) {
	constexpr std::uint16_t BOTTOM_ROW = 0x000f;
	constexpr std::uint16_t TOP_ROW = 0xf000;
	constexpr std::uint16_t MIDDLE = 0x0660; // the middle two columns of the middle two rows
	return anchor == DecalAnchor::Floor ? BOTTOM_ROW : (anchor == DecalAnchor::Ceiling ? TOP_ROW : MIDDLE);
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
	const int step = flatFloor(t);
	if (step < 0)
		r.warnings.push_back(what + " does not fit the cell's shape: its floor is not flat, or rock is over it");
	if (!isWaterProp(prop) && prop != DECOR_WEB && !isWall(grid.at(col, row - 1)) && step <= 0)
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
		if (hasShape(t) && (t.shape & decalSubcells(anchor)) != 0)
			r.warnings.push_back(what + " is hidden by the cell's shape");
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
	if (hasShape(t) && (t.shape & decalSubcells(DecalAnchor::Free)) != 0)
		r.warnings.push_back(what + " is hidden by the cell's shape");
	if (d.prop == DECOR_BRAZIER || d.prop == DECOR_LAMP || isThoth(d.prop) || d.prop == DECOR_STARGATE)
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

// The sub-cell rows a standing player needs over the floor (PLAYER_BODY_HEIGHT; crouched, one).
constexpr int STAND_ROWS = 2;

// The player stands at the middle of shaped cell t, under the cell above: a chest there is opened standing.
bool roomToStand(const Tile& t, const Tile& above) {
	for (int s = 1; s <= 2; s++) {
		const int floor = nextSubcellRow(t, s, 0, false);
		const int ceiling = nextSubcellRow(t, s, floor, true);
		const int over = ceiling == SUBCELLS && !isSolidTile(above) ? STAND_ROWS : 0; // the room goes on above
		if (ceiling - floor + over >= STAND_ROWS)
			return true;
	}
	return false;
}

// Sub-cells (plan subcell-checker): a shape only in an open structure (a Wall or DeepWater cell ignores it); a ladder,
// gate or door needs its cell open from floor to ceiling, a chest room to stand at it.
void checkShapes(const LevelGrid& grid, LevelReport& r) {
	for (int cell = 0; cell < CELLS; cell++) {
		const Tile t = grid.cells[cell];
		if (t.shape != NO_SHAPE && !takesShape(t.structure))
			r.warnings.push_back("a shape in " + std::string(structureDef(t.structure).name) + " at " + at(cell));
		if (!hasShape(t))
			continue;
		const std::string name = tileDef(t.type).name;
		if (t.type == Ladder || t.type == Gate || t.type == Door)
			r.warnings.push_back(
				name + " in a shaped cell at " + at(cell) + ": it needs the cell open from floor to ceiling"
			);
		else if (isTreasure(t) && !roomToStand(t, grid.at(cell % LEVEL_WIDTH, cell / LEVEL_WIDTH + 1)))
			r.warnings.push_back(name + " at " + at(cell) + " without room to stand at it");
	}
}

// What hurts the player who walks into the cell, by name; empty for nothing.
std::string trapIn(const Tile& t) {
	if (t.type == Spike || t.type == Death || t.type == DartPlate ||
		(t.type == RockFall && rockState(t) == RockState::Armed))
		return tileDef(t.type).name;
	return isRhythmTrap(t.fixture.type) ? tileDef(t.fixture.type).name : "";
}

// A crawl space does not end at a drop of a cell or more, or at a trap: crouched, the player comes out into it
// unawares (plan subcell-checker).
void checkCrawlEnds(const LevelGrid& grid, const Walker& walker, LevelReport& r) {
	constexpr int MASK = 0; // the gates closed: a gate beside a crawl space is no drop
	const Nodes& nodes = walker.places();
	std::set<int> warned;
	for (int node = CELLS; node < nodes.count(); node++) {
		const int x = nodes.x(node, 1);
		const int y = nodes.y(node);
		if (!walker.stands(x, y, MASK) || !walker.crawl(x, y, MASK))
			continue;
		for (int dir : {-1, 1}) {
			const int next = x + dir;
			if (!walker.open(next, y, MASK) || (walker.stands(next, y, MASK) && walker.crawl(next, y, MASK)))
				continue; // a wall, or the crawl space goes on
			int fallen = 0;
			const int to = walker.settle(next, y, MASK, fallen);
			const std::string into = to == NONE || fallen >= SUBCELLS ? std::string("a drop")
																	  : trapIn(grid.cells[walker.cellOf(to)]);
			if (!into.empty() && warned.insert(nodes.cell(node)).second)
				r.warnings.push_back("the crawl space at " + at(nodes.cell(node)) + " ends at " + into);
		}
	}
}

// The top of a step two or three quarters high (more than a jump) beside a floor the player reaches is reached some
// other way, a ladder or a drop onto it (plan subcell-checker).
void checkStepTops(const Walker& walker, const std::vector<char>& expanded, LevelReport& r) {
	constexpr int MASK = MASKS - 1; // the gates open: the shape alone
	const Nodes& nodes = walker.places();
	std::vector<char> reached(nodes.count(), 0);
	for (int state = 0; state < walker.states(); state++)
		if (expanded[state] != 0)
			reached[nodeOf(state)] = 1;
	std::set<int> warned;
	for (int node = CELLS; node < nodes.count(); node++) {
		const int x = nodes.x(node, 1);
		const int y = nodes.y(node);
		if (reached[node] != 0 || !walker.stands(x, y, MASK))
			continue;
		for (int dir : {-1, 1})
			for (int rise = 2; rise < SUBCELLS; rise++) {
				const int beside = nodes.at(x + dir, y - rise);
				if (beside == NONE || reached[beside] == 0 || !walker.stands(x + dir, y - rise, MASK))
					continue;
				bool face = true; // the step's side, from the floor beside it up to its top
				for (int k = 1; k <= rise; k++)
					face = face && !walker.open(x, y - k, MASK);
				if (face && warned.insert(nodes.cell(node)).second)
					r.warnings.push_back(
						"the step at " + at(nodes.cell(node)) + " is out of reach: " + std::to_string(rise) +
						" quarters over the floor beside it, more than a jump, and no other way onto it"
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

// A monster spawned at (spawnCol, row) walks to col along the row, unless a shape on the way stops its body (plan
// subcell-checker: a large one does not follow through a crawl space).
bool monsterReaches(const LevelGrid& grid, int spawnCol, int row, int col) {
	const MonsterKind* kind = monsterKind(grid.at(spawnCol, row).attr);
	if (kind == nullptr)
		return true;
	const BodyFit body{kind->heightQuarters, kind->stepQuarters};
	const int dir = col > spawnCol ? 1 : -1;
	for (int c = spawnCol + dir; c != col + dir; c += dir)
		if (!shapePasses(grid.cells, c, row, dir, body))
			return false;
	return true;
}

// The monster spawns (cells) within MONSTER_REACH of the path along its rows that can reach it.
std::set<int> monstersNear(const LevelGrid& grid, const std::vector<CellPos>& path) {
	std::set<int> near;
	for (const CellPos& p : path)
		for (int dx = -MONSTER_REACH; dx <= MONSTER_REACH; dx++)
			if (grid.at(p.col + dx, p.row).type == MonsterSpawn && monsterReaches(grid, p.col + dx, p.row, p.col))
				near.insert(p.row * LEVEL_WIDTH + p.col + dx);
	return near;
}

float difficultyScore(const LevelReport& r, const LevelGrid& grid) {
	float score = 0.04f * static_cast<float>(r.pathLength);
	score += 1.0f * static_cast<float>(r.pathSpikes) + 4.f * static_cast<float>(r.pathDeathTraps);
	score += 3.f * static_cast<float>(r.pathRockFalls) + 1.2f * static_cast<float>(r.pathJumps);
	score += 0.2f * static_cast<float>(r.pathSteps) + 0.1f * static_cast<float>(r.pathCrawls); // plan subcell-checker
	score += 1.f * static_cast<float>(r.pathSandFalls) + 1.5f * static_cast<float>(r.pathQuicksand);
	score += 0.02f * static_cast<float>(r.pathSand);
	score += 2.f * static_cast<float>(r.pathDartPlates);
	score += 1.5f * static_cast<float>(r.pathWallSpikes + r.pathFireVents + r.pathPoisonDrips);
	score += 0.3f * static_cast<float>(r.pathSandGusts);
	score += 0.8f * static_cast<float>(r.pathGates);
	score += 0.8f * static_cast<float>(r.pathTurns); // a light puzzle on the way

	// Monsters near the path count fully, the rest of the level a little (the player may go looking for loot).
	const std::set<int> near = monstersNear(grid, r.path);
	for (int cell = 0; cell < CELLS; cell++) {
		Tile t = grid.cells[cell];
		if (t.type != MonsterSpawn)
			continue;
		const MonsterKind* kind = monsterKind(t.attr);
		const bool swimsHere = kind != nullptr && kind->wading == Wading::Swimmer && inHalfWater(t);
		score += monsterThreat(t.attr) * (swimsHere ? SWIMMER_IN_WATER_THREAT_FACTOR : 1.f) *
				 (near.count(cell) != 0 ? 1.f : 0.25f);
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
	checkSandGusts(grid, r);
	checkDressing(grid, r);
	checkShapes(grid, r);
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
	const int startState = walker.settleCell(r.start.col, r.start.row, litAtStart);
	if (startState == NONE) {
		r.errors.emplace_back("the entrance has no floor below it");
		return r;
	}

	// Dijkstra over (cell, opened colours), keeping the edges for the backward pass.
	checkBossRoom(grid, startState, r);
	checkNameGates(grid, startState, r);
	checkCrawlEnds(grid, walker, r);

	const int stateCount = walker.states();
	std::vector<int> dist(stateCount, NONE);
	std::vector<int> prev(stateCount, NONE);
	std::vector<Move> prevMove(stateCount, Move::Start);
	std::vector<std::vector<int>> incoming(stateCount);
	using Item = std::pair<int, int>; // cost, state
	std::priority_queue<Item, std::vector<Item>, std::greater<>> open;
	dist[startState] = 0;
	open.push({0, startState});
	std::vector<Edge> edges;
	std::vector<char> expanded(stateCount, 0);
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
	for (int s = 0; s < stateCount; s++) {
		if (expanded[s] == 0)
			continue;
		cellReached[walker.cellOf(s)] = 1;
		if (isGoal(grid.cells[walker.cellOf(s)]) && (goal == NONE || dist[s] < dist[goal]))
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
	checkStepTops(walker, expanded, r);
	if (r.treasures > r.reachableTreasures)
		r.warnings.push_back(std::to_string(r.treasures - r.reachableTreasures) + " treasure(s) out of reach");

	if (goal == NONE) {
		if (r.exits > 0)
			r.errors.emplace_back("the exit cannot be reached from the entrance");
		r.valid = false;
		return r;
	}

	// Backwards from every goal state: reachable states that cannot get there are softlocks.
	std::vector<char> canFinish(stateCount, 0);
	std::vector<int> stack;
	for (int s = 0; s < stateCount; s++)
		if (expanded[s] != 0 && isGoal(grid.cells[walker.cellOf(s)])) {
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
	for (int s = 0; s < stateCount; s++)
		if (expanded[s] != 0 && canFinish[s] == 0 && grid.cells[walker.cellOf(s)].type != Death &&
			softCell[walker.cellOf(s)] == 0) {
			softCell[walker.cellOf(s)] = 1;
			r.softlockCells++;
			if (example == NONE)
				example = walker.cellOf(s);
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
		const int cell = walker.cellOf(states[i]);
		// The sub-cells walked inside a shaped cell are one path cell; a crawl in it makes the way in one.
		const Move inCell = prevMove[states[i]];
		r.pathSteps += i > 0 && inCell == Move::Step ? 1 : 0;
		r.pathCrawls += i > 0 && inCell == Move::Crawl ? 1 : 0;
		if (i > 0 && cell == r.path.back().row * LEVEL_WIDTH + r.path.back().col &&
			(inCell == Move::Walk || inCell == Move::Crawl || inCell == Move::Drop || inCell == Move::Step)) {
			if (inCell == Move::Crawl && r.pathMoves.back() == Move::Walk)
				r.pathMoves.back() = Move::Crawl;
			continue;
		}
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
		r.pathSandGusts += t.fixture.type == SandGust ? 1 : 0;
		if (t.type == Gate && gateState(t) == GateState::Closed) {
			r.pathGates++;
			gateColours |= bitOf(t.attr);
		}
	}
	for (int c = 0; c < LOCK_COLOUR_COUNT; c++)
		r.keysNeeded += (gateColours >> c) & 1;
	r.pathBoss = (gateColours & bitOf(BOSS_LOCK)) != 0;

	r.pathMonsters = static_cast<int>(monstersNear(grid, r.path).size());

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
	bool layered = false, fixtures = false; // water, sand or shapes: the structure drawing too
	bool props = false, wallDecals = false;
	for (int row = LEVEL_HEIGHT - 1; row >= 0; row--) {
		std::string line, structureLine, wallLine, decorLine, decalLine;
		bool any = false;
		for (int col = 0; col < LEVEL_WIDTH; col++) {
			Tile t = grid.at(col, row);
			char c = tileGlyph(t);
			any = any || !isWall(t) || hasObject(t) || hasFixture(t);
			layered = layered || (t.structure != Structure::Wall && t.structure != Structure::Empty) || hasShape(t);
			fixtures = fixtures || hasFixture(t);
			props = props || hasPlacedProp(t.dressing);
			wallDecals = wallDecals || hasPlacedDecal(t.dressing);
			if (c == '.' && onPath[row * LEVEL_WIDTH + col] != 0)
				c = '*';
			line += c;
			structureLine += cellGlyph(t);
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
