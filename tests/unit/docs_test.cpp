// The level docs are written by hand; these tests fail when a table in the code grows and the docs do not.
#include "../../external/doctest/doctest.h"
#include "../../src/test/scenario_script.h"
#include "../../src/world/items.h"
#include "../../src/world/monster_kinds.h"
#include "../../src/world/tile_defs.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

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

bool has(const std::string& text, const std::string& part) { return text.find(lower(part)) != std::string::npos; }
} // namespace

TEST_CASE("the editor readme lists every structure, tile type, monster, lock colour and item") {
	const std::string readme = readLower("tools/editor/readme.md");
	for (int s = 0; s < STRUCTURE_COUNT; s++) {
		const Structure structure = static_cast<Structure>(s);
		CHECK_MESSAGE(
			has(readme, std::string("| `") + structureGlyph(structure) + "` | " + structureDef(structure).name),
			structureDef(structure).name);
	}
	for (int type = 0; type < TILE_TYPE_COUNT; type++)
		if (isTileType(type))
			CHECK_MESSAGE(has(readme, "| " + std::to_string(type) + " | " + tileDef(type).name), tileDef(type).name);
	for (int type = 1; type <= MONSTER_TYPE_MAX; type++)
		CHECK_MESSAGE(has(readme, "| " + std::to_string(type) + " | " + monsterKind(type)->label),
					  monsterKind(type)->label);
	for (int colour = 1; colour <= BOSS_LOCK; colour++) {
		CHECK_MESSAGE(has(readme, "| " + std::to_string(colour) + " | " + lockColour(colour).name),
					  lockColour(colour).name);
		CHECK_MESSAGE(has(readme, lockColour(colour).gem), lockColour(colour).gem);
	}
	for (int i = 0; i < ITEM_KIND_COUNT; i++) {
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
	std::set<std::string> fields;	  // the expect row's `F` list
	bool inCommands = false;
	for (std::string line; std::getline(f, line);) {
		if (line.rfind("## ", 0) == 0)
			inCommands = line == "## Commands";
		if (!inCommands || line.rfind("| `", 0) != 0)
			continue;
		const std::string cell = line.substr(2, line.find(" | ", 2) - 2);
		for (size_t open = cell.find('`'); open != std::string::npos;
			 open = cell.find('`', cell.find('`', open + 1) + 1)) {
			const size_t close = cell.find('`', open + 1);
			const std::string code = cell.substr(open + 1, close - open - 1);
			documented.insert(code.substr(0, code.find(' ')));
		}
		if (cell.rfind("`expect", 0) == 0) {
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
