// levelcheck: validates level files and ranks them by difficulty. See docs/levels.md.
//   levelcheck [--map] [--quiet] [--script DIR] FILE...
// --script DIR writes DIR/<level name>.txt per valid level: a scenario (docs/testing.md) that plays the path the
// check found in the real game, in god mode, and expects the level to be finished at the end. It kills the boss
// (killboss) where the path reaches it.
// --legend prints the glyphs of the --map drawings and the ASCII level sources, the props, the decals and the
// container looks.
// Exit code: 0 all valid, 1 at least one level cannot be finished, 2 usage / unreadable file.

#include "../../src/level/gameplay_config.h"
#include "../../src/level/containers.h"
#include "../../src/level/decor.h"
#include "../../src/level/inscriptions.h"
#include "../../src/level/level.h"
#include "../../src/level/level_check.h"
#include "../../src/level/path_script.h"
#include "../../src/level/monster_kinds.h"
#include "../../src/level/movement.h"
#include "../../src/level/tile_defs.h"
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include "../../src/core/paths.h"
#include <print>

namespace {
struct Entry {
	std::string path;
	LevelGrid grid;
	LevelReport report;
};

void printReport(const Entry& e, bool map) {
	const LevelReport& r = e.report;
	std::println("== {}: {}", e.path, r.valid ? "OK" : "INVALID");
	for (const std::string& m : r.errors)
		std::println("   error: {}", m);
	for (const std::string& m : r.warnings)
		std::println("   warning: {}", m);
	std::println(
		"   start (col {}, row {}), {}",
		r.start.col,
		r.start.row,
		r.finale ? "goal: ankh (finale)" : "goal: exit"
	);
	std::println(
		"   size: {} open cells, {} reachable, bounds {}x{}",
		r.openCells,
		r.reachableCells,
		r.boundsWidth,
		r.boundsHeight
	);
	std::string perType;
	for (int type = 1; type <= MONSTER_TYPE_MAX; type++)
		if (const MonsterKind* kind = monsterKind(type))
			perType += std::string(perType.empty() ? "" : ", ") + kind->label + " " + std::to_string(r.monsters[type]);
	std::println(
		"   content: {} monsters ({}), {} spikes, {} death traps, {} rock falls ({} sand), {} dart plates, {} wall "
		"spikes, {} fire vents, {} poison drips, {} treasures ({} reachable, {} trapped), {} keys, {} gates, {} "
		"levers, {} riddles, "
		"{} teleporters "
		"({} name "
		"gates), {} inscriptions, {} sand cells ({} quicksand), {} sand gusts, {} light shafts, {} mirrors, {} sun "
		"discs",
		r.monsterCount,
		perType,
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
		std::println(
			"   path: {} moves, {} jumps, {} drops, {} ladder steps, {} spikes, {} death traps, {} rock falls, {} sand "
			"falls, {} dart plates, {} wall spikes, {} fire vents, {} poison drips, {} gates ({} colours), {} "
			"teleports, {} monsters "
			"near, {} sand, {} "
			"quicksand, {} sand gusts, {} mirror turns",
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
		std::println(
			"   sub-cells: {} quarter steps ({} into ceiling spikes), {} crawled",
			r.pathSteps,
			r.pathCeilingSpikes,
			r.pathCrawls
		);
	std::println("   difficulty: {:.1f}", r.difficulty);
	if (map)
		std::print("{}", renderLevel(e.grid, &r));
}
// The campaign level number of a file named lvlN, 0 for any other level.
int campaignNumber(const std::string& path) {
	std::string name = std::filesystem::path(path).filename().string();
	if (name.size() < 4 || !name.starts_with("lvl") || name.find_first_not_of("0123456789", 3) != std::string::npos)
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
			std::println(stderr, "{}", m);
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

struct Options {
	bool map = false;
	bool quiet = false;
	bool legend = false;
	std::string scriptDir;
	std::vector<std::string> files;
};

constexpr const char* USAGE = "usage: levelcheck [--map] [--quiet] [--script DIR] FILE...";

// For tools/level/ascii2level.py: "size WIDTH HEIGHT", "format MAGIC VERSION", "structure GLYPHS" (by
// Structure number), "wall TYPE..." (the wall types, isWallType), "shaped STRUCTURE GLYPH..." (a shaped cell's
// glyph, SHAPED_GLYPHS, after its Structure number, no space), then "CHAR TYPE ATTR VALUE STRUCTURE meaning"
// per glyph of the drawings; a wall type's meaning starts with "wall:".
void printLegend() {
	std::print(
		"size {} {}\nformat {} {}\nstructure {}\nwall",
		LEVEL_WIDTH,
		LEVEL_HEIGHT,
		LEVEL_MAGIC,
		LEVEL_VERSION,
		STRUCTURE_GLYPHS
	);
	for (int type = 0; type < TILE_TYPE_COUNT; type++)
		if (isWallType(type))
			std::print(" {}", type);
	std::print("\nshaped");
	for (int s = 0; s < STRUCTURE_COUNT; s++)
		if (takesShape(static_cast<Structure>(s)))
			std::print(" {}{}", s, SHAPED_GLYPHS[s]);
	std::println();
	for (const GlyphDef& g : glyphLegend())
		std::println(
			"{} {} {} {} {} {}{}",
			g.glyph,
			g.tile.type,
			g.tile.attr,
			g.tile.value,
			static_cast<int>(g.tile.structure),
			isWallType(g.tile.type) ? "wall: " : "",
			g.meaning
		);
	for (int d = 0; d < DECOR_COUNT; d++) // the --map decor and decal drawings (plan placed-decor)
		std::println("prop {} {} {}", decorGlyph(d), d, DECOR_NAMES[d]);
	for (int d = 0; d < DECAL_COUNT; d++)
		std::println("decal {} {} {}", decalLetter(d), d, DECAL_DEFS[d].name);
	for (int c = 0; c < CONTAINER_COUNT; c++) // the --map containers drawing (plan placed-container-looks)
		std::println("container {} {} {}", CONTAINER_GLYPHS[c], c, CONTAINER_NAMES[c]);
	std::println("container {} coffin, plain lid", PLAIN_COFFIN_GLYPH);
	std::println("container {} coffin, inscribed lid", INSCRIBED_COFFIN_GLYPH);
}

// nullopt: a usage error, already printed. --legend stops the parsing: the legend needs nothing else.
std::optional<Options> parseOptions(int argc, char** argv) {
	Options o;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--map") == 0)
			o.map = true;
		else if (strcmp(argv[i], "--quiet") == 0)
			o.quiet = true;
		else if (strcmp(argv[i], "--legend") == 0) {
			o.legend = true;
			return o;
		} else if (strcmp(argv[i], "--script") == 0 && i + 1 < argc)
			o.scriptDir = argv[++i];
		else if (argv[i][0] == '-') {
			std::println(stderr, "{} | levelcheck --legend", USAGE);
			return std::nullopt;
		} else
			o.files.push_back(argv[i]);
	}
	return o;
}

// Loads and checks every file; an unreadable one is reported and sets code to 2.
std::vector<Entry> loadEntries(const std::vector<std::string>& files, int& code) {
	std::vector<Entry> entries;
	for (const std::string& file : files) {
		Entry e;
		e.path = file;
		int version = 0;
		std::string error = loadLevelFile(file.c_str(), e.grid, &version);
		if (!error.empty()) {
			std::println(stderr, "{}: {}", file, error);
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
	return entries;
}

void writeScript(const Entry& e, const std::string& scriptDir, bool quiet) {
	std::string name = std::filesystem::path(e.path).filename().string();
	std::string file = (std::filesystem::path(scriptDir) / (name + ".txt")).string();
	std::ofstream script(file);
	// The game starts on the entrance cell's left edge (Dungeon::Load).
	script << pathScript(e.grid, e.report, e.path, static_cast<float>(e.report.start.col));
	if (!quiet)
		std::println("   script: {}", file);
}

// Ranking, easiest first. The finale stays last whatever its score.
void printRanking(const std::vector<Entry>& entries) {
	std::vector<const Entry*> order;
	order.reserve(entries.size());
	for (const Entry& e : entries)
		order.push_back(&e);
	std::stable_sort(order.begin(), order.end(), [](const Entry* a, const Entry* b) {
		if (a->report.finale != b->report.finale)
			return b->report.finale;
		return a->report.difficulty < b->report.difficulty;
	});
	std::println(
		"\n{:<4} {:<28} {:<7} {:>6} {:>5} {:>5} {:>8} {:>6}  {}",
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
		std::println(
			"{:<4} {:<28} {:<7} {:6.1f} {:5} {:5} {:8} {:6}  {}",
			rank++,
			e->path,
			r.valid ? "yes" : "NO",
			r.difficulty,
			r.pathLength,
			r.reachableCells,
			r.monsterCount,
			r.spikes + r.deathTraps + r.rockFalls + r.dartPlates + r.wallSpikes + r.fireVents + r.poisonDrips,
			notes
		);
	}
}
} // namespace

int main(int argc, char** argv) {
	std::optional<Options> opts = parseOptions(argc, argv);
	if (!opts)
		return 2;
	if (opts->legend) {
		printLegend();
		return 0;
	}
	int code = 0;
	std::vector<Entry> entries = loadEntries(opts->files, code);
	if (entries.empty() && code == 0) {
		std::println(stderr, "{}", USAGE);
		return 2;
	}

	checkInscriptions(entries);
	for (const Entry& e : entries) {
		if (!opts->quiet)
			printReport(e, opts->map);
		if (!e.report.valid && code == 0)
			code = 1;
		if (!opts->scriptDir.empty() && e.report.valid)
			writeScript(e, opts->scriptDir, opts->quiet);
	}
	printRanking(entries);
	return code;
}
