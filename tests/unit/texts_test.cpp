// Texts in content/ (plan texts-to-content): the file format, the shipped files, and every key the code asks for.
#include "../../external/doctest/doctest.h"
#include "../../src/world/items.h"
#include "../../src/world/journal_text.h"
#include "../../src/world/monster_kinds.h"
#include "../../src/world/skills.h"
#include "../../src/world/texts.h"
#include <set>
#include <sstream>

namespace {
std::vector<std::pair<std::string, std::string>> parse(const std::string& file, std::vector<std::string>& errors) {
	std::istringstream in(file);
	std::vector<std::pair<std::string, std::string>> out;
	ParseTexts(in, "t.txt", out, errors);
	return out;
}
} // namespace

TEST_CASE("a text file: sections, fields, indented lines go on with the text") {
	std::vector<std::string> errors;
	const auto texts = parse(
		"# a comment\n"
		"[note.sand]\n"
		"title: Sand\n"
		"text: Sand. Underground.\n"
		"\tSomebody left a door open.\n"
		"\n"
		"[move.summon]\n"
		"dig-out: Calls its young.\n",
		errors
	);
	CHECK(errors.empty());
	REQUIRE(texts.size() == 3);
	CHECK(texts[0] == std::pair<std::string, std::string>{"note.sand.title", "Sand"});
	CHECK(
		texts[1] ==
		std::pair<std::string, std::string>{"note.sand.text", "Sand. Underground. Somebody left a door open."}
	);
	CHECK(texts[2] == std::pair<std::string, std::string>{"move.summon.dig-out", "Calls its young."});
}

TEST_CASE("a text file's mistakes are errors with file:line, the line skipped") {
	std::vector<std::string> errors;
	const auto texts = parse(
		"title: before any section\n"
		"[Bad Section]\n"
		"text: in a bad section\n"
		"[ok]\n"
		"no colon here\n"
		"\tan indented line after an error\n"
		"text: caf\xc3\xa9\n",
		errors
	);
	REQUIRE(errors.size() == 6);
	CHECK(errors[0].rfind("t.txt:1: ", 0) == 0);
	CHECK(errors[1].rfind("t.txt:2: ", 0) == 0);
	CHECK(errors[2].rfind("t.txt:3: ", 0) == 0);
	CHECK(errors[3].rfind("t.txt:5: ", 0) == 0);
	CHECK(errors[4].rfind("t.txt:6: ", 0) == 0);
	CHECK(errors[5].find("non-ASCII") != std::string::npos);
	REQUIRE(texts.size() == 1);
	CHECK(texts[0].first == "ok.text");
	CHECK(texts[0].second == "caf??");
}

TEST_CASE("textSlug makes keys of names") {
	CHECK(textSlug("Serket's favour") == "serkets-favour");
	CHECK(textSlug("giant rat") == "giant-rat");
	CHECK(textSlug("amulet_traps") == "amulet-traps");
	CHECK(textSlug("self-bow") == "self-bow");
}

TEST_CASE("content/texts loads without problems") {
	CHECK(textProblems().empty());
	for (const std::string& problem : textProblems())
		MESSAGE(problem);
	CHECK(gameTexts().Size() > 300);
}

TEST_CASE("every text the code asks for is in content/texts") {
	for (int n = 0; n < FIELD_NOTE_COUNT; n++)
		(void)fieldNoteText(static_cast<FieldNote>(n));
	for (int p = 0; p < DIARY_PAGE_COUNT; p++)
		(void)diaryText(static_cast<DiaryPage>(p));
	for (int n = 1; n <= EXPEDITION_PAGE_COUNT; n++)
		(void)expeditionPageText(n);
	std::set<std::string> monsterKeys;
	for (int id = 1; id <= MONSTER_TYPE_MAX; id++) {
		const MonsterKind* kind = monsterKind(id);
		if (kind == nullptr)
			continue;
		(void)monsterNote(*kind);
		CHECK(monsterKeys.insert(textSlug(kind->label)).second); // no two kinds share a note
		for (int m = 0; m < CREATURE_MOVE_COUNT; m++)
			(void)creatureMoveNote(static_cast<CreatureMove>(m), *kind);
	}
	for (int i = 0; i < ITEM_KIND_COUNT; i++)
		(void)itemText(itemAt(i));
	for (int s = 0; s < SKILL_COUNT; s++)
		(void)skillLore(static_cast<SkillId>(s));
	const std::vector<std::string> missing = missingTexts();
	for (const std::string& key : missing)
		MESSAGE("missing: " << key);
	CHECK(missing.empty());
}
