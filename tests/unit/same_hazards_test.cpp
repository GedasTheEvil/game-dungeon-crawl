// Same hazards for all (plan same-hazards-for-all): a trap, a burning floor or a web treats the player and the monsters
// by one rule; a kind differs only by its numbers. A reckless kind's walk through spikes costs about what it did before
// the plan (its trap share re-tuned), Babi's blaze burns a monster too (not Babi, native to fire), and the player has a
// terrain affinity: hostile to webs.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/level/status_list.h"
#include <algorithm>
#include <cmath>

namespace {
// A walk through floor spikes at its walk pace, in HP after its trap share.
double spikesWalk(const Monster& mon) {
	const MonsterType& kind = *mon.Type();
	return TrapBell::walkHundredths(false, kind.maxHealth, mon.WalkPace(), mon.HalfWidth()) / 100.0 *
		   kind.trapPermille / FULL_TRAP_SHARE;
}

const Monster* find(const SimWorld& world, int type) {
	for (int i = 0; i < MAX_MONSTERS; i++) {
		const Monster& mon = world.dungeon.Monsters()[i];
		if (mon.Active() && mon.Type()->id == type)
			return &mon;
	}
	return nullptr;
}

// Sets the whole row of the player burning for ms.
void blazeRow(SimWorld& world, int ms) {
	const auto row = static_cast<int>(std::floor(world.Y()));
	for (int col = 0; col < LEVEL_WIDTH; col++)
		(void)world.dungeon.PlaceBlaze(col, row, ms);
}
} // namespace

TEST_CASE("a reckless kind's walk through spikes costs within a quarter of what it did before the one formula") {
	// HP a walk through floor spikes cost each reckless kind before the plan: TrapHurt's ramp over its crossing at
	// its walk pace, times its old trap percent.
	const struct {
		int type;
		double before;
	} KINDS_BEFORE[] = {
		{MonsterAnubis, 5.5},
		{MonsterMummy, 713.0},
		{MonsterAnubisBoss, 2.2},
		{MonsterScorpionQueen, 92.5},
		{MonsterApep, 95.0},
		{MonsterSobek, 145.7},
		{MonsterMummyPriest, 100.1},
		{MonsterSeth, 175.1},
		{MonsterRerek, 238.0},
		{MonsterBabi, 4.0},
	};
	SimWorld world;
	REQUIRE(world.Load("tests/levels/empty_row"));
	const MonsterLinks links{&world.player, &world.journal, &world.events, 1, &world.random.gameplay};
	int reckless = 0;
	for (const MonsterType& kind : SimWorld::monsterTypes())
		reckless += kind.courage == Courage::Reckless ? 1 : 0;
	CHECK(reckless == static_cast<int>(std::size(KINDS_BEFORE))); // a new reckless kind: pin it here
	for (const auto& k : KINDS_BEFORE) {
		const MonsterType& kind = SimWorld::monsterTypes()[static_cast<size_t>(k.type)];
		CAPTURE(kind.label);
		REQUIRE(kind.courage == Courage::Reckless);
		Monster mon;
		mon.Spawn(kind, 3, 1, links, world.random.effects);
		const double now = spikesWalk(mon);
		CHECK(now >= k.before * 0.75);
		CHECK(now <= k.before * 1.25);
	}
}

TEST_CASE("a boss takes a smaller share of a trap than its kin") {
	for (const MonsterType& kind : SimWorld::monsterTypes())
		if (kind.isBoss() && kind.courage == Courage::Reckless && kind.kin > 0) {
			CAPTURE(kind.label);
			const MonsterType& kin = SimWorld::monsterTypes()[static_cast<size_t>(kind.kin)];
			if (kin.courage == Courage::Reckless)
				CHECK(kind.trapPermille <= kin.trapPermille);
		}
}

TEST_CASE("a burning floor burns a monster standing in it as it burns the player, and its kill is not the player's") {
	SimWorld world; // tests/levels/reckless_anubis: the Anubis guard at col 6
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/reckless_anubis"));
	world.Tick();
	const Monster* guard = find(world, MonsterAnubis);
	REQUIRE(guard != nullptr);
	const int full = guard->Health();
	const double xp = world.player.stats.CurrentXP();
	blazeRow(world, 20000);
	world.Wait(1500);
	CHECK(guard->Health() < full);
	CHECK(guard->Burns());
	CHECK(world.player.stats.CurrentXP() == xp);
}

TEST_CASE("Babi walks in his own blaze unharmed: native to fire") {
	SimWorld world; // tests/levels/babi_hall: Babi at col 6
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/babi_hall"));
	world.player.stats.AddXP(1000000, world.events);
	REQUIRE(world.dungeon.SpawnMonster(6, static_cast<int>(std::floor(world.Y()))));
	world.Tick();
	const Monster* babi = find(world, MonsterBabi);
	REQUIRE(babi != nullptr);
	REQUIRE(babi->Type()->native.has(Terrain::Fire));
	const int full = babi->Health();
	blazeRow(world, 20000);
	world.Wait(1500);
	CHECK(babi->Health() == full);
	CHECK_FALSE(babi->Burns());
}

TEST_CASE("the player's terrain: a web is hostile (its slow, a condition card), elsewhere neutral") {
	SimWorld world; // tests/levels/mother: a hall a web lies on (mother_test.cpp)
	REQUIRE(world.Load("tests/levels/mother"));
	world.player.god = true;
	const auto row = static_cast<int>(std::floor(world.Y()));
	const auto has = [](const StatusList& list, PlayerHud::Icon icon) {
		return std::any_of(list.items.begin(), list.items.begin() + list.count, [icon](const Status& s) {
			return s.icon == icon;
		});
	};
	CHECK(world.player.stats.TerrainHostile() == Terrains{Terrain::Web});
	CHECK_FALSE(world.player.stats.TerrainNative().any());
	CHECK(world.dungeon.PlayerTerrainMood() == TerrainMood::Neutral);
	CHECK_FALSE(
		has(statusList(world.player.stats, world.dungeon.PlayerTerrainMood()), PlayerHud::Icon::TerrainHostile)
	);
	REQUIRE(world.dungeon.PlaceWeb(3, row));
	REQUIRE(world.WalkTo(3.5f));
	CHECK(world.dungeon.PlayerTerrainMood() == TerrainMood::Hostile);
	CHECK(world.dungeon.PlayerTerrainFactors().speed == doctest::Approx(WEB_SLOW_FACTOR));
	CHECK(world.dungeon.PlayerTerrainFactors().damage == doctest::Approx(1.f)); // only the slow, as before the plan
	const StatusList list = statusList(world.player.stats, world.dungeon.PlayerTerrainMood());
	CHECK(has(list, PlayerHud::Icon::TerrainHostile));
	world.Jump();
	world.Tick();
	CHECK(world.dungeon.PlayerTerrainMood() == TerrainMood::Neutral); // in the air: none of the ground's
}

TEST_CASE("a monster in a death trap is poisoned once per entry, as the player") {
	SimWorld world; // tests/levels/reckless_spikes: the mummy walks into the spikes
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/reckless_spikes"));
	TrapHurt hurt;
	const Dungeon::TrapContact death{Death, 0.f, 0.5f, 1.f};
	CHECK(Dungeon::TrapVenomDue(hurt, death, 100));
	CHECK_FALSE(Dungeon::TrapVenomDue(hurt, death, 100)); // still in it
	CHECK_FALSE(Dungeon::TrapVenomDue(hurt, std::nullopt, 0)); // out
	CHECK(Dungeon::TrapVenomDue(hurt, death, 100)); // in again
	const Dungeon::TrapContact spikes{Spike, 0.f, 0.5f, 1.f};
	CHECK_FALSE(Dungeon::TrapVenomDue(hurt, spikes, 100)); // the spikes have no venom
}

TEST_CASE("the player and a monster of the same max HP and share take the same walk through spikes") {
	TrapHurt player, monster;
	const Dungeon::TrapContact spikes{Spike, 0.5f, 4.5f, 0.5f};
	player.lastX = monster.lastX = 4.f;
	GameClock::enableVirtual();
	GameClock::advance(TRAP_HURT_INTERVAL_MS);
	const int byPlayer = Dungeon::TrapWalkHundredths(player, 4.02f, spikes, 1000, WALK_SPEED, false);
	const int byMonster = Dungeon::TrapWalkHundredths(monster, 4.02f, spikes, 1000, WALK_SPEED, false);
	CHECK(byPlayer > 0);
	CHECK(byPlayer == byMonster);
}
