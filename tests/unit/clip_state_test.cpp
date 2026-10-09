// The animation clip state of the monsters and the player, tick by tick (docs/plan/solved/scenarios-to-unit-tests.md
// technique 8): which clip each shows (ModelState) and its frame, on the sim harness with the real model files. The
// look of the poses stays in the scenarios (mummy, cobra, bats, ...).
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include <vector>

namespace {
std::vector<const Monster*> active(const SimWorld& world) {
	std::vector<const Monster*> list;
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active())
			list.push_back(&world.dungeon.Monsters()[i]);
	return list;
}

float frameOf(const Monster& mon) {
	const ModelState shown = mon.Type()->model.Shown(mon.State());
	return mon.Playback()[static_cast<int>(shown)].frame;
}
} // namespace

TEST_CASE("monsters of one type on one model walk out of step") {
	SimWorld world; // tests/levels/monsters: two worms beside the player's landing
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/monsters"));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(8.1f));
	REQUIRE(world.Climb(-5.9f));
	REQUIRE(world.Walk(-2.f));
	std::vector<const Monster*> worms;
	for (const Monster* mon : active(world))
		if (mon->Type()->id == MonsterWorm)
			worms.push_back(mon);
	REQUIRE(worms.size() >= 2);
	// One playback per monster: the clips' frames differ more often than not (in lockstep they never would).
	int apart = 0;
	for (int t = 0; t < 40; t++) {
		const bool same = worms[0]->State() == worms[1]->State() && frameOf(*worms[0]) == frameOf(*worms[1]);
		apart += same ? 0 : 1;
		world.WaitTicks(3);
	}
	CHECK(apart >= 20);
}

TEST_CASE("a mummy lies still in its coffin, climbs out once (the rise clip from its start), then walks") {
	SimWorld world; // tests/levels/mummy: the mummy at col 10
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/mummy"));
	world.player.stats.AddXP(200000, world.events);
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(8.8f));
	world.WaitTicks(10);
	const Monster* mummy = world.dungeon.NearestMonster();
	REQUIRE(mummy != nullptr);
	CHECK(mummy->State() == ModelState::Idle);
	CHECK(mummy->lurking());
	REQUIRE(world.WalkTo(9.1f));
	world.WaitTicks(2);
	CHECK(mummy->State() == ModelState::Rise);
	CHECK(mummy->Rising());
	float last = -1.f;
	int ticks = 0;
	while (mummy->Rising() && ticks++ < 500) {
		const float frame = frameOf(*mummy);
		CHECK(frame >= last); // plays forward, once
		last = frame;
		world.Tick();
	}
	CHECK_FALSE(mummy->Rising());
	CHECK(mummy->Type()->model.Finished(ModelState::Rise, mummy->Playback()));
	world.Wait(1000);
	CHECK(mummy->State() != ModelState::Rise); // it walks (or strikes)
	CHECK(mummy->State() != ModelState::Idle);
}

TEST_CASE("a killed monster plays its die clip from the start and holds the last frame") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/hitbox_rat"));
	REQUIRE(world.Walk(1.f));
	world.Wait(8000);
	const Monster* rat = world.dungeon.NearestMonster();
	REQUIRE(rat != nullptr);
	CHECK(rat->State() == ModelState::Attack); // biting the player
	world.dungeon.AttackNearest(1000, {100, 0, 0}, 1.f, 1);
	CHECK_FALSE(rat->Alive());
	CHECK(rat->State() == ModelState::Die);
	CHECK(frameOf(*rat) == 0.f);
	world.Wait(5000);
	CHECK(rat->State() == ModelState::Die);
	CHECK(rat->Type()->model.Finished(ModelState::Die, rat->Playback()));
	const float held = frameOf(*rat);
	world.Wait(1000);
	CHECK(frameOf(*rat) == held);
}

TEST_CASE("the cobra lies coiled, rears up once, then spits its clip once and comes on") {
	SimWorld world; // tests/levels/cobra
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/cobra"));
	REQUIRE(world.Walk(3.f)); // it comes into view
	const Monster* cobra = world.dungeon.NearestMonster();
	REQUIRE(cobra != nullptr);
	CHECK(cobra->State() == ModelState::Idle);
	REQUIRE(world.Walk(2.f));
	std::vector<ModelState> seen{ModelState::Idle};
	for (int t = 0; t < 250; t++) {
		if (seen.empty() || seen.back() != cobra->State())
			seen.push_back(cobra->State());
		world.Tick();
	}
	// Idle, then the rise, the spit, and its walk or bite after: in that order.
	auto at = [&](ModelState s) {
		for (size_t i = 0; i < seen.size(); i++)
			if (seen[i] == s)
				return static_cast<int>(i);
		return -1;
	};
	CHECK(at(ModelState::Idle) == 0);
	CHECK(at(ModelState::Rise) > at(ModelState::Idle));
	CHECK(at(ModelState::Spit) > at(ModelState::Rise));
}

TEST_CASE("a bat hangs on its roost clip until it swoops, then flies") {
	SimWorld world; // tests/levels/bats
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/bats"));
	world.WaitTicks(5);
	const Monster* bat = world.dungeon.NearestMonster();
	REQUIRE(bat != nullptr);
	CHECK(bat->State() == ModelState::Idle);
	CHECK(bat->Roosting());
	REQUIRE(world.Walk(3.85f));
	world.WaitTicks(10);
	CHECK_FALSE(bat->Roosting());
	CHECK((bat->State() == ModelState::Move || bat->State() == ModelState::Attack));
}

TEST_CASE("the player stands, walks, jumps once, climbs with the height and holds the die clip") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/classic2"));
	world.WaitTicks(5);
	CHECK(world.player.State() == ModelState::Idle);
	world.player.setModelState(ModelState::Jump); // the game loop's pose while jumping
	CHECK(world.player.Playback()[static_cast<int>(ModelState::Jump)].frame == 0.f);
	world.Wait(3000);
	CHECK(world.player.Model().Finished(ModelState::Jump, world.player.Playback()));

	const int frames = world.player.Model().Clip(ModelState::Climb).frames;
	REQUIRE(frames > 1);
	world.player.showClimb(0.f);
	CHECK(world.player.climbing());
	CHECK(world.player.Playback()[static_cast<int>(ModelState::Climb)].frame == 0.f);
	world.player.showClimb(0.5f);
	CHECK(world.player.Playback()[static_cast<int>(ModelState::Climb)].frame ==
		  doctest::Approx(static_cast<float>(frames) / 2.f));
	world.player.showClimb(1.25f); // one cycle per tile climbed
	CHECK(world.player.Playback()[static_cast<int>(ModelState::Climb)].frame ==
		  doctest::Approx(static_cast<float>(frames) / 4.f));

	world.player.stats.LoseHP(1000);
	world.Tick();
	CHECK(world.player.State() == ModelState::Die);
	world.Wait(5000);
	CHECK(world.player.Model().Finished(ModelState::Die, world.player.Playback()));
	world.player.Reanimate();
	CHECK(world.player.State() == ModelState::Idle);
}
