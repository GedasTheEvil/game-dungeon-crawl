// The walk clip keeps pace with the walk (plan walk-clip-stride): one stride of the model per round of the clip.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include <map>
#include <string>

TEST_CASE("every walker has a stride, the same for every kind on its model; a flyer has none") {
	std::map<std::string, float> strides;
	for (int id = 1; id <= MONSTER_TYPE_MAX; id++) {
		const MonsterKind& kind = *monsterKind(id);
		CAPTURE(kind.name);
		if (kind.locomotion == Locomotion::Fly || kind.speed <= 0.f) {
			CHECK(kind.walkStride == 0.f);
			continue;
		}
		CHECK(kind.walkStride > 0.f);
		const auto [it, first] = strides.emplace(kind.model, kind.walkStride);
		CHECK(it->second == kind.walkStride);
	}
}

TEST_CASE("a scorpion's legs slow down with its walk in half water") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/walk_stride"));
	world.Tick();
	const Monster* scorpion = nullptr;
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active() && world.dungeon.Monsters()[i].Type()->id == MonsterScorpion)
			scorpion = &world.dungeon.Monsters()[i];
	REQUIRE(scorpion != nullptr);
	float wading = 0.f, walking = 0.f;
	for (int t = 0; t < 12000; t += UPDATE_TICK_MS) { // the last pace in the water, the last on the floor
		world.Tick();
		if (scorpion->State() == ModelState::Move)
			(scorpion->InWater() ? wading : walking) = scorpion->WalkClipSpeed();
	}
	REQUIRE(wading > 0.f);
	REQUIRE(walking > 0.f);
	CHECK(wading == doctest::Approx(walking * WADE_SPEED_FACTOR));
	// On land: one stride per round, its stride in tiles at its drawn scale.
	const MonsterKind& kind = *scorpion->Type();
	const float stride = kind.walkStride * kind.scale / 40.f;
	const float rounds = scorpion->WalkStep() * 1000.f / MONSTER_STEP_PERIOD_MS / stride;
	CHECK(walking == doctest::Approx(scorpion->Type()->model.WalkSpeed(rounds)));
}
