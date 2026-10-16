#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/world/decor.h"
#include "../../src/world/level_gen.h"

TEST_CASE("a walker stops at a wall: the rat behind it never bites") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/monster_walls"));
	world.Wait(15000);
	CHECK(world.Hp() == 50);
	CHECK(world.player.Alive());
}

TEST_CASE("a coward keeps out of an armed rock fall, a giant rat leaps over it") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/coward_rock"));
	world.player.stats.AddXP(1000, world.events);
	world.Wait(20000);
	CHECK(world.Hp() == 62);
	CHECK(world.dungeon.NearestMonsterHealth() == 12);

	REQUIRE(world.Load("tests/levels/coward_rock_leap"));
	world.Wait(6000);
	CHECK(world.Hp() < 62);
	CHECK(world.dungeon.NearestMonsterHealth() == 60);
}

namespace {
const Monster* firstMonster(const SimWorld& world) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active())
			return &world.dungeon.Monsters()[i];
	return nullptr;
}
} // namespace

TEST_CASE("a coward that cannot reach the player runs out of the bow's range") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/coward_flee_pit"));
	world.Tick();
	const Monster* rat = firstMonster(world);
	REQUIRE(rat != nullptr);
	const float from = rat->CentreX();
	world.Wait(1000);
	CHECK(rat->CentreX() < from);
	CHECK(rat->Facing() == -1); // runs away, its back to the player
	world.Wait(20000);
	const float gap = (world.X() - world.player.HalfWidth()) - rat->Right();
	CHECK(gap >= COWARD_SAFE_GAP);
	CHECK(gap < COWARD_SAFE_GAP + 0.1f); // no farther than it has to
	CHECK(rat->Facing() == 1); // stands, watching the player
	CHECK(world.Hp() == 50);
}

TEST_CASE("a fleeing coward comes back once it can reach the player") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/coward_flee_rock"));
	world.Wait(3000);
	const Monster* rat = firstMonster(world);
	REQUIRE(rat != nullptr);
	CHECK(rat->CentreX() < 7.f); // ran from the rock fall's edge
	const int hp = world.Hp();
	REQUIRE(world.WalkTo(7.5f)); // past the rock fall: nothing between them
	world.Wait(5000);
	CHECK(world.Hp() < hp);
}

TEST_CASE("the cobra rears up and spits from afar: its venom poisons the player") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/cobra"));
	REQUIRE(world.WalkTo(world.X() + 5.f));
	const int hp = world.Hp();
	for (int t = 0; t < 250 && !world.player.stats.poison.Any(); t++)
		world.Tick();
	CHECK(world.Saw(MonsterCobra, CreatureMove::Rear));
	CHECK(world.Saw(MonsterCobra, CreatureMove::AttackRanged));
	CHECK(world.player.stats.poison.Mask() == 2); // medium
	CHECK(world.Hp() < hp);
}

TEST_CASE("the Anubis shoots a bolt from afar, no poison, then hits up close") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/anubis_bolt"));
	world.player.stats.AddXP(1000000, world.events);
	const int hp = world.Hp();
	for (int t = 0; t < 1500 && world.Hp() >= hp; t++)
		world.Tick();
	REQUIRE(world.Saw(MonsterAnubis, CreatureMove::AttackRanged));
	CHECK(world.Hp() < hp);
	CHECK(world.player.burning.Any()); // a fire bolt (plan anubis-fire-bolt)
	CHECK(world.player.burning.LeftMs() <= BURN_MS);
	CHECK_FALSE(world.player.stats.poison.Any());
	CHECK_FALSE(world.Saw(MonsterAnubis, CreatureMove::Poison));
	const Monster* anubis = firstMonster(world);
	REQUIRE(anubis != nullptr);
	CHECK(anubis->Left() - (world.X() + world.player.HalfWidth()) > MONSTER_BITE_REACH); // hit from afar
	world.Wait(4000);
	CHECK(anubis->Left() - (world.X() + world.player.HalfWidth()) <= MONSTER_BITE_REACH); // came on to hit
}

TEST_CASE("the Anubis bolt is fire, its melee is not; the boss's bolt burns longer") {
	const MonsterKind& guard = *monsterKind(MonsterAnubis);
	const MonsterKind& boss = *monsterKind(MonsterAnubisBoss);
	REQUIRE(guard.ranged.has_value());
	REQUIRE(boss.ranged.has_value());
	CHECK(guard.ranged->burns());
	CHECK(boss.ranged->burns());
	CHECK(guard.attackMix[FIRE] == 0);
	CHECK(boss.attackMix[FIRE] == 0);
	CHECK(boss.ranged->burnMs > guard.ranged->burnMs);

	SimWorld world;
	REQUIRE(world.Load("tests/levels/anubis_bolt"));
	world.player.stats.AddXP(1000000, world.events); // lives through both hits
	world.player.TakeHit(boss.damage, boss.attackMix, world.events);
	CHECK_FALSE(world.player.burning.Any());
	world.player.TakeHit(boss.ranged->damage, *boss.ranged->mix, world.events, false, boss.ranged->burnMs);
	CHECK(world.player.burning.LeftMs() == boss.ranged->burnMs);
}

TEST_CASE("Sobek charges along the row through the player and is stunned by the wall") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/sobek"));
	REQUIRE(world.WalkTo(10.8f));
	bool stunned = false;
	for (int t = 0; t < 400 && !stunned; t++) {
		world.Tick();
		stunned = world.dungeon.Boss() != nullptr && world.dungeon.Boss()->Stunned();
	}
	CHECK(world.Saw(MonsterSobek, CreatureMove::Charge));
	CHECK(stunned);
	CHECK(world.dungeon.Boss()->CentreX() < 10.8f); // ran on past the player, into the west wall
}

TEST_CASE("Apep dives into the floor, out of reach, and comes up elsewhere on the row") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/apep"));
	REQUIRE(world.WalkTo(9.5f));
	bool hidden = false;
	float downAt = 0.f;
	for (int t = 0; t < 600 && !hidden; t++) {
		world.Tick();
		const Monster* apep = world.dungeon.Boss();
		hidden = apep != nullptr && apep->Hidden();
		if (hidden)
			downAt = apep->CentreX();
	}
	REQUIRE(hidden);
	CHECK(world.Saw(MonsterApep, CreatureMove::Burrow));
	const int hp = world.dungeon.BossHealth();
	world.dungeon.AttackNearest(1000, {100, 0, 0}, 30.f, downAt < world.X() ? -1 : 1); // a minion may take it
	CHECK(world.dungeon.BossHealth() == hp);
	for (int t = 0; t < 300 && world.dungeon.Boss()->Burrowing(); t++)
		world.Tick();
	CHECK_FALSE(world.dungeon.Boss()->Burrowing());
	CHECK(world.dungeon.Boss()->CentreX() != doctest::Approx(downAt));
}

TEST_CASE("a bat leaves its roost when the player comes near, swoops through and bites") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/bats"));
	world.Wait(100);
	CHECK_FALSE(world.Saw(MonsterBat, CreatureMove::Swoop));
	REQUIRE(world.WalkTo(world.X() + 3.85f));
	world.Wait(2000);
	CHECK(world.Saw(MonsterBat, CreatureMove::Swoop));
	CHECK(world.Hp() < 50);
}

// plan bats-high-ceiling, tests/levels/bats_high: the bat (col 8) roosts in the upper cell of a corridor two cells
// high; a teleporter (col 3) leads out of its sight.
TEST_CASE("a bat under a high ceiling dives to the player's row, bites, and falls to their melee") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/bats_high"));
	REQUIRE(world.WalkTo(4.5f)); // the bat spawns in view
	const Monster* bat = world.dungeon.NearestMonster();
	REQUIRE(bat != nullptr);
	const auto floor = static_cast<int>(world.Y());
	CHECK(bat->Row() == floor + 1);
	world.Equip(ItemKind::ShortSword);
	REQUIRE(world.WalkTo(7.f));
	world.Wait(1500);
	CHECK(world.Saw(MonsterBat, CreatureMove::Swoop));
	CHECK(bat->Row() == floor);
	CHECK(world.Hp() < 50);
	for (int t = 0; t < 10000 && bat->Alive(); t += 100) {
		world.Attack();
		world.Wait(100);
	}
	CHECK_FALSE(bat->Alive());
	world.Wait(1000);
	CHECK(bat->Row() == floor); // fell to the floor, not left hanging over the open cell
}

TEST_CASE("a bat under a high ceiling flies back up to its roost when the player leaves") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/bats_high"));
	REQUIRE(world.WalkTo(4.5f)); // the bat spawns in view
	const Monster* bat = world.dungeon.NearestMonster();
	REQUIRE(bat != nullptr);
	const int roost = bat->Row();
	REQUIRE(world.WalkTo(7.f));
	world.Wait(1000);
	CHECK(bat->Row() == roost - 1);
	REQUIRE(world.WalkTo(3.5f));
	world.Interact(); // the teleporter: out of its sight
	world.WaitTicks(SimWorld::GATE_TRAVEL_TICKS);
	REQUIRE(world.Y() > 4.f);
	world.Wait(5000);
	CHECK(bat->Alive());
	CHECK(bat->Row() == roost);
	CHECK(bat->Roosting());
	CHECK(bat->CentreX() == doctest::Approx(8.5f));
}

// tests/levels/bats_ladder: a ladder (col 6) up a shaft two cells wide, the bat (col 8) on the lower floor.
TEST_CASE("a bat follows the player up a ladder and bites them on the rungs") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/bats_ladder"));
	REQUIRE(world.WalkTo(4.5f)); // the bat spawns in view
	const Monster* bat = world.dungeon.NearestMonster();
	REQUIRE(bat != nullptr);
	const auto floor = static_cast<int>(world.Y());
	REQUIRE(world.WalkTo(7.f)); // wakes it
	world.Wait(300);
	REQUIRE(world.WalkTo(6.45f));
	REQUIRE(world.Climb(1.f));
	const int hp = world.Hp();
	world.Wait(3000);
	CHECK(bat->Row() == floor + 1);
	CHECK(world.Hp() < hp);
}

TEST_CASE("the water basin: things in half water are drawn down into it, eased in from a dry bank") {
	// tests/levels/water: dry floor at x 1-5, half water at x 5-14 on the player's row, dry again from 14.
	SimWorld world;
	REQUIRE(world.Load("tests/levels/water"));
	const Dungeon& d = world.dungeon;
	const auto row = static_cast<int>(world.Y());
	constexpr float DEPTH = RenderConfig::WATER_BASIN_DEPTH;
	constexpr float RAMP = RenderConfig::WATER_SINK_RAMP;
	CHECK(d.WaterSink(3.5f, row) == 0.f); // dry floor
	CHECK(d.WaterSink(5.f, row) == 0.f); // the bank's edge
	CHECK(d.WaterSink(5.f + RAMP / 2.f, row) == doctest::Approx(DEPTH / 2.f)); // half way down the ramp
	CHECK(d.WaterSink(5.f + RAMP, row) == doctest::Approx(DEPTH));
	CHECK(d.WaterSink(9.5f, row) == doctest::Approx(DEPTH)); // the middle of the pool
	CHECK(d.WaterSink(14.f - RAMP / 2.f, row) == doctest::Approx(DEPTH / 2.f)); // up the far bank
	CHECK(d.WaterSink(14.5f, row) == 0.f);
	CHECK(d.WaterSink(9.5f, row + 2) == 0.f); // the ladder's row above the water
	float last = 0.f;
	for (float x = 5.f; x <= 5.f + RAMP; x += 0.01f) {
		const float sink = d.WaterSink(x, row);
		CHECK(sink >= last);
		last = sink;
	}
}

TEST_CASE("half water halves the walk, and the spikes under its surface still hurt") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/water"));
	REQUIRE(world.WalkTo(2.f));
	world.HoldWalk(1, 1000);
	CHECK(world.X() == doctest::Approx(3.f).epsilon(0.04)); // WALK_SPEED, a tile a second
	REQUIRE(world.WalkTo(6.f));
	CHECK(world.dungeon.PlayerWading());
	world.HoldWalk(1, 1000);
	CHECK(world.X() == doctest::Approx(6.5f).epsilon(0.02));
	CHECK(world.dungeon.PlayerSink() == doctest::Approx(RenderConfig::WATER_BASIN_DEPTH));
	REQUIRE(world.WalkTo(11.5f));
	CHECK(world.Hp() < 50);
}

TEST_CASE("a level's decorations come from the tiers its depth unlocks: cave, worked tunnel, tomb, temple") {
	SimWorld world;
	Dungeon& d = world.dungeon;
	REQUIRE(d.LoadCampaignLevel(1));
	CHECK(d.DecorTierUsed() == 0);
	REQUIRE(d.LoadCampaignLevel(5));
	CHECK(d.DecorTierUsed() <= 1);
	REQUIRE(d.LoadCampaignLevel(9));
	CHECK(d.DecorTierUsed() >= 1);
	CHECK(d.DecorTierUsed() <= 2);
	REQUIRE(d.LoadCampaignLevel(20));
	CHECK(d.DecorTierUsed() == 3);
	// A generated level maps its difficulty to a depth (the scenario's `level gen:7:D`).
	for (const int difficulty : {1, 10}) {
		GenOptions options;
		options.seed = 7;
		options.difficulty = difficulty;
		const GenResult gen = generateLevel(options);
		REQUIRE(gen.ok);
		d.LoadGrid(gen.grid, difficulty == 1 ? "gen:7:1" : "gen:7:10", genDecorDepth(difficulty));
		CHECK(d.DecorTierUsed() == (difficulty == 1 ? 0 : 3));
	}
	// A test level outside the campaign gets every tier.
	REQUIRE(world.Load("tests/levels/statues29063"));
	CHECK(d.DecorTierUsed() == 3);
}

TEST_CASE("the blue lotus floats until the player wades near, rears up, stays rooted and its bite drains stamina") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/lotus"));
	REQUIRE(world.WalkTo(11.5f - FLOATING_WAKE_RANGE - 0.3f)); // the lotus is in col 11
	const Monster* lotus = firstMonster(world);
	REQUIRE(lotus != nullptr);
	const float x = lotus->CentreX();
	CHECK(x == doctest::Approx(11.5f));
	world.Wait(500);
	CHECK(lotus->lurking());
	CHECK(lotus->InWater());
	REQUIRE(world.WalkTo(x - FLOATING_WAKE_RANGE + 0.1f));
	world.Tick();
	CHECK_FALSE(lotus->lurking());
	CHECK(world.Saw(MonsterLotus, CreatureMove::Bloom));
	REQUIRE(world.WalkTo(lotus->Left() - world.player.HalfWidth() - ROOTED_BITE_REACH * 0.5f));
	int lowest = world.player.stats.Stamina(); // the first bite may land on the way in
	for (int t = 0; t < 120; t++) { // two bites, 0.9 s apart
		world.Tick();
		lowest = std::min(lowest, world.player.stats.Stamina());
	}
	CHECK(lowest <= world.player.stats.MaxStamina() / 2);
	CHECK(world.Saw(MonsterLotus, CreatureMove::Sedate));
	CHECK(lotus->CentreX() == doctest::Approx(x)); // rooted
}
