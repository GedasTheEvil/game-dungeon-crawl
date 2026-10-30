// A hit wakes the monster (plan hit-wakes-monster): hit by the player from afar, it comes for them where it can, runs
// from them where it cannot; a rooted one stays.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
constexpr DamageMix ARROW = {0, 0, 100, 0};

// The rat a reckless walker that does not climb: it would never run as a coward, nor find a way up.
const MonsterTypes& recklessTypes() {
	static const MonsterTypes types = [] {
		MonsterTypes t = SimWorld::monsterTypes();
		t[MonsterRat].courage = Courage::Reckless;
		t[MonsterRat].climbs = false;
		return t;
	}();
	return types;
}

const Monster* find(const SimWorld& world, int kind) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active() && world.dungeon.Monsters()[i].Type()->id == kind)
			return &world.dungeon.Monsters()[i];
	return nullptr;
}

// Ticks for ms.
void run(SimWorld& world, int ms) {
	for (int t = 0; t < ms; t += UPDATE_TICK_MS)
		world.Tick();
}
} // namespace

TEST_CASE("a roosting bat hit from beyond its sight leaves the roost for the player") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/hit_wakes_bat"));
	REQUIRE(world.dungeon.SpawnMonster(4, static_cast<int>(std::floor(world.Y()))));
	world.Tick();
	const Monster* bat = find(world, MonsterBat);
	REQUIRE(bat != nullptr);
	const float roost = bat->CentreX();
	REQUIRE(world.X() - roost > BAT_SIGHT + 5.f);
	run(world, 1000);
	CHECK(bat->Roosting()); // it has not seen the player

	REQUIRE(world.dungeon.AttackNearest(1, ARROW, 12.f, -1, 0.f));
	CHECK(bat->Provoked());
	run(world, 2500);
	CHECK_FALSE(bat->Roosting());
	CHECK(bat->CentreX() > roost + 2.f); // on its way to the player
}

TEST_CASE("a walker hit by the player, then left with no way to them, runs off; a rooted one stays") {
	SimWorld world;
	world.player.god = true;
	world.dungeon.Link(
		{&world.player,
		 &world.journal,
		 &world.items,
		 &world.random,
		 nullptr,
		 &world.events,
		 &recklessTypes(),
		 &world.inscriptions,
		 &world.names}
	);
	REQUIRE(world.Load("tests/levels/hit_wakes_ledge"));
	const auto row = static_cast<int>(std::floor(world.Y()));
	REQUIRE(world.dungeon.SpawnMonster(6, row));
	REQUIRE(world.dungeon.SpawnMonster(3, row));
	world.Tick();
	const Monster* rat = find(world, MonsterRat);
	const Monster* plant = find(world, MonsterPlant);
	REQUIRE(rat != nullptr);
	REQUIRE(plant != nullptr);

	// Not hit: on the ledge out of its reach, the rat stays where it is.
	REQUIRE(world.dungeon.PlacePlayer(8, row + 2));
	const float before = rat->CentreX();
	run(world, 2000);
	CHECK(rat->CentreX() == doctest::Approx(before));

	// Hit from the floor (the plant first, nearer), then the player is up on the ledge again over the rat.
	REQUIRE(world.dungeon.PlacePlayer(14, row));
	REQUIRE(world.dungeon.AttackNearest(1, ARROW, 12.f, -1, 0.f));
	REQUIRE(rat->Provoked());
	REQUIRE(world.dungeon.PlacePlayer(1, row));
	REQUIRE(world.dungeon.AttackNearest(1, ARROW, 12.f, 1, 0.f));
	REQUIRE(plant->Provoked());
	const float plantAt = plant->CentreX();
	REQUIRE(world.dungeon.PlacePlayer(static_cast<int>(rat->CentreX()), row + 2));
	const float under = rat->CentreX();
	run(world, 3000);
	CHECK(rat->FleeDir() != 0);
	CHECK(std::fabs(rat->CentreX() - under) > 1.f); // on its way out from under the player
	CHECK(plant->CentreX() == doctest::Approx(plantAt));
}
