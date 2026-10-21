// Campaign zones (plan campaign-zones): the zone table covers the campaign, each boss lives in its zone.
#include "../../external/doctest/doctest.h"
#include "../../src/world/campaign.h"
#include "../../src/world/decor.h"
#include "../../src/world/level.h"
#include "../../src/world/monster_kinds.h"
#include <algorithm>
#include <iterator>

namespace {
// The monster types on a campaign level, and whether it holds the ankh.
struct LevelContents {
	bool monsters[MONSTER_TYPE_MAX + 1] = {};
	bool ankh = false;
};

LevelContents contentsOf(int number) {
	LevelGrid grid;
	REQUIRE(loadLevelFile(campaignLevelFile(number).c_str(), grid).empty());
	LevelContents contents;
	for (int cell = 0; cell < LEVEL_WIDTH * LEVEL_HEIGHT; cell++) {
		const Tile& c = grid.cells[cell];
		if (c.type == MonsterSpawn && c.attr >= 1 && c.attr <= MONSTER_TYPE_MAX)
			contents.monsters[c.attr] = true;
		contents.ankh |= c.type == Ankh;
	}
	return contents;
}
} // namespace

TEST_CASE("the zones cover the campaign in order, without gaps or overlaps") {
	REQUIRE(ZONE_COUNT > 0);
	CHECK(ZONES[0].first == 1);
	for (int z = 0; z < ZONE_COUNT; z++) {
		CAPTURE(z);
		CHECK(ZONES[z].first <= ZONES[z].last);
		if (z > 0)
			CHECK(ZONES[z].first == ZONES[z - 1].last + 1);
	}
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		CAPTURE(n);
		REQUIRE(zoneOf(n) != nullptr);
		CHECK(n >= zoneOf(n)->first);
		CHECK(n <= zoneOf(n)->last);
	}
	CHECK(zoneOf(0) == nullptr);
	CHECK(zoneOf(CAMPAIGN_LEVELS + 1) == nullptr);
	CHECK(zoneOf(DECOR_DEPTH_ALL) == nullptr);
}

TEST_CASE("each zone's id is its own, its boss's kin among its monsters") {
	for (int z = 0; z < ZONE_COUNT; z++) {
		const Zone& zone = ZONES[z];
		CAPTURE(zone.name);
		for (int other = 0; other < z; other++)
			CHECK(ZONES[other].id != zone.id);
		if (zone.boss == 0)
			continue;
		REQUIRE(isBossMonster(zone.boss));
		CHECK(std::count(std::begin(zone.monsters), std::end(zone.monsters), monsterKind(zone.boss)->kin) == 1);
	}
}

TEST_CASE("each boss is in exactly one zone: its own, and the last zone ends with the ankh") {
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		CAPTURE(n);
		const LevelContents contents = contentsOf(n);
		const Zone& zone = *zoneOf(n);
		for (int type = 1; type <= MONSTER_TYPE_MAX; type++)
			if (contents.monsters[type] && isBossMonster(type))
				CHECK(type == zone.boss);
		CHECK(contents.ankh == (n == CAMPAIGN_LEVELS));
	}
	for (int z = 0; z < ZONE_COUNT; z++) {
		const Zone& zone = ZONES[z];
		CAPTURE(zone.name);
		if (zone.boss == 0)
			continue;
		int levels = 0;
		for (int n = zone.first; n <= zone.last; n++)
			levels += contentsOf(n).monsters[zone.boss] ? 1 : 0;
		CHECK(levels == 1);
	}
	CHECK(ZONES[ZONE_COUNT - 1].id == ZoneId::HallOfJudgment);
}

TEST_CASE("a zone's title is its hour and gate") {
	CHECK(zoneTitle(ZONES[0]) == "The First Hour: the Gate of the Tomb");
	CHECK(zoneTitle(*zoneOf(CAMPAIGN_LEVELS)) == "The Twelfth Hour: the Gate of Judgment");
}
