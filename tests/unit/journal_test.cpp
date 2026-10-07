#include "../../external/doctest/doctest.h"
#include "../../src/world/journal.h"
#include <sstream>

namespace {
JournalRiddle riddle(const char* question) {
	JournalRiddle r;
	r.theme = "Palindrome";
	r.question = {question, "second line"};
	r.answers = {"panama"};
	r.hint = "It reads the same \"backwards\".";
	r.level = 3;
	r.lateXP = 50;
	return r;
}
} // namespace

TEST_CASE("a late riddle answer gives a tenth of the gate's XP, at least 50") {
	CHECK(lateRiddleXP(500) == 50);
	CHECK(lateRiddleXP(300) == 50);
	CHECK(lateRiddleXP(2400) == 240);
}

TEST_CASE("a riddle met again keeps one journal entry") {
	Journal j;
	size_t first = j.MeetRiddle(riddle("A man, a plan"));
	JournalRiddle again = riddle("A man, a plan");
	again.level = 7;
	again.lateXP = 400;
	CHECK(j.MeetRiddle(again) == first);
	CHECK(j.MeetRiddle(riddle("Another one")) == 1);
	REQUIRE(j.Riddles().size() == 2);
	CHECK(j.Riddles()[0].level == 3); // the first meeting counts
	CHECK(j.Riddles()[0].lateXP == 50);
}

TEST_CASE("the first answer stays written down") {
	Journal j;
	size_t i = j.MeetRiddle(riddle("Q"));
	j.Solve(i, true, "Panama");
	j.Solve(i, false, "panama!");
	CHECK(j.Riddles()[i].solved);
	CHECK(j.Riddles()[i].solvedLate);
	CHECK(j.Riddles()[i].written == "Panama");
}

TEST_CASE("the journal survives a save and load") {
	Journal j;
	size_t i = j.MeetRiddle(riddle("A man, a plan"));
	j.ShowHint(i);
	j.Solve(j.MeetRiddle(riddle("Second")), false, "a man");
	std::stringstream s;
	s << "1 2 3 ";
	j.Dump(s);

	int a = 0, b = 0, c = 0;
	s >> a >> b >> c;
	Journal loaded;
	loaded.Load(s);
	REQUIRE(loaded.Riddles().size() == 2);
	const JournalRiddle& r = loaded.Riddles()[0];
	CHECK(r.theme == "Palindrome");
	CHECK(r.question == std::vector<std::string>{"A man, a plan", "second line"});
	CHECK(r.answers == std::vector<std::string>{"panama"});
	CHECK(r.hint == "It reads the same \"backwards\".");
	CHECK(r.level == 3);
	CHECK(r.lateXP == 50);
	CHECK(r.hintShown);
	CHECK_FALSE(r.solved);
	CHECK(loaded.Riddles()[1].solved);
	CHECK_FALSE(loaded.Riddles()[1].solvedLate);
	CHECK(loaded.Riddles()[1].written == "a man");
}

TEST_CASE("a save from before the journal loads an empty journal") {
	std::stringstream old("");
	Journal j;
	j.MeetRiddle(riddle("Q"));
	j.Load(old);
	CHECK(j.Riddles().empty());
}

TEST_CASE("creatures are written down in the order they were met, once each") {
	Journal j;
	j.SeeCreature(5, 1);
	j.HitByCreature(7, 2);
	j.KillCreature(5, 3);
	j.SeeMove(7, 4, CreatureMove::Swoop);
	REQUIRE(j.Creatures().size() == 2);
	const JournalCreature& rat = j.Creatures()[0];
	CHECK(rat.type == 5);
	CHECK(rat.level == 1); // the first meeting
	CHECK(rat.killed);
	CHECK_FALSE(rat.hitBy);
	const JournalCreature& bat = j.Creatures()[1];
	CHECK(bat.hitBy);
	CHECK_FALSE(bat.killed);
	CHECK(bat.Saw(CreatureMove::Swoop));
	CHECK_FALSE(bat.Saw(CreatureMove::Leap));
}

TEST_CASE("creatures survive a save and load") {
	Journal j;
	j.KillCreature(10, 6);
	j.SeeMove(10, 6, CreatureMove::Leap);
	j.SeeMove(10, 6, CreatureMove::Heal);
	std::stringstream s;
	j.Dump(s);
	Journal loaded;
	loaded.Load(s);
	REQUIRE(loaded.Creatures().size() == 1);
	CHECK(loaded.Creatures()[0].type == 10);
	CHECK(loaded.Creatures()[0].level == 6);
	CHECK(loaded.Creatures()[0].killed);
	CHECK(loaded.Creatures()[0].Saw(CreatureMove::Leap));
	CHECK(loaded.Creatures()[0].Saw(CreatureMove::Heal));
}

TEST_CASE("a journal saved before the creatures loads its riddles") {
	std::stringstream v1("journal 1 1\n\"T\" 1 \"Q\" 1 a \"\" 2 50 0 1 0 \"a\"\n");
	Journal j;
	j.Load(v1);
	REQUIRE(j.Riddles().size() == 1);
	CHECK(j.Riddles()[0].solved);
	CHECK(j.Creatures().empty());
}

TEST_CASE("field notes are written once, in the order they were learnt, and saved") {
	Journal j;
	j.LearnNote(FieldNote::Stamina);
	j.LearnNote(FieldNote::Health);
	j.LearnNote(FieldNote::Stamina);
	REQUIRE(j.Notes().size() == 2);
	CHECK(j.Notes()[0] == FieldNote::Stamina);
	CHECK(j.Notes()[1] == FieldNote::Health);

	std::stringstream s;
	j.Dump(s);
	Journal loaded;
	loaded.Load(s);
	CHECK(loaded.Notes() == j.Notes());
}

TEST_CASE("field notes from before the notes per kind are dropped, the rest of the journal stays") {
	std::stringstream v4("journal 4 0 0 2 3 4\n"); // no riddles, no creatures, two old notes
	Journal j;
	j.Load(v4);
	CHECK(j.Notes().empty());
}
