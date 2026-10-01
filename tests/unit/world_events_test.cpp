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

TEST_CASE("a weapon found writes the weapons note, a potion does not") {
	ItemBag bag;
	Journal journal;
	bag.Find(ItemKind::SmallHealth, journal);
	CHECK(bag.Count(ItemKind::SmallHealth) == 1);
	CHECK(journal.Notes().empty());
	bag.Find(ItemKind::Spear, journal);
	CHECK(bag.Count(ItemKind::Spear) == 1);
	REQUIRE(journal.Notes().size() == 1);
	CHECK(journal.Notes()[0] == FieldNote::Weapons);
}
