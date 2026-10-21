// Trapped chests and tomb curses (plan trapped-chests): a TrappedTreasure's opening rolls its contents and its trap on
// the gameplay stream, the trap ward's chance stops the trap, a smashed ushabti may curse; the checker does not count a
// trapped chest's antidote.
#include "../../external/doctest/doctest.h"
#include "../../src/world/level_check.h"
#include "../../src/world/loot.h"
#include "sim_world.h"
#include <algorithm>
#include <string>
#include <vector>

namespace {
constexpr int ROW = 2; // tests/levels/trapped_chests: the corridor's row
constexpr int TRAPPED_COL = 3;
constexpr int PLAIN_COL = 5;
constexpr int DEEP_LEVEL = 20; // the temple tier: every trap kind

bool noted(const SimWorld& world, FieldNote note) {
	return std::any_of(world.said.begin(), world.said.end(),
					   [note](const WorldEvent& e) { return e.kind == WorldEvent::Kind::Note && e.note == note; });
}

int scarabs(const SimWorld& world) {
	int n = 0;
	for (int i = 0; i < MAX_MONSTERS; i++) {
		const Monster& mon = world.dungeon.Monsters()[i];
		n += mon.Active() && mon.Alive() && mon.Type()->id == MonsterScarab && mon.Minion() ? 1 : 0;
	}
	return n;
}

std::vector<std::string> statuses(const SimWorld& world) {
	std::vector<std::string> lines;
	for (const WorldEvent& e : world.said)
		if (e.kind == WorldEvent::Kind::Status)
			lines.push_back(e.text);
	return lines;
}

// The level on seed, at depth level, the chest at col a plain wooden one; the player walks up to it.
void load(SimWorld& world, uint64_t seed, int level = DEEP_LEVEL, int col = TRAPPED_COL) {
	REQUIRE(world.Load("tests/levels/trapped_chests", seed));
	world.dungeon.SetLevelNumber(level);
	REQUIRE(world.dungeon.PlaceContainer(col, ROW, static_cast<int>(Container::Chest), 0));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(static_cast<float>(col) + 0.5f));
	world.said.clear();
}

// The trap that went off, by its status line; "" for none.
std::string trapOf(const SimWorld& world) {
	if (world.Told("Poison darts"))
		return "darts";
	if (world.Told("green gas"))
		return "gas";
	if (world.Told("execration"))
		return "curse";
	if (world.Told("Scarabs"))
		return "swarm";
	return "";
}
} // namespace

TEST_CASE("the chest traps by depth add up; darts and gas early, curses and the swarm deep; jars curse with disease") {
	for (int tier = 0; tier < DECOR_TIER_COUNT; tier++) {
		int total = 0;
		for (int k = 0; k < CHEST_TRAP_COUNT; k++)
			total += CHEST_TRAP_PERCENT[tier][k];
		CHECK(total == 100);
	}
	CHECK(CHEST_TRAP_PERCENT[0][static_cast<int>(ChestTrap::Curse)] == 0);
	CHECK(CHEST_TRAP_PERCENT[0][static_cast<int>(ChestTrap::Swarm)] == 0);
	CHECK(CHEST_TRAP_PERCENT[DECOR_TIER_COUNT - 1][static_cast<int>(ChestTrap::Swarm)] > 0);
	Rng rng(7);
	for (int i = 0; i < 50; i++) {
		CHECK(RollChestTrap(Container::Canopic, 1, rng) == ChestTrap::Curse);
		CHECK(RollChestTrap(Container::Jar, 1, rng) == ChestTrap::Curse);
		CHECK(RollChestCurse(Container::Canopic, rng) == Curse::Disease);
		const Curse c = RollChestCurse(Container::Chest, rng);
		CHECK((c != Curse::Palsy && c != Curse::Insanity && c != Curse::Disease));
	}
}

TEST_CASE("in the cave the chests' traps come out at their shares: darts 60%, gas 40%") {
	int gas = 0;
	constexpr int RUNS = 200;
	for (uint64_t seed = 1; seed <= RUNS; seed++) {
		SimWorld world;
		load(world, seed, 1);
		world.player.god = true;
		world.Interact();
		world.WaitTicks(1);
		const std::string trap = trapOf(world);
		CHECK((trap == "darts" || trap == "gas"));
		gas += trap == "gas" ? 1 : 0;
	}
	CHECK(gas > RUNS * 28 / 100);
	CHECK(gas < RUNS * 52 / 100);
}

TEST_CASE("each trap kind goes off once on opening and does what it says") {
	std::vector<std::string> seen;
	for (uint64_t seed = 1; seed <= 80 && seen.size() < CHEST_TRAP_COUNT; seed++) {
		CAPTURE(seed);
		SimWorld world;
		load(world, seed);
		const int hp = world.Hp();
		CHECK(world.dungeon.InteractVerb() == Interaction::Open); // as any chest
		world.Interact();
		world.WaitTicks(1);
		const std::string trap = trapOf(world);
		REQUIRE_FALSE(trap.empty());
		CHECK(noted(world, FieldNote::TrappedChests));
		if (trap == "darts") {
			CHECK(world.player.stats.poison.LeftMs(PoisonTier::Medium) > 0);
			CHECK(world.Hp() < hp);
		} else if (trap == "gas") {
			CHECK(world.player.stats.poison.LeftMs(PoisonTier::Weak) > 0);
		} else if (trap == "curse") {
			CHECK(world.player.stats.curses.Any());
			CHECK(noted(world, FieldNote::TombCurses));
		} else {
			CHECK(scarabs(world) == CHEST_SWARM_SIZE);
		}
		if (std::find(seen.begin(), seen.end(), trap) == seen.end())
			seen.push_back(trap);
		const size_t lines = statuses(world).size();
		world.Wait(containerOpenMsMax());
		if (world.dungeon.InteractVerb() == Interaction::Take)
			world.Interact();
		world.WaitTicks(2);
		CHECK(trapOf(world) == trap);
		CHECK(std::count_if(world.said.begin(), world.said.end(), [](const WorldEvent& e) {
				  return e.kind == WorldEvent::Kind::Note && e.note == FieldNote::TrappedChests;
			  }) == 1);
		CHECK(statuses(world).size() <= lines + 2); // at most the find and its gain, no second trap
	}
	CHECK(seen.size() == CHEST_TRAP_COUNT);
}

TEST_CASE("the gas poisons while the player stands in it, not once it has gone or they have stepped out") {
	for (uint64_t seed = 1; seed <= 80; seed++) {
		SimWorld world;
		load(world, seed, 1); // the cave: darts or gas
		world.Interact();
		world.WaitTicks(1);
		if (trapOf(world) != "gas")
			continue;
		world.player.stats.poison = {};
		world.Wait(CHEST_GAS_POISON_MS + UPDATE_TICK_MS);
		CHECK(world.player.stats.poison.LeftMs(PoisonTier::Weak) > 0); // still in it
		world.player.stats.poison = {};
		REQUIRE(world.WalkTo(static_cast<float>(TRAPPED_COL) + 2.5f));
		world.Wait(CHEST_GAS_POISON_MS + UPDATE_TICK_MS);
		CHECK(world.player.stats.poison.LeftMs(PoisonTier::Weak) == 0);
		return;
	}
	FAIL("no gas in 80 seeds");
}

TEST_CASE("about half the trapped chests hold their item and one bonus more, the rest are empty") {
	int full = 0;
	bool tookBonus = false;
	constexpr int RUNS = 100;
	for (uint64_t seed = 1; seed <= RUNS; seed++) {
		CAPTURE(seed);
		SimWorld world;
		load(world, seed);
		world.player.god = true;
		world.Interact();
		world.Wait(containerOpenMsMax() + UPDATE_TICK_MS);
		if (world.dungeon.Cell(TRAPPED_COL, ROW).type == TrappedTreasure) {
			full++;
			CHECK(world.dungeon.InteractVerb() == Interaction::Take);
			world.Interact();
			world.WaitTicks(1);
			CHECK(world.Told("Found: "));
			tookBonus = tookBonus || world.Told("\n+ ");
			CHECK(world.dungeon.Cell(TRAPPED_COL, ROW).type == NoObject);
		} else {
			CHECK(world.Told("The trap was the treasure"));
			CHECK(world.dungeon.Cell(TRAPPED_COL, ROW).type == NoObject);
			CHECK(world.dungeon.ContainerStands(TRAPPED_COL, ROW));
			CHECK(world.dungeon.InteractVerb() == Interaction::None);
		}
	}
	CHECK(full > RUNS * 35 / 100);
	CHECK(full < RUNS * 65 / 100);
	CHECK(tookBonus);
}

TEST_CASE("a trapped chest's loot: the plain chest's, then one bonus more") {
	OwnedWeapons owned{};
	for (uint64_t seed = 1; seed <= 20; seed++) {
		Rng a(seed), b(seed);
		const std::vector<ItemKind> plain = RollChestLoot(ItemKind::SmallHealth, owned, DEEP_LEVEL, a);
		const std::vector<ItemKind> trapped = RollTrappedChestLoot(ItemKind::SmallHealth, owned, DEEP_LEVEL, b);
		REQUIRE(trapped.size() == plain.size() + 1);
		CHECK(std::equal(plain.begin(), plain.end(), trapped.begin()));
	}
}

TEST_CASE("the same seed opens a trapped chest the same way") {
	for (uint64_t seed = 1; seed <= 10; seed++) {
		SimWorld a, b;
		load(a, seed);
		load(b, seed);
		a.Interact();
		b.Interact();
		a.Wait(1000);
		b.Wait(1000);
		CHECK(statuses(a) == statuses(b));
		CHECK(a.dungeon.Cell(TRAPPED_COL, ROW).type == b.dungeon.Cell(TRAPPED_COL, ROW).type);
	}
}

TEST_CASE("over a save and load a closed trapped chest stays trapped; the plain one beside it has no trap") {
	SimWorld world;
	load(world, 1);
	world.SaveAndLoad();
	CHECK(world.dungeon.Cell(TRAPPED_COL, ROW).type == TrappedTreasure);
	REQUIRE(world.dungeon.PlaceContainer(TRAPPED_COL, ROW, static_cast<int>(Container::Chest), 0));
	world.said.clear();
	world.Interact();
	world.WaitTicks(1);
	CHECK(noted(world, FieldNote::TrappedChests));
	REQUIRE(world.WalkTo(static_cast<float>(PLAIN_COL) + 0.5f));
	world.said.clear();
	world.OpenChest();
	world.WaitTicks(1);
	CHECK(trapOf(world).empty());
	CHECK(world.Told("Found: "));
}

TEST_CASE("the trap ward: the grand one stops every chest trap, a lesser one about a quarter; the contents stay") {
	int fired = 0, full = 0;
	constexpr int RUNS = 120;
	for (uint64_t seed = 1; seed <= RUNS; seed++) {
		SimWorld grand, lesser;
		load(grand, seed);
		load(lesser, seed);
		grand.Wear(ItemKind::TrapWardGrand);
		lesser.Wear(ItemKind::TrapWardLesser);
		grand.said.clear();
		lesser.said.clear();
		grand.Interact();
		lesser.Interact();
		grand.WaitTicks(1);
		lesser.WaitTicks(1);
		CHECK(trapOf(grand).empty());
		CHECK(grand.Told("Eye of Horus"));
		CHECK_FALSE(grand.player.stats.curses.Any());
		CHECK(grand.player.stats.poison.LeftMs(PoisonTier::Medium) == 0);
		fired += trapOf(lesser).empty() ? 0 : 1;
		full += grand.dungeon.Cell(TRAPPED_COL, ROW).type == TrappedTreasure ? 1 : 0;
	}
	CHECK(fired > RUNS * 60 / 100); // 75% go off
	CHECK(fired < RUNS * 90 / 100);
	CHECK(full > RUNS * 30 / 100); // the ward leaves the contents roll alone
	CHECK(full < RUNS * 70 / 100);
}

TEST_CASE("a canopic jar's trap is a curse of disease") {
	SimWorld world;
	load(world, 3, 1);
	REQUIRE(world.dungeon.PlaceContainer(TRAPPED_COL, ROW, static_cast<int>(Container::Canopic), 0));
	world.Interact();
	world.WaitTicks(1);
	CHECK(world.player.stats.curses.Has(Curse::Disease));
}

TEST_CASE("smashing a living ushabti may curse with fatigue, on the gameplay stream; the grand trap ward stops it") {
	auto smash = [](SimWorld& world, uint64_t seed, bool ward) {
		REQUIRE(world.Load("tests/levels/ushabti", seed));
		world.player.god = true;
		world.facing = 1;
		if (ward)
			world.Wear(ItemKind::TrapWardGrand);
		world.WaitTicks(2);
		for (int i = 0; i < 40 && world.dungeon.AttackNearest(10, DamageMix{100, 0, 0}, 7.f, 1); i++)
			world.WaitTicks(1);
		world.WaitTicks(1);
		return world.player.stats.curses.Has(Curse::Fatigue);
	};
	int cursed = 0;
	constexpr int RUNS = 40;
	for (uint64_t seed = 1; seed <= RUNS; seed++) {
		SimWorld plain, warded;
		cursed += smash(plain, seed, false) ? 1 : 0;
		CHECK_FALSE(smash(warded, seed, true));
		SimWorld again;
		CHECK(smash(again, seed, false) == plain.player.stats.curses.Has(Curse::Fatigue)); // the same seed, the same
	}
	CHECK(cursed > 0);
	CHECK(cursed < RUNS / 2);
}

TEST_CASE("the checker counts trapped treasures but not a trapped chest's antidote") {
	LevelGrid grid;
	REQUIRE(loadLevelFile("tests/levels/trapped_chests", grid).empty());
	const ItemFileId antidote = fileIdOf(ItemKind::Antidote);
	auto at = [&grid](int col) -> Tile& { return grid.cells[ROW * LEVEL_WIDTH + col]; };
	setObject(at(15), Tile{MonsterSpawn, MonsterScorpion, 0});
	setObject(at(TRAPPED_COL), Tile{TrappedTreasure, antidote.type, antidote.id});
	LevelReport r = checkLevel(grid);
	CHECK(r.treasures == 5);
	CHECK(r.trappedTreasures == 4);
	CHECK(r.reachableTreasures == 5);
	CHECK(std::find(r.warnings.begin(), r.warnings.end(), "poisoners but no antidote in reach") != r.warnings.end());
	setObject(at(PLAIN_COL), Tile{Treasure, antidote.type, antidote.id});
	r = checkLevel(grid);
	CHECK(std::find(r.warnings.begin(), r.warnings.end(), "poisoners but no antidote in reach") == r.warnings.end());
}
