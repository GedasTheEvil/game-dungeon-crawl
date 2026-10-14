// Sim checks of the old amulet, poison, scorpion, bow and ranged-weapon scenarios (plan lean-scenario-tests).
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
ItemKind amulet(AmuletType type, AmuletTier tier) { return *amuletKind(type, tier); }

// tests/levels/spikes_one: the player walks 3.5 tiles onto the spikes and stands there a second.
int hpAfterSpikes(std::optional<ItemKind> worn) {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/spikes_one"));
	world.WaitTicks(5);
	if (worn)
		world.Wear(*worn);
	world.Walk(3.5f);
	world.Wait(1000);
	return world.Hp();
}
} // namespace

TEST_CASE("trap guard cuts the spike damage: none without it 30 HP, grand none, lesser about a quarter off") {
	CHECK(hpAfterSpikes(std::nullopt) == 20);
	CHECK(hpAfterSpikes(amulet(AmuletType::TrapWard, AmuletTier::Grand)) == 50);
	CHECK(hpAfterSpikes(amulet(AmuletType::TrapWard, AmuletTier::Lesser)) == 28);
}

TEST_CASE("regeneration heals 1 HP a second while safe, none while poisoned or chased") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/empty_row"));
	world.WaitTicks(10);
	world.player.stats.LoseHP(25);
	world.Wear(amulet(AmuletType::Regeneration, AmuletTier::Normal));
	CHECK(world.dungeon.PlayerSafe());
	CHECK(world.Hp() == 25);
	world.Wait(2000);
	CHECK(world.Hp() == 27);
	world.player.Poison(PoisonTier::Weak, world.events, world.random.gameplay);
	world.Wait(2000);
	CHECK(world.Hp() < 27);

	SimWorld chased; // tests/levels/rat_corridor: a rat comes for the player
	chased.player.god = true;
	REQUIRE(chased.Load("tests/levels/rat_corridor"));
	chased.WaitTicks(10);
	chased.Wear(amulet(AmuletType::Regeneration, AmuletTier::Grand));
	chased.player.stats.LoseHP(20);
	chased.Walk(2.f);
	chased.Wait(1000);
	CHECK_FALSE(chased.dungeon.PlayerSafe());
	CHECK(chased.Hp() == 30);
}

TEST_CASE("poison can kill the player, and the death ends it") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/classic1"));
	world.player.stats.LoseHP(world.Hp() - 10);
	world.player.Poison(PoisonTier::Strong, world.events, world.random.gameplay);
	world.Wait(5000);
	CHECK_FALSE(world.player.Alive());
	CHECK_FALSE(world.player.stats.poison.Any());
}

TEST_CASE("a scorpion walks up and stings (weak poison); the club kills it, it is weak to blows") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/scorpions"));
	REQUIRE(world.Walk(1.5f));
	world.Wait(5000);
	CHECK(world.player.stats.poison.Any());
	CHECK(world.Hp() < 50);
	for (int i = 0; i < 4; i++) {
		world.Attack();
		world.Wait(1000);
	}
	CHECK(world.Nearest() == 0);
	CHECK(world.player.Alive());
}

TEST_CASE("the bow's arrow lands on the floor ahead; facing the plant it hits, ten arrows kill it") {
	SimWorld world; // tests/levels/archery: the plant at col 13, the player at col 1
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/archery"));
	world.WaitTicks(30);
	world.Equip(ItemKind::SelfBow);
	REQUIRE(world.Walk(10.f));
	REQUIRE(world.Walk(-0.2f)); // facing away from the plant
	world.WaitTicks(70);
	const int plant = world.Nearest();
	CHECK(world.Attack());
	world.WaitTicks(100);
	CHECK(world.Nearest() == plant);
	REQUIRE(world.Walk(0.2f)); // turned round: the plant 2.5 tiles ahead
	world.WaitTicks(70);
	const int xp = world.Xp();
	for (int i = 0; i < 12 && world.Xp() == xp; i++) {
		world.Attack();
		world.WaitTicks(70);
	}
	CHECK(world.Nearest() < plant);
	CHECK(world.Xp() > xp);
}

TEST_CASE("each ranged weapon hits the plant from inside its range") {
	struct Shot {
		ItemKind weapon;
		float walk; // tiles right of the start
	};
	for (const Shot& s : {Shot{ItemKind::CompositeBow, 10.f},
						  {ItemKind::Sling, 11.f},
						  {ItemKind::ThrowingStick, 11.6f},
						  {ItemKind::Javelin, 11.4f}}) {
		CAPTURE(std::string(itemText(s.weapon).name));
		SimWorld world;
		world.player.god = true;
		REQUIRE(world.Load("tests/levels/archery"));
		world.WaitTicks(30);
		world.Equip(s.weapon);
		REQUIRE(world.Walk(s.walk));
		world.WaitTicks(70);
		CHECK(world.Attack());
		world.WaitTicks(120);
		CHECK(world.Nearest() < 30); // the plant loses health
	}
}

// plan death-spikes-poison
TEST_CASE("the death trap poisons strongly once per entry; plain spikes do not; the ward cuts the damage only") {
	{
		SimWorld world;
		REQUIRE(world.Load("tests/levels/spikes_one"));
		world.WaitTicks(5);
		world.Walk(3.5f);
		world.Wait(1000);
		CHECK_FALSE(world.player.stats.poison.Any());
	}
	SimWorld world;
	REQUIRE(world.Load("tests/levels/death_one"));
	world.WaitTicks(5);
	world.Wear(amulet(AmuletType::TrapWard, AmuletTier::Lesser));
	world.Walk(3.5f);
	world.Wait(1000);
	CHECK(world.player.stats.poison.Running(PoisonTier::Strong));
	CHECK(world.Hp() < 50);
}
