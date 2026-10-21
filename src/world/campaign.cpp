#include "campaign.h"
#include "../core/paths.h"

std::string campaignLevelFile(int number) { return CONTENT_DIR "levels/lvl" + std::to_string(number); }

std::string zoneTitle(const Zone& zone) {
	static constexpr const char* HOURS[] = {"First",   "Second", "Third", "Fourth", "Fifth",	"Sixth",
											"Seventh", "Eighth", "Ninth", "Tenth",	"Eleventh", "Twelfth"};
	static_assert(sizeof(HOURS) / sizeof(HOURS[0]) == static_cast<size_t>(ZoneId::HallOfJudgment) + 1,
				  "an hour a zone");
	const auto hour = static_cast<size_t>(zone.id);
	return std::string("The ") + HOURS[hour] + " Hour: " + zone.gate;
}
