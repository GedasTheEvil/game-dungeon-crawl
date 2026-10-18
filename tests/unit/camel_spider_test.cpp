// The camel spider (plan camel-spider): a fast walker that keeps out of the light of torches, braziers and lamps.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/world/decor.h"

namespace {
const Monster* spider(const SimWorld& world) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active() && world.dungeon.Monsters()[i].Type()->id == MonsterCamelSpider)
			return &world.dungeon.Monsters()[i];
	return nullptr;
}

// tests/levels/camel_spider: the spider in col 7, the player in col 2 of one long hall; no torches but the test's
// (lit before the spider comes into view, on the first tick).
int loadHall(SimWorld& world, int torch = -1) {
	REQUIRE(world.Load("tests/levels/camel_spider"));
	const auto row = static_cast<int>(std::floor(world.Y()));
	for (int col = 0; col < LEVEL_WIDTH; col++)
		world.dungeon.PlaceTorch(col, row, col == torch);
	world.Tick();
	return row;
}

int cellOf(float x) { return static_cast<int>(std::floor(x)); }
} // namespace

TEST_CASE("a flame lights its own cell and the next one either way along its row, nothing farther") {
	SimWorld world;
	const int row = loadHall(world);
	for (int col = 0; col < LEVEL_WIDTH; col++)
		CHECK_FALSE(world.dungeon.Lit(col, row));
	world.dungeon.PlaceTorch(10, row, true);
	CHECK_FALSE(world.dungeon.Lit(10 - LIGHT_SHUN_CELLS - 1, row));
	for (int col = 10 - LIGHT_SHUN_CELLS; col <= 10 + LIGHT_SHUN_CELLS; col++)
		CHECK(world.dungeon.Lit(col, row));
	CHECK_FALSE(world.dungeon.Lit(10 + LIGHT_SHUN_CELLS + 1, row));
	CHECK_FALSE(world.dungeon.Lit(10, row + 1));
	REQUIRE(world.dungeon.PlaceDecor(30, row, DECOR_BRAZIER));
	CHECK(world.dungeon.Lit(31, row));
	REQUIRE(world.dungeon.PlaceDecor(30, row, DECOR_LAMP));
	CHECK(world.dungeon.Lit(29, row));
}

TEST_CASE("in the dark the camel spider runs at the player, faster than they walk, and bites") {
	SimWorld world;
	loadHall(world);
	const Monster* s = spider(world);
	REQUIRE(s != nullptr);
	const float from = s->CentreX();
	world.Wait(1000);
	CHECK(from - s->CentreX() > 1.3f); // the player walks ~1.25 tiles a second
	const int hp = world.Hp();
	world.Wait(15000);
	CHECK(world.Hp() < hp);
	CHECK_FALSE(world.Saw(MonsterCamelSpider, CreatureMove::ShunLight));
}

TEST_CASE("the camel spider will not follow the player into the light: it keeps out of the bow's range") {
	SimWorld world;
	const int row = loadHall(world, 3);
	REQUIRE(world.dungeon.Lit(2, row)); // the player (col 2) stands in the torch's light, cols 2-4
	const Monster* s = spider(world);
	REQUIRE(s != nullptr);
	const int hp = world.Hp();
	for (int t = 0; t < 600; t++) {
		world.Tick();
		REQUIRE_FALSE(world.dungeon.Lit(cellOf(s->Left() + 0.01f), row));
	}
	CHECK(world.Hp() == hp);
	CHECK(s->Left() - (world.X() + world.player.HalfWidth()) >= COWARD_SAFE_GAP - 0.2f);
	CHECK(world.Saw(MonsterCamelSpider, CreatureMove::ShunLight));

	// The player steps out of the light (west of it is wall, so the torch goes out): it comes at once.
	world.dungeon.PlaceTorch(3, row, false);
	world.Wait(8000);
	CHECK(world.Hp() < hp);
}

TEST_CASE("a torch lit in its face turns the charging camel spider back at the edge of its light") {
	SimWorld world;
	const int row = loadHall(world);
	const Monster* s = spider(world);
	REQUIRE(s != nullptr);
	while (s->CentreX() > 5.6f)
		world.Tick();
	const int hp = world.Hp();
	world.dungeon.PlaceTorch(3, row, true); // cols 2-4
	float nearest = s->Left();
	for (int t = 0; t < 600; t++) {
		world.Tick();
		nearest = std::min(nearest, s->Left());
	}
	CHECK(nearest >= 5.f);
	CHECK(world.Hp() == hp);
	CHECK(s->Left() - (world.X() + world.player.HalfWidth()) >= COWARD_SAFE_GAP - 0.2f);
}

TEST_CASE("a lit stretch between the camel spider and the player is a wall to it") {
	SimWorld world;
	loadHall(world, 5); // cols 4-6 lit, the player west of them in the dark
	const Monster* s = spider(world);
	REQUIRE(s != nullptr);
	const int hp = world.Hp();
	for (int t = 0; t < 900; t++) {
		world.Tick();
		REQUIRE(cellOf(s->Left() + 0.01f) > 5 + LIGHT_SHUN_CELLS);
	}
	CHECK(world.Hp() == hp);
}

TEST_CASE("a camel spider in the light runs for the nearer dark") {
	SimWorld world;
	const int row = loadHall(world, 6); // the spider (col 7) in its light, cols 5-7: the dark is east
	const Monster* s = spider(world);
	REQUIRE(s != nullptr);
	world.Wait(1500);
	CHECK_FALSE(world.dungeon.Lit(cellOf(s->CentreX()), row));
	CHECK(s->CentreX() > 7.5f);
}
