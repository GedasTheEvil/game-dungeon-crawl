// Ceiling spikes (plan ceiling-spikes) on tests/levels/ceiling_spikes: a jump into them costs a walk over the floor
// kind through armour (the death trap's also its strong poison), a walk under them nothing; a pouncing giant rat takes
// its walk through the floor kind; a cowardly leaper does not leap under them; the loader and the checker reject one
// with no underside a jump reaches; the checker warns when the only way on jumps into them.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/world/level_check.h"
#include "../../src/world/movement.h"
#include "../../src/world/tile_defs.h"
#include <algorithm>
#include <sstream>

namespace {
constexpr int HALL = 2, POUNCE_ROW = 5, COWARD_ROW = 8, OPEN_ROW = 11;
constexpr int SPIKES_COL = 4, DEATH_COL = 7;

bool heard(const SimWorld& world, WorldSound sound) {
	return std::any_of(world.said.begin(), world.said.end(), [&](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Sound && e.sound == sound;
	});
}

const Monster* ratOn(const SimWorld& world, int row) {
	for (int i = 0; i < MAX_MONSTERS; i++) {
		const Monster& mon = world.dungeon.Monsters()[i];
		if (mon.Active() && mon.Type()->id == MonsterGiantRat && mon.Row() == row)
			return &mon;
	}
	return nullptr;
}

// A jump in place at col of the hall, by a player with 1000 max HP (a crossing's damage is then 200, or 400 for the
// death trap: the floors, not the percent). The HP lost.
int jumpUnder(SimWorld& world, int col) {
	REQUIRE(world.Load("tests/levels/ceiling_spikes"));
	world.player.stats.AddMaxHP(1900); // 50 -> 1000, healed
	REQUIRE(world.player.stats.CurrentMaxHP() == 1000);
	REQUIRE(world.dungeon.PlacePlayer(col, HALL));
	world.Tick();
	const int before = world.player.stats.CurrentHP();
	world.Jump();
	world.Wait(1500);
	return before - world.player.stats.CurrentHP();
}

LevelGrid hall(std::uint16_t shape, int fixture) {
	LevelGrid grid;
	const std::string row = "#S.....E#";
	for (int col = 0; col < static_cast<int>(row.size()); col++)
		for (const GlyphDef& g : glyphLegend())
			if (g.glyph == row[static_cast<size_t>(col)])
				grid.set(col, 1, cellWith(g.tile));
	Tile& cell = grid.cells[LEVEL_WIDTH + 4];
	cell.shape = shape;
	cell.fixture = Fixture{fixture, 0, 0};
	return grid;
}

bool warned(const LevelReport& r, const std::string& part) {
	return std::any_of(r.warnings.begin(), r.warnings.end(), [&](const std::string& w) {
		return w.find(part) != std::string::npos;
	});
}
} // namespace

TEST_CASE("a jump into ceiling spikes costs a walk over the floor spikes, through armour; no bump") {
	SimWorld world;
	const int lost = jumpUnder(world, SPIKES_COL);
	CHECK(lost == world.player.stats.HitDamage(SPIKES_CROSS_HP, SPIKE_ATTACK_MIX, true, false));
	CHECK(lost >= SPIKES_CROSS_HP * 9 / 10);
	CHECK(heard(world, WorldSound::SpikeThunk));
	CHECK_FALSE(heard(world, WorldSound::HeadBump));
	CHECK_FALSE(world.Told("You bump your head"));
	CHECK_FALSE(world.player.Dazed()); // no daze roll on top
	CHECK_FALSE(world.player.stats.poison.Any());
}

TEST_CASE("a jump into a ceiling death trap costs a walk over the death trap and poisons strongly") {
	SimWorld world;
	const int lost = jumpUnder(world, DEATH_COL);
	CHECK(lost >= world.player.stats.HitDamage(DEATH_TRAP_CROSS_HP, SPIKE_ATTACK_MIX, true, false));
	CHECK(world.player.stats.poison.Any());
	CHECK(world.Told("The spike tips are wet with venom."));
}

TEST_CASE("a jump into ceiling spikes kills a new player") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/ceiling_spikes"));
	REQUIRE(world.dungeon.PlacePlayer(SPIKES_COL, HALL));
	world.Tick();
	world.Jump();
	world.Wait(1500);
	CHECK_FALSE(world.player.Alive());
}

TEST_CASE("walking, crouching and standing under ceiling traps is safe") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/ceiling_spikes"));
	const int full = world.player.stats.CurrentHP();
	CHECK(world.WalkTo(9.5f));
	world.player.crouchHeld = true;
	world.Tick();
	CHECK(world.WalkTo(DEATH_COL + 0.5f));
	world.player.crouchHeld = false;
	world.Wait(2000);
	CHECK(world.player.stats.CurrentHP() == full);
	CHECK_FALSE(heard(world, WorldSound::SpikeThunk));
}

TEST_CASE("a giant rat pouncing into ceiling spikes takes a walk through the floor spikes, and no stun") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/ceiling_spikes"));
	REQUIRE(world.dungeon.PlacePlayer(15, POUNCE_ROW));
	world.Tick();
	const Monster* rat = ratOn(world, POUNCE_ROW);
	REQUIRE(rat != nullptr);
	const int full = rat->Health();
	bool pounced = false;
	for (int t = 0; t < 6000 / UPDATE_TICK_MS && !pounced; t++, world.Tick())
		pounced = rat->pouncing();
	REQUIRE(pounced);
	while (rat->jumping())
		world.Tick();
	world.Tick();
	CHECK(heard(world, WorldSound::SpikeThunk));
	CHECK_FALSE(heard(world, WorldSound::HeadBump));
	const int expected = TrapHurt::rampTotal(rat->TrapCrossHits()) * rat->Type()->trapDamagePct / 100;
	CHECK(expected > 0);
	CHECK(full - rat->Health() == std::min(full, expected));
	CHECK_FALSE(rat->Reeling());
	CHECK(rat->BumpedLately()); // no second pounce under them at once
}

TEST_CASE("a cowardly leaper does not leap under ceiling spikes; under a bare low ceiling it does") {
	for (const int row : {COWARD_ROW, OPEN_ROW}) {
		CAPTURE(row);
		SimWorld world;
		world.player.god = true;
		REQUIRE(world.Load("tests/levels/ceiling_spikes"));
		REQUIRE(world.dungeon.PlacePlayer(8, row));
		world.Tick();
		const Monster* rat = nullptr;
		for (int t = 0; t < 100 && rat == nullptr; t++, world.Tick()) // the view comes down to it
			rat = ratOn(world, row);
		REQUIRE(rat != nullptr);
		bool leapt = false;
		for (int t = 0; t < 8000 / UPDATE_TICK_MS && !leapt; t++, world.Tick())
			leapt = rat->jumping();
		CHECK(leapt == (row == OPEN_ROW));
		if (row == COWARD_ROW)
			CHECK(rat->CentreX() < 6.f); // walks in under them (safe), stops at the floor trap
	}
}

TEST_CASE("the undersides: solid sub-cells over open ones in the cell, the bottom row none") {
	Tile t;
	t.shape = 0xf000; // the top row
	CHECK(ceilingUndersides(t) == 0xf000);
	t.shape = 0xff00;
	CHECK(ceilingUndersides(t) == 0x0f00);
	t.shape = 0xf00f; // a quarter step under a ceiling three quarters up
	CHECK(ceilingUndersides(t) == 0xf000);
	t.shape = 0x000f; // a step: nothing over open space
	CHECK(ceilingUndersides(t) == 0);
	t.shape = 0x0080; // a block on its own in the second row: its underside
	CHECK(ceilingUndersides(t) == 0x0080);
	t.shape = 0xf000;
	CHECK(jumpReachesUnderside(t)); // three quarters up: the jump's top reaches it
	t.shape = 0x000f;
	CHECK_FALSE(jumpReachesUnderside(t));
}

TEST_CASE("the loader and the checker reject a ceiling trap with no underside a jump reaches") {
	const LevelGrid ok = hall(0xf000, CeilingSpikes);
	CHECK(checkLevel(ok).valid);
	std::stringstream good;
	writeLevel(good, ok.cells);
	LevelGrid read;
	CHECK(readLevel(good, read.cells).empty());

	for (const std::uint16_t shape : {NO_SHAPE, static_cast<std::uint16_t>(0x000f)}) {
		CAPTURE(shape);
		const LevelGrid bad = hall(shape, CeilingDeath);
		CHECK_FALSE(checkLevel(bad).valid);
		std::stringstream file;
		writeLevel(file, bad.cells);
		LevelGrid back;
		CHECK(readLevel(file, back.cells).find("ceiling death trap at 4 1") != std::string::npos);
	}
}

TEST_CASE("the checker warns when the only way on jumps into ceiling spikes") {
	const LevelReport bare = checkLevel(hall(0xf00f, NoObject)); // a quarter step under a low ceiling
	REQUIRE(bare.valid);
	CHECK(bare.pathSteps == 1);
	CHECK(bare.pathCeilingSpikes == 0);
	CHECK_FALSE(warned(bare, "ceiling spikes"));

	const LevelReport spiked = checkLevel(hall(0xf00f, CeilingSpikes));
	REQUIRE(spiked.valid);
	CHECK(spiked.pathCeilingSpikes == 1);
	CHECK(warned(spiked, "the only way to the exit jumps into ceiling spikes"));

	const LevelReport flat = checkLevel(hall(0xf000, CeilingSpikes)); // no jump needed under it
	CHECK(flat.pathCeilingSpikes == 0);
	CHECK_FALSE(warned(flat, "ceiling spikes"));
}
