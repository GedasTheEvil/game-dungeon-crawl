// The Mother of Webs (plan camel-spider-boss): the camel spider's boss. Faster than it, she keeps out of the light
// too, but puts out the torches and lamps she passes (never a brazier); she spins webs over the floor that slow the
// player, a blade cuts them, fire burns them; camel spiders drop from the ceiling round her.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/world/decor.h"

namespace {
const Monster* find(const SimWorld& world, int type) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active() && world.dungeon.Monsters()[i].Type()->id == type)
			return &world.dungeon.Monsters()[i];
	return nullptr;
}

// tests/levels/mother: the Mother in col 12 of one long hall, the player in col 2, out of her sight. No flame on the
// row but the test's: the scattered torches go, a scattered brazier or lamp turns to pottery. A player who lives
// through the fight.
int loadHall(SimWorld& world) {
	REQUIRE(world.Load("tests/levels/mother"));
	world.player.stats.AddXP(1000000, world.events);
	const auto row = static_cast<int>(std::floor(world.Y()));
	for (int col = 0; col < LEVEL_WIDTH; col++) {
		world.dungeon.PlaceTorch(col, row, false);
		if (const int prop = world.dungeon.PropAt(col, row); prop == DECOR_BRAZIER || prop == DECOR_LAMP)
			world.dungeon.PlaceDecor(col, row, 1);
	}
	for (int col = 0; col < LEVEL_WIDTH; col++)
		REQUIRE_FALSE(world.dungeon.Lit(col, row));
	return row;
}

// She appears (as in view) with her first spiders, the player still in col 2.
const Monster* wake(SimWorld& world, int row) {
	REQUIRE(world.dungeon.SpawnMonster(12, row));
	world.Tick();
	const Monster* m = find(world, MonsterMotherOfWebs);
	REQUIRE(m != nullptr);
	return m;
}

// The attack key, held until a swing starts; then the swing and its recovery.
void swing(SimWorld& world) {
	for (int t = 0; t < 100 && !world.Attack(); t++)
		world.Tick();
	world.Wait(1500);
}

int cellOf(float x) { return static_cast<int>(std::floor(x)); }
} // namespace

TEST_CASE("the Mother of Webs is the camel spider's boss: faster, shuns the light, douses, spins, spiders drop") {
	const MonsterKind& m = *monsterKind(MonsterMotherOfWebs);
	const MonsterKind& spider = *monsterKind(MonsterCamelSpider);
	CHECK(m.isBoss());
	CHECK(m.kin == MonsterCamelSpider);
	CHECK(m.boss.minion == MonsterCamelSpider);
	CHECK(m.boss.summon == Summon::Drop);
	CHECK(m.shunsLight);
	CHECK(m.speed > spider.speed);
	CHECK(m.douseCells > LIGHT_SHUN_CELLS); // she puts a torch out before she reaches its light
	REQUIRE(m.webs);
	CHECK(m.webs->max > 0);
	CHECK(m.resist[static_cast<size_t>(DamageType::Slash)] == NORMAL); // the camel spider's weaknesses, no more
	CHECK(m.resist[FIRE] == NORMAL);
	CHECK(m.resist[static_cast<size_t>(DamageType::Pierce)] == RESISTS);
}

TEST_CASE("in the dark she runs at the player faster than the camel spider, and her spiders drop round her") {
	SimWorld world;
	const Monster* m = wake(world, loadHall(world));
	const float from = m->CentreX();
	world.Wait(1000);
	CHECK(from - m->CentreX() > 1.6f); // the camel spider's ~1.45 tiles a second
	CHECK(world.dungeon.LivingMinions() >= monsterKind(MonsterMotherOfWebs)->boss.minAlive);
	const Monster* spider = find(world, MonsterCamelSpider);
	REQUIRE(spider != nullptr);
	CHECK(spider->Minion());
	CHECK(spider->SummonedBy() == Summon::Drop);
	CHECK(world.Saw(MonsterMotherOfWebs, CreatureMove::Summon));
}

TEST_CASE("she puts out the torches in her way before she reaches their light, and comes on") {
	SimWorld world;
	const int row = loadHall(world);
	world.dungeon.PlaceTorch(8, row, true); // cols 7-9
	world.dungeon.PlaceTorch(3, row, true); // cols 2-4: the player stands in its light
	const Monster* m = wake(world, row);
	const int hp = world.Hp();
	for (int t = 0; t < 600; t++) {
		world.Tick();
		REQUIRE_FALSE(world.dungeon.Lit(cellOf(m->Left() + 0.01f), row));
		REQUIRE_FALSE(world.dungeon.Lit(cellOf(m->Right() - 0.01f), row));
	}
	CHECK(world.dungeon.Doused(8, row));
	CHECK(world.dungeon.Doused(3, row));
	CHECK_FALSE(world.dungeon.Lit(2, row));
	CHECK(world.Saw(MonsterMotherOfWebs, CreatureMove::Douse));
	CHECK(world.Hp() < hp); // the light gone, she bites
}

TEST_CASE("a brazier stays lit and keeps her off: she waits out of the bow's range") {
	SimWorld world;
	const int row = loadHall(world);
	REQUIRE(world.dungeon.PlaceDecor(5, row, DECOR_BRAZIER)); // cols 4-6 lit, between her and the player (col 2)
	const Monster* m = wake(world, row);
	const int hp = world.Hp();
	for (int t = 0; t < 900; t++) {
		world.Tick();
		REQUIRE_FALSE(world.dungeon.Lit(cellOf(m->Left() + 0.01f), row));
	}
	CHECK_FALSE(world.dungeon.Doused(5, row));
	CHECK(world.dungeon.Lit(5, row));
	CHECK(world.Hp() == hp);
	CHECK(m->Left() - (world.X() + world.player.HalfWidth()) >= COWARD_SAFE_GAP - 0.2f);
	CHECK(world.Saw(MonsterMotherOfWebs, CreatureMove::ShunLight));
}

TEST_CASE("she spins webs over the floor as she goes, up to her limit") {
	SimWorld world;
	const int row = loadHall(world);
	wake(world, row);
	world.player.god = true;
	world.Wait(15000);
	int webs = 0;
	for (int col = 0; col < LEVEL_WIDTH; col++)
		webs += world.dungeon.WebAt(col, row) ? 1 : 0;
	CHECK(webs >= 1);
	CHECK(webs <= MOTHER_WEB_MAX);
	CHECK(world.Saw(MonsterMotherOfWebs, CreatureMove::SpinWeb));
}

TEST_CASE("a web slows the player standing in it; a jump is not slowed") {
	SimWorld world;
	const int row = loadHall(world);
	REQUIRE(world.dungeon.PlaceWeb(3, row));
	REQUIRE_FALSE(world.dungeon.PlaceWeb(3, row)); // one lies there
	REQUIRE_FALSE(world.dungeon.PlaceWeb(3, row + 1)); // rock
	CHECK(world.dungeon.PlayerWalkFactor() == doctest::Approx(1.f));
	REQUIRE(world.WalkTo(3.5f));
	CHECK(world.dungeon.PlayerInWeb());
	CHECK(world.dungeon.PlayerWalkFactor() == doctest::Approx(WEB_SLOW_FACTOR));
	CHECK(world.Told("Your feet stick in the web"));
	world.Jump();
	world.Tick();
	CHECK_FALSE(world.dungeon.PlayerInWeb());
}

TEST_CASE("a blade's swing cuts the web in its reach, a club's does not") {
	SimWorld world;
	const int row = loadHall(world);
	REQUIRE(world.dungeon.PlaceWeb(3, row));
	REQUIRE(world.WalkTo(2.8f)); // facing it, its edge in reach
	world.Equip(ItemKind::Club);
	swing(world);
	CHECK(world.dungeon.WebAt(3, row));
	world.Equip(ItemKind::Khopesh);
	swing(world);
	CHECK_FALSE(world.dungeon.WebAt(3, row));
}

TEST_CASE("a fire arrow burns the webs it flies low over, a plain one does not") {
	SimWorld world;
	const int row = loadHall(world);
	for (int col = 3; col <= 8; col++)
		REQUIRE(world.dungeon.PlaceWeb(col, row));
	auto whole = [&] {
		int n = 0;
		for (int col = 3; col <= 8; col++)
			n += world.dungeon.WebAt(col, row) ? 1 : 0;
		return n;
	};
	world.facing = 1;
	world.Equip(ItemKind::SelfBow);
	swing(world);
	CHECK(whole() == 6);
	world.player.fireArrows = 1;
	swing(world);
	CHECK(whole() < 6);
}

TEST_CASE("a load lights the torches again and takes the webs away") {
	SimWorld world;
	const int row = loadHall(world);
	world.dungeon.PlaceTorch(9, row, true);
	wake(world, row);
	world.player.god = true;
	world.Wait(3000);
	REQUIRE(world.dungeon.Doused(9, row));
	REQUIRE(world.dungeon.PlaceWeb(3, row));
	REQUIRE(world.Load("tests/levels/mother"));
	CHECK_FALSE(world.dungeon.Doused(9, row));
	CHECK_FALSE(world.dungeon.WebAt(3, row));
}
