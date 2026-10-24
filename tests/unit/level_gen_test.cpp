// Zone levels (plan campaign-levelgen): levels in the campaign's shape for a zone, passing the checker with no
// warnings over a range of seeds per zone.
#include "../../external/doctest/doctest.h"
#include "../../src/world/campaign.h"
#include "../../src/world/level.h"
#include "../../src/world/level_gen.h"
#include <cstdlib>
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
