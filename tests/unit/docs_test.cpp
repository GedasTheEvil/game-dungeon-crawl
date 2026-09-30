// The level docs are written by hand; these tests fail when a table in the code grows and the docs do not.
#include "../../external/doctest/doctest.h"
#include "../../src/world/items.h"
#include "../../src/world/monster_kinds.h"
#include "../../src/world/tile_defs.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
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

TEST_CASE("the editor readme lists every tile type, monster, lock colour and item") {
	const std::string readme = readLower("tools/editor/readme.md");
	for (int type = 0; type < TILE_TYPE_COUNT; type++)
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
