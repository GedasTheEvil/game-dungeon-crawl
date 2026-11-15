// The player's actions outside the screens (plan item-actions-out-of-screens): an offering's XP, counter and gift; a
// merge into sky iron's note; a riddle's reward and counter; Hieroglyphs' translation XP and the skill's note.
#include "../../external/doctest/doctest.h"
#include "../../src/level/player_actions.h"
#include <algorithm>
#include <string>

namespace {
struct Actors {
	PlayerStats stats;
	ItemBag bag;
	Journal journal;
	WorldEvents events;
	PlayerActor Actor() { return {stats, bag, journal, events, BagUser{true, 50, 50, 100, 100, false, 0}}; }
	[[nodiscard]] bool Noted(FieldNote note) const { return std::ranges::contains(journal.Notes(), note); }
	[[nodiscard]] bool Played(WorldSound sound) {
		return std::ranges::any_of(events.Take(), [sound](const WorldEvent& e) {
			return e.kind == WorldEvent::Kind::Sound && e.sound == sound;
		});
	}
};

JournalInscription copied(int id) {
	JournalInscription entry;
	entry.id = id;
	entry.title = "Copy " + std::to_string(id);
	entry.text = {"a line"};
	entry.level = 3;
	entry.translated = false;
	return entry;
}
} // namespace

TEST_CASE("an offering at a shrine adds offerXP, counts the offering, gives the forced gift and prays") {
	Actors a;
	a.stats.AddXP(5000, a.events);
	a.bag.Add(ItemKind::SmallHealth); // a potion: every one is a spare
	const int before = static_cast<int>(a.stats.CurrentXP());
	const int xp = offerXP(ItemKind::SmallHealth, a.stats.CurrentXP());
	(void)a.events.Take();
	PlayerActor actor = a.Actor();
	const ActionResult offered = offerItem(actor, ItemKind::SmallHealth, God::Montu, 7, true);
	REQUIRE(offered.done);
	CHECK(a.bag.Count(ItemKind::SmallHealth) == 0);
	CHECK(static_cast<int>(a.stats.CurrentXP()) >= before + xp); // a gift may be XP-free, never less
	CHECK(a.stats.run.offerings == 1);
	CHECK(offered.text.starts_with("Montu accepts the offering. +" + std::to_string(xp) + " XP. Montu grants "));
	CHECK(a.Played(WorldSound::ShrinePrayMontu));

	PlayerActor again = a.Actor(); // none left: refused, nothing counted
	CHECK_FALSE(offerItem(again, ItemKind::SmallHealth, God::Montu, 7, true).done);
	CHECK(a.stats.run.offerings == 1);
}

TEST_CASE("two gold rings merged at a shrine make a sky iron one: its note once, the new ring selected") {
	Actors a;
	a.bag.Add(ItemKind::GarnetGold);
	a.bag.Add(ItemKind::GarnetGold);
	CHECK_FALSE(a.Noted(FieldNote::SkyIron));
	PlayerActor actor = a.Actor();
	const ActionResult merged = mergeRings(actor, ItemKind::GarnetGold, God::Ptah);
	REQUIRE(merged.done);
	CHECK(merged.select == ItemKind::GarnetSkyIron);
	CHECK(a.bag.Count(ItemKind::GarnetSkyIron) == 1);
	CHECK(a.bag.Count(ItemKind::GarnetGold) == 0);
	CHECK(std::ranges::count(a.journal.Notes(), FieldNote::SkyIron) == 1);
	CHECK(merged.text == "Ptah joins the rings: " + std::string(itemText(ItemKind::GarnetSkyIron).name));
}

TEST_CASE("a solved riddle adds its XP and counts in the run") {
	Actors a;
	JournalRiddle riddle;
	riddle.theme = "Test";
	riddle.question = {"What walks on four legs?"};
	riddle.answers = {"man"};
	const size_t index = a.journal.MeetRiddle(riddle);
	const double before = a.stats.CurrentXP();
	solveRiddle(a.stats, a.journal, a.events, index, false, "man", 120);
	CHECK(a.stats.CurrentXP() == before + 120);
	CHECK(a.stats.run.riddles == 1);
	CHECK(a.journal.Riddles()[index].solved);
	CHECK(a.journal.Riddles()[index].written == "man");
}

TEST_CASE("learning Hieroglyphs with two copied inscriptions: twice InscriptionXP and the skill's note") {
	Actors a;
	REQUIRE(a.journal.ReadInscription(copied(2)));
	REQUIRE(a.journal.ReadInscription(copied(5)));
	a.stats.heka.Earn(5);
	const double before = a.stats.CurrentXP();
	const int xp = 2 * a.stats.InscriptionXP();
	const std::optional<std::string> learnt = learnSkill(a.stats, a.journal, a.events, SkillId::Hieroglyphs, true);
	REQUIRE(learnt);
	CHECK(learnt->starts_with(
		"Learnt: " + std::string(skillDef(SkillId::Hieroglyphs).name) + ". 2 inscriptions translated, +" +
		std::to_string(xp) + " XP"
	));
	CHECK(a.stats.CurrentXP() == before + xp);
	CHECK(a.journal.Inscriptions()[0].translated);
	if (const std::optional<FieldNote> note = skillNote(SkillId::Hieroglyphs))
		CHECK(a.Noted(*note));
	CHECK_FALSE(learnSkill(a.stats, a.journal, a.events, SkillId::Hieroglyphs, true)); // learnt already
}

TEST_CASE("an amulet put on, then off, by the Use button; a potion drunk counts in the run") {
	Actors a;
	a.bag.Add(ItemKind::SmallHealth);
	const auto amulet = static_cast<ItemKind>(static_cast<int>(ItemKind::GarnetCopper) - 1); // the last amulet
	REQUIRE(isAmulet(amulet));
	a.bag.Add(amulet);
	PlayerActor on = a.Actor();
	CHECK(useItem(on, amulet).text.ends_with(" put on"));
	CHECK(a.bag.Worn() == amulet);
	PlayerActor off = a.Actor();
	CHECK(useItem(off, amulet).text.ends_with(" taken off"));
	CHECK_FALSE(a.bag.Worn());
	PlayerActor drink = a.Actor();
	drink.user.hp = 30; // not at full: the potion is not refused
	REQUIRE(useItem(drink, ItemKind::SmallHealth).done);
	CHECK(a.bag.Count(ItemKind::SmallHealth) == 0);
	CHECK(a.stats.run.Drunk(ItemKind::SmallHealth) == 1);
	CHECK(a.Played(WorldSound::Drink));
}
