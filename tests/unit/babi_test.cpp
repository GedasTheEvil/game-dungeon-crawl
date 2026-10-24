// Babi (plan babi-boss): the baboon's boss. The baboon's kiting and throw, stronger: a wider band, a quicker retreat
// and a leap over the gap behind him; volleys of two or three coals, every third throw a big coal that sets the floor
// burning; baboons climb out round him; his death opens the boss gates.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
const Monster* find(const SimWorld& world, int type) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active() && world.dungeon.Monsters()[i].Type()->id == type)
			return &world.dungeon.Monsters()[i];
	return nullptr;
}

// The level loaded, Babi appeared (as in view) on his spawn cell, the player strong enough to live through it.
const Monster* wake(SimWorld& world, const char* level, int col) {
	REQUIRE(world.Load(level));
	world.player.stats.AddXP(1000000, world.events);
	const auto row = static_cast<int>(std::floor(world.Y()));
	REQUIRE(world.dungeon.SpawnMonster(col, row));
	world.Tick();
	const Monster* babi = find(world, MonsterBabi);
	REQUIRE(babi != nullptr);
	return babi;
}

// Ticks until his next throw lets its shot go: the shots it threw (a volley's later ones still waiting).
int nextThrow(SimWorld& world, const Monster& babi) {
	for (int t = 0; t < 8000; t += UPDATE_TICK_MS) {
		const bool released = babi.ShotReleased();
		const int before = world.dungeon.ShotCount();
		world.Tick();
		if (!released && babi.ShotReleased())
			return world.dungeon.ShotCount() - before;
	}
	FAIL("no throw");
	return 0;
}
} // namespace

TEST_CASE("Babi is the baboon's boss: farther, wider, quicker, a leaper, volleys and the big coal, baboons dig out") {
	const MonsterKind& babi = *monsterKind(MonsterBabi);
	const MonsterKind& baboon = *monsterKind(MonsterBaboon);
	CHECK(babi.isBoss());
	CHECK(babi.kin == MonsterBaboon);
	CHECK(babi.boss.minion == MonsterBaboon);
	CHECK(babi.boss.summon == Summon::DigOut);
	CHECK(babi.locomotion == Locomotion::WalkJump);
	CHECK(babi.climbs);
	CHECK(babi.scale == doctest::Approx(2.f * baboon.scale));
	REQUIRE(babi.ranged.has_value());
	REQUIRE(babi.kite.has_value());
	CHECK(babi.ranged->range > baboon.ranged->range);
	CHECK(babi.ranged->cooldownMs < baboon.ranged->cooldownMs);
	CHECK(babi.ranged->burns());
	CHECK(babi.kite->far <= babi.ranged->range);
	CHECK(babi.kite->far - babi.kite->near > baboon.kite->far - baboon.kite->near);
	CHECK(babi.kite->backOffSpeed > baboon.kite->backOffSpeed);
	CHECK(babi.kite->backOffSpeed < 57.f); // a sprint still catches him
	REQUIRE(babi.ranged->volley.has_value());
	CHECK(babi.ranged->volley->minCount == 2);
	CHECK(babi.ranged->volley->maxCount == 3);
	CHECK(babi.ranged->volley->bigEvery == 3);
	CHECK(babi.ranged->volley->bigDamage > babi.ranged->damage);
	CHECK(babi.resist[FIRE] < NORMAL); // resists fire, not immune
	CHECK(babi.resist[FIRE] > 0);
	CHECK(babi.resist[static_cast<size_t>(DamageType::Pierce)] == NORMAL); // the baboon's weakness gone
	CHECK(&ClipFilesOf(babi) == &THROWER_CLIPS);
}

TEST_CASE("he throws volleys of two or three coals, every third throw one big coal") {
	SimWorld world;
	world.player.god = true;
	const Monster* babi = wake(world, "tests/levels/babi_hall", 6);
	CHECK(world.dungeon.LivingMinions() >= monsterKind(MonsterBabi)->boss.minAlive);
	const Monster* baboon = find(world, MonsterBaboon);
	REQUIRE(baboon != nullptr);
	CHECK(baboon->Minion());
	CHECK(baboon->SummonedBy() == Summon::DigOut);
	for (int k = 1; k <= 6; k++) {
		const int shots = nextThrow(world, *babi);
		if (k % 3 == 0) {
			CHECK(shots == 1);
			CHECK(world.dungeon.ShotCount(true) >= 1);
		} else {
			CHECK(shots >= 2);
			CHECK(shots <= 3);
		}
	}
	CHECK(world.Saw(MonsterBabi, CreatureMove::AttackRanged));
	CHECK(world.Saw(MonsterBabi, CreatureMove::Blaze));
	CHECK(world.Saw(MonsterBabi, CreatureMove::Summon));
}

TEST_CASE("the big coal sets the floor burning where it lands, at the player's feet") {
	SimWorld world;
	world.player.god = true;
	const Monster* babi = wake(world, "tests/levels/babi_hall", 6);
	const auto row = static_cast<int>(std::floor(world.Y()));
	for (int k = 0; k < 3; k++)
		nextThrow(world, *babi);
	REQUIRE(world.dungeon.ShotCount(true) == 1);
	bool blazed = false;
	for (int t = 0; t < 3000 && !blazed; t += UPDATE_TICK_MS) {
		world.Tick();
		for (int col = 1; col < static_cast<int>(std::floor(babi->Left())); col++)
			blazed = blazed || world.dungeon.BlazeAt(col, row);
	}
	CHECK(blazed);
	CHECK(world.dungeon.PlayerInBlaze()); // the player stood still: it burst at their feet
	world.Wait(monsterKind(MonsterBabi)->ranged->volley->blazeMs + 100);
	CHECK_FALSE(world.dungeon.PlayerInBlaze());
}

TEST_CASE("a burning floor hurts the player standing in it and sets them burning; a jump clears it; not in water") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/babi_hall"));
	world.player.stats.AddXP(1000000, world.events);
	const auto row = static_cast<int>(std::floor(world.Y()));
	REQUIRE_FALSE(world.dungeon.PlaceBlaze(3, row + 1, 4000)); // rock
	REQUIRE(world.dungeon.PlaceBlaze(2, row, 4000));
	const int hp = world.Hp();
	world.Tick();
	CHECK(world.dungeon.PlayerInBlaze());
	world.Wait(1000);
	CHECK(world.Hp() < hp);
	CHECK(world.player.burning.Any());
	CHECK(world.Told("The floor is burning under your feet"));
	world.Jump();
	world.Tick();
	CHECK_FALSE(world.dungeon.PlayerInBlaze());

	REQUIRE(world.Load("tests/levels/baboon_water"));
	CHECK_FALSE(world.dungeon.PlaceBlaze(2, static_cast<int>(std::floor(world.Y())), 4000)); // half water
}

TEST_CASE("walked at, he backs off to the edge of his ledge and leaps the gap behind him, staying on his row") {
	SimWorld world;
	world.player.god = true;
	const Monster* babi = wake(world, "tests/levels/babi_ledge", 7);
	const int row = babi->Row();
	bool leapt = false;
	for (int t = 0; t < 8000 && !leapt; t += UPDATE_TICK_MS) {
		world.Step(1);
		world.Tick();
		leapt = babi->jumping();
	}
	REQUIRE(leapt);
	world.Wait(MONSTER_JUMP_MS + 100);
	CHECK(babi->CentreX() > 11.f); // over the gap (col 10), on the far ledge
	CHECK(babi->Row() == row);
	CHECK(babi->Alive());
	CHECK(world.Saw(MonsterBabi, CreatureMove::LeapAway));
}

TEST_CASE("in his arena: the teleporter in, his death opens the boss gate") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/babi"));
	REQUIRE(world.WalkTo(4.5f));
	world.Interact();
	world.WaitTicks(SimWorld::GATE_TRAVEL_TICKS);
	const auto row = static_cast<int>(std::floor(world.Y()));
	REQUIRE(row == 3);
	REQUIRE(world.dungeon.SpawnMonster(20, row));
	world.Wait(2000);
	CHECK(world.dungeon.BossHealth() == monsterKind(MonsterBabi)->maxHealth);
	world.dungeon.HurtBoss(world.dungeon.BossHealth());
	world.WaitTicks(5);
	CHECK(world.dungeon.BossHealth() == 0);
	CHECK(world.Told("The guardian is slain!\nThe boss gate grinds open"));
}
