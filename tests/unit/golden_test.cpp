// Golden dumps (docs/plan/scenarios-to-unit-tests.md technique 6): every campaign level's decorations and fires as
// text, diffed against tests/unit/golden/decor_lvlNN.txt. A change to the scatter shows up here as a diff. After a
// wanted change: GOLDEN_UPDATE=1 ./build/unit -tc="*golden*" writes the files anew; review them with git diff.
#include "../../external/doctest/doctest.h"
#include "../../src/world/campaign.h"
#include "../../src/world/decor.h"
#include "../../src/world/decor_scatter.h"
#include "../../src/world/level.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>

namespace {
const char* const FLAME_NAMES[] = {"brazier", "lamp", "torch"};

// One line per cell that is not solid rock: its surfaces, prop, decal, torch, ladder piece and flames.
std::string dumpDecor(const Tile* cells, const DecorLayout& layout) {
	std::ostringstream out;
	char buf[96];
	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++) {
			const int k = j * LEVEL_WIDTH + i;
			if (isWall(cells[k]))
				continue;
			const SurfaceCell& s = layout.surface[k];
			snprintf(buf, sizeof(buf), "%d,%d surface %d/%d/%d%s", i, j, s.wall, s.floor, s.ceiling,
					 s.wallMirror ? "m" : "");
			out << buf;
			if (const DecorCell& d = layout.decor[k]; d.type >= 0) {
				snprintf(buf, sizeof(buf), " prop %s%s %.3f", DECOR_NAMES[d.type], d.mirror ? "m" : "", d.offsetX);
				out << buf;
			}
			if (const DecalCell& d = layout.decal[k]; d.type >= 0) {
				snprintf(buf, sizeof(buf), " decal %d%s %.3f,%.3f", d.type, d.mirror ? "m" : "", d.x, d.y);
				out << buf;
			}
			if (layout.torch[k])
				out << " torch";
			if (const LadderCell& l = layout.ladder[k]; l.style >= 0) {
				snprintf(buf, sizeof(buf), " ladder %d/%d%s", l.style, l.piece, l.mirror ? "m" : "");
				out << buf;
			}
			Flame flames[MAX_CELL_FLAMES];
			const int n = flamesAt(layout, k, flames);
			for (int f = 0; f < n; f++) {
				snprintf(buf, sizeof(buf), " fire %s %.3f,%.3f,%.3f", FLAME_NAMES[static_cast<int>(flames[f].kind)],
						 flames[f].x, flames[f].y, flames[f].z);
				out << buf;
			}
			out << '\n';
		}
	return out.str();
}

std::string readFile(const std::string& path) {
	std::ifstream in(path);
	return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

TEST_CASE("golden: every campaign level scatters the decorations and fires it did") {
	const bool update = std::getenv("GOLDEN_UPDATE") != nullptr;
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		CAPTURE(n);
		LevelGrid grid;
		const std::string file = campaignLevelFile(n);
		REQUIRE(loadLevelFile(file.c_str(), grid).empty());
		auto layout = std::make_unique<DecorLayout>();
		scatterDecor(grid.cells, file.c_str(), n, *layout);
		const std::string dump = dumpDecor(grid.cells, *layout);
		char golden[64];
		snprintf(golden, sizeof(golden), "tests/unit/golden/decor_lvl%02d.txt", n);
		if (update)
			std::ofstream(golden) << dump;
		const std::string expected = readFile(golden);
		CHECK_MESSAGE(dump == expected, "differs from ", golden, " (GOLDEN_UPDATE=1 to write it anew)");
	}
}
