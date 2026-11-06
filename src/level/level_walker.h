// The checker's path model (plan split-large-sources, moved from level_check.cpp): the level in sub-cells, its
// standing places (Nodes) and the player's moves between them (Walker). Internal to the checker.
#ifndef LEVEL_WALKER_H
#define LEVEL_WALKER_H

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
#include <optional>
#include <queue>
#include <set>

namespace levelcheck {
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
constexpr int COST_CEILING_SPIKES = 30; // a jump into them costs a walk over the floor kind (plan ceiling-spikes)
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

inline int nodeOf(int state) { return state / MASKS; }
inline int maskOf(int state) { return state % MASKS; }
inline int bitOf(int colour) { return isGateColour(colour) ? 1 << (colour - 1) : 0; }
inline int subCell(int x, int y) { return (y / SUBCELLS) * LEVEL_WIDTH + x / SUBCELLS; }
inline bool subInBounds(int x, int y) { return x >= 0 && x < SUB_WIDTH && y >= 0 && y < SUB_HEIGHT; }

// A fine cell (plan subcell-checker) is walked by its sub-cells: a shaped one, or one over a shaped cell whose top row
// is solid in part (the player stands on the solid sub-cells and falls by the open ones). A ladder's cell is not (a
// ladder in a shape is warned about, checkShapes).
bool fineCell(const LevelGrid& grid, int cell);

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
	[[nodiscard]] const std::vector<Turn>& turns(int state) const;

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

	void successors(int state, std::vector<Edge>& out, bool turning = true) const;

	// The jump from state `from` that lands in state `to` (a Step, SpikedStep or Jump edge), for the script.
	[[nodiscard]] std::optional<PathTakeOff> takeOff(int from, int to) const;

  private:
	const LevelGrid& grid;
	Nodes nodes;
	TeleportPairs pairs;
	bool teleports;
	bool sealed;
	mutable std::map<int, std::vector<Turn>> turnCache; // by state

	// One sub-cell along the row from (x, y): a walk (a crawl into a crawl space, a drop off an edge), or a jump up
	// onto a quarter step (no higher: Jump::ARC, movement.h).
	template <typename Add> void walk(int x, int y, int dir, int mask, Add& add) const;

	// Into a crawl space from sub-cell (x, y) of a ladder's cell (plan subcell-checker-mismatches): there the down key
	// climbs, it does not crouch (Dungeon::updateCrouch), and the body (Player::HalfWidth, under a sub-cell either side
	// of the middle) meets the crawl space's ceiling while the middle is still in the ladder's cell. A sub-cell of head
	// room between the ladder and the crawl space is enough.
	[[nodiscard]] bool intoCrawlOffLadder(int x, int y, int mask) const {
		return grid.cells[subCell(x, y)].type == Ladder && !crawl(x, y, mask);
	}

	// Sub-cell (x, y) is the underside of a ceiling trap's cell (plan ceiling-spikes).
	[[nodiscard]] bool spikedAt(int x, int y) const {
		if (!subInBounds(x, y))
			return false;
		const Tile& t = grid.cells[subCell(x, y)];
		return isCeilingTrap(t.fixture.type) && (ceilingUndersides(t) & subcellBit(x % SUBCELLS, y % SUBCELLS)) != 0;
	}

	// Up to JUMP_SUBCELLS along the row, from a floor: over a gap or a trap (jumpedOver), landing on a floor no higher
	// than the take-off and no lower than its cell's bottom (a jump off a step lands on the floor beside it); or onto
	// a quarter step past the next sub-cell (over water, say: the arc's feet are over a quarter from about a third of a
	// tile out to past a cell, Jump::ARC). A rock fall is not jumped: the player passes through its cell either way,
	// and it drops all the same. Never into quicksand: the player would land deep in it, too far in to walk out (plan
	// paths-failures).
	template <typename Add> void jump(int x, int y, int dir, int mask, Add& add) const;
};

// States the walker reaches from the start states.
std::vector<char> reachableStates(const Walker& walker, const std::vector<int>& startStates);
std::vector<char> cellsOf(const Walker& walker, const std::vector<char>& states);
// Cells the walker reaches from the start state (any opened colours).
std::vector<char> reachableCells(const Walker& walker, int startState);
} // namespace levelcheck

#endif
