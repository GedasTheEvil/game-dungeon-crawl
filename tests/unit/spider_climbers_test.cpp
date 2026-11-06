// Spider climbers (plan spider-climbers): the camel spider and the tiny spider follow the player up a ladder, but a lit
// ladder top keeps them below; the tiny spider fits through a crawl space, comes in groups, and the Mother of Webs
// drops it every other summon.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/level/campaign.h"
#include "../../src/level/climb_path.h"
#include "../../src/level/decor.h"
#include "../../src/level/level_gen.h"
#include <cmath>

namespace {
const Monster* find(const SimWorld& world, int type, bool minion = false) {
	for (int i = 0; i < MAX_MONSTERS; i++) {
		const Monster& mon = world.dungeon.Monsters()[i];
		if (mon.Active() && mon.Type()->id == type && mon.Minion() == minion)
			return &mon;
	}
	return nullptr;
}

// Both floors dark but for the torch at torchCol on the upper floor (-1: none).
void darken(SimWorld& world, int upper, int torchCol) {
	for (const int row : {upper, upper - 2})
		for (int col = 0; col < LEVEL_WIDTH; col++) {
			world.dungeon.PlaceTorch(col, row, row == upper && col == torchCol);
			if (const int prop = world.dungeon.PropAt(col, row); prop == DECOR_BRAZIER || prop == DECOR_LAMP)
				world.dungeon.PlaceDecor(col, row, 1);
		}
}

// The player from the entrance down the ladder (col 6) to the lower floor, right until the spider has seen them, back
// up and left to col 2 (out of a torch's light at the ladder top). Returns the upper floor's row.
int showAndFlee(SimWorld& world, const char* level, int torchCol) {
	REQUIRE(world.Load(level));
	world.player.god = true;
	const auto upper = static_cast<int>(world.Y());
	darken(world, upper, torchCol);
	REQUIRE(world.WalkTo(6.45f));
	REQUIRE(world.Climb(-1.97f));
	REQUIRE(world.WalkTo(8.f));
	world.Wait(500);
	REQUIRE(world.WalkTo(6.45f));
	REQUIRE(world.Climb(2.f));
	REQUIRE(world.WalkTo(2.5f));
	return upper;
}
} // namespace

TEST_CASE("the camel spider, the tiny spider and the Mother of Webs climb; the tiny one is small, in groups") {
	for (const int type : {MonsterCamelSpider, MonsterTinySpider, MonsterMotherOfWebs}) {
		const MonsterKind& kind = *monsterKind(type);
		CAPTURE(kind.label);
		CHECK(kind.climbs);
		CHECK(kind.climbRise > 0.f);
		CHECK(kind.shunsLight);
	}
	const MonsterKind& tiny = *monsterKind(MonsterTinySpider);
	const MonsterKind& camel = *monsterKind(MonsterCamelSpider);
	CHECK(std::string(tiny.model) == camel.model);
	CHECK(std::string(tiny.texture) != camel.texture); // its own, darker tint
	CHECK(tiny.scale < camel.scale / 2);
	CHECK(tiny.heightQuarters == 1); // through a crawl space
	CHECK(camel.heightQuarters > 1); // its big kin does not fit
	CHECK(tiny.group == 4);
	CHECK(camel.group == 1);
	CHECK_FALSE(isPoisoner(MonsterTinySpider)); // a plain bite
	const MonsterKind& mother = *monsterKind(MonsterMotherOfWebs);
	CHECK(mother.boss.smallMinion == MonsterTinySpider);
}

TEST_CASE("a camel spider follows the player up the ladder in the dark") {
	SimWorld world;
	const int upper = showAndFlee(world, "tests/levels/climb_spider", -1);
	const Monster* spider = find(world, MonsterCamelSpider);
	REQUIRE(spider != nullptr);
	world.Wait(10000);
	CHECK(world.Saw(MonsterCamelSpider, CreatureMove::Climb));
	CHECK(spider->Row() == upper);
	CHECK_FALSE(spider->OnRungs());
}

TEST_CASE("a tiny spider follows the player up the ladder in the dark") {
	SimWorld world;
	const int upper = showAndFlee(world, "tests/levels/climb_tiny", -1);
	const Monster* spider = find(world, MonsterTinySpider);
	REQUIRE(spider != nullptr);
	world.Wait(10000);
	CHECK(world.Saw(MonsterTinySpider, CreatureMove::Climb));
	CHECK(spider->Row() == upper);
}

TEST_CASE("a torch at the ladder top keeps the camel spider and the tiny spider below") {
	for (const auto& [level, type] :
		 {std::pair{"tests/levels/climb_spider", MonsterCamelSpider},
		  std::pair{"tests/levels/climb_tiny", MonsterTinySpider}}) {
		CAPTURE(level);
		SimWorld world;
		const int upper = showAndFlee(world, level, 6); // cols 5-7 lit, the player in col 2 in the dark
		REQUIRE(world.dungeon.Lit(6, upper));
		REQUIRE_FALSE(world.dungeon.Lit(2, upper));
		const Monster* spider = find(world, type);
		REQUIRE(spider != nullptr);
		for (int t = 0; t < 10000; t += UPDATE_TICK_MS) {
			world.Tick();
			REQUIRE(spider->Row() < upper); // it may have started up after the player on the rungs, never off the top
		}
		CHECK(spider->Row() == upper - 2); // and back down to its floor
		CHECK_FALSE(spider->OnRungs());
	}
}

TEST_CASE("the climb path of a light shunner goes round no lit cell") {
	LevelGrid g{};
	REQUIRE(loadLevelFile("tests/levels/climb_spider", g).empty());
	int ladderCol = -1, top = -1, foot = LEVEL_HEIGHT;
	for (int row = 0; row < LEVEL_HEIGHT; row++)
		for (int col = 0; col < LEVEL_WIDTH; col++)
			if (g.at(col, row).type == Ladder) {
				ladderCol = col;
				top = std::max(top, row);
				foot = std::min(foot, row);
			}
	REQUIRE(ladderCol >= 0);
	ClimbMap dark;
	dark.Build(g.cells, ladderCol - 4, top, ClimbAbility{}, CLIMB_PATH_MAX);
	CHECK(dark.Steps(ladderCol + 6, foot) == 6 + 2 + 4);
	ClimbAbility shuns{};
	shuns.avoids = [ladderCol, top](int col, int row) { return row == top && std::abs(col - ladderCol) <= 1; };
	ClimbMap lit;
	lit.Build(g.cells, ladderCol - 4, top, shuns, CLIMB_PATH_MAX);
	CHECK(lit.Steps(ladderCol + 6, foot) == -1);
	CHECK(lit.Steps(ladderCol - 3, top) == 1); // the dark upper floor is still its own
}

TEST_CASE("every other spider that drops round the Mother of Webs is a tiny one") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/mother"));
	world.player.stats.AddXP(1000000, world.events);
	const auto row = static_cast<int>(std::floor(world.Y()));
	for (int col = 0; col < LEVEL_WIDTH; col++)
		world.dungeon.PlaceTorch(col, row, false);
	REQUIRE(world.dungeon.SpawnMonster(12, row));
	world.Tick();
	REQUIRE(world.dungeon.LivingMinions() == 2);
	const Monster* camel = find(world, MonsterCamelSpider, true);
	const Monster* tiny = find(world, MonsterTinySpider, true);
	REQUIRE(camel != nullptr);
	REQUIRE(tiny != nullptr);
	CHECK(tiny->SummonedBy() == Summon::Drop);
}

TEST_CASE("levelgen puts tiny spiders in groups of 2 to 4 side by side") {
	int groups = 0;
	for (uint32_t seed = 1; seed <= 12; seed++) {
		GenOptions options;
		options.seed = seed;
		options.difficulty = 3;
		options.zone = static_cast<int>(ZoneId::TombEntrance);
		const GenResult gen = generateLevel(options);
		REQUIRE(gen.ok);
		for (int row = 0; row < LEVEL_HEIGHT; row++) {
			int run = 0;
			for (int col = 0; col <= LEVEL_WIDTH; col++) {
				const bool tiny = col < LEVEL_WIDTH && gen.grid.at(col, row).type == MonsterSpawn &&
								  gen.grid.at(col, row).attr == MonsterTinySpider;
				if (tiny) {
					run++;
					continue;
				}
				if (run > 0) {
					CAPTURE(seed);
					CAPTURE(row);
					CHECK(run >= 2); // never alone
					CHECK(run <= 4 * 2); // two groups may meet, never more
					groups++;
				}
				run = 0;
			}
		}
	}
	CHECK(groups > 0);
}

TEST_CASE("the play level: dark but the right ladder top, the spiders follow up the left ladder") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/spider_climbers"));
	world.player.god = true;
	const auto upper = static_cast<int>(world.Y());
	for (const int row : {upper, upper - 2})
		for (int col = 1; col < LEVEL_WIDTH - 1; col++) {
			CAPTURE(col);
			CHECK(world.dungeon.Lit(col, row) == (row == upper && col >= 29 && col <= 31));
		}
	REQUIRE(world.WalkTo(7.45f));
	REQUIRE(world.Climb(-1.97f));
	REQUIRE(world.WalkTo(13.f)); // the tiny ones come into view
	world.Wait(500);
	REQUIRE(world.WalkTo(7.45f));
	REQUIRE(world.Climb(2.f));
	REQUIRE(world.WalkTo(3.f));
	world.Wait(15000);
	const Monster* camel = find(world, MonsterCamelSpider);
	const Monster* tiny = find(world, MonsterTinySpider);
	REQUIRE(camel != nullptr);
	REQUIRE(tiny != nullptr);
	CHECK(camel->Row() == upper);
	CHECK(tiny->Row() == upper);
}

TEST_CASE("a hurt tiny spider climbs to a lit ladder top (plan hurt-spiders-ignore-light)") {
	SimWorld world;
	const int upper = showAndFlee(world, "tests/levels/climb_tiny", 6);
	REQUIRE(world.dungeon.Lit(6, upper));
	const Monster* spider = find(world, MonsterTinySpider);
	REQUIRE(spider != nullptr);
	const_cast<Monster&>(*spider).TakeWeaponHit(1, DamageMix{0, 0, 100});
	REQUIRE(spider->Health() < spider->MaxHealth());
	world.Wait(10000);
	CHECK(world.Saw(MonsterTinySpider, CreatureMove::Climb));
	CHECK(spider->Row() == upper);
}
