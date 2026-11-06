#include "level_check.h"
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
#include <algorithm>
#include <cstdio>
#include <map>
#include <numeric>
#include <optional>
#include <queue>
#include <set>

namespace levelcheck {
bool isGoal(const Tile& t) { return t.type == Ankh || (t.type == Door && t.attr == GateExit); }

std::string at(int cell) {
	char buf[32];
	snprintf(buf, sizeof(buf), "(col %d, row %d)", cell % LEVEL_WIDTH, cell / LEVEL_WIDTH);
	return buf;
}
} // namespace levelcheck

using namespace levelcheck;

namespace {

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
		if (isCeilingTrap(t.fixture.type) && !jumpReachesUnderside(t)) // as the loader (plan ceiling-spikes)
			r.errors.push_back(
				std::string(tileDef(t.fixture.type).name) + " at " + at(cell) +
				": no low ceiling in the cell's shape a jump reaches"
			);
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
		const bool step = inCell == Move::Step || inCell == Move::SpikedStep;
		r.pathSteps += i > 0 && step ? 1 : 0;
		r.pathCeilingSpikes += i > 0 && inCell == Move::SpikedStep ? 1 : 0;
		r.pathCrawls += i > 0 && inCell == Move::Crawl ? 1 : 0;
		std::optional<PathTakeOff> takeOff;
		if (i > 0 && (step || inCell == Move::Jump))
			takeOff = walker.takeOff(states[i - 1], states[i]);
		if (i > 0 && cell == r.path.back().row * LEVEL_WIDTH + r.path.back().col &&
			(inCell == Move::Walk || inCell == Move::Crawl || inCell == Move::Drop || step)) {
			if (takeOff)
				r.pathTakeOffs.back().push_back(*takeOff);
			if (inCell == Move::Crawl && r.pathMoves.back() == Move::Walk)
				r.pathMoves.back() = Move::Crawl;
			continue;
		}
		r.path.push_back({cell % LEVEL_WIDTH, cell / LEVEL_WIDTH});
		r.pathMoves.push_back(prevMove[states[i]]);
		r.pathTakeOffs.emplace_back();
		if (takeOff)
			r.pathTakeOffs.back().push_back(*takeOff);
		r.pathRise.push_back(
			i > 0 ? walker.places().y(nodeOf(states[i])) - walker.places().y(nodeOf(states[i - 1])) : 0
		);
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
	if (r.pathCeilingSpikes > 0) // a jump into them costs a walk over the floor kind (plan ceiling-spikes)
		r.warnings.emplace_back("the only way to the exit jumps into ceiling spikes");
	if (r.pathDartPlates > 0 && !antidote)
		r.warnings.emplace_back("a dart trap on the path but no antidote in reach");

	r.difficulty = difficultyScore(r, grid);
	r.valid = r.errors.empty();
	return r;
}

char propGlyph(const Tile& t) {
	const int prop = t.dressing.prop;
	if (prop >= 0)
		return decorGlyph(prop);
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

char containerGlyph(const Tile& t) {
	const Dressing& d = t.dressing;
	if (!hasPlacedContainer(d))
		return isSolidStructure(t.structure) ? '#' : '.';
	if (d.lid == CoffinLid::Plain)
		return PLAIN_COFFIN_GLYPH;
	return d.lid == CoffinLid::Inscribed ? INSCRIBED_COFFIN_GLYPH : CONTAINER_GLYPHS[d.container];
}

std::string renderLevel(const LevelGrid& grid, const LevelReport* report) {
	std::vector<char> onPath(CELLS, 0);
	if (report != nullptr)
		for (const CellPos& p : report->path)
			onPath[p.row * LEVEL_WIDTH + p.col] = 1;

	std::string objects, structure, wall, decor, decals, containers;
	bool layered = false, fixtures = false; // water, sand or shapes: the structure drawing too
	bool props = false, wallDecals = false, looks = false;
	for (int row = LEVEL_HEIGHT - 1; row >= 0; row--) {
		std::string line, structureLine, wallLine, decorLine, decalLine, containerLine;
		bool any = false;
		for (int col = 0; col < LEVEL_WIDTH; col++) {
			Tile t = grid.at(col, row);
			char c = tileGlyph(t);
			any = any || !isWall(t) || hasObject(t) || hasFixture(t);
			layered = layered || (t.structure != Structure::Wall && t.structure != Structure::Empty) || hasShape(t);
			fixtures = fixtures || hasFixture(t);
			props = props || hasPlacedProp(t.dressing);
			wallDecals = wallDecals || hasPlacedDecal(t.dressing);
			looks = looks || hasPlacedContainer(t.dressing);
			if (c == '.' && onPath[row * LEVEL_WIDTH + col] != 0)
				c = '*';
			line += c;
			structureLine += cellGlyph(t);
			wallLine += hasFixture(t) ? tileGlyph(fixtureAsTile(t.fixture)) : isSolidStructure(t.structure) ? '#' : '.';
			decorLine += propGlyph(t);
			decalLine += decalGlyph(t);
			containerLine += containerGlyph(t);
		}
		if (any) {
			char head[8];
			snprintf(head, sizeof(head), "%2d ", row);
			objects += head + line + '\n';
			structure += head + structureLine + '\n';
			wall += head + wallLine + '\n';
			decor += head + decorLine + '\n';
			decals += head + decalLine + '\n';
			containers += head + containerLine + '\n';
		}
	}
	return objects + (layered ? "structure\n" + structure : "") + (fixtures ? "wall\n" + wall : "") +
		   (props ? "decor\n" + decor : "") + (wallDecals ? "decals\n" + decals : "") +
		   (looks ? "containers\n" + containers : "");
}
