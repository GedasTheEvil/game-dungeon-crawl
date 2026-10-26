// Sand and quicksand (plan desert-chapter): sand slows the walk and stops the sprint, a jump still works; quicksand
// sinks the player (slower, stuck, hurt) until a jump frees them; a sand fall hits, slows and leaves a drift.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/world/campaign.h"
#include "../../src/world/level_check.h"
#include <algorithm>
#include <cmath>

namespace {
bool noted(const SimWorld& world, FieldNote note) {
	return std::any_of(world.said.begin(), world.said.end(), [note](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Note && e.note == note;
	});
}

// tests/levels/sand: dry floor to col 5, sand 6-11, dry 12-19 with a sand fall at 15, quicksand 20-21, dry on.
void loadSand(SimWorld& world) {
	REQUIRE(world.Load("tests/levels/sand"));
	world.player.stats.AddXP(1000000, world.events);
}

// Tiles walked east in ticks steps of the walk key.
float walked(SimWorld& world, int ticks) {
	const float from = world.X();
	for (int t = 0; t < ticks; t++) {
		world.Step(1);
		world.Tick();
	}
	return world.X() - from;
}
} // namespace

TEST_CASE("sand: the walk at 3/4 speed, no sprint, the field note, a jump still works") {
	SimWorld world;
	loadSand(world);
	REQUIRE(world.WalkTo(3.f));
	const float dry = walked(world, 20);
	REQUIRE(world.WalkTo(7.f));
	CHECK(world.dungeon.PlayerInSand());
	CHECK(noted(world, FieldNote::Sand));
	const float sand = walked(world, 20);
	CHECK(sand == doctest::Approx(dry * SAND_SPEED_FACTOR).epsilon(0.02));

	world.player.stats.SetSprintRequested(true);
	CHECK_FALSE(world.dungeon.PlayerCanSprint());
	const int stamina = world.player.stats.Stamina();
	const float sprint = walked(world, 20);
	CHECK(sprint == doctest::Approx(sand).epsilon(0.02)); // no faster
	CHECK(world.player.stats.Stamina() >= stamina); // and no drain
	world.player.stats.SetSprintRequested(false);

	world.Jump();
	world.Tick();
	CHECK(world.player.jump.jumping);
	CHECK_FALSE(world.dungeon.PlayerInSand()); // in the air
}

TEST_CASE("quicksand: slower each second, stuck, then hurt; a jump frees the player for more stamina") {
	SimWorld world;
	loadSand(world);
	REQUIRE(world.WalkTo(20.3f));
	REQUIRE(world.dungeon.PlayerInQuicksand());
	CHECK(noted(world, FieldNote::Quicksand));
	const float start = walked(world, 5);
	CHECK(start > 0.f);
	world.Wait(QUICKSAND_STUCK_MS);
	CHECK(world.dungeon.PlayerWalkFactor() == 0.f);
	CHECK(walked(world, 10) == 0.f); // stuck
	CHECK(world.dungeon.QuicksandSink() > 0.f);
	const int hp = world.Hp();
	world.Wait(QUICKSAND_HURT_MS - QUICKSAND_STUCK_MS + QUICKSAND_HURT_EVERY_MS);
	CHECK(world.Hp() < hp);
	CHECK(world.dungeon.PlayerInDanger());

	world.player.stats.SetStamina(world.player.stats.MaxStamina());
	const int stamina = world.player.stats.Stamina();
	world.facing = 1;
	world.Jump();
	world.Tick();
	CHECK(world.player.jump.jumping);
	CHECK(world.player.stats.Stamina() <= stamina - QUICKSAND_JUMP_STAMINA);
	CHECK(world.dungeon.PlayerSunkMs() == 0);
}

TEST_CASE("quicksand: no jump out without the stamina for it") {
	SimWorld world;
	loadSand(world);
	REQUIRE(world.WalkTo(20.3f));
	world.player.stats.SetStamina(QUICKSAND_JUMP_STAMINA - 1);
	world.Jump();
	world.Tick();
	CHECK_FALSE(world.player.jump.jumping);
}

TEST_CASE("a sand fall: a hit and a short slow for the player under it, the cell turns to sand") {
	SimWorld world;
	loadSand(world);
	REQUIRE(world.WalkTo(15.5f));
	REQUIRE(world.dungeon.Cell(15, 2).type == RockFall);
	const int hp = world.Hp();
	world.Wait(ROCK_WARN_MS + ROCK_FALL_MS + 100);
	CHECK(world.Hp() < hp);
	CHECK(world.Hp() > hp - ROCK_GRAZE_DAMAGE); // lighter than a rock
	CHECK(world.dungeon.Cell(15, 2).structure == Structure::Sand);
	CHECK(world.dungeon.PlayerInSand());
	CHECK(world.dungeon.PlayerWalkFactor() == doctest::Approx(SAND_SLOW_FACTOR * SAND_SPEED_FACTOR));
	world.Wait(SAND_SLOW_MS);
	CHECK(world.dungeon.PlayerWalkFactor() == doctest::Approx(SAND_SPEED_FACTOR));
}

TEST_CASE("a sand fall walked through without stopping does not hit") {
	SimWorld world;
	loadSand(world);
	REQUIRE(world.WalkTo(13.f));
	const int hp = world.Hp();
	REQUIRE(world.WalkTo(18.f));
	world.Wait(ROCK_WARN_MS + ROCK_FALL_MS);
	CHECK(world.Hp() == hp);
	CHECK(world.dungeon.Cell(15, 2).structure == Structure::Sand);
}

namespace {
// tests/levels/sand_worm: sand 6-10 with a worm at col 7, dry 11-13, sand 14-24 with a worm at col 21.
const Monster* wormNear(const SimWorld& world, float x) {
	for (int i = 0; i < MAX_MONSTERS; i++) {
		const Monster& m = world.dungeon.Monsters()[i];
		if (m.Active() && m.Type()->id == MonsterWorm && std::fabs(static_cast<float>(m.Col()) + 0.5f - x) < 1.f)
			return &m;
	}
	return nullptr;
}
} // namespace

TEST_CASE("a worm in sand lies under it, hidden, and comes up at the sand's edge for a player beside it") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/sand_worm"));
	world.player.stats.AddXP(1000000, world.events);
	REQUIRE(world.WalkTo(4.f));
	const Monster* w = wormNear(world, 7.5f);
	REQUIRE(w != nullptr);
	CHECK(w->UnderSand());
	CHECK(w->Hidden());
	CHECK_FALSE(w->Alerted());
	CHECK(w->BurrowRipple().has_value());
	REQUIRE(world.WalkTo(5.5f)); // on the dry floor, 2 tiles off
	world.Wait(BURROW_UNDER_MS + BURROW_SURFACE_MS + 100);
	CHECK_FALSE(w->UnderSand());
	CHECK_FALSE(w->Hidden());
	CHECK(w->Alerted());
	CHECK(world.Saw(MonsterWorm, CreatureMove::Burrow));
	CHECK(w->CentreX() < 7.f); // came up at the west end of its sand (6.5), walking on
}

TEST_CASE("a worm in sand comes up under the player in its sand, and dives again to reach them") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/sand_worm"));
	world.player.stats.AddXP(1000000, world.events);
	REQUIRE(world.WalkTo(18.5f));
	const Monster* w = wormNear(world, 21.5f);
	REQUIRE(w != nullptr);
	REQUIRE(w->UnderSand());
	REQUIRE(world.WalkTo(19.2f)); // within its wake range, in its sand
	const float x = world.X();
	world.Wait(BURROW_UNDER_MS + BURROW_SURFACE_MS + 50);
	CHECK(w->Alerted());
	CHECK(w->CentreX() == doctest::Approx(x).epsilon(0.02)); // under the player's feet

	bool dived = false; // away through the sand, past its dive range minimum
	while (world.X() > 15.f) {
		world.Step(-1);
		world.Tick();
		dived = dived || w->Hidden();
	}
	for (int t = 0; t < (BURROW_EVERY_MS + 3000) / UPDATE_TICK_MS && !dived; t++) {
		world.Tick();
		dived = w->Hidden();
	}
	CHECK(dived);
	world.Wait(BURROW_DIVE_MS + BURROW_UNDER_MS + BURROW_SURFACE_MS);
	CHECK(std::fabs(w->CentreX() - world.X()) < 1.f);
}

TEST_CASE("a worm off the sand walks as before; no boss lacks its kin's sand burrowing") {
	CHECK(monsterKind(MonsterWorm)->sandBurrow);
	for (int type = 1; type <= MONSTER_TYPE_MAX; type++) {
		const MonsterKind& k = *monsterKind(type);
		if (k.isBoss() && monsterKind(k.kin)->sandBurrow)
			CHECK(k.sandBurrow);
	}
}

TEST_CASE("the campaign: Khepri at 4, the desert at 19-23 with Seth at its end, Sobek at 28, the Anubis boss last") {
	CHECK(CAMPAIGN_LEVELS == 33);
	auto bossOf = [](int n) {
		LevelGrid grid;
		REQUIRE(loadLevelFile(campaignLevelFile(n).c_str(), grid).empty());
		int boss = 0;
		for (int cell = 0; cell < LEVEL_WIDTH * LEVEL_HEIGHT; cell++)
			if (grid.cells[cell].type == MonsterSpawn && isBossMonster(grid.cells[cell].attr))
				boss = grid.cells[cell].attr;
		return boss;
	};
	CHECK(bossOf(4) == MonsterBossScarab);
	CHECK(bossOf(9) == MonsterVampireBat);
	CHECK(bossOf(13) == MonsterMotherOfWebs);
	CHECK(bossOf(18) == MonsterApep);
	CHECK(bossOf(23) == MonsterSeth);
	CHECK(bossOf(28) == MonsterSobek);
	CHECK(bossOf(32) == MonsterMummyPriest);
	CHECK(bossOf(33) == MonsterAnubisBoss);
	const Zone* desert = zoneOf(19);
	REQUIRE(desert != nullptr);
	CHECK(desert->id == ZoneId::SethsDesert);
	CHECK(desert->last == 23);
	for (int n = desert->first; n <= desert->last; n++) {
		CAPTURE(n);
		LevelGrid grid;
		REQUIRE(loadLevelFile(campaignLevelFile(n).c_str(), grid).empty());
		const LevelReport r = checkLevel(grid);
		CHECK(r.valid);
		CHECK(r.warnings.empty());
		CHECK(r.sandCells > 50);
		CHECK(r.quicksandCells > 0);
		CHECK(r.sandFalls > 0);
		CHECK(r.monsters[MonsterCrocodile] == 0); // no water until the oasis
		for (int cell = 0; cell < LEVEL_WIDTH * LEVEL_HEIGHT; cell++) // the worms lie in the sand
			if (grid.cells[cell].type == MonsterSpawn && grid.cells[cell].attr == MonsterWorm)
				CHECK(inSand(grid.cells[cell]));
	}
}

TEST_CASE(
	"sprint dust (plan sprint-toon-blur-and-extras): a puff per sprint step on a dry floor, none walking or with "
	"motion effects off"
) {
	SimWorld world;
	loadSand(world);
	REQUIRE(world.WalkTo(3.f));
	walked(world, 20);
	CHECK(world.dungeon.DustPuffs() == 0); // walking
	world.player.stats.SetSprintRequested(true);
	walked(world, 20);
	CHECK(world.dungeon.DustPuffs() > 0);
	world.player.stats.SetSprintRequested(false);
}

TEST_CASE("sprint dust: none with motion effects off") {
	SimWorld world;
	loadSand(world);
	world.dungeon.SetMotionEffects(false);
	REQUIRE(world.WalkTo(3.f));
	world.player.stats.SetSprintRequested(true);
	walked(world, 20);
	CHECK(world.dungeon.DustPuffs() == 0);
}
