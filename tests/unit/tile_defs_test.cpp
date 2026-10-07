#include "../../external/doctest/doctest.h"
#include "../../src/world/monster_kinds.h"
#include "../../src/world/tile_defs.h"
#include <set>
#include <string>

TEST_CASE("every tile type has a row") {
	std::set<std::string> names;
	int types = 0;
	for (int type = 0; type < TILE_TYPE_COUNT; type++)
		if (isTileType(type)) {
			CHECK(std::string(tileDef(type).name) != "Unknown");
			names.insert(tileDef(type).name);
			types++;
		}
	CHECK(names.size() == static_cast<size_t>(types));
	CHECK(types == TILE_TYPE_COUNT - 2); // 0 (now the wall structure) and 7 (Area3D) are gone
	CHECK(std::string(tileDef(0).name) == "Unknown");
	CHECK(std::string(tileDef(7).name) == "Unknown");
	CHECK(std::string(tileDef(TILE_TYPE_COUNT).name) == "Unknown");
	CHECK(std::string(tileDef(-1).name) == "Unknown");
}

TEST_CASE("every structure has a row and its own glyph") {
	std::set<char> glyphs;
	for (int s = 0; s < STRUCTURE_COUNT; s++) {
		CHECK(structureDef(static_cast<Structure>(s)).name != nullptr);
		glyphs.insert(structureGlyph(static_cast<Structure>(s)));
	}
	CHECK(glyphs.size() == static_cast<size_t>(STRUCTURE_COUNT));
}

TEST_CASE("every monster type has a row, six bosses") {
	std::set<char> glyphs;
	int bosses = 0;
	for (int type = 1; type <= MONSTER_TYPE_MAX; type++) {
		const MonsterKind* kind = monsterKind(type);
		REQUIRE(kind != nullptr);
		CHECK(kind->id == type);
		glyphs.insert(kind->glyph);
		bosses += kind->isBoss() ? 1 : 0;
	}
	CHECK(glyphs.size() == static_cast<size_t>(MONSTER_TYPE_MAX));
	CHECK(bosses == 6);
	CHECK(isBossMonster(MonsterBossScarab));
	CHECK(isBossMonster(MonsterVampireBat));
	CHECK(isBossMonster(MonsterAnubisBoss));
	CHECK_FALSE(isBossMonster(MonsterGiantScarab));
	CHECK(monsterKind(0) == nullptr);
	CHECK(monsterKind(MONSTER_TYPE_MAX + 1) == nullptr);
}

TEST_CASE("the legend: unique glyphs, and each tile draws as its glyph") {
	std::set<char> seen;
	for (const GlyphDef& g : glyphLegend()) {
		CHECK_MESSAGE(seen.insert(g.glyph).second, "glyph used twice: ", g.glyph);
		CHECK_MESSAGE(tileGlyph(g.tile) == g.glyph, "glyph ", g.glyph);
	}
	CHECK(seen.count('*') == 0); // the path mark of levelcheck --map
}

TEST_CASE("every tile type and monster type has a glyph in the legend") {
	std::set<int> types;
	std::set<int> monsters;
	for (const GlyphDef& g : glyphLegend()) {
		types.insert(g.tile.type);
		if (g.tile.type == MonsterSpawn)
			monsters.insert(g.tile.attr);
	}
	for (int type = 0; type < TILE_TYPE_COUNT; type++)
		if (isTileType(type) && type != NoObject) // no object: '#' or '.', by the structure
			CHECK_MESSAGE(types.count(type) == 1, tileDef(type).name);
	CHECK(monsters.size() == static_cast<size_t>(MONSTER_TYPE_MAX));
}

TEST_CASE("glyphs the game does not know") {
	CHECK(tileGlyph({MonsterSpawn, 99, 0}) == UNKNOWN_MONSTER_GLYPH);
	CHECK(tileGlyph({Key, 9, 0}) == 'q');
	CHECK(tileGlyph({Gate, 9, 0}) == 'Q');
	CHECK(tileGlyph({Key, BOSS_LOCK, 0}) == 'q'); // no boss key
	CHECK(tileGlyph({Gate, BOSS_LOCK, 0}) == 'Z');
}

TEST_CASE("lock colours") {
	CHECK(std::string(lockColour(1).name) == "red");
	CHECK(std::string(lockColour(BOSS_LOCK).gem) == "Obsidian");
	CHECK(lockColour(BOSS_LOCK).keyGlyph == '\0');
}
