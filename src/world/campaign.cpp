#include "campaign.h"
#include "zone_look.h"
#include "../core/paths.h"

std::string campaignLevelFile(int number) { return CONTENT_DIR "levels/lvl" + std::to_string(number); }

std::string zoneTitle(ZoneId id) {
	static constexpr const char* HOURS[] = {"First",   "Second", "Third", "Fourth", "Fifth",	"Sixth",
											"Seventh", "Eighth", "Ninth", "Tenth",	"Eleventh", "Twelfth"};
	static_assert(sizeof(HOURS) / sizeof(HOURS[0]) == ZONE_ID_COUNT, "an hour a zone");
	const auto hour = static_cast<size_t>(id);
	return std::string("The ") + HOURS[hour] + " Hour: " + zoneInfo(id).gate;
}

LevelBanner levelBanner(int number) {
	const Zone* zone = zoneOf(number);
	if (zone == nullptr)
		return {false, "Level " + std::to_string(number), "", ""};
	if (number != zone->first)
		return {false, "Level " + std::to_string(number), zoneInfo(zone->id).name, ""};
	const char* guardian = zoneLook(zone->id).guardian;
	return {true, zoneTitle(zone->id), zoneInfo(zone->id).name,
			zone->boss != 0 && guardian != nullptr ? std::string("Its guardian: ") + guardian : ""};
}

int zoneBySlug(const std::string& slug) {
	for (int z = 0; z < ZONE_ID_COUNT; z++)
		if (slug == ZONE_INFO[z].slug)
			return z;
	return NO_ZONE;
}
