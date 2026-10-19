// The living ushabti (plan living-ushabti): stands among the decor ushabti as one of them until the player comes
// near, then steps off its place and walks; glazed faience, broken by blows.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
const Monster* ushabti(const SimWorld& world) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active() && world.dungeon.Monsters()[i].Type()->id == MonsterUshabti)
			return &world.dungeon.Monsters()[i];
	return nullptr;
}
} // namespace

TEST_CASE("the ushabti stands still among its fellows until the player comes near, then steps off and follows") {
	SimWorld world; // tests/levels/ushabti: the ushabti in col 7, the player in col 2
	REQUIRE(world.Load("tests/levels/ushabti"));
	world.WaitTicks(2);
	const Monster* u = ushabti(world);
	REQUIRE(u != nullptr);
	const float home = u->CentreX();
	world.Wait(3000);
	CHECK(u->lurking());
	CHECK(u->CentreX() == home);
	CHECK(world.dungeon.MonsterBarsShown() == 0);
	REQUIRE(world.WalkTo(home - FIGURINE_WAKE_RANGE - 0.2f));
	world.WaitTicks(5);
	CHECK(u->lurking()); // a step short: still one of the figures
	REQUIRE(world.WalkTo(home - FIGURINE_WAKE_RANGE + 0.1f));
	world.WaitTicks(2);
	CHECK_FALSE(u->lurking());
	CHECK(u->Rising()); // steps off its place before it acts
	CHECK(world.Saw(MonsterUshabti, CreatureMove::StepOff));

	REQUIRE(world.WalkTo(2.5f)); // back off: it follows, slower than the player walks
	world.Wait(1500);
	const float from = u->CentreX();
	world.Wait(2000);
	CHECK(from - u->CentreX() > 0.3f);
	CHECK(from - u->CentreX() < 2.f);
	const int hp = world.Hp();
	world.Wait(20000);
	CHECK(world.Hp() < hp); // the hoe
}

TEST_CASE("a blow wakes the ushabti; faience breaks under it at double damage, an arrow's point skids off") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/ushabti"));
	world.player.god = true;
	world.facing = 1;
	world.WaitTicks(2);
	const Monster* u = ushabti(world);
	REQUIRE(u != nullptr);
	REQUIRE(u->lurking());
	REQUIRE(world.dungeon.AttackNearest(10, DamageMix{0, 0, 100}, 7.f, 1)); // an arrow from afar, out of its range
	CHECK_FALSE(u->lurking());
	CHECK(u->Health() == u->MaxHealth() - 5);
	REQUIRE(world.dungeon.AttackNearest(10, DamageMix{100, 0, 0}, 7.f, 1));
	CHECK(u->Health() == u->MaxHealth() - 25);
	const int xp = world.Xp();
	while (u->Alive())
		REQUIRE(world.dungeon.AttackNearest(10, DamageMix{100, 0, 0}, 7.f, 1));
	CHECK(world.Xp() > xp);
}

TEST_CASE("the Anubis boss: every other servant out of the wall is an ushabti, awake and coming") {
	SimWorld world; // tests/levels/anubis_wall: his first guard steps out of the west wall onto col 1
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/anubis_wall"));
	world.WaitTicks(2);
	world.Wait(monsterKind(MonsterAnubisBoss)->boss.summonMs + 200);
	const Monster* u = ushabti(world);
	REQUIRE(u != nullptr);
	CHECK(u->Minion());
	CHECK_FALSE(u->lurking());
	int guards = 0;
	for (int i = 0; i < MAX_MONSTERS; i++) {
		const Monster& m = world.dungeon.Monsters()[i];
		guards += m.Active() && m.Minion() && m.Type()->id == MonsterAnubis ? 1 : 0;
	}
	CHECK(guards == 1);
}
