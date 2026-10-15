// Wall inscriptions (plan hieroglyph-cartouches): the text files, reading one in the level, the journal, the campaign.
#include "../../external/doctest/doctest.h"
#include "../../src/world/campaign.h"
#include "../../src/world/inscriptions.h"
#include "../../src/world/journal.h"
#include "../../src/world/progression.h"
#include "sim_world.h"
#include <set>
#include <sstream>
#include "../../src/core/paths.h"

namespace {
JournalInscription entry(int id, int level) {
	return {id,
			"The curse of Meni",
			{"The crocodile be against him in the water,", "the \"snake\" on land."},
			"Oddly specific.",
			level};
}
} // namespace

TEST_CASE("an inscription file: entries by id, the atlas lines skipped, problems told by line") {
	std::istringstream in("# comment\n"
						  "id: 2\n"
						  "title: The curse\n"
						  "style: column\n"
						  "G: I3 N35A\n"
						  "T: Line one\n"
						  "T: Line two\n"
						  "N: A note\n"
						  "\n"
						  "id: 3\n"
						  "title: No text\n"
						  "id: 99\n"
						  "T: out of range\n"
						  "id: 4\n"
						  "T: only text\n"
						  "X: what\n");
	std::vector<InscriptionText> out;
	std::vector<std::string> errors;
	ParseInscriptions(in, "f.txt", out, errors);
	REQUIRE(out.size() == 2);
	CHECK(out[0].id == 2);
	CHECK(out[0].title == "The curse");
	CHECK(out[0].text == std::vector<std::string>{"Line one", "Line two"});
	CHECK(out[0].note == "A note");
	CHECK(out[1].id == 4);
	REQUIRE(errors.size() == 4);
	CHECK(errors[0] == "f.txt:10: inscription 3 has no T: line, skipped");
	CHECK(errors[1] == "f.txt:12: id must be 1 to 64, entry skipped");
	CHECK(errors[2] == "f.txt:13: T: before any id: line, ignored");
	CHECK(errors[3].find("f.txt:16:") == 0);
}

TEST_CASE("an inscription read twice is written in the journal once, and kept by a save and load") {
	Journal j;
	CHECK(j.ReadInscription(entry(2, 7)));
	CHECK_FALSE(j.ReadInscription(entry(2, 9)));
	CHECK(j.ReadInscription(entry(5, 10)));
	REQUIRE(j.Inscriptions().size() == 2);
	CHECK(j.Inscriptions()[0].level == 7); // the first reading counts

	std::stringstream file;
	j.Dump(file);
	Journal loaded;
	loaded.Load(file);
	REQUIRE(loaded.Inscriptions().size() == 2);
	const JournalInscription& e = loaded.Inscriptions()[0];
	CHECK(e.id == 2);
	CHECK(e.title == "The curse of Meni");
	CHECK(e.text == entry(2, 7).text);
	CHECK(e.note == "Oddly specific.");
	CHECK(e.level == 7);
	CHECK(loaded.Inscriptions()[1].id == 5);
}

TEST_CASE("a journal saved before the inscriptions loads with none") {
	std::istringstream old("journal 5 0\n0\n1 3\n");
	Journal j;
	j.Load(old);
	CHECK(j.Notes().size() == 1);
	CHECK(j.Inscriptions().empty());
}

TEST_CASE("interact in front of an inscription: a fixed status line, its text in the journal once") {
	SimWorld world; // level 6: the offering formula two cells east of the entrance
	world.player.god = true;
	world.player.stats.heka.Earn(5);
	REQUIRE(world.player.stats.heka.Learn(SkillId::Hieroglyphs, true));
	REQUIRE(world.inscriptions.Load(CONTENT_DIR "inscriptions").empty());
	REQUIRE(world.Load(6));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(3.5f));
	world.WaitTicks(5);
	const int xp = riddleXP(world.player.stats.CurrentLevel()) / 3; // plan inscription-xp
	const int xpBefore = world.Xp();
	REQUIRE(xp >= 166);
	world.Interact();
	world.WaitTicks(1);
	CHECK(world.Told("Deciphered the hieroglyphs. Journal updated. +" + std::to_string(xp) + " XP"));
	CHECK(world.Xp() == xpBefore + xp);
	CHECK_FALSE(world.Told("The offering formula"));
	CHECK_FALSE(world.Told("Translation written"));
	REQUIRE(world.journal.Inscriptions().size() == 1);
	CHECK(world.journal.Inscriptions()[0].id == 1);
	CHECK(world.journal.Inscriptions()[0].level == 6);
	world.Interact();
	world.WaitTicks(1);
	CHECK(world.Told("Already seen"));
	CHECK(world.journal.Inscriptions().size() == 1);
	CHECK(world.Xp() == xpBefore + xp); // reading again gives none

	world.SaveAndLoad();
	CHECK(world.journal.Inscriptions().size() == 1);
}

TEST_CASE("without the hieroglyph skill: interact copies the glyphs once, learning translates the copy") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.inscriptions.Load(CONTENT_DIR "inscriptions").empty());
	REQUIRE(world.Load(6));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(3.5f));
	world.WaitTicks(5);
	const int xpBefore = world.Xp();
	world.Interact();
	world.WaitTicks(1);
	CHECK(world.Xp() == xpBefore); // a copy gives no XP
	CHECK(world.Told("Cannot understand this. Copied the hieroglyphs to the journal."));
	REQUIRE(world.journal.Inscriptions().size() == 1);
	CHECK_FALSE(world.journal.Inscriptions()[0].translated);
	world.Interact();
	world.WaitTicks(1);
	CHECK(world.Told("Already seen. In the journal."));
	CHECK(world.journal.Inscriptions().size() == 1);

	world.SaveAndLoad(); // the flag is saved
	REQUIRE(world.journal.Inscriptions().size() == 1);
	CHECK_FALSE(world.journal.Inscriptions()[0].translated);

	CHECK(world.Xp() == xpBefore);

	// Learning translates each copy once; the XP is the full part at the level then (plan inscription-xp).
	const int copies = static_cast<int>(world.journal.TranslateInscriptions().size());
	CHECK(copies == 1);
	world.player.stats.AddXP(copies * world.player.stats.InscriptionXP(), world.events);
	CHECK(world.Xp() == xpBefore + riddleXP(world.player.stats.CurrentLevel()) / 3);
	CHECK(world.journal.TranslateInscriptions().empty()); // nothing more to give
	CHECK(world.journal.Inscriptions()[0].translated);
	CHECK(world.journal.Inscriptions()[0].title == "The offering formula");

	// Reading the translated copy with the skill gives no XP again.
	const int xpAfter = world.Xp();
	world.player.stats.heka.Earn(5);
	REQUIRE(world.player.stats.heka.Learn(SkillId::Hieroglyphs, true));
	world.Interact();
	world.WaitTicks(1);
	CHECK(world.Told("Already seen"));
	CHECK(world.Xp() == xpAfter);

	// A copy kept in the journal and read with the skill later: translated at once.
	Journal j;
	JournalInscription copy = entry(2, 7);
	copy.translated = false;
	CHECK(j.ReadInscription(copy));
	CHECK_FALSE(j.ReadInscription(copy));
	CHECK(j.ReadInscription(entry(2, 9))); // translated now
	CHECK(j.Inscriptions()[0].translated);
	CHECK(j.Inscriptions()[0].level == 9);
}

TEST_CASE("a journal saved before the translated flag loads its inscriptions translated") {
	std::istringstream old("journal 6 0\n0\n0\n1\n2 7 \"Title\" 1 \"Line\" \"Note\"\n");
	Journal j;
	j.Load(old);
	REQUIRE(j.Inscriptions().size() == 1);
	CHECK(j.Inscriptions()[0].translated);
}

TEST_CASE("a boon: line: the four kinds, a bad one told by line") {
	std::istringstream in("id: 1\nT: a\nboon: might 1\n"
						  "id: 2\nT: b\nboon: armor 2\n"
						  "id: 3\nT: c\nboon: life 10\n"
						  "id: 4\nT: d\nboon: stamina 10\n"
						  "id: 5\nT: e\nboon: luck 1\nboon: might x\nboon: might 0\nboon: might\n");
	std::vector<InscriptionText> out;
	std::vector<std::string> errors;
	ParseInscriptions(in, "f.txt", out, errors);
	REQUIRE(out.size() == 5);
	CHECK(out[0].boon == Boon{BoonKind::Might, 1});
	CHECK(out[1].boon == Boon{BoonKind::Armor, 2});
	CHECK(out[2].boon == Boon{BoonKind::Life, 10});
	CHECK(out[3].boon == Boon{BoonKind::Stamina, 10});
	CHECK_FALSE(out[4].boon.Any());
	REQUIRE(errors.size() == 4);
	CHECK(errors[0].find("f.txt:15: boon:") == 0);
	CHECK(errors[3].find("f.txt:18: boon:") == 0);
	CHECK(boonText(out[0].boon) == "Might +1");
	CHECK(boonText(out[2].boon) == "Max health +10%");
	CHECK(boonText(out[3].boon) == "Max stamina +10%");
}

TEST_CASE("a boon: given by the first reading with the skill only, kept on the journal page") {
	SimWorld world; // level 6: inscription 1 two cells east of the entrance
	world.player.god = true;
	world.player.stats.heka.Earn(5);
	REQUIRE(world.player.stats.heka.Learn(SkillId::Hieroglyphs, true));
	world.inscriptions.Add(
		{.id = 1, .title = "Kings", .text = {"The strength of kings."}, .boon = {BoonKind::Might, 1}});
	REQUIRE(world.Load(6));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(3.5f));
	world.WaitTicks(5);
	const int might = world.player.stats.CurrentMight();
	const int xp = world.player.stats.InscriptionXP();
	world.Interact();
	world.WaitTicks(1);
	CHECK(world.Told("Deciphered the hieroglyphs. Journal updated. +" + std::to_string(xp) + " XP, Might +1"));
	CHECK(world.player.stats.CurrentMight() == might + 1);
	world.Interact();
	world.WaitTicks(1);
	CHECK(world.player.stats.CurrentMight() == might + 1); // the second reading gives none

	world.SaveAndLoad();
	CHECK(world.player.stats.CurrentMight() == might + 1);
	REQUIRE(world.journal.Inscriptions().size() == 1);
	CHECK(world.journal.Inscriptions()[0].boon == Boon{BoonKind::Might, 1});
}

TEST_CASE("a stamina boon on a copy: nothing until the skill is learnt, then once; kept by a save") {
	SimWorld world;
	world.player.god = true;
	world.inscriptions.Add(
		{.id = 1, .title = "Runner", .text = {"He does not tire."}, .boon = {BoonKind::Stamina, 10}});
	REQUIRE(world.Load(6));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(3.5f));
	world.WaitTicks(5);
	const int maxStamina = world.player.stats.MaxStamina();
	world.Interact();
	world.WaitTicks(1);
	CHECK(world.Told("Copied the hieroglyphs"));
	CHECK(world.player.stats.MaxStamina() == maxStamina); // a copy gives nothing

	world.SaveAndLoad(); // the copy keeps its boon
	REQUIRE(world.journal.Inscriptions().size() == 1);
	CHECK(world.journal.Inscriptions()[0].boon == Boon{BoonKind::Stamina, 10});

	const std::vector<Boon> boons = world.journal.TranslateInscriptions();
	REQUIRE(boons.size() == 1);
	CHECK(world.player.stats.Grant(boons[0]) == "Max stamina +10%");
	CHECK(world.player.stats.MaxStamina() == maxStamina * 110 / 100);
	CHECK(world.journal.TranslateInscriptions().empty()); // learning gives it once

	world.SaveAndLoad();
	CHECK(world.player.stats.MaxStamina() == maxStamina * 110 / 100);
}

TEST_CASE("a journal saved before the boons loads its inscriptions without one") {
	std::istringstream old("journal 8 0\n0\n0\n1\n2 7 \"Title\" 1 \"Line\" \"Note\" 0\n");
	Journal j;
	j.Load(old);
	REQUIRE(j.Inscriptions().size() == 1);
	CHECK_FALSE(j.Inscriptions()[0].translated);
	CHECK_FALSE(j.Inscriptions()[0].boon.Any());
}

TEST_CASE("the campaign's boons: about 1 in 5 inscriptions, all four kinds, at most one per level") {
	InscriptionBook book;
	REQUIRE(book.Load(CONTENT_DIR "inscriptions").empty());
	std::set<BoonKind> kinds;
	int boons = 0;
	int texts = 0;
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		LevelGrid grid;
		REQUIRE(loadLevelFile(campaignLevelFile(n).c_str(), grid).empty());
		int here = 0;
		for (int cell = 0; cell < LEVEL_WIDTH * LEVEL_HEIGHT; cell++) {
			const Fixture& t = grid.cells[cell].fixture;
			const InscriptionText* text = t.type == Inscription ? book.Find(t.attr) : nullptr;
			if (text == nullptr)
				continue;
			texts++;
			if (!text->boon.Any())
				continue;
			here++;
			kinds.insert(text->boon.kind);
		}
		boons += here;
		CHECK_MESSAGE(here <= 1, "lvl", n, " has ", here, " boons");
	}
	CHECK(kinds.size() == 4);
	CHECK(boons * 6 >= texts); // between 1 in 6 and 1 in 4
	CHECK(boons * 4 <= texts);
}

TEST_CASE("every campaign inscription has a text, stands from level 6 on, and each text once") {
	InscriptionBook book;
	REQUIRE(book.Load(CONTENT_DIR "inscriptions").empty());
	std::set<int> used;
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		LevelGrid grid;
		REQUIRE(loadLevelFile(campaignLevelFile(n).c_str(), grid).empty());
		int count = 0;
		for (int cell = 0; cell < LEVEL_WIDTH * LEVEL_HEIGHT; cell++) {
			const Fixture& t = grid.cells[cell].fixture;
			if (t.type != Inscription)
				continue;
			count++;
			CHECK_MESSAGE(book.Find(t.attr) != nullptr, "lvl", n, ": inscription ", t.attr, " has no text");
			CHECK_MESSAGE(used.insert(t.attr).second, "lvl", n, ": inscription ", t.attr, " is used twice");
		}
		if (n < INSCRIPTIONS_FROM_LEVEL)
			CHECK_MESSAGE(count == 0, "lvl", n, " is before the hieroglyph skill");
		else
			CHECK_MESSAGE(count > 0, "lvl", n, " has no inscription");
	}
	CHECK(used.size() == book.Size()); // no text left over
}
