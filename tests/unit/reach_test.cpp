// Hitboxes and weapon reach (docs/plan/solved/scenarios-to-unit-tests.md technique 4): every size of walker stops where
// it bites, MONSTER_BITE_REACH from the player's box, and every melee weapon reaches it there, in normal and toon mode
// (the figures and their boxes are larger). The look of the reach against the weapon:
// tests/scenarios/melee_weapons.txt.
#include "../../external/doctest/doctest.h"
#include "../../src/entities/figures.h"
#include "sim_world.h"
#include <string>

namespace {
struct Walker {
	const char* level; // tests/levels/hitbox_*: the player at col 1, the walker two cells right
	int health;
	int comesMs; // time to walk up to the player
	bool boss;
};
constexpr Walker WALKERS[] = {
	{"tests/levels/hitbox_scarab", 10, 8000, false},	   {"tests/levels/hitbox_rat", 12, 8000, false},
	{"tests/levels/hitbox_worm", 30, 25000, false},		   {"tests/levels/hitbox_giant_scarab", 90, 12000, false},
	{"tests/levels/hitbox_boss_scarab", 320, 12000, true}, {"tests/levels/hitbox_giant_rat", 60, 8000, false}};

std::vector<ItemKind> meleeWeapons() {
	std::vector<ItemKind> kinds;
	for (int i = 0; i < WEAPON_KIND_COUNT; i++)
		if (!isRanged(static_cast<ItemKind>(i)))
			kinds.push_back(static_cast<ItemKind>(i));
	return kinds;
}

// The walker's health: the boss's, else the nearest monster's.
int health(const SimWorld& world, const Walker& w) {
	return w.boss ? world.dungeon.BossHealth() : world.dungeon.NearestMonsterHealth();
}

// Toon mode for the scope of a test.
struct Toon {
	explicit Toon(bool on) { Figures::SetToon(on); }
	~Toon() { Figures::SetToon(false); }
	Toon(const Toon&) = delete;
	Toon& operator=(const Toon&) = delete;
};
} // namespace

TEST_CASE("every melee weapon reaches at least as far as a walker stops to bite") {
	for (const ItemKind kind : meleeWeapons()) {
		CAPTURE(std::string(itemText(kind).name));
		CHECK(weaponReach(kind) >= MONSTER_BITE_REACH);
	}
}

TEST_CASE("toon mode draws the figures larger, and their hitboxes with them") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/hitbox_giant_rat"));
	const float normal = world.player.HalfWidth();
	const Toon toon(true);
	CHECK(world.player.HalfWidth() == doctest::Approx(normal * 1.2f));
}

TEST_CASE("every size of walker stops where it bites, and every melee weapon hits it there") {
	for (const bool toonMode : {false, true}) {
		const Toon toon(toonMode);
		for (const Walker& w : WALKERS)
			for (const ItemKind kind : meleeWeapons()) {
				CAPTURE(toonMode);
				CAPTURE(std::string(w.level));
				CAPTURE(std::string(itemText(kind).name));
				SimWorld world;
				world.player.god = true;
				REQUIRE(world.Load(w.level));
				REQUIRE(world.WalkTo(world.X() + 1.f));
				world.Wait(w.comesMs);
				const int before = health(world, w);
				CHECK(before == w.health);								// it came up to the player and stopped, unhurt
				CHECK(world.X() == doctest::Approx(2.f).epsilon(0.01)); // the player did not move on
				// A boss may have a minion in front of it: strike until one reaches it (as the scenario did).
				for (int swing = 0; swing < (w.boss ? 16 : 1) && health(world, w) == before; swing++) {
					const int damage = world.player.stats.Damage(weaponDamage(kind, weaponDef(kind).damage, 1));
					CHECK(world.dungeon.AttackNearest(damage, weaponDef(kind).mix, weaponReach(kind), 1));
					world.Wait(1000);
				}
				CHECK(health(world, w) < before);
			}
	}
}

TEST_CASE("an arrow hits a giant rat two tiles away behind spikes it cannot leap") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/hitbox_bow"));
	REQUIRE(world.WalkTo(world.X() + 3.5f));
	world.Wait(500);
	REQUIRE(world.dungeon.NearestMonsterHealth() == 60);
	world.dungeon.Shoot(MissileKind::Arrow, 10, weaponDef(ItemKind::SelfBow).mix, 1, 0.5f * world.player.Height(),
						weaponReach(ItemKind::SelfBow));
	world.Wait(1500);
	CHECK(world.dungeon.NearestMonsterHealth() < 60);
}
