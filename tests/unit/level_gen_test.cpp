// Zone levels (plan campaign-levelgen): levels in the campaign's shape for a zone, passing the checker with no
// warnings over a range of seeds per zone.
#include "../../external/doctest/doctest.h"
#include "../../src/world/campaign.h"
#include "../../src/world/decor.h"
#include "../../src/world/level.h"
#include "../../src/world/level_gen.h"
#include <cstdlib>
#include <set>
#include <sstream>
#include <string>

namespace {
GenOptions zoneOptions(uint32_t seed, int difficulty, int zone) {
	GenOptions options;
	options.seed = seed;
	options.difficulty = difficulty;
	options.zone = zone;
	return options;
}

std::string levelText(const LevelGrid& grid) {
	std::ostringstream out;
	writeLevel(out, grid.cells, grid.meta);
	return out.str();
}
} // namespace

TEST_CASE("zone levels fill the grid in the campaign's shape and pass the checker") {
	GenShape total;
	int gates = 0;
	for (int zone = 0; zone < ZONE_ID_COUNT; zone++) {
		for (uint32_t seed = 1; seed <= 2; seed++) {
			const int difficulty = 1 + (zone * 2 + static_cast<int>(seed)) % GEN_MAX_DIFFICULTY;
			CAPTURE(zone);
			CAPTURE(seed);
			CAPTURE(difficulty);
			const GenResult gen = generateLevel(zoneOptions(seed, difficulty, zone));
			REQUIRE(gen.ok);
			const LevelReport& r = gen.report;
			CHECK(r.valid);
			CHECK(r.warnings.empty());
			CHECK(gen.grid.meta.zone == zone);
			CHECK(std::abs(r.openCells - ZONE_OPEN_CELLS) < ZONE_OPEN_CELLS / 5);
			CHECK(r.boundsWidth >= LEVEL_WIDTH - 6); // the whole grid
			CHECK(r.boundsHeight >= LEVEL_HEIGHT - 8);
			CHECK(gen.shape.halls >= 10);
			total.tallHalls += gen.shape.tallHalls;
			total.loops += gen.shape.loops;
			total.hubs += gen.shape.hubs;
			total.ledges += gen.shape.ledges;
			total.shafts += gen.shape.shafts;
			gates += r.gates;
		}
	}
	CHECK(total.tallHalls > 0);
	CHECK(total.loops > 0);
	CHECK(total.hubs > 0);
	CHECK(total.ledges > 0);
	CHECK(total.shafts > 0);
	CHECK(gates > 0); // keys and levers in the wings
}

TEST_CASE("a zone level's terrain and monsters follow the zone table") {
	for (int zone = 0; zone < ZONE_ID_COUNT; zone++) {
		const ZoneTerrain& terrain = zoneTerrain(static_cast<ZoneId>(zone));
		const ZoneFauna& fauna = zoneFauna(static_cast<ZoneId>(zone));
		int water = 0, deep = 0, sand = 0, quicksand = 0, sandFalls = 0, dark = 0, torches = 0, braziers = 0;
		int shafts = 0, discs = 0, turns = 0;
		std::set<int> kinds; // of the set's, met
		int strangers = 0; // monsters from outside the set (mimics aside)
		int spidersDark = 0, spidersLit = 0, wormsInSand = 0, coffins = 0, figures = 0;
		for (uint32_t seed = 1; seed <= 4; seed++) {
			CAPTURE(zone);
			CAPTURE(seed);
			const GenResult gen = generateLevel(zoneOptions(seed, 7, zone));
			REQUIRE(gen.ok);
			CHECK(gen.report.warnings.empty());
			turns += gen.report.pathTurns;
			for (const Tile& t : gen.grid.cells) {
				water += inHalfWater(t) ? 1 : 0;
				deep += t.structure == Structure::DeepWater ? 1 : 0;
				sand += t.structure == Structure::Sand ? 1 : 0;
				quicksand += inQuicksand(t) ? 1 : 0;
				sandFalls += isSandFall(t) ? 1 : 0;
				dark += t.dressing.torch == TorchPlacing::None ? 1 : 0;
				torches += t.dressing.torch == TorchPlacing::Torch ? 1 : 0;
				braziers += t.dressing.prop == DECOR_BRAZIER ? 1 : 0;
				shafts += t.type == LightShaft ? 1 : 0;
				discs += t.fixture.type == SunDisc ? 1 : 0;
				coffins += t.dressing.prop == DECOR_COFFIN_MUMMY ? 1 : 0;
				figures += t.dressing.prop == DECOR_USHABTI ? 1 : 0;
				if (t.type != MonsterSpawn || t.attr == MonsterMimic || t.attr == MonsterCursedMimic)
					continue;
				bool inSet = false;
				for (const ZoneMonster& m : fauna.monsters)
					inSet = inSet || (m.weight > 0 && m.type == t.attr);
				strangers += inSet ? 0 : 1;
				kinds.insert(t.attr);
				if (t.attr == MonsterCamelSpider)
					(t.dressing.torch == TorchPlacing::None ? spidersDark : spidersLit)++;
				wormsInSand += t.attr == MonsterWorm && inSand(t) ? 1 : 0;
			}
		}
		CAPTURE(zone);
		CHECK(strangers == 0);
		CHECK(kinds.size() >= 3);
		if (zone == static_cast<int>(ZoneId::WebHalls))
			CHECK(spidersDark > spidersLit); // the spiders run in the dark halls
		if (terrain.sand == TerrainAmount::Much)
			CHECK(wormsInSand > 0); // the worms lie under the drifts
		if (zone == static_cast<int>(ZoneId::Necropolis))
			CHECK(coffins > 0); // mummies among decoy coffins
		if (zone == static_cast<int>(ZoneId::PriestsQuarter))
			CHECK(figures > 0); // living ushabti among decor ones
		CHECK((water > 0) == (terrain.water != TerrainAmount::None));
		CHECK((deep > 0) == (terrain.water >= TerrainAmount::Some));
		CHECK((sand > 0) == (terrain.sand != TerrainAmount::None));
		CHECK((quicksand > 0) == (terrain.sand >= TerrainAmount::Some));
		CHECK((sandFalls > 0) == (terrain.sand == TerrainAmount::Much));
		CHECK((dark > 0) == (terrain.dark != TerrainAmount::None));
		CHECK((torches > 0) == (terrain.dark != TerrainAmount::None));
		CHECK((braziers > 0) == terrain.braziers);
		CHECK((shafts > 0) == (terrain.lightShafts || terrain.mirrors));
		CHECK((discs > 0) == terrain.mirrors);
		CHECK((turns > 0) == terrain.mirrors); // a disc opens a gate on the way: the mirror turned
		if (terrain.water == TerrainAmount::Much)
			CHECK(water > 4 * 60); // flooded halls
	}
}

TEST_CASE("a zone level's traps follow the zone table; test levels keep theirs") {
	for (int zone = 0; zone < ZONE_ID_COUNT; zone++) {
		const ZoneTraps& traps = zoneTraps(static_cast<ZoneId>(zone));
		int darts = 0, wallSpikes = 0, fireVents = 0, drips = 0, gauntlets = 0, pairs = 0;
		for (uint32_t seed = 1; seed <= 3; seed++) {
			CAPTURE(zone);
			CAPTURE(seed);
			const GenResult gen = generateLevel(zoneOptions(seed, 8, zone));
			REQUIRE(gen.ok);
			const LevelReport& r = gen.report;
			CHECK(r.warnings.empty()); // an antidote with the plates and drips, every run crossable
			darts += r.dartPlates;
			wallSpikes += r.wallSpikes;
			fireVents += r.fireVents;
			drips += r.poisonDrips;
			gauntlets += gen.shape.gauntlets;
			for (int cell = 0; cell + 1 < LEVEL_CELL_COUNT; cell++)
				pairs += isRhythmTrap(gen.grid.cells[cell].fixture.type) &&
								 isRhythmTrap(gen.grid.cells[cell + 1].fixture.type)
							 ? 1
							 : 0;
		}
		CAPTURE(zone);
		CHECK((darts > 0) == (traps.darts > 0));
		CHECK((wallSpikes > 0) == (traps.wallSpikes > 0));
		CHECK((fireVents > 0) == (traps.fireVents > 0));
		CHECK((drips > 0) == (traps.poisonDrips > 0));
		CHECK((gauntlets > 0) == (traps.gauntlets > 0));
		CHECK(gauntlets <= 3 * traps.gauntlets);
		if (zone == static_cast<int>(ZoneId::SerpentTemple))
			CHECK(pairs > 0); // two in step, from difficulty 6
	}
	for (int difficulty = GEN_MIN_DIFFICULTY; difficulty <= GEN_MAX_DIFFICULTY; difficulty++) {
		GenOptions options;
		options.seed = 5;
		options.difficulty = difficulty;
		const GenResult gen = generateLevel(options);
		REQUIRE(gen.ok);
		CHECK(gen.report.dartPlates + gen.report.wallSpikes + gen.report.fireVents + gen.report.poisonDrips == 0);
		CHECK(gen.shape.gauntlets == 0);
	}
}

TEST_CASE("a zone level comes out the same for the same seed, and its size follows the target") {
	const int zone = static_cast<int>(ZoneId::CisternsOfSobek);
	const GenResult a = generateLevel(zoneOptions(9, 6, zone));
	const GenResult b = generateLevel(zoneOptions(9, 6, zone));
	REQUIRE(a.ok);
	CHECK(levelText(a.grid) == levelText(b.grid));
	CHECK(levelText(a.grid) != levelText(generateLevel(zoneOptions(10, 6, zone)).grid));

	GenOptions small = zoneOptions(9, 6, zone);
	small.openCells = 450;
	const GenResult s = generateLevel(small);
	REQUIRE(s.ok);
	CHECK(s.report.warnings.empty());
	CHECK(std::abs(s.report.openCells - 450) < 450 / 5);
}

TEST_CASE("gen specs: a test level, or a zone level by the zone's slug") {
	GenOptions options;
	REQUIRE(parseGenSpec("gen:7:3", options));
	CHECK(options.seed == 7);
	CHECK(options.difficulty == 3);
	CHECK(options.zone == NO_ZONE);
	REQUIRE(parseGenSpec("gen:7:4:necropolis", options));
	CHECK(options.difficulty == 4);
	CHECK(options.zone == static_cast<int>(ZoneId::Necropolis));
	CHECK_FALSE(parseGenSpec("gen:7:4:nowhere", options));
	CHECK_FALSE(parseGenSpec("gen:7:4x", options));
	CHECK_FALSE(parseGenSpec("gen:7", options));
	CHECK_FALSE(parseGenSpec("lvl3", options));
}
