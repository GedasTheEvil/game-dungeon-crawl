#include "campaign.h"
#include "zone_look.h"
#include "../core/paths.h"
#include <map>

std::string campaignLevelFile(int number) { return CONTENT_DIR "levels/lvl" + std::to_string(number); }

std::string zoneTitle(ZoneId id) {
	static constexpr const char* HOURS[] = {
		"First",
		"Second",
		"Third",
		"Fourth",
		"Fifth",
		"Sixth",
		"Seventh",
		"Eighth",
		"Ninth",
		"Tenth",
		"Eleventh",
		"Twelfth"
	};
	static_assert(sizeof(HOURS) / sizeof(HOURS[0]) == ZONE_ID_COUNT, "an hour a zone");
	const auto hour = static_cast<size_t>(id);
	return std::string("The ") + HOURS[hour] + " Hour: " + zoneInfo(id).gate;
}

const std::string& campaignLevelTitle(int number) {
	static std::map<int, std::string> cache;
	auto at = cache.find(number);
	if (at == cache.end())
		at = cache.emplace(number, zoneOf(number) == nullptr ? "" : levelFileTitle(campaignLevelFile(number).c_str()))
				 .first;
	return at->second;
}

LevelBanner levelBanner(int number, const std::string& title) {
	const Zone* zone = zoneOf(number);
	const std::string level = "Level " + std::to_string(number) + (title.empty() ? "" : ": " + title);
	if (zone == nullptr)
		return {false, level, "", ""};
	if (number != zone->first)
		return {false, level, zoneInfo(zone->id).name, ""};
	const char* guardian = zoneLook(zone->id).guardian;
	return {
		true,
		zoneTitle(zone->id),
		zoneInfo(zone->id).name + (title.empty() ? "" : ", " + title),
		zone->boss != 0 && guardian != nullptr ? std::string("Its guardian: ") + guardian : ""
	};
}

int zoneBySlug(const std::string& slug) {
	for (int z = 0; z < ZONE_ID_COUNT; z++)
		if (slug == ZONE_INFO[z].slug)
			return z;
	return NO_ZONE;
}
