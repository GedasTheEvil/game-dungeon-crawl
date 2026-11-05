// More skills than heka (plan heka-surplus): Anubis gives none, the stat skills, the Wisdom of Seshat, Disarm trap and
// the Heka screen's scrolling list.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/ui/skills_layout.h"
#include "../../src/world/skills.h"
#include <algorithm>
#include <sstream>
#include <string>

namespace {
// Heka for the skills, as the scenario's `heka`.
void earn(PlayerStats& stats, int count) {
	for (int boss = 1; boss <= count; boss++)
		stats.heka.Earn(boss * 5);
}

PlayerStats saveAndLoad(const PlayerStats& stats) {
	std::stringstream f;
	stats.Dump(f);
	PlayerStats loaded;
	loaded.LoadDump(f);
	return loaded;
}

// A corridor on row 1 from the entrance (col 1) to the exit (col 18), a plain teleporter at col 16 to a sealed room on
// row 4 (cols 10 to 14, its teleporter at col 10).
LevelGrid sealedRoomLevel() {
	LevelGrid grid;
	for (int col = 1; col <= 18; col++)
		grid.set(col, 1, Tile{});
	for (int col = 10; col <= 14; col++)
		grid.set(col, 4, Tile{});
	grid.set(1, 1, cellWith({Door, GateEntrance, 0}));
	grid.set(16, 1, cellWith({Door, GateTeleport, 2}));
	grid.set(18, 1, cellWith({Door, GateExit, 0}));
	grid.set(10, 4, cellWith({Door, GateTeleport, 2}));
	return grid;
}

void learn(SimWorld& world, SkillId id) {
	world.player.stats.heka.Earn(5);
	REQUIRE(world.player.stats.Learn(id, true));
}
} // namespace

TEST_CASE("Anubis, the last level's boss, gives no heka; the others do") {
	Heka heka;
	CHECK_FALSE(heka.Earn(CAMPAIGN_LEVELS));
	CHECK(heka.Earned() == 0);
	CHECK(heka.Earn(CAMPAIGN_LEVELS - 9)); // Rerek's (lvl45)
	CHECK(heka.Earned() == 1);
}

TEST_CASE("more skills than the campaign's heka: at least 4 left unlearnt at the end") {
	int bosses = 0;
	for (int level = 1; level <= CAMPAIGN_LEVELS; level++)
		bosses += level == 4 || level == 9 || level == 13 || level == 18 || level == 22 || level == 27 || level == 32 ||
						  level == 37 || level == 41 || level == 45
					  ? 1
					  : 0;
	CHECK(SKILL_COUNT - bosses >= 4);
}

TEST_CASE("the HUD's heka waits only while some skill can still be learnt") {
	Heka heka;
	CHECK_FALSE(heka.Waiting()); // no heka
	for (int boss = 1; boss <= SKILL_COUNT + 1; boss++)
		heka.Earn(boss * 2);
	CHECK(heka.Waiting());
	for (int i = 0; i < SKILL_COUNT; i++)
		REQUIRE(heka.Learn(static_cast<SkillId>(i)));
	CHECK(heka.Free() == 1);
	CHECK_FALSE(heka.Waiting()); // a heka free, nothing left to buy
}

TEST_CASE("the stat skills: health, might, armor and stamina with the skill, the same after a load") {
	PlayerStats stats;
	earn(stats, 4);
	const int hp = stats.CurrentMaxHP();
	const int might = stats.CurrentMight();
	const int armor = stats.CurrentArmor();
	const int stamina = stats.MaxStamina();
	stats.LoseHP(10);
	const int wounded = stats.CurrentHP();
	REQUIRE(stats.Learn(SkillId::KaOfOsiris));
	CHECK(stats.CurrentMaxHP() == hp + hp * SKILL_HEALTH_PERCENT / 100);
	CHECK(stats.CurrentHP() == wounded + (stats.CurrentMaxHP() - hp)); // the new share, no heal beyond it
	REQUIRE(stats.Learn(SkillId::StrengthOfMontu));
	CHECK(stats.CurrentMight() == might + SKILL_MIGHT);
	REQUIRE(stats.Learn(SkillId::DjedOfPtah));
	CHECK(stats.CurrentArmor() == armor + SKILL_ARMOR);
	stats.SetStamina(stamina);
	REQUIRE(stats.Learn(SkillId::BreathOfShu));
	CHECK(stats.MaxStamina() == stamina * (100 + SKILL_STAMINA_PERCENT) / 100);
	CHECK(stats.Stamina() == stats.MaxStamina()); // full stays full

	const PlayerStats loaded = saveAndLoad(stats);
	CHECK(loaded.CurrentMaxHP() == stats.CurrentMaxHP());
	CHECK(loaded.CurrentHP() == stats.CurrentHP());
	CHECK(loaded.CurrentMight() == stats.CurrentMight());
	CHECK(loaded.CurrentArmor() == stats.CurrentArmor());
	CHECK(loaded.MaxStamina() == stats.MaxStamina());
	CHECK(loaded.Stamina() == stats.Stamina()); // not cut to the max without the skill

	PlayerStats plain;
	CHECK(saveAndLoad(plain).CurrentMaxHP() == hp);
	CHECK(saveAndLoad(plain).CurrentMight() == might);
}

TEST_CASE("Speed of Mafdet: a fifth faster on firm ground, not in the air, nor wading or in sand") {
	PlayerStats stats;
	earn(stats, 1);
	REQUIRE(stats.Learn(SkillId::SpeedOfMafdet));
	CHECK(stats.WalkFactor(false) == doctest::Approx(1.f + SKILL_SPEED_PERCENT / 100.f));
	CHECK(stats.WalkFactor(true) == doctest::Approx(1.f));
	CHECK(stats.WalkFactor(false, false) == doctest::Approx(1.f));
}

TEST_CASE("Speed of Mafdet: the walk is faster, the jump carries as far as without it") {
	auto walked = [](bool skill, bool jump) {
		SimWorld world;
		REQUIRE(world.Load("tests/levels/empty_row"));
		if (skill)
			learn(world, SkillId::SpeedOfMafdet);
		const float from = world.X();
		world.facing = 1;
		if (!jump) {
			world.HoldWalk(1, 2000);
			return world.X() - from;
		}
		world.Jump();
		while (world.player.jump.jumping || world.player.jump.falling) {
			world.Step(1);
			world.Tick();
		}
		return world.X() - from;
	};
	CHECK(walked(true, false) == doctest::Approx(walked(false, false) * 1.2f).epsilon(0.03));
	CHECK(walked(true, true) == doctest::Approx(walked(false, true)));
}

TEST_CASE("Speed of Mafdet: wading keeps its own factor") {
	auto waded = [](bool skill) {
		SimWorld world;
		world.player.god = true;
		REQUIRE(world.Load("tests/levels/water"));
		if (skill)
			learn(world, SkillId::SpeedOfMafdet);
		REQUIRE(world.WalkTo(10.8f));
		for (int t = 0; t < 10 && !world.dungeon.PlayerWading(); t++)
			world.Tick();
		REQUIRE(world.dungeon.PlayerWading());
		return world.dungeon.PlayerStepFactor();
	};
	CHECK(waded(true) == doctest::Approx(waded(false)));
}

TEST_CASE("the sealed rooms: a pocket reached only by its teleporter, not the level round the entrance") {
	const LevelGrid grid = sealedRoomLevel();
	const std::vector<std::vector<int>> rooms = secretRooms(grid.cells);
	REQUIRE(rooms.size() == 1);
	CHECK(rooms[0].size() == 5);
	for (int col = 10; col <= 14; col++)
		CHECK(std::ranges::find(rooms[0], 4 * LEVEL_WIDTH + col) != rooms[0].end());
}

TEST_CASE("Wisdom of Seshat: a sealed room goes on the draft map once the player is near, not without the skill") {
	for (const bool skill : {false, true}) {
		CAPTURE(skill);
		SimWorld world;
		world.player.god = true;
		world.dungeon.LoadGrid(sealedRoomLevel(), "sealed-room");
		world.player.Reanimate();
		if (skill)
			learn(world, SkillId::WisdomOfSeshat);
		world.WaitTicks(2);
		CHECK_FALSE(world.dungeon.Explored(12, 4)); // far
		REQUIRE(world.WalkTo(9.5f));
		world.WaitTicks(1);
		CHECK(world.dungeon.Explored(12, 4) == skill);
		CHECK(world.dungeon.Explored(14, 4) == skill); // the whole room
		CHECK(world.dungeon.Explored(15, 4) == skill); // its walls
	}
}

TEST_CASE("the Heka screen's list scrolls: every card shows at some scroll, in the panel, the selection followed") {
	using namespace SkillsLayout;
	CHECK(MAX_SCROLL == ROWS - VISIBLE_ROWS);
	for (int scroll = 0; scroll <= MAX_SCROLL; scroll++)
		for (int a = 0; a < SKILL_COUNT; a++) {
			if (!cardVisible(a, scroll)) {
				const Rect r = cardRect(a, scroll);
				CHECK(cardAt(r.cx(), r.cy(), scroll) != a);
				continue;
			}
			const Rect r = cardRect(a, scroll);
			CHECK(r.inset(-1.f).within(CARDS_PANEL)); // the selection frame too
			CHECK(r.y + r.h < POINTS_BASELINE - 1.f);
			CHECK(cardAt(r.cx(), r.cy(), scroll) == a);
			CHECK_FALSE(r.overlaps(scrollTrack()));
		}
	CHECK(scrollTrack().within(CARDS_PANEL));
	CHECK(scrollTo(SKILL_COUNT - 1, 0) == MAX_SCROLL);
	CHECK(scrollTo(0, MAX_SCROLL) == 0);
	CHECK(scrollTo(2 * COLUMNS, 1) == 1); // in view: the list stays
	CHECK(scrollThumb(0).y + scrollThumb(0).h == doctest::Approx(scrollTrack().y + scrollTrack().h));
	CHECK(scrollThumb(MAX_SCROLL).y == doctest::Approx(scrollTrack().y));
}

TEST_CASE("Disarm trap: a trapped chest's trap goes off about 1 time in 5, and it is never empty") {
	constexpr int RUNS = 100;
	int sprung = 0;
	for (uint64_t seed = 1; seed <= RUNS; seed++) {
		CAPTURE(seed);
		SimWorld world;
		REQUIRE(world.Load("tests/levels/trapped_chests", seed));
		world.player.god = true;
		learn(world, SkillId::DisarmTrap);
		world.dungeon.SetLevelNumber(20);
		REQUIRE(world.dungeon.PlaceContainer(3, 2, static_cast<int>(Container::Chest), 0));
		world.WaitTicks(30);
		REQUIRE(world.WalkTo(3.5f));
		world.said.clear();
		world.Interact();
		world.WaitTicks(1);
		const bool undone = world.Told("I undo the trap");
		const bool fired = world.Told("Poison darts") || world.Told("green gas") || world.Told("execration") ||
						   world.Told("Scarabs");
		CHECK(undone != fired);
		sprung += fired ? 1 : 0;
		CHECK_FALSE(world.Told("The trap was the treasure"));
		world.Wait(containerOpenMsMax() + UPDATE_TICK_MS);
		REQUIRE(world.dungeon.InteractVerb() == Interaction::Take);
		world.Interact();
		world.WaitTicks(1);
		CHECK(world.Told("Found: "));
	}
	CHECK(sprung > RUNS * (100 - DISARM_TRAP_PERCENT - 10) / 100);
	CHECK(sprung < RUNS * (100 - DISARM_TRAP_PERCENT + 10) / 100);
}

TEST_CASE("Disarm trap: an inscribed coffin's curse fails as often") {
	constexpr int RUNS = 60;
	int cursed = 0;
	for (uint64_t seed = 1; seed <= RUNS; seed++) {
		SimWorld world;
		REQUIRE(world.Load("tests/levels/tomb_curse", seed));
		learn(world, SkillId::DisarmTrap);
		world.dungeon.SetLevelNumber(20);
		REQUIRE(world.dungeon.PlaceContainer(6, 2, static_cast<int>(Container::Coffin), 0, true));
		world.WaitTicks(30);
		REQUIRE(world.WalkTo(6.5f));
		world.Interact();
		world.WaitTicks(1);
		cursed += world.player.stats.curses.Any() ? 1 : 0;
		CHECK(world.player.stats.curses.Any() != world.Told("It unravels"));
	}
	CHECK(cursed > 0);
	CHECK(cursed < RUNS * (100 - DISARM_TRAP_PERCENT + 15) / 100);
}
