// Test weapons (plan test-weapons): 1 HP a hit whatever might, an amulet, a weakness or a power hit say, for scenarios
// that need many hits on one monster. Never loot.
#include "../../external/doctest/doctest.h"
#include "../../src/level/loot.h"
#include "sim_world.h"
#include "../../src/level/tuning/gameplay_shared.h"

namespace {
// The fire level's mummy (weak to slash, pierce and fire) awake and next to the player, who holds the weapon, has
// might and a grand amulet of the weapon's type.
void readyTestWeapon(SimWorld& world, ItemKind weapon, ItemKind amulet) {
	world.player.god = true;
	REQUIRE(world.Load("tests/levels/fire"));
	world.Equip(weapon);
	world.player.stats.AddMight(20);
	world.Wear(amulet);
	REQUIRE(world.WalkTo(12.f)); // the mummy wakes and climbs out
	REQUIRE(world.WalkTo(9.5f)); // back
	world.Wait(3000);
	const Monster* mummy = world.dungeon.NearestMonster();
	REQUIRE(mummy != nullptr);
	REQUIRE(world.WalkTo(mummy->Left() - world.player.HalfWidth() - 0.1f)); // up to it, facing it
	REQUIRE(world.Nearest() == monsterKind(MonsterMummy)->maxHealth);
}

// One swing or shot, up to just after its hit lands: the mummy's HP lost.
int lostByOne(SimWorld& world, ItemKind weapon) {
	const int before = world.Nearest();
	REQUIRE(world.Attack());
	world.Wait(weaponDef(weapon).motion.hitMs + UPDATE_TICK_MS);
	return before - world.Nearest();
}
} // namespace

TEST_CASE("test weapons: exactly 1 HP a hit, with might and a damage amulet on, on a monster weak to the type") {
	const struct {
		ItemKind weapon;
		ItemKind amulet;
	} CASES[] = {
		{ItemKind::SmashTest, ItemKind::CrushingGrand},
		{ItemKind::StabTest, ItemKind::PiercingGrand},
		{ItemKind::CutTest, ItemKind::CleavingGrand},
		{ItemKind::FireTest, ItemKind::CrushingGrand},
	};
	for (const auto& c : CASES) {
		SimWorld world;
		readyTestWeapon(world, c.weapon, c.amulet);
		CHECK(lostByOne(world, c.weapon) == 1);
	}
}

TEST_CASE("test weapons: the bow test hits for 1 HP and never spends a shot") {
	SimWorld world;
	readyTestWeapon(world, ItemKind::BowTest, ItemKind::PiercingGrand);
	REQUIRE(world.WalkTo(world.X() - 3.f));
	world.facing = 1; // back at the mummy
	const int before = world.Nearest();
	REQUIRE(world.Attack());
	world.Wait(2000);
	CHECK(before - world.Nearest() == 1);
	CHECK(
		world.items.ShotsLeft(ItemKind::BowTest, weaponDef(ItemKind::BowTest).shots) ==
		weaponDef(ItemKind::BowTest).shots
	);
}

TEST_CASE("test weapons: by file ids 8 to 11 and ranged 5, never loot") {
	CHECK(itemFromFile(ItemType::MELEE_WEAPON, 8) == ItemKind::SmashTest);
	CHECK(itemFromFile(ItemType::MELEE_WEAPON, 9) == ItemKind::StabTest);
	CHECK(itemFromFile(ItemType::MELEE_WEAPON, 10) == ItemKind::CutTest);
	CHECK(itemFromFile(ItemType::MELEE_WEAPON, 11) == ItemKind::FireTest);
	CHECK(itemFromFile(ItemType::RANGED_WEAPON, 5) == ItemKind::BowTest);
	CHECK_FALSE(isRanged(ItemKind::FireTest));
	CHECK(isRanged(ItemKind::BowTest));
	OwnedWeapons owned{};
	for (int i = 0; i < WEAPON_KIND_COUNT; i++)
		owned[static_cast<size_t>(i)] = isTestWeapon(itemAt(i));
	Rng rng(1);
	for (int i = 0; i < 200; i++) {
		CHECK_FALSE(isTestWeapon(RollTreasureItem(20, rng)));
		CHECK_FALSE(isTestWeapon(RollMimicLoot(owned, rng)));
		const std::optional<ItemKind> drop = RollKillDrop(false, owned, 20, rng);
		CHECK_FALSE((drop && isTestWeapon(*drop)));
	}
}
