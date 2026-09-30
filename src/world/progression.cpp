#include "progression.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr int MIN_RIDDLE_XP = 500;
constexpr double RIDDLE_XP_LEVEL_PART = 0.3; // of the XP from the current level to the next
} // namespace

double levelXP(int level) { return level <= 1 ? 0.0 : 1000 * std::pow(level - 1, 1.4); }

float levelProgress(int level, double xp) {
	double start = levelXP(level);
	double gap = levelXP(level + 1) - start;
	return gap > 0 ? std::clamp(static_cast<float>((xp - start) / gap), 0.f, 1.f) : 0.f;
}

int riddleXP(int level) {
	double gap = levelXP(level + 1) - levelXP(level);
	return std::max(MIN_RIDDLE_XP, static_cast<int>(gap * RIDDLE_XP_LEVEL_PART));
}
