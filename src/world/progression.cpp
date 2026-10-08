#include "progression.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr int MIN_RIDDLE_XP = 500;
constexpr double RIDDLE_XP_LEVEL_PART = 0.3; // of the XP from the current level to the next
constexpr int STEEP_FROM = 25;
constexpr double STEEP_RAMP = 0.0064;
} // namespace

// Up to STEEP_FROM the curve the first 15 campaign levels were tuned on; past it the gaps grow faster (a factor
// 1 + STEEP_RAMP (level - STEEP_FROM)^2, no jump at STEEP_FROM), so the 30 levels end at about player level 60, not
// 200+ (docs/plan/solved/longer-campaign.md).
double levelXP(int level) {
	if (level <= 1)
		return 0.0;
	const double past = std::max(level - STEEP_FROM, 0);
	return 1000 * std::pow(level - 1, 1.4) * (1 + STEEP_RAMP * past * past);
}

float levelProgress(int level, double xp) {
	double start = levelXP(level);
	double gap = levelXP(level + 1) - start;
	return gap > 0 ? std::clamp(static_cast<float>((xp - start) / gap), 0.f, 1.f) : 0.f;
}

int riddleXP(int level) {
	double gap = levelXP(level + 1) - levelXP(level);
	return std::max(MIN_RIDDLE_XP, static_cast<int>(gap * RIDDLE_XP_LEVEL_PART));
}
