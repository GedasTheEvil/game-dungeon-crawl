// Campaign zones (plan campaign-zones): the zone table covers the campaign, each boss lives in its zone.
#include "../../external/doctest/doctest.h"
#include "../../src/world/campaign.h"
#include "../../src/world/decor.h"
#include "../../src/world/journal_text.h"
#include "../../src/world/level.h"
#include "../../src/world/monster_kinds.h"
#include <algorithm>
#include <iterator>
#include <set>
#include <sstream>
#include <string>

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
		CAPTURE(zoneInfo(zone.id).name);
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
		CAPTURE(zoneInfo(zone.id).name);
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
	CHECK(zoneTitle(ZONES[0].id) == "The First Hour: the Gate of the Tomb");
	CHECK(zoneTitle(zoneOf(CAMPAIGN_LEVELS)->id) == "The Twelfth Hour: the Gate of Judgment");
}

TEST_CASE("each campaign level file names the zone of its range") {
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		CAPTURE(n);
		LevelGrid grid;
		int version = 0;
		REQUIRE(loadLevelFile(campaignLevelFile(n).c_str(), grid, &version).empty());
		CHECK(version == LEVEL_VERSION);
		CHECK(grid.meta.zone == static_cast<int>(zoneOf(n)->id));
	}
}

TEST_CASE("each campaign level has a title of its own (plan campaign-texts)") {
	std::set<std::string> seen;
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		CAPTURE(n);
		LevelGrid grid;
		REQUIRE(loadLevelFile(campaignLevelFile(n).c_str(), grid).empty());
		CHECK_FALSE(grid.meta.title.empty());
		CHECK(seen.insert(grid.meta.title).second);
		CHECK(campaignLevelTitle(n) == grid.meta.title);
		const LevelBanner banner = levelBanner(n, grid.meta.title); // a zone's first level shows it under the zone
		CHECK((banner.title + banner.subtitle).find(grid.meta.title) != std::string::npos);
	}
	CHECK(campaignLevelTitle(CAMPAIGN_LEVELS + 1).empty());
}

TEST_CASE("a level title survives writing and reading; a level without one has none") {
	LevelGrid grid;
	grid.meta.zone = static_cast<int>(ZoneId::SethsDesert);
	grid.meta.title = "The Hall of Drifts";
	std::stringstream file;
	writeLevel(file, grid.cells, grid.meta);
	LevelGrid back;
	REQUIRE(readLevel(file, back.cells, nullptr, &back.meta).empty());
	CHECK(back.meta.title == "The Hall of Drifts");
	CHECK(back.meta.zone == grid.meta.zone);

	grid.meta.title.clear();
	std::stringstream plain;
	writeLevel(plain, grid.cells, grid.meta);
	LevelGrid other;
	REQUIRE(readLevel(plain, other.cells, nullptr, &other.meta).empty());
	CHECK(other.meta.title.empty());
}

TEST_CASE("the banner shows the level's title") {
	CHECK(levelBanner(2, "The Beetle Passage").title == "Level 2: The Beetle Passage");
	CHECK(levelBanner(1, "The Jackal Statue").subtitle == "Tomb entrance, The Jackal Statue");
	CHECK(levelBanner(2).title == "Level 2");
}

TEST_CASE("each zone has an hour note, in ZoneId order") {
	for (int z = 0; z < ZONE_ID_COUNT; z++) {
		CAPTURE(z);
		const FieldNote note = hourNote(z);
		CHECK(static_cast<int>(note) < FIELD_NOTE_COUNT);
		CHECK(std::string(fieldNoteText(note).text).size() > 40);
	}
	CHECK(static_cast<int>(hourNote(ZONE_ID_COUNT - 1)) + 1 == static_cast<int>(FieldNote::AmuletFirst));
	CHECK(std::string(fieldNoteText(hourNote(0)).title) == "The First Hour");
}
