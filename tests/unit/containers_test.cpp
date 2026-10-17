// Treasure containers (plan treasure-containers): the look of each cell by depth, stable over loads and apart from
// the gameplay stream; a chest opens with a press, shows its item after the clip and gives it with the next press;
// what was opened stays so over a save and load.
#include "../../external/doctest/doctest.h"
#include "../../src/world/campaign.h"
#include "../../src/world/containers.h"
#include "../../src/world/decor_scatter.h"
#include "sim_world.h"
#include <algorithm>
#include <memory>

namespace {
constexpr int ROW = 2; // tests/levels/containers: the corridor's row
constexpr int CHEST_COL = 3;
constexpr int SECOND_COL = 5;
constexpr int WALL_COL = 21; // the right wall beside it

bool noted(const SimWorld& world, FieldNote note) {
	return std::any_of(world.said.begin(), world.said.end(),
					   [note](const WorldEvent& e) { return e.kind == WorldEvent::Kind::Note && e.note == note; });
}

bool sameLook(const TreasureLook& a, const TreasureLook& b) {
	return a.kind == b.kind && a.yaw == b.yaw && a.x == b.x && a.z == b.z;
}

void load(SimWorld& world, uint64_t seed = 1) {
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/containers", seed));
	world.WaitTicks(30);
}
} // namespace

TEST_CASE("the containers by depth: wooden chests in every tier, a third or more; each tier's shares add up") {
	for (int tier = 0; tier < DECOR_TIER_COUNT; tier++) {
		CAPTURE(tier);
		int total = 0;
		for (int k = 0; k < CONTAINER_COUNT; k++)
			total += CONTAINER_PERCENT[tier][k];
		CHECK(total == 100);
		CHECK(CONTAINER_PERCENT[tier][static_cast<int>(Container::Chest)] >= 40); // the mimic hides among them
		for (int k = 0; tier > 0 && k < CONTAINER_COUNT; k++) // the tiers add up, as the decorations
			CHECK((CONTAINER_PERCENT[tier - 1][k] == 0 || CONTAINER_PERCENT[tier][k] > 0));
	}
	CHECK(CONTAINER_PERCENT[0][static_cast<int>(Container::Table)] == 0); // a cave has no offering tables
	for (const ContainerDef& def : CONTAINER_DEFS)
		CHECK(def.openMs > 0);
}

TEST_CASE("a treasure's look is the same on every load, from its tier, against a side wall only where there is one") {
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		CAPTURE(n);
		LevelGrid grid;
		const std::string file = campaignLevelFile(n);
		REQUIRE(loadLevelFile(file.c_str(), grid).empty());
		auto a = std::make_unique<DecorLayout>();
		auto b = std::make_unique<DecorLayout>();
		scatterDecor(grid.cells, file.c_str(), n, *a);
		scatterDecor(grid.cells, ("elsewhere/" + file.substr(file.rfind('/') + 1)).c_str(), n, *b);
		for (int c = 0; c < LEVEL_CELL_COUNT; c++) {
			if (grid.cells[c].type != Treasure)
				continue;
			const TreasureLook& look = a->treasure[c];
			CHECK(sameLook(look, b->treasure[c]));
			CHECK(CONTAINER_PERCENT[decorTier(n)][static_cast<int>(look.kind)] > 0);
			const int col = c % LEVEL_WIDTH;
			if (look.x < 0.f) // pushed left: its back against rock
				CHECK((col == 0 || isWall(grid.cells[c - 1])));
			if (look.x > 0.f)
				CHECK((col == LEVEL_WIDTH - 1 || isWall(grid.cells[c + 1])));
		}
	}
}

TEST_CASE("a chest opens with a press: nothing during its clip, then Take; the gameplay stream does not move") {
	SimWorld world;
	SimWorld idle; // the same level and seed, nobody opens anything
	load(world);
	load(idle);
	world.dungeon.PlaceContainer(CHEST_COL, ROW, static_cast<int>(Container::Chest), 0);
	REQUIRE(world.WalkTo(CHEST_COL + 0.5f));
	REQUIRE(idle.WalkTo(CHEST_COL + 0.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::Open);
	world.Interact();
	world.WaitTicks(1);
	idle.WaitTicks(1);
	CHECK(std::any_of(world.said.begin(), world.said.end(), [](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Sound && e.sound == WorldSound::LidWood;
	}));
	CHECK(world.dungeon.InteractVerb() == Interaction::None);
	world.Wait(containerOpenMsMax());
	idle.Wait(containerOpenMsMax());
	CHECK(world.dungeon.InteractVerb() == Interaction::Take);
	CHECK(world.random.gameplay.next() == idle.random.gameplay.next());
	CHECK(world.dungeon.Cell(CHEST_COL, ROW).type == Treasure); // still there, showing its item
	world.Interact();
	world.WaitTicks(1);
	CHECK(world.Told("Found: "));
	CHECK(world.dungeon.Cell(CHEST_COL, ROW).type == NoObject);
	CHECK(world.dungeon.ContainerStands(CHEST_COL, ROW)); // the empty container stays
	CHECK(world.dungeon.InteractVerb() == Interaction::None);
}

TEST_CASE("over a save and load an open chest stays open and an emptied one stays standing, the rest closed") {
	SimWorld world;
	load(world);
	REQUIRE(world.WalkTo(CHEST_COL + 0.5f));
	world.OpenChest(); // emptied
	REQUIRE(world.WalkTo(SECOND_COL + 0.5f));
	world.Interact(); // opened, its item left in it
	world.Wait(containerOpenMsMax());
	world.SaveAndLoad();
	CHECK(world.dungeon.InteractVerb() == Interaction::Take);
	CHECK(world.dungeon.ContainerStands(CHEST_COL, ROW));
	CHECK(world.dungeon.Cell(CHEST_COL, ROW).type == NoObject);
	REQUIRE(world.WalkTo(WALL_COL + 0.5f));
	CHECK(world.dungeon.InteractVerb() == Interaction::Open);
}

TEST_CASE("the scenario's container command gives a cell any look; a stance index outside STANCES is refused") {
	SimWorld world;
	load(world);
	CHECK(world.dungeon.PlaceContainer(CHEST_COL, ROW, static_cast<int>(Container::Jar), 0));
	CHECK_FALSE(world.dungeon.PlaceContainer(CHEST_COL, ROW, CONTAINER_COUNT, 0));
	CHECK_FALSE(world.dungeon.PlaceContainer(CHEST_COL, ROW, 0, STANCE_COUNT));
	CHECK_FALSE(world.dungeon.PlaceContainer(-1, ROW, 0, 0));
}

TEST_CASE("the first canopic jars opened write their field note; a jar smashes") {
	SimWorld world;
	load(world);
	REQUIRE(world.dungeon.PlaceContainer(CHEST_COL, ROW, static_cast<int>(Container::Canopic), 0));
	REQUIRE(world.dungeon.PlaceContainer(SECOND_COL, ROW, static_cast<int>(Container::Jar), 3));
	REQUIRE(world.WalkTo(CHEST_COL + 0.5f));
	world.Interact();
	world.WaitTicks(1);
	CHECK(noted(world, FieldNote::CanopicJars));
	REQUIRE(world.WalkTo(SECOND_COL + 0.5f));
	world.said.clear();
	world.Interact();
	world.WaitTicks(1);
	CHECK(std::any_of(world.said.begin(), world.said.end(), [](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Sound && e.sound == WorldSound::JarSmash;
	}));
	CHECK_FALSE(noted(world, FieldNote::CanopicJars));
}
