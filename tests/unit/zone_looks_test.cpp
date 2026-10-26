// Zone looks (plan zone-looks): every zone has a stone, a decor set, a light and an ambience; the campaign's levels
// are dressed in their zone's stone; the banner names the hour, the gate and the guardian.
#include "../../external/doctest/doctest.h"
#include "../../src/core/paths.h"
#include "../../src/world/campaign.h"
#include "../../src/world/decor.h"
#include "../../src/world/decor_scatter.h"
#include "../../src/world/level.h"
#include "../../src/world/zone_look.h"
#include <filesystem>
#include <memory>
#include <string>

namespace {
bool fileExists(const std::string& path) { return std::filesystem::exists(path); }

// The walls of a campaign level's dressed (stone) rows, counted by style.
struct WallCount {
	int styles[WALL_STYLE_COUNT] = {};
	int floors[FLOOR_STYLE_COUNT] = {};
};

WallCount wallsOf(int number) {
	LevelGrid grid;
	REQUIRE(loadLevelFile(campaignLevelFile(number).c_str(), grid).empty());
	auto layout = std::make_unique<DecorLayout>();
	scatterDecor(grid.cells, campaignLevelFile(number).c_str(), number, grid.meta.zone, *layout);
	WallCount count;
	for (int k = 0; k < LEVEL_WIDTH * LEVEL_HEIGHT; k++)
		if (!isWall(grid.cells[k])) {
			count.styles[layout->surface[k].wall]++;
			count.floors[layout->surface[k].floor]++;
		}
	return count;
}
} // namespace

TEST_CASE("every zone has a decor set, a light, torches and an ambience; a boss zone a guardian") {
	for (int z = 0; z < ZONE_ID_COUNT; z++) {
		CAPTURE(z);
		const ZoneLook& look = zoneLook(static_cast<ZoneId>(z));
		int props = 0, decals = 0;
		for (uint8_t w : look.props)
			props += w;
		for (uint8_t w : look.decals)
			decals += w;
		CHECK(props > 0);
		CHECK(decals > 0);
		CHECK(look.ambient[0] + look.ambient[1] + look.ambient[2] > 0.1f);
		CHECK(look.fogDensity >= 0.f);
		CHECK(look.torchPercent > 0);
		CHECK(look.torchPercent <= 100);
		REQUIRE(look.ambience != nullptr);
		CHECK(fileExists(MEDIA_DIR "sounds/ambience/" + std::string(look.ambience) + ".wav"));
		CHECK((look.guardian == nullptr) == (static_cast<ZoneId>(z) == ZoneId::Necropolis)); // the one without a boss
		for (int d = DECOR_THOTH + 1; d < DECOR_THOTH + DECOR_THOTH_VARIANTS; d++)
			CHECK(look.props[d] == 0); // the Thoth pick is the first variant's
	}
	CHECK(&levelLook(NO_ZONE) == &DEFAULT_LOOK);
	CHECK(&levelLook(ZONE_ID_COUNT) == &DEFAULT_LOOK);
	CHECK(DEFAULT_LOOK.ambience == nullptr);
}

TEST_CASE("every surface style has its texture") {
	for (const char* name : WALL_STYLE_NAMES)
		CHECK(fileExists(MEDIA_DIR "textures/dungeon/" + std::string(name) + ".png"));
	for (const char* name : FLOOR_STYLE_NAMES)
		CHECK(fileExists(MEDIA_DIR "textures/dungeon/" + std::string(name) + ".png"));
	for (const char* name : CEILING_STYLE_NAMES)
		CHECK(fileExists(MEDIA_DIR "textures/dungeon/" + std::string(name) + ".png"));
}

TEST_CASE("a campaign level is dressed in its zone's stone") {
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		CAPTURE(n);
		const WallCount c = wallsOf(n);
		const ZoneStone stone = zoneInfo(zoneOf(n)->id).stone;
		const int painted = c.styles[WALL_PLASTER] + c.styles[WALL_PLASTER_WORN] + c.styles[WALL_PLASTER_BROKEN];
		const int lime = c.styles[WALL_STONE] + c.styles[WALL_STONE_CRACKED] + c.styles[WALL_STONE_SAND];
		const int sandstone = c.styles[WALL_SANDSTONE] + c.styles[WALL_SANDSTONE_WORN];
		const int wet = c.styles[WALL_WETSTONE] + c.styles[WALL_WETSTONE_MOSS];
		const int basalt = c.styles[WALL_BASALT] + c.styles[WALL_BASALT_GLOW];
		const int granite = c.styles[WALL_GRANITE] + c.styles[WALL_GRANITE_WORN];
		switch (stone) {
		case ZoneStone::Limestone:
		case ZoneStone::Plaster:
			CHECK(sandstone + wet + basalt + granite == 0);
			break;
		case ZoneStone::Sandstone:
			CHECK(sandstone > 0);
			CHECK(painted + lime + wet + basalt + granite == 0);
			break;
		case ZoneStone::WetStone:
			CHECK(wet > 0);
			CHECK(c.floors[FLOOR_WETSTONE] > 0);
			CHECK(painted + lime + sandstone + basalt + granite == 0);
			break;
		case ZoneStone::Basalt:
			CHECK(basalt > 0);
			CHECK(painted + lime + sandstone + wet + granite == 0);
			break;
		case ZoneStone::Granite:
			CHECK(granite > 0);
			CHECK(c.floors[FLOOR_GRANITE] > 0);
			CHECK(painted + lime + sandstone + wet + basalt == 0);
			break;
		}
	}
}

TEST_CASE("the temples' plaster: their levels are painted more than the tombs before them") {
	auto paintedShare = [](int n) {
		const WallCount c = wallsOf(n);
		const int painted = c.styles[WALL_PLASTER] + c.styles[WALL_PLASTER_WORN] + c.styles[WALL_PLASTER_BROKEN];
		int all = 0;
		for (int s : c.styles)
			all += s;
		return static_cast<float>(painted) / static_cast<float>(all);
	};
	for (int n = 14; n <= 18; n++) { // the serpent temple
		CAPTURE(n);
		CHECK(zoneInfo(zoneOf(n)->id).stone == ZoneStone::Plaster);
		CHECK(paintedShare(n) > 0.4f);
	}
}

TEST_CASE("the level banner: a zone's first level names the hour, the gate and its guardian") {
	const LevelBanner first = levelBanner(1);
	CHECK(first.zone);
	CHECK(first.title == "The First Hour: the Gate of the Tomb");
	CHECK(first.subtitle == "Tomb entrance");
	CHECK(first.guardian == std::string("Its guardian: ") + zoneLook(ZoneId::TombEntrance).guardian);

	const LevelBanner second = levelBanner(2);
	CHECK_FALSE(second.zone);
	CHECK(second.title == "Level 2");
	CHECK(second.subtitle == "Tomb entrance");
	CHECK(second.guardian.empty());

	const LevelBanner priests = levelBanner(zoneOf(15)->first);
	CHECK(priests.zone);
	CHECK(priests.title == "The Fourth Hour: the Gate of the Priests");

	for (int z = 0; z < ZONE_COUNT; z++) {
		const Zone& zone = ZONES[z];
		CAPTURE(zoneInfo(zone.id).name);
		const LevelBanner b = levelBanner(zone.first);
		CHECK(b.zone);
		CHECK(b.title == zoneTitle(zone.id));
		const char* guardian = zoneLook(zone.id).guardian;
		CHECK(b.guardian.empty() == (zone.boss == 0 || guardian == nullptr));
	}
	CHECK_FALSE(levelBanner(CAMPAIGN_LEVELS + 1).zone);
}

TEST_CASE("sand spills onto a stone floor beside a sand floor, on both sides, nowhere else") {
	int edges = 0;
	for (int n = 1; n <= CAMPAIGN_LEVELS; n++) {
		CAPTURE(n);
		LevelGrid grid;
		REQUIRE(loadLevelFile(campaignLevelFile(n).c_str(), grid).empty());
		auto layout = std::make_unique<DecorLayout>();
		scatterDecor(grid.cells, campaignLevelFile(n).c_str(), n, grid.meta.zone, *layout);
		auto floored = [&](int i, int j) {
			return LevelGrid::inBounds(i, j) && j > 0 && !isWall(grid.at(i, j)) && !inHalfWater(grid.at(i, j)) &&
				   isWall(grid.at(i, j - 1));
		};
		auto sandAt = [&](int i, int j) {
			return floored(i, j) && layout->surface[j * LEVEL_WIDTH + i].floor == FLOOR_SAND;
		};
		for (int j = 0; j < LEVEL_HEIGHT; j++)
			for (int i = 0; i < LEVEL_WIDTH; i++) {
				const SurfaceCell& s = layout->surface[j * LEVEL_WIDTH + i];
				const bool stone = floored(i, j) && s.floor != FLOOR_SAND;
				CHECK(s.sandLeft == (stone && sandAt(i - 1, j)));
				CHECK(s.sandRight == (stone && sandAt(i + 1, j)));
				edges += (s.sandLeft ? 1 : 0) + (s.sandRight ? 1 : 0);
			}
	}
	CHECK(edges > 100);
}
