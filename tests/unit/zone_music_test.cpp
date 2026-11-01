// Zone music (plan zone-music): the menu's track, a zone's own, soundtrack.ogg in no zone or for a missing file.
#include "../../external/doctest/doctest.h"
#include "../../src/world/campaign.h"
#include "../../src/world/zone_music.h"
#include <filesystem>
#include <string>

namespace {
bool all(const std::string&) { return true; }
bool none(const std::string&) { return false; }
} // namespace

TEST_CASE("the music file: the menu's, the zone's, the fallback") {
	const int tomb = static_cast<int>(ZoneId::TombEntrance);
	CHECK(musicFile(tomb, false, all) == MEDIA_DIR "sounds/music/tomb_entrance.ogg");
	CHECK(musicFile(ZONE_ID_COUNT - 1, false, all) == MEDIA_DIR "sounds/music/hall_of_judgment.ogg");
	CHECK(musicFile(tomb, true, all) == MEDIA_DIR "sounds/music/menu.ogg");
	CHECK(musicFile(NO_ZONE, true, all) == MEDIA_DIR "sounds/music/menu.ogg");
	CHECK(musicFile(NO_ZONE, false, all) == MUSIC_FALLBACK);
	CHECK(musicFile(tomb, false, none) == MUSIC_FALLBACK);
	CHECK(musicFile(tomb, true, none) == MUSIC_FALLBACK);
	CHECK(musicFile(tomb, false, [](const std::string& file) {
			  return file != MUSIC_DIR "tomb_entrance.ogg";
		  }) == MUSIC_FALLBACK);
}

TEST_CASE("every zone and the menu have a music file of their own") {
	const auto own = [](const std::string& file) { return std::filesystem::exists(file); };
	for (int zone = 0; zone < ZONE_ID_COUNT; zone++) {
		CAPTURE(zoneInfo(static_cast<ZoneId>(zone)).slug);
		CHECK(musicFile(zone, false, own) != MUSIC_FALLBACK);
	}
	CHECK(musicFile(NO_ZONE, true, own) != MUSIC_FALLBACK);
}
