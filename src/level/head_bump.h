#ifndef HEAD_BUMP_H
#define HEAD_BUMP_H

// The head bump's one rule (plans head-bump, monster-head-bump): what a knock on the head costs and whether it dazes.
// Shared by Player::HeadBump and Monster::HeadBump; under a ceiling trap the spikes replace it (plan ceiling-spikes).

#include <algorithm>
#include "rng.h"
#include "../core/gameplay_config.h"

// The larger of HEAD_BUMP_MIN_HP and HEAD_BUMP_MAX_HP_PCT % of the max HP, straight off the HP.
[[nodiscard]] constexpr int headBumpHp(int maxHp) {
	return std::max(HEAD_BUMP_MIN_HP, maxHp * HEAD_BUMP_MAX_HP_PCT / 100);
}

struct HeadBumpDaze {
	bool daze; // this bump dazes (the player) or stuns (a monster)
	int ms; // for how long, HEAD_BUMP_DAZE_MIN_MS .. HEAD_BUMP_DAZE_MAX_MS
};

// Always draws both numbers, so the gameplay stream moves the same either way.
[[nodiscard]] inline HeadBumpDaze rollHeadBumpDaze(Rng& rng) {
	const bool daze = rng.percent(HEAD_BUMP_DAZE_PCT);
	return {daze, rng.range(HEAD_BUMP_DAZE_MIN_MS, HEAD_BUMP_DAZE_MAX_MS)};
}

#endif
