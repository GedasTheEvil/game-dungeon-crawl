// A monster's leap into a low ceiling (plan monster-head-bump), on tests/levels/monster_head_bump: a giant rat pounces
// at the player under a ceiling three quarters up (room 10 world units over its head, the arc's top 12), Khepri on the
// upper hall. The rule is the player's (src/level/head_bump.h, tests/unit/subcell_physics_test.cpp).
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/level/head_bump.h"
#include <algorithm>

namespace {
constexpr int PLAYER_COL = 15, LOW_ROW = 2, HIGH_ROW = 5;

bool heard(const SimWorld& world, WorldSound sound) {
	return std::any_of(world.said.begin(), world.said.end(), [&](const WorldEvent& e) {
		return e.kind == WorldEvent::Kind::Sound && e.sound == sound;
	});
}

const Monster* monsterOfType(const SimWorld& world, int type) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active() && world.dungeon.Monsters()[i].Type()->id == type)
			return &world.dungeon.Monsters()[i];
	return nullptr;
}

template <typename Pred> bool waitFor(SimWorld& world, int ms, Pred pred) {
	for (int t = 0; t < ms / UPDATE_TICK_MS; t++) {
		if (pred())
			return true;
		world.Tick();
	}
	return pred();
}

// What a pounce under the low ceiling did.
struct Pounce {
	bool pounced = false, bumped = false, stunned = false;
	int lost = 0; // HP of the pouncer
	float topLift = 0.f; // the highest the arc went, world units
	int stunMs = 0; // how long it stood after the landing
};

// Loads the level, puts the player under the ceiling on the hall's row, waits for the kind's pounce and its landing.
Pounce pounceUnderCeiling(SimWorld& world, int type, int row, int seed) {
	Pounce out;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/monster_head_bump"));
	world.random.Seed(static_cast<uint64_t>(seed));
	REQUIRE(world.dungeon.PlacePlayer(PLAYER_COL, row));
	world.Tick();
	const Monster* mon = monsterOfType(world, type);
	REQUIRE(mon != nullptr);
	const int full = mon->Health();
	out.pounced = waitFor(world, 6000, [&] { return mon->pouncing(); });
	while (mon->jumping()) {
		out.topLift = std::max(out.topLift, mon->DrawnLift());
		world.Tick();
	}
	out.bumped = heard(world, WorldSound::HeadBump);
	out.lost = full - mon->Health();
	for (int t = 0; mon->Reeling() && t < 200; t++, out.stunMs += UPDATE_TICK_MS)
		world.Tick();
	out.stunned = out.stunMs > 0;
	return out;
}
} // namespace

TEST_CASE("a giant rat that pounces under a low ceiling stops rising and loses the head bump's HP") {
	SimWorld world;
	const Pounce p = pounceUnderCeiling(world, MonsterGiantRat, LOW_ROW, 1);
	REQUIRE(p.pounced);
	CHECK(p.bumped);
	CHECK(p.lost == headBumpHp(monsterKind(MonsterGiantRat)->maxHealth));
	CHECK(p.topLift > 5.f); // it did rise
	CHECK(p.topLift < 10.f + 0.01f); // the room over its head: 3/4 tile - 2 quarters
	CHECK(monsterOfType(world, MonsterGiantRat)->Alive());
}

TEST_CASE("the same pounce in a tall hall: no bump, no damage, the full arc") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/pounce"));
	world.Tick();
	const Monster* rat = monsterOfType(world, MonsterGiantRat);
	REQUIRE(rat != nullptr);
	const int full = rat->Health();
	REQUIRE(waitFor(world, 4000, [&] { return rat->pouncing(); }));
	float top = 0.f;
	while (rat->jumping()) {
		top = std::max(top, rat->DrawnLift());
		world.Tick();
	}
	CHECK(top > 11.f);
	CHECK_FALSE(heard(world, WorldSound::HeadBump));
	CHECK(rat->Health() == full);
}

TEST_CASE("about half the bumps stun a leaper for 1 to 2 s") {
	int stunned = 0;
	const int runs = 40;
	for (int seed = 1; seed <= runs; seed++) {
		SimWorld world;
		const Pounce p = pounceUnderCeiling(world, MonsterGiantRat, LOW_ROW, seed);
		REQUIRE(p.bumped);
		if (p.stunned) {
			stunned++;
			CHECK(p.stunMs >= HEAD_BUMP_DAZE_MIN_MS - 3 * UPDATE_TICK_MS);
			CHECK(p.stunMs <= HEAD_BUMP_DAZE_MAX_MS + 3 * UPDATE_TICK_MS);
		}
	}
	CHECK(stunned > runs * 20 / 100);
	CHECK(stunned < runs * 80 / 100);
}

TEST_CASE("a boss leaper bumps as its kin: Khepri's stun is shorter and holds its cooldown") {
	int stunned = 0, bumps = 0;
	for (int seed = 1; seed <= 24; seed++) {
		SimWorld world;
		const Pounce p = pounceUnderCeiling(world, MonsterBossScarab, HIGH_ROW, seed);
		if (!p.bumped)
			continue;
		bumps++;
		CHECK(p.lost == headBumpHp(monsterKind(MonsterBossScarab)->maxHealth));
		if (p.stunned) {
			stunned++;
			CHECK(p.stunMs <= HEAD_BUMP_DAZE_MAX_MS * BOSS_STUN_PERCENT / 100 + 3 * UPDATE_TICK_MS);
			Monster& boss = const_cast<Monster&>(*monsterOfType(world, MonsterBossScarab));
			CHECK_FALSE(boss.StunHit(STUN_MS, false)); // the cooldown
		}
	}
	CHECK(bumps > 12);
	CHECK(stunned > 0);
}

TEST_CASE("after a bump the rat does not pounce again under that ceiling for a few seconds") {
	SimWorld world;
	const Pounce p = pounceUnderCeiling(world, MonsterGiantRat, LOW_ROW, 1);
	REQUIRE(p.bumped);
	const Monster* rat = monsterOfType(world, MonsterGiantRat);
	REQUIRE(rat->BumpedLately());
	const int cooldown = monsterKind(MonsterGiantRat)->pounce->cooldownMs;
	CHECK(cooldown < MONSTER_HEAD_BUMP_MEMORY_MS); // the memory is what holds it, not the cooldown
	// The player steps back to pounce range; the cooldown runs out, the memory still holds.
	REQUIRE(world.dungeon.PlacePlayer(17, LOW_ROW));
	CHECK_FALSE(waitFor(world, cooldown + 500, [&] { return rat->pouncing(); }));
}
