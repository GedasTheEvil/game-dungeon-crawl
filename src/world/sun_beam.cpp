#include "sun_beam.h"
#include <algorithm>

namespace {
constexpr int CELLS = LEVEL_WIDTH * LEVEL_HEIGHT;

// The cell next to cell the way d goes; -1 out of the level.
int stepFrom(int cell, BeamDir d) {
	const int col = cell % LEVEL_WIDTH + beamDx(d);
	const int row = cell / LEVEL_WIDTH + beamDy(d);
	return LevelGrid::inBounds(col, row) ? row * LEVEL_WIDTH + col : -1;
}
} // namespace

std::vector<BeamStep> traceBeams(const Tile* cells, const BeamBlock& blocked) {
	std::vector<BeamStep> steps;
	for (int source = 0; source < CELLS; source++) {
		if (cells[source].type != LightShaft)
			continue;
		std::vector<char> seen(static_cast<size_t>(CELLS) * BEAM_DIRS, 0); // (cell, out) this beam went through
		int cell = source;
		BeamDir in = shaftDir(cells[source]);
		while (true) {
			BeamDir out = in;
			if (cells[cell].type == Mirror && cell != source) {
				out = mirrorDir(cells[cell]);
				if (out == opposite(in)) { // it faces the beam: caught
					steps.push_back({cell, in, out, true});
					break;
				}
			}
			const int next = stepFrom(cell, out);
			const bool ends = next < 0 || blocked(next) || seen[cell * BEAM_DIRS + static_cast<int>(out)] != 0;
			steps.push_back({cell, in, out, ends});
			if (ends)
				break;
			seen[cell * BEAM_DIRS + static_cast<int>(out)] = 1;
			cell = next;
			in = out;
		}
	}
	return steps;
}

std::vector<char> sunlitCells(const Tile* cells, const BeamBlock& blocked, const std::function<bool(int cell)>& free,
							  std::vector<std::vector<MirrorSetting>>* ways) {
	// A search over (cell, the way the beam comes in): a free mirror sends it any way but back.
	constexpr int NONE = -1;
	std::vector<char> lit(CELLS, 0);
	std::vector<int> parent(static_cast<size_t>(CELLS) * BEAM_DIRS,
							NONE - 1); // NONE - 1: not seen; NONE: a shaft's start
	std::vector<int> stack;
	auto state = [](int cell, BeamDir in) { return cell * BEAM_DIRS + static_cast<int>(in); };
	auto push = [&](int to, int from) {
		if (parent[to] == NONE - 1) {
			parent[to] = from;
			stack.push_back(to);
		}
	};
	for (int source = 0; source < CELLS; source++)
		if (cells[source].type == LightShaft)
			push(state(source, shaftDir(cells[source])), NONE);
	if (ways != nullptr)
		ways->assign(CELLS, {});
	while (!stack.empty()) {
		const int s = stack.back();
		stack.pop_back();
		const int cell = s / BEAM_DIRS;
		const BeamDir in = beamDir(s);
		if (lit[cell] == 0 && ways != nullptr) { // the mirrors on the way here, as it came
			std::vector<MirrorSetting> way;
			for (int at = s; parent[at] != NONE; at = parent[at]) {
				const int from = parent[at];
				if (cells[from / BEAM_DIRS].type == Mirror && free(from / BEAM_DIRS))
					way.push_back({from / BEAM_DIRS, beamDir(at)});
			}
			std::reverse(way.begin(), way.end());
			// A mirror passed twice: the later way from the first pass (the cells in between are lit some other way).
			for (size_t i = 0; i < way.size(); i++)
				for (size_t j = way.size(); j-- > i + 1;)
					if (way[j].cell == way[i].cell) {
						way.erase(way.begin() + static_cast<long>(i), way.begin() + static_cast<long>(j));
						break;
					}
			(*ways)[cell] = way;
		}
		lit[cell] = 1;
		const Tile& t = cells[cell];
		const bool mirror = t.type == Mirror && parent[s] != NONE; // a shaft's own cell sends it its way
		for (int k = 0; k < BEAM_DIRS; k++) {
			const BeamDir out = beamDir(k);
			if (mirror ? (out == opposite(in) || (!free(cell) && out != mirrorDir(t))) : out != in)
				continue;
			const int next = stepFrom(cell, out);
			if (next >= 0 && !blocked(next))
				push(state(next, out), s);
		}
	}
	return lit;
}
