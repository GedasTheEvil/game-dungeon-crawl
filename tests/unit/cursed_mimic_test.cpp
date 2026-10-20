// The cursed mimic (plan cursed-mimic): a darker chest that lies in wait like the mimic, then hops after the player,
// leaps traps, and a quarter of its bites lay insanity. Killed, it leaves its chest where it died.
#include "../../external/doctest/doctest.h"
#include "sim_world.h"
#include "../../src/world/level_gen.h"
#include "../../src/world/loot.h"

namespace {
const Monster* find(const SimWorld& world) {
	for (int i = 0; i < MAX_MONSTERS; i++)
		if (world.dungeon.Monsters()[i].Active() && world.dungeon.Monsters()[i].Type()->id == MonsterCursedMimic)
			return &world.dungeon.Monsters()[i];
	return nullptr;
}

// tests/levels/cursed_mimic: the player in col 5, a death trap in col 7, the cursed mimic in col 9.
const Monster* woken(SimWorld& world) {
	REQUIRE(world.Load("tests/levels/cursed_mimic"));
	world.player.god = true;
	world.Tick();
	const Monster* m = find(world);
	REQUIRE(m != nullptr);
	REQUIRE(world.dungeon.AttackNearest(1, DamageMix{0, 0, 100}, 7.f, 1)); // an arrow wakes it
	return m;
}
} // namespace

TEST_CASE("the cursed mimic is the mimic's stronger kin, a hopper whose bite may lay insanity") {
	const MonsterKind& mimic = *monsterKind(MonsterMimic);
	const MonsterKind& cursed = *monsterKind(MonsterCursedMimic);
	CHECK(cursed.glyph == 'x');
	CHECK(cursed.locomotion == Locomotion::AmbushHop);
	CHECK(ambushes(cursed.locomotion));
	CHECK(leaps(cursed.locomotion));
	CHECK(cursed.maxHealth > mimic.maxHealth);
	CHECK(cursed.damage > mimic.damage);
	CHECK(cursed.xp > mimic.xp);
	CHECK(cursed.scale > mimic.scale);
	CHECK(cursed.threat > mimic.threat);
	CHECK(cursed.speed > 0);
	REQUIRE(cursed.meleeCurses[0].kind == Curse::Insanity);
	CHECK(cursed.meleeCurses[0].percent == 25);
	for (int type = 1; type <= MONSTER_TYPE_MAX; type++)
		if (type != MonsterCursedMimic)
			for (const CurseEntry& e : monsterKind(type)->meleeCurses)
				CHECK(e.kind != Curse::Insanity); // the only source of insanity
}

TEST_CASE("the cursed mimic lies still as a chest however near the player comes, until hit") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/cursed_mimic"));
	world.Tick();
	const Monster* m = find(world);
	REQUIRE(m != nullptr);
	world.player.god = true; // over the death trap, to its side
	REQUIRE(world.WalkTo(8.1f));
	world.Wait(3000);
	CHECK(m->lurking());
	CHECK(m->CentreX() == doctest::Approx(9.5f));
	CHECK(world.Hp() == 50);
}

TEST_CASE("woken, the cursed mimic hops after the player, leaps the trap and bites; a quarter of its bites madden") {
	SimWorld world;
	const Monster* m = woken(world);
	CHECK(world.Saw(MonsterCursedMimic, CreatureMove::Ambush));
	world.Wait(6000);
	CHECK(m->CentreX() < 7.f); // over the death trap in col 7
	CHECK(world.Saw(MonsterCursedMimic, CreatureMove::Leap));
	CHECK(m->Alive());
	bool mad = false;
	for (int t = 0; t < 30000 && !mad; t += UPDATE_TICK_MS) {
		world.Tick();
		mad = world.player.stats.curses.Has(Curse::Insanity);
	}
	CHECK(mad);
	CHECK(world.Saw(MonsterCursedMimic, CreatureMove::Curse));
}

TEST_CASE("killed, the cursed mimic leaves its chest where it died, not on its spawn cell") {
	SimWorld world;
	const Monster* m = woken(world);
	world.Wait(6000);
	REQUIRE(m->CentreX() < 7.f);
	const int chests = world.dungeon.ChestCount();
	REQUIRE(world.dungeon.AttackNearest(1000, DamageMix{100, 0, 0}, 2.f, 1));
	world.Wait(5000); // its die clip
	CHECK_FALSE(find(world));
	CHECK(world.dungeon.ChestCount() == chests + 1);
	CHECK(world.dungeon.Cell(9, 2).type != MonsterSpawn); // slain: not back after a load
	CHECK(world.dungeon.Cell(9, 2).type != Treasure);
	int col = -1;
	for (int c = 1; c < 9; c++)
		if (world.dungeon.Cell(c, 2).type == Treasure)
			col = c;
	CHECK(col >= 4);
	CHECK(col <= 6);
}

TEST_CASE("the cursed mimic's chest holds an amulet half the time, else the mimic's loot") {
	OwnedWeapons owned{};
	owned[static_cast<size_t>(itemIndex(ItemKind::Club))] = true;
	Rng rng{7};
	int amulets = 0;
	constexpr int ROLLS = 2000;
	for (int i = 0; i < ROLLS; i++) {
		const ItemKind loot = RollCursedMimicLoot(owned, 25, rng);
		amulets += isAmulet(loot) ? 1 : 0;
		if (!isAmulet(loot))
			CHECK((loot == ItemKind::Club || (isPotion(loot) && potionDef(loot).mimicLoot)));
	}
	CHECK(amulets > ROLLS * 45 / 100);
	CHECK(amulets < ROLLS * 55 / 100);
}

TEST_CASE("levelgen turns some of its mimics into cursed ones, only on the hardest levels") {
	auto count = [](int difficulty, int type) {
		int n = 0;
		for (uint32_t seed = 1; seed <= 30; seed++) {
			GenOptions opt;
			opt.seed = seed;
			opt.difficulty = difficulty;
			const GenResult gen = generateLevel(opt);
			for (const Tile& t : gen.grid.cells)
				n += t.type == MonsterSpawn && t.attr == type ? 1 : 0;
		}
		return n;
	};
	CHECK(count(5, MonsterCursedMimic) == 0);
	CHECK(count(9, MonsterCursedMimic) > 0);
	CHECK(count(9, MonsterMimic) > 0);
}
