#ifndef ZONE_MUSIC_H
#define ZONE_MUSIC_H

// The music (plan zone-music): one track per zone and one for the menu, media/sounds/music/<name>.ogg;
// soundtrack.ogg plays where there is none. Several zones share a track by a copy or a symlink of the file.

#include <functional>
#include <string>
#include "campaign.h"
#include "../core/paths.h"

#define MUSIC_DIR MEDIA_DIR "sounds/music/"
constexpr const char* MUSIC_FALLBACK = MUSIC_DIR "soundtrack.ogg";

// The file to play on the menu (the start menu, the death and win screens) or in a level's zone (LevelMeta::zone):
// menu.ogg, <zone slug>.ogg; the fallback in no zone, or when exists says the file is missing.
[[nodiscard]] inline std::string musicFile(int zone, bool menu, const std::function<bool(const std::string&)>& exists) {
	const std::string file = menu			? MUSIC_DIR "menu.ogg"
							 : isZone(zone) ? MUSIC_DIR + std::string(zoneInfo(static_cast<ZoneId>(zone)).slug) + ".ogg"
											: MUSIC_FALLBACK;
	return exists(file) ? file : MUSIC_FALLBACK;
}

#endif
