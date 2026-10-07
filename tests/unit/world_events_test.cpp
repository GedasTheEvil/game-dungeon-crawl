#include "../../external/doctest/doctest.h"
#include "../../src/world/item_bag.h"
#include "../../src/world/journal.h"
#include "../../src/world/world_events.h"

TEST_CASE("world events come out in order, once") {
	WorldEvents events;
	events.Play(WorldSound::GateOpen);
	events.Status("Gained %d XP", 300);
	events.Note(FieldNote::Keys);
	events.AskRiddle();
	const std::vector<WorldEvent> list = events.Take();
	REQUIRE(list.size() == 4);
	CHECK(list[0].kind == WorldEvent::Kind::Sound);
	CHECK(list[0].sound == WorldSound::GateOpen);
	CHECK(list[1].kind == WorldEvent::Kind::Status);
	CHECK(list[1].text == "Gained 300 XP");
	CHECK(list[2].kind == WorldEvent::Kind::Note);
	CHECK(list[2].note == FieldNote::Keys);
	CHECK(list[3].kind == WorldEvent::Kind::AskRiddle);
	CHECK(events.Take().empty());
}

TEST_CASE("a potion found writes its group's note, a weapon the weapons note and its main type's") {
	ItemBag bag;
	Journal journal;
	bag.Find(ItemKind::SmallHealth, journal);
	CHECK(bag.Count(ItemKind::SmallHealth) == 1);
	REQUIRE(journal.Notes().size() == 1);
	CHECK(journal.Notes()[0] == FieldNote::HealthPotions);
	bag.Find(ItemKind::LargeHealth, journal); // the same group: nothing new
	CHECK(journal.Notes().size() == 1);
	bag.Find(ItemKind::Spear, journal);
	CHECK(bag.Count(ItemKind::Spear) == 1);
	REQUIRE(journal.Notes().size() == 3);
	CHECK(journal.Notes()[1] == FieldNote::Weapons);
	CHECK(journal.Notes()[2] == FieldNote::Pierce);
	bag.Find(ItemKind::Club, journal); // 85% blunt, 15% slash: blunt only
	REQUIRE(journal.Notes().size() == 4);
	CHECK(journal.Notes()[3] == FieldNote::Blunt);
	bag.Find(ItemKind::Antidote, journal);
	CHECK(journal.Notes().back() == FieldNote::Antidote);
}

TEST_CASE("every weapon teaches the note of the type it deals most of") {
	CHECK(damageNote(mainType(weaponMix(ItemKind::Dagger))) == FieldNote::Pierce);
	CHECK(damageNote(mainType(weaponMix(ItemKind::EpsilonAxe))) == FieldNote::Slash);
	CHECK(damageNote(mainType(weaponMix(ItemKind::DuckbillAxe))) == FieldNote::Pierce);
	CHECK(damageNote(mainType(weaponMix(ItemKind::ThrowingStick))) == FieldNote::Blunt);
	for (int i = 0; i < WEAPON_KIND_COUNT; i++) {
		const DamageMix& mix = weaponMix(itemAt(i));
		CHECK(mix[0] + mix[1] + mix[2] == 100);
	}
}
