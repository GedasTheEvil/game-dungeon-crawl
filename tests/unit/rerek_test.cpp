// Rerek, the sand serpent (plan worm-boss): the worm's boss. Lies under the sand, wakes and dives from farther than the
// worm, swims under it faster, comes up under the player or (every other dive) past them in a spray of sand, swallows
// the player into the sand, and worms dig out round him into the sand, where they lie in wait.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
const Monster* rerek(const SimWorld& world) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active() && world.dungeon.Monsters()[i].Type()->id == MonsterRerek)
			return &world.dungeon.Monsters()[i];
	return nullptr;
}

// tests/levels/rerek: Rerek in col 24 under a sand floor (cols 4-35), the player in the sand west of him, out of his
// wake range. A player who lives through the fight.
constexpr float REREK_X = 24.5f;
const Monster* loadSand(SimWorld& world) {
	REQUIRE(world.Load("tests/levels/rerek"));
	world.player.stats.AddXP(1000000, world.events);
	REQUIRE(world.WalkTo(19.f)); // he comes into view
	world.Tick();
	return rerek(world);
}

// Woken (under the player near REREK_X - his wake range) and up again.
const Monster* loadAwake(SimWorld& world) {
	const Monster* r = loadSand(world);
	REQUIRE(r != nullptr);
	world.player.god = true;
	REQUIRE(world.WalkTo(REREK_X - REREK_WAKE_RANGE + 0.1f));
	world.Wait(REREK_UNDER_MS + BURROW_SURFACE_MS + 100);
	REQUIRE(r->Alerted());
	REQUIRE_FALSE(r->Burrowing());
	return r;
}

// Ticks until pred holds, at most ms.
template <typename Pred> bool waitFor(SimWorld& world, int ms, Pred pred) {
	for (int t = 0; t < ms / UPDATE_TICK_MS; t++) {
		if (pred())
			return true;
		world.Tick();
	}
	return pred();
}
} // namespace

TEST_CASE("Rerek is the worm's boss: the sand burrowing farther and faster, comes up behind, sprays, swallows") {
	const MonsterKind& r = *monsterKind(MonsterRerek);
	const MonsterKind& worm = *monsterKind(MonsterWorm);
	CHECK(r.isBoss());
	CHECK(r.kin == MonsterWorm);
	CHECK(r.boss.minion == MonsterWorm);
	CHECK(r.boss.summon == Summon::DigOut);
	REQUIRE(r.sandBurrow);
	REQUIRE(worm.sandBurrow);
	CHECK(r.sandBurrow->wakeRange > worm.sandBurrow->wakeRange);
	CHECK(r.sandBurrow->diveRange > worm.sandBurrow->diveRange);
	CHECK(r.sandBurrow->underMs < worm.sandBurrow->underMs);
	CHECK(r.sandBurrow->everyMs < worm.sandBurrow->everyMs);
	CHECK(r.sandBurrow->behind);
	CHECK_FALSE(worm.sandBurrow->behind);
	REQUIRE(r.hold);
	CHECK(r.hold->swallow);
	CHECK(r.hold->breakMs > 0);
	CHECK(r.spray);
	CHECK_FALSE(r.poison);
	CHECK(r.speed < monsterKind(MonsterApep)->speed); // out of sand he walks, slowly
	CHECK(r.resist[static_cast<size_t>(DamageType::Blunt)] == RESISTS);
	CHECK(r.resist[static_cast<size_t>(DamageType::Slash)] == NORMAL); // the worm's weakness dropped
	CHECK(r.resist[FIRE] == NORMAL);
}

TEST_CASE("Rerek lies under the sand and comes up under the player from farther than a worm, spraying sand") {
	SimWorld world;
	const Monster* r = loadSand(world);
	REQUIRE(r != nullptr);
	CHECK(r->UnderSand());
	CHECK(r->Hidden());
	CHECK(r->BurrowRipple().has_value());
	REQUIRE(world.WalkTo(REREK_X - REREK_WAKE_RANGE + 0.1f)); // in his wake range, out of a worm's
	REQUIRE(REREK_X - world.X() > SAND_WORM_WAKE_RANGE);
	const float x = world.X();
	REQUIRE(waitFor(world, REREK_UNDER_MS + 200, [&] { return !r->Hidden(); }));
	CHECK(r->Alerted());
	CHECK(world.Saw(MonsterRerek, CreatureMove::Burrow));
	CHECK(r->CentreX() == doctest::Approx(x).epsilon(0.02)); // under the player's feet
	CHECK(world.dungeon.SandHaze() > 0.5f); // the spray
	world.Wait(300);
	CHECK(std::fabs(world.X() - x) >= REREK_SPRAY_SHOVE - 0.05f); // thrown off him
}

TEST_CASE("Rerek dives from farther than a worm, and every other time comes up past the player") {
	SimWorld world;
	const Monster* r = loadAwake(world);
	const auto col = static_cast<int>(r->CentreX());
	REQUIRE(world.dungeon.PlacePlayer(col - 8, 2)); // out of a worm's dive range, in his
	const float x = world.X();
	REQUIRE(waitFor(world, REREK_EVERY_MS + 500, [&] { return r->Burrowing(); }));
	CHECK(r->Dives() == 1);
	REQUIRE(waitFor(world, BURROW_DIVE_MS + REREK_UNDER_MS + 200, [&] { return !r->Hidden(); }));
	CHECK(r->CentreX() == doctest::Approx(x - BURROW_BEHIND).epsilon(0.01)); // past the player, on the far side
	REQUIRE(waitFor(world, BURROW_SURFACE_MS + 100, [&] { return !r->Burrowing(); }));

	REQUIRE(world.dungeon.PlacePlayer(static_cast<int>(r->CentreX()) + 4, 2)); // the next one: under the player
	const float x2 = world.X();
	REQUIRE(waitFor(world, REREK_EVERY_MS + 500, [&] { return r->Burrowing(); }));
	CHECK(r->Dives() == 2);
	REQUIRE(waitFor(world, BURROW_DIVE_MS + REREK_UNDER_MS + 200, [&] { return !r->Hidden(); }));
	CHECK(r->CentreX() == doctest::Approx(x2).epsilon(0.01));
}

TEST_CASE("a worm's first dive comes up under the player, not past them as Rerek's") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/sand_worm"));
	world.player.god = true;
	REQUIRE(world.WalkTo(18.5f)); // the worm at col 21, in sand 14-24
	REQUIRE(world.WalkTo(19.2f));
	world.Wait(BURROW_UNDER_MS + BURROW_SURFACE_MS + 100);
	const Monster* w = world.dungeon.NearestMonster();
	REQUIRE(w != nullptr);
	REQUIRE(w->Alerted());
	const float wx = w->CentreX();
	REQUIRE(world.dungeon.PlacePlayer(static_cast<int>(wx) + 3, 2));
	REQUIRE(waitFor(world, BURROW_EVERY_MS + 500, [&] { return w->Burrowing(); }));
	const float x = world.X();
	REQUIRE(waitFor(world, BURROW_DIVE_MS + BURROW_UNDER_MS + 200, [&] { return !w->Hidden(); }));
	CHECK(w->CentreX() == doctest::Approx(x).epsilon(0.01)); // its first dive: under the player
}

TEST_CASE("Rerek's swallow pulls the player into the sand; hits on him break it sooner") {
	SimWorld world;
	const Monster* r = loadAwake(world);
	world.player.god = false;
	world.Give(ItemKind::Khopesh);
	world.Equip(ItemKind::Khopesh);
	REQUIRE(world.dungeon.GrabNearest());
	REQUIRE(waitFor(world, GRAB_TELL_MS + 100, [&] { return r->Holding(); }));
	CHECK(world.Saw(MonsterRerek, CreatureMove::Grab));
	CHECK(world.Told("Swallowed"));
	world.Wait(REREK_SWALLOW_SINK_MS);
	CHECK(world.dungeon.PlayerSink() == doctest::Approx(REREK_SWALLOW_DEPTH * RenderConfig::TILE_SIZE));
	CHECK(r->HoldRoll() == 0.f); // no death roll
	CHECK(std::fabs(world.dungeon.HeldViewRoll()) <= HELD_SHAKE_DEG);
	const int start = GameClock::now();
	for (int k = 0; k < 6 && r->Holding(); k++) {
		world.facing = r->CentreX() < world.X() ? -1 : 1;
		world.Attack();
		world.Wait(weaponDef(ItemKind::Khopesh).motion.AttackMs());
	}
	CHECK_FALSE(r->Holding());
	CHECK(GameClock::now() - start + REREK_SWALLOW_SINK_MS < monsterKind(MonsterRerek)->hold->holdMs);
	CHECK(world.dungeon.PlayerSink() == 0.f);
}

TEST_CASE("worms dig out round Rerek into the sand and lie in wait there") {
	SimWorld world;
	const Monster* r = loadSand(world);
	REQUIRE(r != nullptr);
	int buried = 0;
	for (int i = 0; i < MAX_MONSTERS; i++) {
		const Monster& m = world.dungeon.Monsters()[i];
		if (m.Active() && m.Minion() && m.Type()->id == MonsterWorm) {
			CHECK(m.UnderSand());
			CHECK_FALSE(m.Emerging());
			buried++;
		}
	}
	CHECK(buried == monsterKind(MonsterRerek)->boss.minAlive);
}
