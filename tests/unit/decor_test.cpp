#include "../../external/doctest/doctest.h"
#include "../../src/core/gameplay_config.h"
#include "../../src/world/campaign.h"
#include "../../src/world/decor.h"
#include "../../src/world/decor_scatter.h"
#include "../../src/world/level_gen.h"
#include "../../src/world/tile_defs.h"
#include <memory>
#include <string>

TEST_CASE("decoration tiers by depth: cave, worked tunnel, tomb, temple") {
	CHECK(decorTier(1) == 0);
	CHECK(decorTier(3) == 0);
	CHECK(decorTier(4) == 1);
	CHECK(decorTier(7) == 1);
	CHECK(decorTier(8) == 2);
	CHECK(decorTier(12) == 2);
	CHECK(decorTier(13) == 3);
	CHECK(decorTier(30) == 3);
	CHECK(decorTier(DECOR_DEPTH_ALL) == DECOR_TIER_COUNT - 1);
}

TEST_CASE("a generated level's difficulty stands for a campaign depth") {
	CHECK(genDecorDepth(1) == 1);
	CHECK(decorTier(genDecorDepth(3)) == 1);
	CHECK(decorTier(genDecorDepth(10)) == DECOR_TIER_COUNT - 1);
}

TEST_CASE("every scattered prop and decal has a tier, the first one a cave") {
	for (int d = 0; d < DECOR_SCATTERED; d++) {
		CHECK(DECOR_TIERS[d] >= 0);
		CHECK(DECOR_TIERS[d] < DECOR_TIER_COUNT);
	}
	CHECK(DECOR_TIERS[DECOR_COFFIN] == -1); // placed with its mummy, at any depth
	for (int v = 1; v < DECOR_THOTH_VARIANTS; v++)
		CHECK(DECOR_TIERS[DECOR_THOTH + v] == DECOR_TIERS[DECOR_THOTH]);
	for (int t : DECAL_TIERS) {
		CHECK(t >= 0);
		CHECK(t < DECOR_TIER_COUNT);
	}
	CHECK(ROUGH_PERCENT[0] == 100); // the first levels are all cave
	CHECK(PAINTED_PERCENT[0] == 0);
}

namespace {
bool noCoffinBoss(int /*monsterType*/) { return false; }

Tile cellAt(const LevelGrid& grid, int col, int row) { return grid.cells[row * LEVEL_WIDTH + col]; }
int idx(int col, int row) { return row * LEVEL_WIDTH + col; }

// A heap layout (it is large) scattered over a campaign level at its own depth.
struct Scattered {
	LevelGrid grid;
	std::unique_ptr<DecorLayout> layout = std::make_unique<DecorLayout>();
	int depth = 0;
};

Scattered campaignLevel(int number) {
	Scattered s;
	s.depth = number;
	const std::string file = campaignLevelFile(number);
	REQUIRE(loadLevelFile(file.c_str(), s.grid).empty());
	scatterDecor(s.grid.cells, file.c_str(), number, noCoffinBoss, *s.layout);
	return s;
}
} // namespace

TEST_CASE("the scatter is the same on every load, keyed by the level's file name") {
	LevelGrid grid;
	REQUIRE(loadLevelFile(campaignLevelFile(9).c_str(), grid).empty());
	auto a = std::make_unique<DecorLayout>();
	auto b = std::make_unique<DecorLayout>();
	auto c = std::make_unique<DecorLayout>();
	const DecorCounts first = scatterDecor(grid.cells, "levels/lvl9", 9, noCoffinBoss, *a);
	const DecorCounts again = scatterDecor(grid.cells, "elsewhere/lvl9", 9, noCoffinBoss, *b);
	scatterDecor(grid.cells, "levels/lvl10", 9, noCoffinBoss, *c);
	CHECK(first.props > 0);
	CHECK(first.props == again.props);
	CHECK(first.decals == again.decals);
	CHECK(first.torches == again.torches);
	bool same = true;
	bool differs = false;
	for (int k = 0; k < LEVEL_CELL_COUNT; k++) {
		same = same && a->decor[k].type == b->decor[k].type && a->decal[k].type == b->decal[k].type &&
			   a->torch[k] == b->torch[k] && a->surface[k].wall == b->surface[k].wall;
		differs = differs || a->decor[k].type != c->decor[k].type || a->decal[k].type != c->decal[k].type;
	}
	CHECK(same);
	CHECK(differs);
}

TEST_CASE("campaign props stand on the floor of empty cells, from the tiers the depth unlocks") {
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		CAPTURE(n);
		const Scattered s = campaignLevel(n);
		const int tier = decorTier(n);
		CHECK(decorTierUsed(s.grid.cells, *s.layout) <= tier);
		for (int j = 0; j < LEVEL_HEIGHT; j++)
			for (int i = 0; i < LEVEL_WIDTH; i++) {
				const Tile t = cellAt(s.grid, i, j);
				const DecorCell& prop = s.layout->decor[idx(i, j)];
				if (t.type == MonsterSpawn && t.attr == MonsterMummy)
					CHECK(prop.type == DECOR_COFFIN);
				else if (prop.type >= 0) {
					CHECK(isEmptyCell(t));
					CHECK((j > 0 && isWall(cellAt(s.grid, i, j - 1))));
					CHECK(DECOR_TIERS[prop.type] <= tier);
					CHECK(prop.type < DECOR_SCATTERED);
				}
				const DecalCell& decal = s.layout->decal[idx(i, j)];
				if (decal.type >= 0) {
					CHECK_FALSE(isWall(t));
					CHECK(DECAL_TIERS[decal.type] <= tier);
					CHECK(decal.x >= 0.f);
					CHECK(decal.x <= 1.f);
				}
			}
	}
}

TEST_CASE("torches keep their distance and stay off the cells with a fire or a statue") {
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		CAPTURE(n);
		const Scattered s = campaignLevel(n);
		for (int j = 0; j < LEVEL_HEIGHT; j++) {
			int last = -100;
			for (int i = 0; i < LEVEL_WIDTH; i++) {
				if (!s.layout->torch[idx(i, j)])
					continue;
				const int prop = s.layout->decor[idx(i, j)].type;
				CHECK(i - last > 4);
				CHECK(tileDef(cellAt(s.grid, i, j).type).torch);
				CHECK_FALSE(isWall(cellAt(s.grid, i, j)));
				CHECK(prop != DECOR_BRAZIER);
				CHECK(prop != DECOR_LAMP);
				CHECK_FALSE(isThoth(prop));
				last = i;
			}
		}
	}
}

TEST_CASE("a ladder shaft keeps one style: the bottom piece on the floor, the top piece at the top") {
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		CAPTURE(n);
		const Scattered s = campaignLevel(n);
		for (int i = 0; i < LEVEL_WIDTH; i++)
			for (int j = 0; j < LEVEL_HEIGHT; j++) {
				const LadderCell& cell = s.layout->ladder[idx(i, j)];
				if (cellAt(s.grid, i, j).type != Ladder) {
					CHECK(cell.style == -1);
					continue;
				}
				REQUIRE(cell.style >= 0);
				const bool ladderBelow = j > 0 && cellAt(s.grid, i, j - 1).type == Ladder;
				const bool ladderAbove = j + 1 < LEVEL_HEIGHT && cellAt(s.grid, i, j + 1).type == Ladder;
				if (ladderBelow) {
					const LadderCell& below = s.layout->ladder[idx(i, j - 1)];
					CHECK(below.style == cell.style);
					if (cell.piece < LADDER_MID_COUNT && below.piece < LADDER_MID_COUNT)
						CHECK(below.piece != cell.piece);
					CHECK(cell.piece != LADDER_BOTTOM);
				} else if (j == 0 || isSolidTile(cellAt(s.grid, i, j - 1)))
					CHECK(cell.piece == LADDER_BOTTOM);
				if (!ladderAbove && cell.piece != LADDER_BOTTOM)
					CHECK(cell.piece == LADDER_TOP);
				if (cell.style >= 0 && !LADDER_MIRRORS[cell.style])
					CHECK_FALSE(cell.mirror);
			}
	}
}

TEST_CASE("the first levels are all rough cave rock, the deep ones have painted walls") {
	const Scattered cave = campaignLevel(1);
	const Scattered temple = campaignLevel(20);
	int painted = 0;
	for (int k = 0; k < LEVEL_WIDTH * LEVEL_HEIGHT; k++) {
		if (!isWall(cave.grid.cells[k])) {
			CHECK(cave.layout->surface[k].ceiling == CEILING_ROUGH);
			CHECK(cave.layout->surface[k].wall >= WALL_ROUGH);
		}
		if (!isWall(temple.grid.cells[k]) && temple.layout->surface[k].wall <= WALL_PLASTER_BROKEN)
			painted++;
	}
	CHECK(painted > 0);
}

TEST_CASE("coffins stand round a boss whose minions climb out of them, alive or slain") {
	LevelGrid grid;
	for (int i = 1; i < 30; i++)
		grid.set(i, 1, Tile{});
	grid.set(10, 1, Tile{MonsterSpawn, MonsterAnubisBoss, 0});
	auto anubis = [](int type) { return type == MonsterAnubisBoss; };
	for (int i = 1; i < 30; i++) {
		CAPTURE(i);
		const bool near = i != 10 && i >= 10 - MINION_SUMMON_REACH && i <= 10 + MINION_SUMMON_REACH;
		CHECK(bossCoffinCell(grid.cells, i, 1, anubis) == near);
		CHECK_FALSE(bossCoffinCell(grid.cells, i, 1, noCoffinBoss));
	}
	auto layout = std::make_unique<DecorLayout>();
	scatterDecor(grid.cells, "coffins", 30, anubis, *layout);
	CHECK(layout->decor[idx(10 - MINION_SUMMON_REACH, 1)].type == DECOR_COFFIN);
	CHECK(layout->decor[idx(10 + MINION_SUMMON_REACH, 1)].type == DECOR_COFFIN);
	CHECK(layout->decor[idx(10, 1)].type == -1); // the boss's own tile stays bare

	grid.set(10, 1, slainBossObject(MonsterAnubisBoss)); // after a load: the boss is dead, his chamber the same
	CHECK(bossCoffinCell(grid.cells, 9, 1, anubis));
	scatterDecor(grid.cells, "coffins", 30, anubis, *layout);
	CHECK(layout->decor[idx(10, 1)].type == -1);
}
