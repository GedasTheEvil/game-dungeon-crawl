// Sim checks of the old journal, HUD and start scenarios (docs/plan/solved/lean-scenario-tests.md).
#include "../../external/doctest/doctest.h"
#include "sim_world.h"

namespace {
bool hasNote(const SimWorld& world, FieldNote note) {
	const std::vector<FieldNote>& notes = world.journal.Notes();
	return std::find(notes.begin(), notes.end(), note) != notes.end();
}
} // namespace

TEST_CASE("field notes: a first potion of a group, a first weapon (its type's note too) write one note each") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/rats"));
	CHECK(world.journal.Notes().empty());
	world.Give(ItemKind::SmallHealth);
	CHECK(world.journal.Notes().size() == 1);
	CHECK(hasNote(world, FieldNote::HealthPotions));
	world.Give(ItemKind::LargeHealth); // the same group: nothing new
	CHECK(world.journal.Notes().size() == 1);
	world.Give(ItemKind::ShortSword);
	CHECK(hasNote(world, FieldNote::Weapons));
	CHECK(hasNote(world, FieldNote::Slash));
	CHECK(world.journal.Notes().size() == 3);
}

TEST_CASE("the rats are written down when met; killing the rat gives XP") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/rats"));
	REQUIRE(world.WalkTo(world.X() + 1.5f));
	const int xp = world.Xp();
	for (int t = 0; t < 400; t++) {
		world.Attack();
		world.Wait(100);
		if (world.Xp() > xp)
			break;
	}
	CHECK(world.Xp() > xp);
	const JournalCreature* rat = world.Creature(MonsterRat);
	REQUIRE(rat != nullptr);
	CHECK(rat->killed);
}

TEST_CASE("a save and load at the start keeps full HP, the journal and the position") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/classic1"));
	const int hp = world.Hp();
	const float x = world.X();
	world.SaveAndLoad();
	CHECK(world.Hp() == hp);
	CHECK(world.X() == doctest::Approx(x));
}

TEST_CASE("the weapon in hand is the last one equipped") {
	SimWorld world;
	REQUIRE(world.Load("tests/levels/classic1"));
	world.Equip(ItemKind::Club);
	world.Equip(ItemKind::ShortSword);
	world.Equip(ItemKind::Spear);
	world.Equip(ItemKind::SelfBow);
	CHECK(world.items.Equipped() == ItemKind::SelfBow);
	CHECK(isRanged(world.items.Equipped()));
}
