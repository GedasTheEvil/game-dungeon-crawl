#include "level_check.h"
#include "movement.h"
#include "items.h"
#include "monster_kinds.h"
#include "tile_defs.h"
#include <algorithm>
#include <cstdio>
#include <map>
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
constexpr int COST_SPIKE = 6;
constexpr int COST_DEATH = 30;
constexpr int COST_ROCK_FALL = 15;
constexpr int COST_DART_PLATE = 8;
constexpr int COST_MONSTER = 2;
constexpr int COST_TELEPORT = 4;
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
	explicit Walker(const LevelGrid& grid, bool teleports = true)
		: grid(grid), pairs(grid.cells), teleports(teleports) {}

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
		if (t.type == Spike)
			return COST_SPIKE;
		if (t.type == Death)
			return COST_DEATH;
		if (t.type == RockFall && rockState(t) == RockState::Armed)
			return COST_ROCK_FALL;
		if (t.type == DartPlate)
			return COST_DART_PLATE;
		if (t.type == MonsterSpawn)
			return COST_MONSTER;
		return 0;
	}

	void successors(int state, std::vector<Edge>& out) const {
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

		if (here.type == Lever && isLockColour(here.attr) && !leverPulled(here)) // a pulled lever opens nothing
			add(stateOf(col, row, mask | bitOf(here.attr)), COST_MOVE, Move::Pull);

		if (teleports && isTeleporter(here)) {
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

			// Over one cell, a gap in the floor or a trap on it, from a floor (a ladder's foot too), not out of half
			// water. A jump from mid-ladder is not modelled: its reach depends on the walk key held during the jump.
			// A rock fall is not jumped: the player passes through its cell either way, and it drops all the same.
			int nextType = grid.at(next, row).type;
			bool overHazard = fallen == 0 && (nextType == Spike || nextType == Death);
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
				else if (t.attr == GateTeleport)
					r.teleporters++;
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
				break;
			case DartPlate:
				r.dartPlates++;
				break;
			case Treasure:
				r.treasures++;
				break;
			case Key:
				r.keys++;
				break;
			case Gate:
				r.gates++;
				break;
			case Lever:
				r.levers++;
				break;
			default:
				break;
			}
		}
	if (maxCol >= 0) {
		r.boundsWidth = maxCol - minCol + 1;
		r.boundsHeight = maxRow - minRow + 1;
	}
}

std::string at(int cell) {
	char buf[32];
	snprintf(buf, sizeof(buf), "(col %d, row %d)", cell % LEVEL_WIDTH, cell / LEVEL_WIDTH);
	return buf;
}

void checkLocks(const LevelGrid& grid, LevelReport& r) {
	int keyMask = 0, leverMask = 0, gateMask = 0;
	for (int cell = 0; cell < CELLS; cell++) {
		Tile t = grid.cells[cell];
		if (((t.type == Key || t.type == Lever) && !isLockColour(t.attr)) ||
			(t.type == Gate && !isGateColour(t.attr))) {
			r.errors.push_back("bad lock colour " + std::to_string(t.attr) + " at " + at(cell));
			continue;
		}
		if (t.type == Key)
			keyMask |= bitOf(t.attr);
		if (t.type == Lever)
			leverMask |= bitOf(t.attr);
		if (t.type == Gate && gateState(t) == GateState::Closed)
			gateMask |= bitOf(t.attr);
	}
	for (int c = 1; c <= LOCK_COLOUR_COUNT; c++)
		if ((gateMask & bitOf(c)) != 0 && ((keyMask | leverMask) & bitOf(c)) == 0)
			r.warnings.push_back(std::string("no key or lever opens the ") + lockColour(c).name + " gates");

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
			r.warnings.push_back(std::string(tileDef(t.type).name) + " in " + structureDef(t.structure).name + " at " +
								 at(cell));
	}
}

// A dart plate shoots from the nearer wall on its row within DART_RANGE (Dungeon::pressPlate): with none it only
// clicks.
void checkDartPlates(const LevelGrid& grid, LevelReport& r) {
	for (int cell = 0; cell < CELLS; cell++) {
		if (grid.cells[cell].type != DartPlate)
			continue;
		const int col = cell % LEVEL_WIDTH, row = cell / LEVEL_WIDTH;
		bool wall = false;
		for (int k = 1; k <= DART_RANGE && !wall; k++)
			wall = isSolidTile(grid.at(col - k, row)) || isSolidTile(grid.at(col + k, row));
		if (!wall)
			r.warnings.push_back("dart plate at " + at(cell) + " has no wall within " + std::to_string(DART_RANGE) +
								 " cells on its row to shoot from");
	}
}

// Half water stands on deep water or a wall, deep water only under water; a ladder starts in the water and goes up,
// none goes down into it; a crocodile lives in or next to the water.
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
			r.warnings.push_back(std::to_string(cells.size()) + " teleporter(s) with pair id " + std::to_string(id) +
								 ", e.g. at " + at(cells[0]) + ": a pair needs exactly two");
}

float difficultyScore(const LevelReport& r, const LevelGrid& grid) {
	float score = 0.04f * static_cast<float>(r.pathLength);
	score += 1.0f * static_cast<float>(r.pathSpikes) + 4.f * static_cast<float>(r.pathDeathTraps);
	score += 3.f * static_cast<float>(r.pathRockFalls) + 1.2f * static_cast<float>(r.pathJumps);
	score += 2.f * static_cast<float>(r.pathDartPlates);
	score += 0.8f * static_cast<float>(r.pathGates);

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
	return kind != nullptr ? kind->threat : UNKNOWN_MONSTER_THREAT;
}

LevelReport checkLevel(const LevelGrid& grid) {
	LevelReport r;
	countContent(grid, r);
	checkLocks(grid, r);
	checkObjectsInRock(grid, r);
	checkWater(grid, r);
	checkDeadGates(grid, r);
	checkTeleporters(grid, r);
	checkDartPlates(grid, r);

	if (r.entrances == 0)
		r.errors.emplace_back("no entrance (Door with attribute 1)");
	if (r.entrances > 1)
		r.warnings.push_back(std::to_string(r.entrances) + " entrances, the game starts at the last one " +
							 at(r.start.row * LEVEL_WIDTH + r.start.col));
	if (r.exits == 0)
		r.errors.emplace_back("no exit (Door with attribute 2) and no ankh");
	if (r.entrances == 0) {
		r.valid = false;
		return r;
	}

	Walker walker(grid);
	int fallen = 0;
	int startState = walker.settle(r.start.col, r.start.row, 0, fallen);
	if (startState == NONE) {
		r.errors.emplace_back("the entrance has no floor below it");
		return r;
	}

	// Dijkstra over (cell, opened colours), keeping the edges for the backward pass.
	checkBossRoom(grid, startState, r);

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
		if (t.type == Treasure) {
			r.reachableTreasures++;
			antidote = antidote || itemFromFile(t.attr, t.value) == ItemKind::Antidote;
		}
	}
	if (poisoners && !antidote)
		r.warnings.emplace_back("poisoners but no antidote in reach");
	for (int cell = 0; cell < CELLS; cell++) {
		Tile t = grid.cells[cell];
		if (cellReached[cell] == 0 && (t.type == Key || t.type == Lever))
			r.warnings.push_back(std::string(t.type == Key ? "key" : "lever") + " out of reach at " + at(cell));
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
		r.warnings.push_back(std::to_string(r.softlockCells) +
							 " cell(s) the player can reach but not leave for the exit, e.g. " + at(example));

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
		}
		r.pathSpikes += t.type == Spike ? 1 : 0;
		r.pathDeathTraps += t.type == Death ? 1 : 0;
		r.pathRockFalls += t.type == RockFall && rockState(t) == RockState::Armed ? 1 : 0;
		r.pathDartPlates += t.type == DartPlate ? 1 : 0;
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
	if (r.pathDartPlates > 0 && !antidote)
		r.warnings.emplace_back("a dart trap on the path but no antidote in reach");

	r.difficulty = difficultyScore(r, grid);
	r.valid = r.errors.empty();
	return r;
}

std::string renderLevel(const LevelGrid& grid, const LevelReport* report) {
	std::vector<char> onPath(CELLS, 0);
	if (report != nullptr)
		for (const CellPos& p : report->path)
			onPath[p.row * LEVEL_WIDTH + p.col] = 1;

	std::string objects, structure;
	bool water = false;
	for (int row = LEVEL_HEIGHT - 1; row >= 0; row--) {
		std::string line, structureLine;
		bool any = false;
		for (int col = 0; col < LEVEL_WIDTH; col++) {
			Tile t = grid.at(col, row);
			char c = tileGlyph(t);
			any = any || !isWall(t) || hasObject(t);
			water = water || t.structure == Structure::HalfWater || t.structure == Structure::DeepWater;
			if (c == '.' && onPath[row * LEVEL_WIDTH + col] != 0)
				c = '*';
			line += c;
			structureLine += structureGlyph(t.structure);
		}
		if (any) {
			char head[8];
			snprintf(head, sizeof(head), "%2d ", row);
			objects += head + line + '\n';
			structure += head + structureLine + '\n';
		}
	}
	return water ? objects + "structure\n" + structure : objects;
}
