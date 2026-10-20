// Holds (plans crocodile-hold-bite, giant-scorpion-grab): a bite that grabs. The jaws or claws open wide (the tell),
// the lunge, then the hold: the player cannot walk, jump or drink; the crocodile's death roll ticks, the giant
// scorpion's hold ends in a sure sting unless the player's hits break it.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
const Monster* find(const SimWorld& world, int type) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active() && world.dungeon.Monsters()[i].Type()->id == type)
			return &world.dungeon.Monsters()[i];
	return nullptr;
}

// Ticks until done() or ms pass; done() then.
template <typename Done> bool waitFor(SimWorld& world, Done done, int ms) {
	for (int t = 0; t < ms && !done(); t += UPDATE_TICK_MS)
		world.Tick();
	return done();
}

// tests/levels/crocodile: the crocodile in col 8 of a pool (cols 6-13); the player wades in to col 7, in its wake
// range, tough enough to live through its bites. The crocodile is in its tell when this returns.
const Monster* crocodileTell(SimWorld& world) {
	REQUIRE(world.Load("tests/levels/crocodile"));
	world.player.stats.AddXP(1000000, world.events);
	REQUIRE(world.WalkTo(7.2f));
	REQUIRE(world.dungeon.PlayerWading());
	const Monster* c = find(world, MonsterCrocodile);
	REQUIRE(c != nullptr);
	REQUIRE(waitFor(world, [&] { return c->Grabbing(); }, 60000));
	return c;
}

// tests/levels/giant_scorpion: the giant scorpion in col 6, the player in col 2 (god mode: the poison is still laid).
// The scorpion holds the player when this returns, their poison cured.
const Monster* scorpionHold(SimWorld& world) {
	REQUIRE(world.Load("tests/levels/giant_scorpion"));
	world.player.god = true;
	world.Tick(); // it appears
	const Monster* s = find(world, MonsterGiantScorpion);
	REQUIRE(s != nullptr);
	REQUIRE(waitFor(world, [&] { return s->Holding(); }, 60000));
	world.player.stats.poison.Cure();
	return s;
}
} // namespace

TEST_CASE("in half water the crocodile's bite may grab: after the tell the player is held, then let go") {
	SimWorld world;
	const Monster* c = crocodileTell(world);
	CHECK_FALSE(c->Holding()); // the jaws gape, the player can still step back
	CHECK(world.dungeon.PlayerHeldBy() == nullptr);
	REQUIRE(waitFor(world, [&] { return c->Holding(); }, GRAB_TELL_MS + UPDATE_TICK_MS));
	CHECK(world.dungeon.PlayerHeldBy() == c);
	CHECK(world.Saw(MonsterCrocodile, CreatureMove::Grab));
	CHECK(world.Told("Caught in its jaws"));

	const float x = world.X();
	const int hp = world.Hp();
	CHECK_FALSE(world.Step(1));
	CHECK(world.X() == x);
	world.Jump();
	world.Tick();
	CHECK_FALSE(world.player.jump.jumping);
	CHECK_FALSE(world.Attack()); // no attack in its jaws

	const HoldRules rules = c->Hold();
	world.Wait(rules.holdMs);
	CHECK_FALSE(c->Holding());
	CHECK(world.dungeon.PlayerHeldBy() == nullptr);
	CHECK(hp - world.Hp() >= rules.holdMs / rules.tickMs); // the death roll's ticks, at least 1 HP each
	CHECK(world.Step(-1)); // free
}

TEST_CASE("stepping back out of reach in the tell makes the crocodile's lunge miss") {
	SimWorld world;
	const Monster* c = crocodileTell(world);
	for (int t = 0; t <= GRAB_TELL_MS; t += UPDATE_TICK_MS) {
		world.Step(-1);
		world.Tick();
		REQUIRE_FALSE(c->Holding());
	}
	CHECK_FALSE(c->Grabbing());
	CHECK_FALSE(world.Saw(MonsterCrocodile, CreatureMove::Grab));
}

TEST_CASE("on dry land the crocodile bites but never grabs") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/crocodile"));
	world.player.stats.AddXP(1000000, world.events);
	REQUIRE(world.WalkTo(7.2f)); // it wakes
	const Monster* c = find(world, MonsterCrocodile);
	REQUIRE(c != nullptr);
	REQUIRE(waitFor(world, [&] { return c->Alerted(); }, 1000));
	world.player.god = true; // back out of the pool, whatever it does on the way
	REQUIRE(world.WalkTo(3.f));
	REQUIRE_FALSE(world.dungeon.PlayerWading());
	REQUIRE(waitFor(world, [&] { return world.dungeon.PlayerHeldBy() == nullptr; }, 4000));
	world.player.god = false;
	const int hp = world.Hp();
	for (int t = 0; t < 30000; t += UPDATE_TICK_MS) {
		world.Tick();
		REQUIRE_FALSE(c->Grabbing());
	}
	CHECK(world.Hp() < hp);
}

TEST_CASE("a jump over the giant scorpion in its tell makes the lunge miss (plan monster-attack-reach)") {
	SimWorld world; // tests/levels/giant_scorpion: the scorpion comes from the right
	REQUIRE(world.Load("tests/levels/giant_scorpion"));
	world.player.god = true;
	world.Tick();
	const Monster* s = find(world, MonsterGiantScorpion);
	REQUIRE(s != nullptr);
	REQUIRE(waitFor(world, [&] { return s->Grabbing(); }, 60000));
	world.facing = 1;
	world.Jump();
	REQUIRE(waitFor(world, [&] { return !s->Grabbing() || s->Holding(); }, GRAB_TELL_MS + 100));
	CHECK_FALSE(s->Holding());
	CHECK_FALSE(world.Saw(MonsterGiantScorpion, CreatureMove::Grab));
}

TEST_CASE("the giant scorpion's hold allows the attack and ends in a sure, poisoned sting") {
	SimWorld world;
	const Monster* s = scorpionHold(world);
	CHECK(world.Saw(MonsterGiantScorpion, CreatureMove::Grab));
	CHECK(world.Told("Caught in its claws"));
	CHECK(world.dungeon.AttackAllowed(false));
	CHECK_FALSE(world.Step(1));
	world.Wait(s->Hold().holdMs);
	CHECK_FALSE(s->Holding());
	CHECK(world.player.stats.poison.Running(PoisonTier::Medium));
}

TEST_CASE("a hit late in the giant scorpion's hold breaks it: no sting") {
	SimWorld world;
	const Monster* s = scorpionHold(world);
	const HoldRules rules = s->Hold();
	const int hitMs = weaponDef(world.items.Equipped()).motion.hitMs;
	world.facing = s->CentreX() > world.X() ? 1 : -1;
	world.Wait(rules.holdMs - rules.breakMs / 2 - hitMs); // the hit lands in the last breakMs: free at once
	REQUIRE(world.Attack());
	REQUIRE(waitFor(world, [&] { return !s->Holding(); }, hitMs + 2 * UPDATE_TICK_MS));
	CHECK(world.Told("You break free"));
	CHECK(s->Alive());
	CHECK_FALSE(world.player.stats.poison.Any());
}

TEST_CASE("the bosses hold harder than their kin: Sobek more often and longer, the queen longer and harder to break") {
	const HoldRules& crocodile = *monsterKind(MonsterCrocodile)->hold;
	const HoldRules& sobek = *monsterKind(MonsterSobek)->hold;
	CHECK(sobek.chancePercent > crocodile.chancePercent);
	CHECK(sobek.holdMs / sobek.tickMs == 4);
	CHECK(crocodile.holdMs / crocodile.tickMs == 3);
	CHECK(sobek.inWater);
	const HoldRules& giant = *monsterKind(MonsterGiantScorpion)->hold;
	const HoldRules& queen = *monsterKind(MonsterScorpionQueen)->hold;
	CHECK(queen.holdMs > giant.holdMs);
	CHECK(queen.breakMs < giant.breakMs);
	CHECK(queen.sting);
	CHECK(monsterKind(MonsterScorpionQueen)->poison == PoisonTier::Strong);
}
