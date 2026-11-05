// levelcheck: validates level files and ranks them by difficulty. See docs/levels.md.
//   levelcheck [--map] [--quiet] [--script DIR] FILE...
// --script DIR writes DIR/<level name>.txt per valid level: a scenario (docs/testing.md) that plays the path the
// check found in the real game, in god mode, and expects the level to be finished at the end. It kills the boss
// (killboss) where the path reaches it.
// --legend prints the glyphs of the --map drawings and the ASCII level sources, the props and the decals.
// Exit code: 0 all valid, 1 at least one level cannot be finished, 2 usage / unreadable file.

#include "../../src/core/gameplay_config.h"
#include "../../src/world/decor.h"
#include "../../src/world/inscriptions.h"
#include "../../src/world/level.h"
#include "../../src/world/level_check.h"
#include "../../src/world/path_script.h"
#include "../../src/world/monster_kinds.h"
#include "../../src/world/movement.h"
#include "../../src/world/tile_defs.h"
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "../../src/core/paths.h"

namespace {
struct Entry {
	std::string path;
	LevelGrid grid;
	LevelReport report;
};

void printReport(const Entry& e, bool map) {
	const LevelReport& r = e.report;
	printf("== %s: %s\n", e.path.c_str(), r.valid ? "OK" : "INVALID");
	for (const std::string& m : r.errors)
		printf("   error: %s\n", m.c_str());
	for (const std::string& m : r.warnings)
		printf("   warning: %s\n", m.c_str());
	printf(
		"   start (col %d, row %d), %s\n",
		r.start.col,
		r.start.row,
		r.finale ? "goal: ankh (finale)" : "goal: exit"
	);
	printf(
		"   size: %d open cells, %d reachable, bounds %dx%d\n",
		r.openCells,
		r.reachableCells,
		r.boundsWidth,
		r.boundsHeight
	);
	std::string perType;
	for (int type = 1; type <= MONSTER_TYPE_MAX; type++)
		if (const MonsterKind* kind = monsterKind(type))
			perType += std::string(perType.empty() ? "" : ", ") + kind->label + " " + std::to_string(r.monsters[type]);
	printf(
		"   content: %d monsters (%s), %d spikes, %d death traps, %d rock falls (%d sand), %d dart plates, %d wall "
		"spikes, %d fire vents, %d poison drips, %d treasures (%d reachable, %d trapped), %d keys, %d gates, %d "
		"levers, %d riddles, "
		"%d teleporters "
		"(%d name "
		"gates), %d inscriptions, %d sand cells (%d quicksand), %d sand gusts, %d light shafts, %d mirrors, %d sun "
		"discs\n",
		r.monsterCount,
		perType.c_str(),
		r.spikes,
		r.deathTraps,
		r.rockFalls,
		r.sandFalls,
		r.dartPlates,
		r.wallSpikes,
		r.fireVents,
		r.poisonDrips,
		r.treasures,
		r.reachableTreasures,
		r.trappedTreasures,
		r.keys,
		r.gates,
		r.levers,
		r.riddles,
		r.teleporters,
		r.nameGates,
		r.inscriptions,
		r.sandCells,
		r.quicksandCells,
		r.sandGusts,
		r.lightShafts,
		r.mirrors,
		r.sunDiscs
	);
	if (r.valid || !r.path.empty())
		printf(
			"   path: %d moves, %d jumps, %d drops, %d ladder steps, %d spikes, %d death traps, %d rock falls, %d sand "
			"falls, %d dart plates, %d wall spikes, %d fire vents, %d poison drips, %d gates (%d colours), %d "
			"teleports, %d monsters "
			"near, %d sand, %d "
			"quicksand, %d sand gusts, %d mirror turns\n",
			r.pathLength,
			r.pathJumps,
			r.pathDrops,
			r.pathClimb,
			r.pathSpikes,
			r.pathDeathTraps,
			r.pathRockFalls,
			r.pathSandFalls,
			r.pathDartPlates,
			r.pathWallSpikes,
			r.pathFireVents,
			r.pathPoisonDrips,
			r.pathGates,
			r.keysNeeded,
			r.pathTeleports,
			r.pathMonsters,
			r.pathSand,
			r.pathQuicksand,
			r.pathSandGusts,
			r.pathTurns
		);
	if (r.pathSteps + r.pathCrawls > 0) // plan subcell-checker
		printf(
			"   sub-cells: %d quarter steps (%d into ceiling spikes), %d crawled\n",
			r.pathSteps,
			r.pathCeilingSpikes,
			r.pathCrawls
		);
	printf("   difficulty: %.1f\n", r.difficulty);
	if (map)
		printf("%s", renderLevel(e.grid, &r).c_str());
}
// The campaign level number of a file named lvlN, 0 for any other level.
int campaignNumber(const std::string& path) {
	std::string name = std::filesystem::path(path).filename().string();
	if (name.size() < 4 || name.compare(0, 3, "lvl") != 0 ||
		name.find_first_not_of("0123456789", 3) != std::string::npos)
		return 0;
	return std::stoi(name.substr(3));
}

// Inscriptions need a text in content/inscriptions/*.txt (the repo root's, from the working directory when the tool
// is not in bin/), and each text stands in one place only among the files checked.
void checkInscriptions(std::vector<Entry>& entries) {
	InscriptionBook book;
	const std::string root = Paths::rootDir();
	const std::string dir = (root.empty() ? "" : root + "/") + CONTENT_DIR "inscriptions";
	bool texts = std::filesystem::is_directory(dir);
	if (texts)
		for (const std::string& m : book.Load(dir))
			fprintf(stderr, "%s\n", m.c_str());
	std::vector<std::string> usedIn(INSCRIPTION_MAX + 1);
	for (Entry& e : entries) {
		for (int cell = 0; cell < LEVEL_WIDTH * LEVEL_HEIGHT; cell++) {
			const Fixture& t = e.grid.cells[cell].fixture;
			if (t.type != Inscription || !isInscriptionId(t.attr))
				continue;
			const std::string where = " at (col " + std::to_string(cell % LEVEL_WIDTH) + ", row " +
									  std::to_string(cell / LEVEL_WIDTH) + ")";
			if (texts && book.Find(t.attr) == nullptr)
				e.report.warnings.push_back(
					"inscription " + std::to_string(t.attr) + where + " has no text in content/inscriptions/"
				);
			std::string& first = usedIn[static_cast<size_t>(t.attr)];
			if (!first.empty())
				e.report.warnings.push_back("inscription " + std::to_string(t.attr) + where + " is also in " + first);
			else
				first = e.path + where;
		}
	}
}
} // namespace

int main(int argc, char** argv) {
	bool map = false;
	bool quiet = false;
	std::string scriptDir;
	std::vector<Entry> entries;
	int code = 0;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--map") == 0) {
			map = true;
			continue;
		}
		if (strcmp(argv[i], "--quiet") == 0) {
			quiet = true;
			continue;
		}
		// For tools/level/ascii2level.py: "size WIDTH HEIGHT", "format MAGIC VERSION", "structure GLYPHS" (by
		// Structure number), "wall TYPE..." (the wall types, isWallType), "shaped STRUCTURE GLYPH..." (a shaped cell's
		// glyph, SHAPED_GLYPHS, after its Structure number, no space), then "CHAR TYPE ATTR VALUE STRUCTURE meaning"
		// per glyph of the drawings; a wall type's meaning starts with "wall:".
		if (strcmp(argv[i], "--legend") == 0) {
			printf(
				"size %d %d\nformat %s %d\nstructure %s\nwall",
				LEVEL_WIDTH,
				LEVEL_HEIGHT,
				LEVEL_MAGIC,
				LEVEL_VERSION,
				STRUCTURE_GLYPHS
			);
			for (int type = 0; type < TILE_TYPE_COUNT; type++)
				if (isWallType(type))
					printf(" %d", type);
			printf("\nshaped");
			for (int s = 0; s < STRUCTURE_COUNT; s++)
				if (takesShape(static_cast<Structure>(s)))
					printf(" %d%c", s, SHAPED_GLYPHS[s]);
			printf("\n");
			for (const GlyphDef& g : glyphLegend())
				printf(
					"%c %d %d %d %d %s%s\n",
					g.glyph,
					g.tile.type,
					g.tile.attr,
					g.tile.value,
					static_cast<int>(g.tile.structure),
					isWallType(g.tile.type) ? "wall: " : "",
					g.meaning
				);
			for (int d = 0; d < DECOR_COUNT; d++) // the --map decor and decal drawings (plan placed-decor)
				printf("prop %c %d %s\n", decorGlyph(d), d, DECOR_NAMES[d]);
			for (int d = 0; d < DECAL_COUNT; d++)
				printf("decal %c %d %s\n", 'a' + d, d, DECAL_DEFS[d].name);
			return 0;
		}
		if (strcmp(argv[i], "--script") == 0 && i + 1 < argc) {
			scriptDir = argv[++i];
			continue;
		}
		if (argv[i][0] == '-') {
			fprintf(stderr, "usage: levelcheck [--map] [--quiet] [--script DIR] FILE... | levelcheck --legend\n");
			return 2;
		}
		Entry e;
		e.path = argv[i];
		int version = 0;
		std::string error = loadLevelFile(argv[i], e.grid, &version);
		if (!error.empty()) {
			fprintf(stderr, "%s: %s\n", argv[i], error.c_str());
			code = 2;
			continue;
		}
		e.report = checkLevel(e.grid);
		if (campaignNumber(e.path) > 0 && e.grid.meta.title.empty()) // plan campaign-texts
			e.report.warnings.push_back("a campaign level without a title (the file's title line, docs/levels.md)");
		if (version != LEVEL_VERSION)
			e.report.warnings.push_back(
				"level format v" + std::to_string(version) + ", convert it: levelconvert " + e.path
			);
		entries.push_back(std::move(e));
	}
	if (entries.empty() && code == 0) {
		fprintf(stderr, "usage: levelcheck [--map] [--quiet] [--script DIR] FILE...\n");
		return 2;
	}

	checkInscriptions(entries);
	for (const Entry& e : entries) {
		if (!quiet)
			printReport(e, map);
		if (!e.report.valid && code == 0)
			code = 1;
		if (!scriptDir.empty() && e.report.valid) {
			std::string name = std::filesystem::path(e.path).filename().string();
			std::string file = (std::filesystem::path(scriptDir) / (name + ".txt")).string();
			std::ofstream script(file);
			// The game starts on the entrance cell's left edge (Dungeon::Load).
			script << pathScript(e.grid, e.report, e.path, static_cast<float>(e.report.start.col));
			if (!quiet)
				printf("   script: %s\n", file.c_str());
		}
	}

	// Ranking, easiest first. The finale stays last whatever its score.
	std::vector<const Entry*> order;
	order.reserve(entries.size());
	for (const Entry& e : entries)
		order.push_back(&e);
	std::stable_sort(order.begin(), order.end(), [](const Entry* a, const Entry* b) {
		if (a->report.finale != b->report.finale)
			return b->report.finale;
		return a->report.difficulty < b->report.difficulty;
	});
	printf(
		"\n%-4s %-28s %-7s %6s %5s %5s %8s %6s  %s\n",
		"rank",
		"level",
		"valid",
		"diff",
		"path",
		"reach",
		"monsters",
		"traps",
		"notes"
	);
	int rank = 1;
	for (const Entry* e : order) {
		const LevelReport& r = e->report;
		std::string notes;
		if (r.finale)
			notes += "finale ";
		if (!r.warnings.empty())
			notes += std::to_string(r.warnings.size()) + " warning(s) ";
		if (!r.errors.empty())
			notes += r.errors.front();
		printf(
			"%-4d %-28s %-7s %6.1f %5d %5d %8d %6d  %s\n",
			rank++,
			e->path.c_str(),
			r.valid ? "yes" : "NO",
			r.difficulty,
			r.pathLength,
			r.reachableCells,
			r.monsterCount,
			r.spikes + r.deathTraps + r.rockFalls + r.dartPlates + r.wallSpikes + r.fireVents + r.poisonDrips,
			notes.c_str()
		);
	}
	return code;
}
