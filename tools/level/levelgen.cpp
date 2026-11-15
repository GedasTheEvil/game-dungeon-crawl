// levelgen: writes random levels. See docs/levels.md.
//   levelgen [--seed N] [--difficulty D] [--zone SLUG [--cells C] [--boss] [--title T] [--page N] [--texts IDS]
//            [--lore IDS] [--stargate] [--fork 1|2] [--finale]] [--count K] [--map] OUT
// With --count K > 1, OUT is a prefix: OUT1 .. OUTK, seeds N .. N+K-1. With --zone, a level in the campaign's shape
// for that zone (plan campaign-levelgen), about C open cells; with --boss, the zone's boss level.
// A zone level's inscriptions come from content/inscriptions/ (run it from the repo root), or --texts 53,54 names
// them, --lore 33,34 the secret rooms' texts; --page puts an expedition page, --title names the level, --stargate makes
// it the Stargate chamber's level,
// --fork 1 (2: the last fork) splits the route in two, --finale (with --boss) puts the ankh behind the boss gate.

#include "../../src/core/paths.h"
#include "../../src/level/campaign.h"
#include "../../src/level/inscriptions.h"
#include "../../src/level/level.h"
#include "../../src/level/level_check.h"
#include "../../src/level/level_gen.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <print>

namespace {
int usage() {
	std::println(
		stderr,
		"usage: levelgen [--seed N] [--difficulty 1-10] [--zone SLUG [--cells C] [--boss] [--title T] [--page N] "
		"[--texts IDS] [--lore IDS] [--stargate] [--fork 1|2] [--finale]] [--count K] [--map] OUT"
	);
	return 2;
}
} // namespace

int main(int argc, char** argv) {
	GenOptions options;
	int count = 1;
	bool map = false;
	const char* out = nullptr;
	for (int i = 1; i < argc; i++) {
		bool hasValue = i + 1 < argc;
		if (strcmp(argv[i], "--seed") == 0 && hasValue)
			options.seed = static_cast<uint32_t>(strtoul(argv[++i], nullptr, 10));
		else if (strcmp(argv[i], "--difficulty") == 0 && hasValue)
			options.difficulty = atoi(argv[++i]);
		else if (strcmp(argv[i], "--zone") == 0 && hasValue) {
			options.zone = zoneBySlug(argv[++i]);
			if (options.zone == NO_ZONE)
				return usage();
		} else if (strcmp(argv[i], "--cells") == 0 && hasValue)
			options.openCells = atoi(argv[++i]);
		else if (strcmp(argv[i], "--count") == 0 && hasValue)
			count = atoi(argv[++i]);
		else if (strcmp(argv[i], "--boss") == 0)
			options.boss = true;
		else if (strcmp(argv[i], "--finale") == 0)
			options.finale = true;
		else if (strcmp(argv[i], "--stargate") == 0)
			options.stargate = true;
		else if (strcmp(argv[i], "--fork") == 0 && hasValue)
			options.fork = atoi(argv[++i]);
		else if (strcmp(argv[i], "--title") == 0 && hasValue)
			options.title = argv[++i];
		else if (strcmp(argv[i], "--page") == 0 && hasValue)
			options.page = atoi(argv[++i]);
		else if ((strcmp(argv[i], "--texts") == 0 || strcmp(argv[i], "--lore") == 0) && hasValue) {
			std::vector<int>& ids = strcmp(argv[i], "--texts") == 0 ? options.inscriptions : options.lore;
			for (const char* p = argv[++i]; *p != '\0';) {
				char* end = nullptr;
				ids.push_back(static_cast<int>(strtol(p, &end, 10)));
				if (end == p)
					return usage();
				p = *end == ',' ? end + 1 : end;
			}
		} else if (strcmp(argv[i], "--map") == 0)
			map = true;
		else if (argv[i][0] != '-' && out == nullptr)
			out = argv[i];
		else
			return usage();
	}
	if (out == nullptr || count < 1 || options.difficulty < GEN_MIN_DIFFICULTY ||
		options.difficulty > GEN_MAX_DIFFICULTY || options.openCells < 0 ||
		(options.boss && (!isZone(options.zone) || zoneBoss(static_cast<ZoneId>(options.zone)) == 0)))
		return usage();

	InscriptionBook texts;
	if (isZone(options.zone)) {
		const std::string root = Paths::rootDir();
		for (const std::string& m : texts.Load((root.empty() ? "" : root + "/") + CONTENT_DIR "inscriptions"))
			std::println(stderr, "{}", m);
		options.texts = &texts;
	}
	int code = 0;
	for (int k = 0; k < count; k++) {
		GenOptions o = options;
		o.seed = options.seed + static_cast<uint32_t>(k);
		GenResult result = generateLevel(o);
		std::string path = count > 1 ? std::string(out) + std::to_string(k + 1) : std::string(out);
		if (!result.ok) {
			std::println(stderr, "{}: no valid level for seed {} after {} attempts", path, o.seed, result.attempts);
			code = 1;
			continue;
		}
		if (!saveLevelFile(path.c_str(), result.grid)) {
			std::println(stderr, "{}: cannot write", path);
			return 2;
		}
		const LevelReport& r = result.report;
		std::println(
			"{}: seed {}, difficulty {} -> score {:.1f} (target {:.1f}, {} attempts), path {}, {} monsters, "
			"{} gates, {} open cells",
			path,
			o.seed,
			o.difficulty,
			r.difficulty,
			result.target,
			result.attempts,
			r.pathLength,
			r.monsterCount,
			r.gates,
			r.openCells
		);
		if (isZone(o.zone)) {
			const GenShape& s = result.shape;
			std::println(
				"   {}: {} halls ({} tall), {} loops, {} hubs, {} ledges, {} shafts; {} wades, {} pools, {} sand "
				"({} quicksand), {} dark halls, {} braziers, {} light shafts, {} sun discs; {} spikes, {} death pits, "
				"{} rock falls, {} dart plates, {} wall spikes, {} fire vents, {} poison drips, {} gauntlets; {} "
				"chained "
				"openers, {} riddle gates, {} name gates, {} hidden rooms, {} treasure tombs{}; {} trapped chests, {} "
				"Hapi "
				"potions, {} resistance potions; {} inscriptions, {} pages, {} forks{}",
				zoneInfo(static_cast<ZoneId>(o.zone)).slug,
				s.halls,
				s.tallHalls,
				s.loops,
				s.hubs,
				s.ledges,
				s.shafts,
				s.wades,
				s.pools,
				r.sandCells,
				r.quicksandCells,
				s.darkHalls,
				s.braziers,
				r.lightShafts,
				s.sunDiscs,
				r.spikes,
				r.deathTraps,
				r.rockFalls,
				r.dartPlates,
				r.wallSpikes,
				r.fireVents,
				r.poisonDrips,
				s.gauntlets,
				s.chains,
				s.riddleGates,
				s.nameGates,
				s.hiddenRooms,
				s.treasureTombs,
				s.bossRoom ? ", a boss room" : "",
				s.trappedChests,
				s.hapiPotions,
				s.resistancePotions,
				s.inscriptions,
				s.pages,
				s.forks,
				o.title.empty() ? "" : (", \"" + o.title + "\"")
			);
		}
		if (map)
			std::print("{}", renderLevel(result.grid, &r));
	}
	return code;
}
