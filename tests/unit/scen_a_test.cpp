// Sim checks of the old amulet, poison, scorpion, bow and ranged-weapon scenarios (plan lean-scenario-tests).
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
ItemKind amulet(AmuletType type, AmuletTier tier) { return *amuletKind(type, tier); }

// A player with 2000 HP (50 x 40), who lives through a trap's crossing (SPIKES_CROSS_HP).
void toughen(SimWorld& world) {
	world.player.stats.AddMaxHP(3900);
	REQUIRE(world.player.stats.CurrentMaxHP() == 2000);
}

// tests/levels/spikes_one: the HP a 2000 HP player loses walking 3.5 tiles onto the spikes and standing there a second.
int lossOnSpikes(std::optional<ItemKind> worn) {
	SimWorld world;
	toughen(world);
	REQUIRE(world.Load("tests/levels/spikes_one"));
	world.WaitTicks(5);
	if (worn)
		world.Wear(*worn);
	world.Walk(3.5f);
	world.Wait(1000);
	return 2000 - world.Hp();
}
} // namespace

TEST_CASE("trap guard cuts the spike damage: grand none, lesser about a quarter off") {
	const int bare = lossOnSpikes(std::nullopt);
	CHECK(bare > 200); // half a crossing's 400 (20% of 2000), and the second standing in it
	CHECK(lossOnSpikes(amulet(AmuletType::TrapWard, AmuletTier::Grand)) == 0);
	const int lesser = lossOnSpikes(amulet(AmuletType::TrapWard, AmuletTier::Lesser));
	CHECK(lesser < bare * 85 / 100);
	CHECK(lesser > bare * 60 / 100);
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
	world.player.stats.AddXP(200000, world.events); // the player of the scorpion nest (plan monster-balance)
	const int full = world.Hp();
	REQUIRE(world.Walk(1.5f));
	world.Wait(5000);
	CHECK(world.player.stats.poison.Any());
	CHECK(world.Hp() < full);
	world.player.god = true;
	int swings = 0;
	for (; swings < 30 && world.Nearest() > 0; swings++) {
		world.Attack();
		world.Wait(1000);
	}
	CHECK(world.Nearest() == 0);
	// Weak to blows: the club's mostly blunt hit counts more than its 10.
	CHECK(swings < (monsterKind(MonsterScorpion)->maxHealth + 9) / 10);
}

TEST_CASE("the bow's arrow lands on the floor ahead; facing the plant it hits, arrows kill it") {
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
	// Tough on pierce, 2 an arrow; out of shots, it waits for one to come back (plan ranged-weaker-than-melee).
	for (int i = 0; i < 60 && world.Xp() == xp; i++)
		world.Wait(world.Attack() ? 1400 : SHOT_RECOVERY_MS);
	CHECK(world.Nearest() < plant);
	CHECK(world.Xp() > xp);
}

TEST_CASE("each ranged weapon hits the plant from inside its range") {
	struct Shot {
		ItemKind weapon;
		float walk; // tiles right of the start
	};
	for (const Shot& s :
		 {Shot{ItemKind::CompositeBow, 10.f},
		  {ItemKind::Sling, 11.f},
		  {ItemKind::ThrowingStick, 11.3f}, // past its minimum range (plan ranged-weaker-than-melee)
		  {ItemKind::Javelin, 11.f}}) {
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
		toughen(world);
		REQUIRE(world.Load("tests/levels/spikes_one"));
		world.WaitTicks(5);
		world.Walk(3.5f);
		world.Wait(1000);
		CHECK_FALSE(world.player.stats.poison.Any());
	}
	SimWorld world;
	toughen(world);
	REQUIRE(world.Load("tests/levels/death_one"));
	world.WaitTicks(5);
	world.Wear(amulet(AmuletType::TrapWard, AmuletTier::Lesser));
	world.Walk(3.5f);
	world.Wait(1000);
	CHECK(world.player.Alive());
	CHECK(world.player.stats.poison.Running(PoisonTier::Strong));
	CHECK(world.Hp() < 2000);
}
