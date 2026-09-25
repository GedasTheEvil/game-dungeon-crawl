#include "level_check.h"
#include <algorithm>
#include <cstdio>
#include <queue>
#include <set>

namespace {
constexpr int MASKS = 1 << LOCK_COLOUR_COUNT; // lock colours opened so far
constexpr int CELLS = LEVEL_WIDTH * LEVEL_HEIGHT;
constexpr int STATES = CELLS * MASKS;
constexpr int NONE = -1;

// Path costs: the cheapest path avoids hazards when there is a way around them.
constexpr int COST_MOVE = 2;
constexpr int COST_JUMP = 4;
constexpr int COST_SPIKE = 6;
constexpr int COST_DEATH = 30;
constexpr int COST_ROCK_FALL = 6;
constexpr int COST_MONSTER = 2;
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
int bitOf(int colour) { return isLockColour(colour) ? 1 << (colour - 1) : 0; }

class Walker {
  public:
	explicit Walker(const LevelGrid& grid) : grid(grid) {}

	[[nodiscard]] bool solid(int col, int row, int mask) const {
		Tint t = grid.at(col, row);
		if (t.a == Gate && t.c != 1)
			return (mask & bitOf(t.b)) == 0;
		return isSolidTile(t);
	}

	[[nodiscard]] bool standable(int col, int row, int mask) const {
		if (!LevelGrid::inBounds(col, row) || solid(col, row, mask))
			return false;
		return grid.at(col, row).a == Ladder || solid(col, row - 1, mask);
	}

	// Picks up what the player touches in this cell.
	[[nodiscard]] int touch(int col, int row, int mask) const {
		Tint t = grid.at(col, row);
		return t.a == Key ? mask | bitOf(t.b) : mask;
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
		Tint t = grid.cells[cell];
		if (t.a == Spike)
			return COST_SPIKE;
		if (t.a == Death)
			return COST_DEATH;
		if (t.a == RockFall && t.c == 0)
			return COST_ROCK_FALL;
		if (t.a == Monster)
			return COST_MONSTER;
		return 0;
	}

	void successors(int state, std::vector<Edge>& out) const {
		out.clear();
		int cell = cellOf(state);
		int mask = maskOf(state);
		int col = cell % LEVEL_WIDTH;
		int row = cell / LEVEL_WIDTH;
		Tint here = grid.at(col, row);

		auto add = [&](int to, int base, Move move) {
			if (to != NONE && to != state)
				out.push_back({to, base + hazardCost(cellOf(to)), move});
		};

		if (here.a == Lever && isLockColour(here.b))
			add(stateOf(col, row, mask | bitOf(here.b)), COST_MOVE, Move::Pull);

		for (int dir : {-1, 1}) {
			int next = col + dir;
			if (!LevelGrid::inBounds(next, row) || solid(next, row, mask))
				continue;
			int fallen = 0;
			int to = settle(next, row, mask, fallen);
			add(to, COST_MOVE + fallen, fallen > 0 ? Move::Drop : Move::Walk);

			// Over a one-cell gap in the floor, from a floor (not from a ladder).
			bool gap = fallen > 0 && here.a != Ladder && solid(col, row - 1, mask);
			int far = col + 2 * dir;
			if (gap && LevelGrid::inBounds(far, row) && standable(far, row, mask))
				add(stateOf(far, row, touch(far, row, touch(next, row, mask))), COST_JUMP, Move::Jump);
		}

		if (here.a == Ladder) {
			for (int dy : {-1, 1})
				if (grid.at(col, row + dy).a == Ladder)
					add(stateOf(col, row + dy, touch(col, row + dy, mask)), COST_MOVE, Move::Climb);
		}
	}

  private:
	const LevelGrid& grid;
};

bool isGoal(const Tint& t) { return t.a == Ankh || (t.a == Door && t.b == GateExit); }

void countContent(const LevelGrid& grid, LevelReport& r) {
	int minCol = LEVEL_WIDTH, maxCol = -1, minRow = LEVEL_HEIGHT, maxRow = -1;
	for (int row = 0; row < LEVEL_HEIGHT; row++)
		for (int col = 0; col < LEVEL_WIDTH; col++) {
			Tint t = grid.at(col, row);
			if (t.a == Wall)
				continue;
			r.openCells++;
			minCol = std::min(minCol, col);
			maxCol = std::max(maxCol, col);
			minRow = std::min(minRow, row);
			maxRow = std::max(maxRow, row);
			switch (t.a) {
			case Door:
				if (t.b == GateEntrance) {
					r.entrances++;
					r.start = {col, row}; // the game takes the last one in file order
				} else if (t.b == GateExit)
					r.exits++;
				else if (t.b == GateRiddle)
					r.riddles++;
				break;
			case Ankh:
				r.exits++;
				r.finale = true;
				break;
			case Monster:
				r.monsters[t.b >= 1 && t.b <= 6 ? t.b : 0]++;
				r.monsterCount++;
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
		Tint t = grid.cells[cell];
		if ((t.a == Key || t.a == Gate || t.a == Lever) && !isLockColour(t.b)) {
			r.errors.push_back("bad lock colour " + std::to_string(t.b) + " at " + at(cell));
			continue;
		}
		if (t.a == Key)
			keyMask |= bitOf(t.b);
		if (t.a == Lever)
			leverMask |= bitOf(t.b);
		if (t.a == Gate && t.c != 1)
			gateMask |= bitOf(t.b);
	}
	for (int c = 1; c <= LOCK_COLOUR_COUNT; c++)
		if ((gateMask & bitOf(c)) != 0 && ((keyMask | leverMask) & bitOf(c)) == 0)
			r.warnings.push_back(std::string("no key or lever opens the ") + LOCK_COLOUR_NAMES[c - 1] + " gates");
}

float difficultyScore(const LevelReport& r, const LevelGrid& grid) {
	float score = 0.04f * static_cast<float>(r.pathLength);
	score += 1.0f * static_cast<float>(r.pathSpikes) + 4.f * static_cast<float>(r.pathDeathTraps);
	score += 1.5f * static_cast<float>(r.pathRockFalls) + 1.2f * static_cast<float>(r.pathJumps);
	score += 0.8f * static_cast<float>(r.pathGates);

	// Monsters near the path count fully, the rest of the level a little (the player may go looking for loot).
	std::set<int> near;
	for (const CellPos& p : r.path)
		for (int dx = -MONSTER_REACH; dx <= MONSTER_REACH; dx++) {
			Tint t = grid.at(p.col + dx, p.row);
			if (t.a == Monster)
				near.insert(p.row * LEVEL_WIDTH + p.col + dx);
		}
	for (int cell = 0; cell < CELLS; cell++) {
		Tint t = grid.cells[cell];
		if (t.a == Monster)
			score += monsterThreat(t.b) * (near.count(cell) != 0 ? 1.f : 0.25f);
	}

	// Jumps over spike pits hurt when missed.
	for (size_t i = 1; i < r.path.size(); i++) {
		const CellPos& a = r.path[i - 1];
		const CellPos& b = r.path[i];
		if (std::abs(b.col - a.col) == 2 && grid.at((a.col + b.col) / 2, a.row - 1).a == Death)
			score += 1.f;
	}

	// Treasure on the way is a little help.
	score -= 0.2f * static_cast<float>(r.reachableTreasures);
	return std::max(0.f, score);
}
} // namespace

float monsterThreat(int type) {
	switch (type) {
	case MonsterScarab:
		return 1.0f;
	case MonsterWorm:
		return 2.5f;
	case MonsterPlant:
		return 1.5f; // does not move
	case MonsterAnubis:
		return 8.f;
	case MonsterRat:
		return 0.6f;
	case MonsterGiantRat:
		return 3.f;
	default:
		return 2.f;
	}
}

LevelReport checkLevel(const LevelGrid& grid) {
	LevelReport r;
	countContent(grid, r);
	checkLocks(grid, r);

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
	for (int cell = 0; cell < CELLS; cell++) {
		if (cellReached[cell] == 0)
			continue;
		r.reachableCells++;
		if (grid.cells[cell].a == Treasure)
			r.reachableTreasures++;
	}
	for (int cell = 0; cell < CELLS; cell++) {
		Tint t = grid.cells[cell];
		if (cellReached[cell] == 0 && (t.a == Key || t.a == Lever))
			r.warnings.push_back(std::string(t.a == Key ? "key" : "lever") + " out of reach at " + at(cell));
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
		if (expanded[s] != 0 && canFinish[s] == 0 && grid.cells[cellOf(s)].a != Death && softCell[cellOf(s)] == 0) {
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
		Tint t = grid.cells[cell];
		if (i > 0) {
			r.pathLength++;
			Move m = prevMove[states[i]];
			r.pathJumps += m == Move::Jump ? 1 : 0;
			r.pathDrops += m == Move::Drop ? 1 : 0;
			r.pathClimb += m == Move::Climb ? 1 : 0;
		}
		r.pathSpikes += t.a == Spike ? 1 : 0;
		r.pathDeathTraps += t.a == Death ? 1 : 0;
		r.pathRockFalls += t.a == RockFall && t.c == 0 ? 1 : 0;
		if (t.a == Gate && t.c != 1) {
			r.pathGates++;
			gateColours |= bitOf(t.b);
		}
	}
	for (int c = 0; c < LOCK_COLOUR_COUNT; c++)
		r.keysNeeded += (gateColours >> c) & 1;

	std::set<int> monstersNear;
	for (const CellPos& p : r.path)
		for (int dx = -MONSTER_REACH; dx <= MONSTER_REACH; dx++)
			if (grid.at(p.col + dx, p.row).a == Monster)
				monstersNear.insert(p.row * LEVEL_WIDTH + p.col + dx);
	r.pathMonsters = static_cast<int>(monstersNear.size());

	if (r.pathDeathTraps > 0)
		r.warnings.emplace_back("the only way to the exit walks over a death trap");

	r.difficulty = difficultyScore(r, grid);
	r.valid = r.errors.empty();
	return r;
}

std::string renderLevel(const LevelGrid& grid, const LevelReport* report) {
	static const char MONSTER_CHARS[] = "mswpntT";
	static const char KEY_CHARS[] = "rbgy";
	static const char GATE_CHARS[] = "RBGY";
	std::vector<char> onPath(CELLS, 0);
	if (report != nullptr)
		for (const CellPos& p : report->path)
			onPath[p.row * LEVEL_WIDTH + p.col] = 1;

	std::string out;
	for (int row = LEVEL_HEIGHT - 1; row >= 0; row--) {
		std::string line;
		bool any = false;
		for (int col = 0; col < LEVEL_WIDTH; col++) {
			Tint t = grid.at(col, row);
			char c = '?';
			switch (t.a) {
			case Wall:
				c = '#';
				break;
			case Empty:
			case Area3D:
				c = '.';
				break;
			case Door:
				c = t.b == GateEntrance ? 'S' : t.b == GateExit ? 'E' : t.b == GateRiddle ? '?' : 'D';
				break;
			case Death:
				c = 'X';
				break;
			case Monster:
				c = MONSTER_CHARS[t.b >= 1 && t.b <= 6 ? t.b : 0];
				break;
			case Spike:
				c = '^';
				break;
			case Ladder:
				c = 'H';
				break;
			case Treasure:
				c = '$';
				break;
			case Ankh:
				c = 'A';
				break;
			case Key:
				c = isLockColour(t.b) ? KEY_CHARS[t.b - 1] : 'k';
				break;
			case Gate:
				c = isLockColour(t.b) ? GATE_CHARS[t.b - 1] : 'Q';
				break;
			case Lever:
				c = '/';
				break;
			case RockFall:
				c = 'v';
				break;
			default:
				break;
			}
			if (t.a != Wall)
				any = true;
			if (c == '.' && onPath[row * LEVEL_WIDTH + col] != 0)
				c = '*';
			line += c;
		}
		if (any) {
			char head[8];
			snprintf(head, sizeof(head), "%2d ", row);
			out += head + line + '\n';
		}
	}
	return out;
}
