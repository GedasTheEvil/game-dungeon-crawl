// The Blessing of Hapi potion (plan water-blessing-potion): its timer and what it changes in half water, alone and with
// the Favour of Hapi skill (plan skill-blessing-of-hapi).
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/world/skills.h"
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace {
void learnHapi(Player& player) {
	player.stats.heka.Earn(5);
	REQUIRE(player.stats.heka.Learn(SkillId::FavourOfHapi, true));
}

// The player in the water level's pool, wading (standing still).
void wadeIn(SimWorld& world) {
	REQUIRE(world.Load("tests/levels/water"));
	REQUIRE(world.WalkTo(10.8f));
	for (int t = 0; t < 10 && !world.dungeon.PlayerWading(); t++)
		world.Tick();
	REQUIRE(world.dungeon.PlayerWading());
}

struct Wade {
	float speed = 0.f; // tiles in a second wading east, shift held or not
	int drank = 0; // the stamina that second took
};

// Which of the two is on and whether shift is held. The world is made here, the clock is the caller's concern.
Wade wade(bool potion, bool skill, bool sprint) {
	SimWorld world;
	world.player.god = true;
	wadeIn(world);
	if (skill)
		learnHapi(world.player);
	if (potion)
		world.player.stats.Drink(potionGain(ItemKind::WaterBlessing));
	world.player.stats.SetStamina(world.player.stats.MaxStamina());
	world.player.stats.SetSprintRequested(sprint);
	const float from = world.X();
	world.HoldWalk(1, 1000);
	REQUIRE(world.dungeon.PlayerWading());
	return {world.X() - from, world.player.stats.MaxStamina() - world.player.stats.Stamina()};
}
} // namespace

TEST_CASE("Blessing of Hapi: lasts 2 minutes, drinking again restarts them, the end is told") {
	GameClock::enableVirtual();
	Player player;
	WorldEvents events;
	CHECK_FALSE(player.stats.WaterBlessed());
	player.stats.Drink(potionGain(ItemKind::WaterBlessing));
	CHECK(player.stats.WaterBlessed());
	CHECK(player.stats.WaterBlessingLeftMs() == 120000);
	player.stats.AdvanceWaterBlessing(90000, events);
	CHECK(player.stats.WaterBlessingLeftMs() == 30000);
	player.stats.Drink(potionGain(ItemKind::WaterBlessing));
	CHECK(player.stats.WaterBlessingLeftMs() == 120000);
	player.stats.AdvanceWaterBlessing(119000, events);
	CHECK(player.stats.WaterBlessed());
	CHECK(events.Take().empty());
	player.stats.AdvanceWaterBlessing(1000, events);
	CHECK_FALSE(player.stats.WaterBlessed());
	const std::vector<WorldEvent> said = events.Take();
	REQUIRE(said.size() == 1);
	CHECK(said[0].text.find("deep again") != std::string::npos);
	player.stats.AdvanceWaterBlessing(1000, events); // nothing more to say
	CHECK(events.Take().empty());
}

TEST_CASE("Blessing of Hapi: with the Favour of Hapi skill the potion lasts 4 minutes") {
	GameClock::enableVirtual();
	Player player;
	learnHapi(player);
	player.stats.Drink(potionGain(ItemKind::WaterBlessing));
	CHECK(player.stats.WaterBlessingLeftMs() == 240000);
	CHECK(PotionEffect::WATER_BLESSING_MS == 120000);
}

TEST_CASE("Blessing of Hapi: the drink says the minutes, death and a new game end it") {
	GameClock::enableVirtual();
	Player player;
	WorldEvents events;
	CHECK(player.stats.Drink(potionGain(ItemKind::WaterBlessing)) == "The water parts for 2 min");
	player.TakeHit(10000, DamageMix{100, 0, 0}, events, true);
	REQUIRE_FALSE(player.Alive());
	CHECK_FALSE(player.stats.WaterBlessed());
	player.Reanimate();
	player.stats.Drink(potionGain(ItemKind::WaterBlessing));
	player.Reanimate();
	CHECK_FALSE(player.stats.WaterBlessed());
}

TEST_CASE("Blessing of Hapi: saved with the time left, a save from before it loads without") {
	GameClock::enableVirtual();
	WorldEvents events;
	PlayerStats stats;
	stats.Drink(potionGain(ItemKind::WaterBlessing));
	stats.AdvanceWaterBlessing(20000, events);
	const std::filesystem::path path = std::filesystem::temp_directory_path() / "water_blessing_test.sav";
	{
		std::ofstream out(path);
		stats.Dump(out);
	}
	PlayerStats loaded;
	{
		std::ifstream in(path);
		loaded.LoadDump(in);
	}
	CHECK(loaded.WaterBlessingLeftMs() == 100000);
	// The old format: no WATER line.
	std::string text;
	{
		std::ifstream in(path);
		std::string line;
		while (std::getline(in, line))
			if (line.rfind("WATER", 0) != 0)
				text += line + "\n";
	}
	{
		std::ofstream out(path);
		out << text;
	}
	PlayerStats old;
	old.Drink(potionGain(ItemKind::WaterBlessing));
	{
		std::ifstream in(path);
		old.LoadDump(in);
	}
	std::filesystem::remove(path);
	CHECK_FALSE(old.WaterBlessed());
	CHECK(old.CurrentLevel() == stats.CurrentLevel());
}

TEST_CASE("Blessing of Hapi: the sprint in half water drains the stamina at half with the potion and the skill") {
	GameClock::enableVirtual();
	PlayerStats plain;
	PlayerStats halved;
	WorldEvents events;
	plain.SetSprintRequested(true);
	halved.SetSprintRequested(true);
	for (int t = 0; t < 10; t++) {
		GameClock::advance(1000);
		plain.NoteWalked();
		plain.UpdateStamina(events);
		halved.NoteWalked(true);
		halved.UpdateStamina(events);
	}
	const int plainDrank = plain.MaxStamina() - plain.Stamina();
	const int halvedDrank = halved.MaxStamina() - halved.Stamina();
	CHECK(plainDrank > 0);
	CHECK(halvedDrank * 2 == doctest::Approx(plainDrank).epsilon(0.15));
}

TEST_CASE("Blessing of Hapi: wade, sprint and jump in half water, the potion alone and with the skill") {
	// The virtual clock is global and the spike traps' phase follows it (wall_spikes_test): give it back whole cycles.
	const int clockFrom = GameClock::now();
	const float landSprint = WALK_SPEED * SPRINT_MULTIPLIER;
	const Wade plain = wade(false, false, false);
	const Wade potion = wade(true, false, false);
	const Wade potionSprint = wade(true, false, true);
	const Wade both = wade(true, true, false);
	const Wade bothSprint = wade(true, true, true);
	const Wade skillSprint = wade(false, true, true);
	constexpr int ALL_SPIKE_PERIODS_MS = 48000; // a multiple of every SPIKES_PERIOD_MS
	GameClock::advance(ALL_SPIKE_PERIODS_MS - (GameClock::now() - clockFrom) % ALL_SPIKE_PERIODS_MS);

	CHECK(plain.speed == doctest::Approx(WALK_SPEED * WADE_SPEED_FACTOR).epsilon(0.1));
	CHECK(potion.speed == doctest::Approx(WALK_SPEED).epsilon(0.1)); // 100% of the walk
	CHECK(potionSprint.speed == doctest::Approx(landSprint * 0.4f).epsilon(0.1));
	CHECK(both.speed == doctest::Approx(WALK_SPEED * 1.25f).epsilon(0.1)); // faster than on land
	CHECK(bothSprint.speed == doctest::Approx(landSprint).epsilon(0.1)); // the full land sprint
	CHECK(skillSprint.speed == doctest::Approx(landSprint * 0.5f).epsilon(0.1));
	// The drain: none without a sprint, one with it; the skill and the potion together halve it.
	CHECK(potion.drank == 0);
	CHECK(both.drank == 0);
	CHECK(potionSprint.drank > 0);
	CHECK(bothSprint.drank > 0);
	CHECK(skillSprint.drank > 0);
	CHECK(bothSprint.drank < potionSprint.drank);
}

TEST_CASE("Blessing of Hapi: no sprint in half water once the potion has run out") {
	SimWorld world;
	world.player.god = true;
	wadeIn(world);
	world.player.stats.Drink(potionGain(ItemKind::WaterBlessing));
	CHECK(world.dungeon.PlayerWalkFactor() == doctest::Approx(WADE_SPEED_FACTOR_BLESSED));
	world.Wait(120000 + 100);
	REQUIRE(world.dungeon.PlayerWading());
	CHECK_FALSE(world.player.stats.WaterBlessed());
	CHECK(world.dungeon.PlayerWalkFactor() == doctest::Approx(WADE_SPEED_FACTOR));
	CHECK(world.dungeon.PlayerSprintMultiplier(SPRINT_MULTIPLIER) == doctest::Approx(1.f));
	CHECK(world.Told("deep again"));
}

TEST_CASE("Blessing of Hapi: the jump works in half water with the potion, and not without") {
	SimWorld world;
	world.player.god = true;
	wadeIn(world);
	const auto jumped = [&] {
		world.said.clear();
		world.Jump();
		world.Tick();
		return std::ranges::any_of(world.said.begin(), world.said.end(), [](const WorldEvent& e) {
			return e.kind == WorldEvent::Kind::CharacterSound && e.character == CharacterSound::Jump &&
				   e.who == PLAYER_CHARACTER;
		});
	};
	CHECK_FALSE(jumped());
	CHECK(world.Told("too deep to jump"));
	world.player.stats.Drink(potionGain(ItemKind::WaterBlessing));
	CHECK(jumped());
	CHECK_FALSE(world.Told("too deep to jump"));
}
