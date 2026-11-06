// The checker's path model: the walker's moves (plan split-large-sources, moved from level_check.cpp).
#include "level_walker.h"

namespace levelcheck {
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

template <typename Add> void Walker::walk(int x, int y, int dir, int mask, Add& add) const {
	const int next = x + dir;
	if (open(next, y, mask)) {
		int fallen = 0;
		const int to = settle(next, y, mask, fallen);
		// Quicksand slows the walk to nothing (QUICKSAND_STUCK_MS) before the player has crossed a cell of it: no
		// walk on from quicksand into more of it, only the jump out (plan paths-failures).
		if (to == NONE || (fallen == 0 && inQuicksandAt(x, y) && inQuicksandAt(next, y)))
			return;
		const bool crawling = fallen == 0 && crawl(next, y, mask);
		if (crawling && intoCrawlOffLadder(x, y, mask))
			return;
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
	// The arc's head reaches into the sub-cell over STEP_ROOM: ceiling spikes there are jumped into.
	const bool spiked = spikedAt(x, y + STEP_ROOM) || spikedAt(next, y + STEP_ROOM);
	add(settle(next, y + 1, mask, fallen),
		COST_STEP + (spiked ? COST_CEILING_SPIKES : 0),
		spiked ? Move::SpikedStep : Move::Step);
}

template <typename Add> void Walker::jump(int x, int y, int dir, int mask, Add& add) const {
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

const std::vector<Walker::Turn>& Walker::turns(int state) const {
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

void Walker::successors(int state, std::vector<Edge>& out, bool turning) const {
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
		const bool step = move == Move::Step || move == Move::SpikedStep;
		const bool along = move == Move::Walk || move == Move::Drop || move == Move::Jump || move == Move::Crawl ||
						   step;
		const bool inCell = along && cellOf(to) == cell;
		const int walk = inCell && move != Move::Jump && !step ? COST_MOVE : 0;
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
				if (stands(x + dir, y + q, mask) &&
					!(crawl(x + dir, y + q, mask) && intoCrawlOffLadder(x, y + q, mask))) {
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

std::optional<PathTakeOff> Walker::takeOff(int from, int to) const {
	const int node = nodeOf(from);
	const int mask = maskOf(from);
	for (int dir : {-1, 1}) {
		const int x = nodes.x(node, dir);
		const int y = nodes.y(node);
		std::optional<Move> found;
		auto add = [&](int target, int, Move move) {
			if (target == to && (move == Move::Step || move == Move::SpikedStep || move == Move::Jump))
				found = move;
		};
		walk(x, y, dir, mask, add);
		jump(x, y, dir, mask, add);
		if (!found)
			continue;
		PathTakeOff t{x, y, nodes.x(nodeOf(to), dir), dir, 0};
		for (int k = 1; *found != Move::Jump && k <= JUMP_SUBCELLS && t.face == 0; k++)
			t.face = open(x + dir * k, y, mask) ? 0 : k;
		return t;
	}
	return std::nullopt;
}

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

} // namespace levelcheck
