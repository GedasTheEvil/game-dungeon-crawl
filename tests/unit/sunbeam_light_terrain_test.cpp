// The checker's light terrain (plan checker-sunbeam-light-terrain): a sunbeam at level start is light, as the game's
// Dungeon::Lit, for a light shunner's spawn: within LIGHT_SHUN_CELLS of the beam along the row, not past it.
#include "../../external/doctest/doctest.h"
#include "../../src/level/level_check.h"
#include "../../src/level/monster_kinds.h"
#include <string>
#include <vector>

namespace {
// tests/levels/mirrors: the light shaft at (3, 3) over the mirror at (3, 2), pointing left: the beam lights (3, 2),
// (2, 2) and (1, 2) at the start.
LevelGrid mirrorsLevel() {
	LevelGrid grid;
	REQUIRE(loadLevelFile("tests/levels/mirrors", grid).empty());
	return grid;
}

void spawn(LevelGrid& grid, int col, int row, int type) {
	Tile& t = grid.cells[row * LEVEL_WIDTH + col];
	t.type = MonsterSpawn;
	t.attr = type;
}

bool warnsHostile(const LevelReport& r) {
	for (const std::string& w : r.warnings)
		if (w.contains("hostile terrain") && w.contains("light"))
			return true;
	return false;
}
} // namespace

TEST_CASE("a sunbeam at level start is light terrain for a spawn: in the beam, beside it, not past it") {
	const LevelGrid grid = mirrorsLevel();
	const MonsterKind& spider = *monsterKind(MonsterCamelSpider);
	REQUIRE(spider.hostile.has(Terrain::Light));
	const std::vector<char> sunlit = startSunlight(grid);
	CHECK(sunlit[2 * LEVEL_WIDTH + 2] != 0);
	CHECK(spawnTerrains(grid, sunlit, 2, 2, spider).has(Terrain::Light)); // in the beam
	CHECK(spawnTerrains(grid, sunlit, 3 + LIGHT_SHUN_CELLS, 2, spider).has(Terrain::Light)); // beside the mirror
	const Terrains past = spawnTerrains(grid, sunlit, 3 + LIGHT_SHUN_CELLS + 1, 2, spider);
	CHECK_FALSE(past.has(Terrain::Light));
	CHECK(past.has(Terrain::Darkness));
}

TEST_CASE("the checker warns of a light shunner spawned in a sunbeam, not of one past it") {
	LevelGrid grid = mirrorsLevel();
	spawn(grid, 3 + LIGHT_SHUN_CELLS + 1, 2, MonsterCamelSpider);
	CHECK_FALSE(warnsHostile(checkLevel(grid)));
	grid = mirrorsLevel();
	spawn(grid, 2, 2, MonsterCamelSpider);
	CHECK(warnsHostile(checkLevel(grid)));
}
