// Stun (plan skill-stun): with the skill, a weapon hit stuns the monster at 25% of its blunt share for 1.5 s: it stops
// moving and attacking; a flyer drops to the floor. A boss for half the time, and not again within 5 s.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
// tests/levels/fire, the mummy woken and come up to the player, who has the Stun skill (skill).
const Monster* readyMummy(SimWorld& world, bool skill) {
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/fire"));
	if (skill) {
		world.player.stats.heka.Earn(5);
		REQUIRE(world.player.stats.heka.Learn(SkillId::Stun));
	}
	REQUIRE(world.WalkTo(12.f)); // it wakes and climbs out
	REQUIRE(world.WalkTo(9.5f));
	world.Wait(3000);
	const Monster* mummy = world.dungeon.NearestMonster();
	REQUIRE(mummy != nullptr);
	REQUIRE_FALSE(mummy->Rising());
	REQUIRE(world.WalkTo(mummy->Left() - world.player.HalfWidth() - 0.1f)); // up to it, facing it
	return mummy;
}

// Hits of no damage with mix (it must live through them), each after the last stun is over: how many stunned.
int stunsIn(SimWorld& world, const Monster& mon, const DamageMix& mix, int hits) {
	int stuns = 0;
	for (int i = 0; i < hits; i++) {
		// It walks round the player in the meantime: the blow goes its way.
		REQUIRE(world.dungeon.AttackNearest(0, mix, 3.f, mon.CentreX() > world.X() ? 1 : -1));
		stuns += mon.Reeling() ? 1 : 0;
		world.Wait(STUN_MS + 2 * UPDATE_TICK_MS);
	}
	return stuns;
}
} // namespace

TEST_CASE("stun: a blunt hit stuns at 25% of its blunt share; a slash never; no skill, never") {
	SimWorld world;
	const Monster* mummy = readyMummy(world, true);
	// 440 HP of the mummy's 480 at most (a hit deals at least 1, a cut 2: it is weak to slash).
	const int maces = stunsIn(world, *mummy, {100, 0, 0}, 160); // 25%: 40
	CHECK(maces >= 25);
	CHECK(maces <= 55);
	const int halves = stunsIn(world, *mummy, {50, 50, 0}, 160); // 12%: 20
	CHECK(halves >= 8);
	CHECK(halves < maces);
	CHECK(stunsIn(world, *mummy, {0, 100, 0}, 60) == 0); // the khopesh's cut
	CHECK(world.Told("Mummy is stunned"));
	CHECK(std::count_if(world.said.begin(), world.said.end(), [](const WorldEvent& e) {
			  return e.kind == WorldEvent::Kind::Note && e.note == FieldNote::Stun;
		  }) >= 1);

	SimWorld unskilled;
	const Monster* other = readyMummy(unskilled, false);
	CHECK(stunsIn(unskilled, *other, {100, 0, 0}, 60) == 0);
}

TEST_CASE("stun: the monster stands and does not attack while stunned, then goes on") {
	SimWorld world;
	const Monster* mummy = readyMummy(world, true);
	REQUIRE(world.dungeon.StunNearest());
	const float x = mummy->CentreX();
	bool bit = false;
	for (int t = 0; t < STUN_MS - UPDATE_TICK_MS; t += UPDATE_TICK_MS) {
		world.Tick();
		bit = bit || mummy->Biting();
	}
	CHECK(mummy->Reeling());
	CHECK(mummy->CentreX() == doctest::Approx(x));
	CHECK_FALSE(bit);
	for (int t = 0; t < 3000; t += UPDATE_TICK_MS) {
		world.Tick();
		bit = bit || mummy->Biting();
	}
	CHECK_FALSE(mummy->Reeling());
	CHECK(bit); // it bites again
}

TEST_CASE("stun: a flyer drops to the floor, then takes off again") {
	SimWorld world; // tests/levels/giant_bat: the giant bat on the ceiling at col 6, the player at col 2
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/giant_bat"));
	REQUIRE(world.dungeon.SpawnMonster(6, static_cast<int>(std::floor(world.Y()))));
	world.WaitTicks(5);
	const Monster* bat = world.dungeon.NearestMonster();
	REQUIRE(bat != nullptr);
	REQUIRE(bat->Roosting());
	const float roost = bat->DrawnLift();
	REQUIRE(roost > 10.f);
	REQUIRE(world.dungeon.StunNearest());
	world.Wait(1000);
	CHECK(bat->Reeling());
	CHECK(bat->DrawnLift() == doctest::Approx(0.f)); // on the floor: a sword reaches it
	world.Wait(3000);
	CHECK_FALSE(bat->Reeling());
	CHECK(bat->DrawnLift() > roost / 2.f); // back up
}

TEST_CASE("stun: a boss for half the time, and not again within 5 s") {
	SimWorld world; // tests/levels/shezmu: the Bat of Shezmu on the ceiling at col 8, the player at col 2
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/shezmu"));
	REQUIRE(world.dungeon.SpawnMonster(8, static_cast<int>(std::floor(world.Y()))));
	world.WaitTicks(5);
	const Monster* boss = world.dungeon.NearestMonster();
	REQUIRE(boss != nullptr);
	REQUIRE(boss->Type()->isBoss());
	REQUIRE(world.dungeon.StunNearest());
	world.Wait(STUN_MS * BOSS_STUN_PERCENT / 100 - 2 * UPDATE_TICK_MS);
	CHECK(boss->Reeling());
	world.Wait(4 * UPDATE_TICK_MS);
	CHECK_FALSE(boss->Reeling()); // half the time
	world.Wait(1000);
	CHECK_FALSE(world.dungeon.StunNearest()); // the cooldown
	CHECK_FALSE(boss->Reeling());
	world.Wait(BOSS_STUN_COOLDOWN_MS);
	CHECK(world.dungeon.StunNearest());
}

TEST_CASE("stun: a lurker cannot be stunned until awake") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/fire"));
	REQUIRE(world.dungeon.SpawnMonster(13, static_cast<int>(std::floor(world.Y()))));
	world.WaitTicks(5);
	const Monster* mummy = world.dungeon.NearestMonster();
	REQUIRE(mummy != nullptr);
	REQUIRE(mummy->lurking());
	CHECK_FALSE(world.dungeon.StunNearest());
	CHECK_FALSE(mummy->Reeling());
}

TEST_CASE("stun: on the play-test level a club stuns the scorpion now and then before it dies") {
	// tests/levels/stun_play: the scorpion at col 6. Seven or so blows that do not kill it, at 21% each: five runs. The
	// seeds far apart: seed n + 1's stream is seed n's one draw on (Rng), so near seeds roll nearly the same.
	int stuns = 0;
	for (uint64_t seed = 1000; seed <= 5000; seed += 1000) {
		SimWorld world;
		world.player.god = true;
		REQUIRE(world.Load("tests/levels/stun_play", seed));
		world.player.stats.heka.Earn(5);
		REQUIRE(world.player.stats.heka.Learn(SkillId::Stun));
		world.Equip(ItemKind::Club);
		REQUIRE(world.WalkTo(4.f));
		world.Wait(1500); // it comes up
		const Monster* scorpion = world.dungeon.NearestMonster();
		REQUIRE(scorpion != nullptr);
		REQUIRE(scorpion->Type()->id == MonsterScorpion);
		const int before = stuns;
		for (int hits = 0; hits < 30 && scorpion->Alive(); hits++) {
			world.facing = scorpion->CentreX() > world.X() ? 1 : -1;
			REQUIRE(world.Attack());
			bool stunned = false;
			for (int t = 0; t < 1500; t += UPDATE_TICK_MS) {
				world.Tick();
				stunned = stunned || scorpion->Reeling();
			}
			stuns += stunned ? 1 : 0;
		}
		CHECK(world.Told("is stunned") == (stuns > before));
	}
	CHECK(stuns >= 2);
}
