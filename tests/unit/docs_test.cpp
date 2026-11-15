// The level docs are written by hand; these tests fail when a table in the code grows and the docs do not.
#include "../../external/doctest/doctest.h"
#include "../../src/level/scenario_script.h"
#include "../../src/level/items.h"
#include "../../src/level/monster_kinds.h"
#include "../../src/level/tile_defs.h"
#include "../../src/level/campaign.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {
// The unit tests run from the repo root (make unit).
std::string readLower(const char* path) {
	std::ifstream f(path);
	REQUIRE_MESSAGE(f, "cannot open ", path);
	std::stringstream text;
	text << f.rdbuf();
	std::string s = text.str();
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return s;
}

std::string lower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return s;
}

bool has(const std::string& text, const std::string& part) { return text.contains(lower(part)); }
} // namespace

TEST_CASE("the editor readme lists every structure, tile type, monster, lock colour and item") {
	const std::string readme = readLower("tools/editor/readme.md");
	for (int s = 0; s < STRUCTURE_COUNT; s++) {
		const Structure structure = static_cast<Structure>(s);
		CHECK_MESSAGE(
			has(readme, std::string("| `") + structureGlyph(structure) + "` | " + structureDef(structure).name),
			structureDef(structure).name
		);
	}
	for (int type = 0; type < TILE_TYPE_COUNT; type++)
		if (isTileType(type))
			CHECK_MESSAGE(has(readme, "| " + std::to_string(type) + " | " + tileDef(type).name), tileDef(type).name);
	for (int type = 1; type <= MONSTER_TYPE_MAX; type++)
		CHECK_MESSAGE(
			has(readme, "| " + std::to_string(type) + " | " + monsterKind(type)->label),
			monsterKind(type)->label
		);
	for (int colour = 1; colour <= BOSS_LOCK; colour++) {
		CHECK_MESSAGE(
			has(readme, "| " + std::to_string(colour) + " | " + lockColour(colour).name),
			lockColour(colour).name
		);
		CHECK_MESSAGE(has(readme, lockColour(colour).gem), lockColour(colour).gem);
	}
	for (int i = 0; i < ITEM_KIND_COUNT; i++) {
		if (isTestWeapon(itemAt(i))) // not for levels (plan test-weapons), so not in the editor's readme
			continue;
		ItemFileId id = fileIdOf(itemAt(i));
		CHECK_MESSAGE(has(readme, std::to_string(id.id) + " " + itemText(itemAt(i)).label), itemText(itemAt(i)).label);
	}
}

TEST_CASE("docs/levels.md gives every monster's threat") {
	const std::string levels = readLower("docs/levels.md");
	for (int type = 1; type <= MONSTER_TYPE_MAX; type++) {
		const MonsterKind* kind = monsterKind(type);
		char entry[64];
		std::snprintf(entry, sizeof(entry), "%s %g", kind->label, static_cast<double>(kind->threat));
		CHECK_MESSAGE(has(levels, entry), "missing \"", entry, "\" in the monsterThreat list");
	}
}

TEST_CASE("docs/testing.md lists every scenario command and expect field, and no other") {
	std::ifstream f("docs/testing.md");
	REQUIRE(f);
	std::set<std::string> documented; // the first word of each `...` in a commands table row's first cell
	std::set<std::string> fields; // the expect row's `F` list
	bool inCommands = false;
	for (std::string line; std::getline(f, line);) {
		if (line.starts_with("## "))
			inCommands = line == "## Commands";
		if (!inCommands || !line.starts_with("| `"))
			continue;
		const std::string cell = line.substr(2, line.find(" | ", 2) - 2);
		for (size_t open = cell.find('`'); open != std::string::npos;
			 open = cell.find('`', cell.find('`', open + 1) + 1)) {
			const size_t close = cell.find('`', open + 1);
			const std::string code = cell.substr(open + 1, close - open - 1);
			documented.insert(code.substr(0, code.find(' ')));
		}
		if (cell.starts_with("`expect")) {
			const size_t from = line.find("F: `") + 4;
			std::istringstream names(line.substr(from, line.find('`', from) - from));
			for (std::string name; names >> name;)
				fields.insert(name);
		}
	}
	std::set<std::string> commands;
	for (const Scenario::CommandDef& def : Scenario::commandDefs())
		commands.insert(def.name);
	std::set<std::string> named;
	for (const Scenario::FieldDef& def : Scenario::fieldDefs())
		named.insert(def.name);
	auto sameAs = [](const std::set<std::string>& docs, const std::set<std::string>& code, const std::string& what) {
		for (const std::string& name : code)
			CHECK_MESSAGE(docs.count(name) == 1, what, " missing in docs/testing.md: ", name);
		for (const std::string& name : docs)
			CHECK_MESSAGE(code.count(name) == 1, what, " in docs/testing.md but not in the code: ", name);
	};
	sameAs(documented, commands, "command");
	sameAs(fields, named, "expect field");
}

TEST_CASE("every code name in docs/glossary.md still exists in src/") {
	// All of src/ as one text, to look the names up in.
	std::string source;
	for (const auto& entry : std::filesystem::recursive_directory_iterator("src")) {
		const std::string ext = entry.path().extension().string();
		if (ext != ".h" && ext != ".cpp")
			continue;
		std::ifstream f(entry.path());
		std::stringstream text;
		text << f.rdbuf();
		source += text.str();
	}
	auto isWordChar = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_'; };
	auto hasWord = [&](const std::string& word) {
		for (size_t at = source.find(word); at != std::string::npos; at = source.find(word, at + 1))
			if ((at == 0 || !isWordChar(source[at - 1])) &&
				(at + word.size() == source.size() || !isWordChar(source[at + word.size()])))
				return true;
		return false;
	};

	std::ifstream f("docs/glossary.md");
	REQUIRE(f);
	int names = 0;
	for (std::string line; std::getline(f, line);) {
		// A term's row: | Term | In game | Meaning | Code |, not the header or the rule under it.
		if (!line.starts_with("| ") || line.starts_with("| Term |"))
			continue;
		const size_t end = line.rfind(" |");
		const size_t start = line.rfind(" | ", end - 1);
		REQUIRE_MESSAGE(start != std::string::npos, "not a table row: ", line);
		const std::string code = line.substr(start + 3, end - start - 3);
		for (size_t open = code.find('`'); open != std::string::npos;
			 open = code.find('`', code.find('`', open + 1) + 1)) {
			const size_t close = code.find('`', open + 1);
			const std::string name = code.substr(open + 1, close - open - 1);
			names++;
			if (name.contains('/')) {
				CHECK_MESSAGE(std::filesystem::exists(name), "no such path: ", name);
				continue;
			}
			// Monster::MeleeGap: each part has to be there.
			for (size_t from = 0;;) {
				const size_t sep = name.find("::", from);
				const std::string part = name.substr(from, sep == std::string::npos ? std::string::npos : sep - from);
				CHECK_MESSAGE(hasWord(part), "not in src/: ", name);
				if (sep == std::string::npos)
					break;
				from = sep + 2;
			}
		}
	}
	CHECK(names > 50);
}

namespace {
// The label stands in the text as a word, plural or not, and not as the tail of a giant's ("bat" in "giant bats").
bool namesMonster(const std::string& text, std::string label) {
	if (label.ends_with('y') && !text.contains(label)) // mummy: mummies
		label = label.substr(0, label.size() - 1) + "ie";
	for (size_t at = text.find(label); at != std::string::npos; at = text.find(label, at + 1)) {
		const size_t end = at + label.size();
		const bool startsWord = at == 0 || !std::isalpha(static_cast<unsigned char>(text[at - 1]));
		const bool endsWord = end == text.size() || !std::isalpha(static_cast<unsigned char>(text[end])) ||
							  (text[end] == 's' &&
							   (end + 1 == text.size() || !std::isalpha(static_cast<unsigned char>(text[end + 1]))));
		const bool giants = !label.starts_with("giant ") && at >= 6 &&
							std::string_view(text).substr(at - 6).starts_with("giant ");
		if (startsWord && endsWord && !giants)
			return true;
	}
	return false;
}

std::string trimmed(const std::string& s) {
	const size_t from = s.find_first_not_of(' ');
	return from == std::string::npos ? "" : s.substr(from, s.find_last_not_of(' ') - from + 1);
}
} // namespace

TEST_CASE("the docs/levels.md zone table matches ZONES and ZONE_FAUNA (plan docs-anchor-identifier-checks)") {
	std::ifstream f("docs/levels.md");
	REQUIRE(f);
	int rows = 0;
	for (std::string line; std::getline(f, line);) {
		// | N | Zone | first-last | Monsters | New mechanic | Boss |
		if (line.size() < 4 || line[0] != '|' || !std::isdigit(static_cast<unsigned char>(line[2])))
			continue;
		std::vector<std::string> cells;
		std::stringstream row(line.substr(1));
		for (std::string cell; std::getline(row, cell, '|');)
			cells.push_back(trimmed(cell));
		if (cells.size() < 6)
			continue; // the Levels table below
		const int n = std::stoi(cells[0]);
		REQUIRE_MESSAGE((n >= 1 && n <= ZONE_COUNT), line);
		const Zone& zone = ZONES[n - 1];
		CHECK_MESSAGE(lower(cells[1]).starts_with(lower(zoneInfo(zone.id).name)), line);
		CHECK_MESSAGE(cells[2] == std::to_string(zone.first) + "-" + std::to_string(zone.last), line);
		const std::string monsters = lower(cells[3]);
		for (const ZoneMonster& m : zoneFauna(zone.id).monsters)
			if (m.type != 0)
				CHECK_MESSAGE(
					namesMonster(monsters, monsterKind(m.type)->label),
					line,
					": no ",
					std::string(monsterKind(m.type)->label)
				);
		const std::string boss = lower(trimmed(cells[5].substr(0, cells[5].find(" ("))));
		if (zone.boss == 0)
			CHECK_MESSAGE(boss == "none", line);
		else {
			const MonsterKind* kind = monsterKind(zone.boss);
			const std::string name = lower(kind->name);
			CHECK_MESSAGE(
				(boss == name || boss == kind->label || name.starts_with(boss + " ")),
				line,
				": not ",
				std::string(kind->name)
			);
		}
		rows++;
	}
	CHECK(rows == ZONE_COUNT);
}
