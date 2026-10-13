// Wall inscriptions (plan hieroglyph-cartouches): the text files, reading one in the level, the journal, the campaign.
#include "../../external/doctest/doctest.h"
#include "../../src/world/campaign.h"
#include "../../src/world/inscriptions.h"
#include "../../src/world/journal.h"
#include "sim_world.h"
#include <set>
#include <sstream>

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
	REQUIRE(world.inscriptions.Load("inscriptions").empty());
	REQUIRE(world.Load(6));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(3.5f));
	world.WaitTicks(5);
	world.Interact();
	world.WaitTicks(1);
	CHECK(world.Told("Deciphered the hieroglyphs"));
	CHECK_FALSE(world.Told("The offering formula"));
	CHECK_FALSE(world.Told("Translation written"));
	REQUIRE(world.journal.Inscriptions().size() == 1);
	CHECK(world.journal.Inscriptions()[0].id == 1);
	CHECK(world.journal.Inscriptions()[0].level == 6);
	world.Interact();
	world.WaitTicks(1);
	CHECK(world.Told("Already seen"));
	CHECK(world.journal.Inscriptions().size() == 1);

	world.SaveAndLoad();
	CHECK(world.journal.Inscriptions().size() == 1);
}

TEST_CASE("without the hieroglyph skill: interact copies the glyphs once, learning translates the copy") {
	SimWorld world;
	world.player.god = true;
	REQUIRE(world.inscriptions.Load("inscriptions").empty());
	REQUIRE(world.Load(6));
	world.WaitTicks(30);
	REQUIRE(world.WalkTo(3.5f));
	world.WaitTicks(5);
	world.Interact();
	world.WaitTicks(1);
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

	CHECK(world.journal.TranslateInscriptions() == 1);
	CHECK(world.journal.TranslateInscriptions() == 0);
	CHECK(world.journal.Inscriptions()[0].translated);
	CHECK(world.journal.Inscriptions()[0].title == "The offering formula");

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

TEST_CASE("every campaign inscription has a text, stands from level 6 on, and each text once") {
	InscriptionBook book;
	REQUIRE(book.Load("inscriptions").empty());
	std::set<int> used;
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		LevelGrid grid;
		REQUIRE(loadLevelFile(campaignLevelFile(n).c_str(), grid).empty());
		int count = 0;
		for (int cell = 0; cell < LEVEL_WIDTH * LEVEL_HEIGHT; cell++) {
			const Tile& t = grid.cells[cell];
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
