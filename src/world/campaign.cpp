#include "campaign.h"
#include "../core/paths.h"

std::string campaignLevelFile(int number) { return CONTENT_DIR "levels/lvl" + std::to_string(number); }
