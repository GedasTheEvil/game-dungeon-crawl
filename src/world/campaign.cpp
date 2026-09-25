#include "campaign.h"
#include "level_gen.h"
#include <algorithm>

CampaignLevel campaignLevel(int number, uint32_t runSeed) {
	CampaignLevel level;
	if (number > CAMPAIGN_OPENING && number <= CAMPAIGN_OPENING + CAMPAIGN_GENERATED) {
		level.generated = true;
		level.seed = runSeed * 1000U + static_cast<uint32_t>(number);
		level.difficulty =
			std::min(GEN_MAX_DIFFICULTY, CAMPAIGN_FIRST_DIFFICULTY + number - CAMPAIGN_OPENING - 1);
		level.name = "gen" + std::to_string(level.seed);
		return level;
	}
	int file = number > CAMPAIGN_OPENING ? number - CAMPAIGN_GENERATED : number;
	level.file = "Levels/lvl" + std::to_string(file);
	level.name = level.file;
	return level;
}
