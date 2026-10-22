#ifndef TRAP_HURT_H
#define TRAP_HURT_H
#include "../core/gameplay_config.h"
#include "../core/timer.h"
#include <algorithm>
#include <cmath>

// Spike and death trap tiles hurt whoever stands in them (the player, a reckless monster), every
// TRAP_HURT_INTERVAL_MS, however many traps: one per body (Dungeon::updateTraps). The damage grows while it stays in
// (TRAP_DAMAGE_RAMP_HITS); a gap of TRAP_STREAK_RESET_MS resets it.
struct TrapHurt {
	Timer timer{TRAP_HURT_INTERVAL_MS};
	int streak = 0;
	int lastHitMs = 0;
	// Called each tick it is in a trap: the damage of the hit due now, else 0.
	int hit() {
		if (!timer.TimePassed())
			return 0;
		const int now = GameClock::now();
		if (now - lastHitMs > TRAP_STREAK_RESET_MS)
			streak = 0;
		lastHitMs = now;
		return 1 + streak++ / TRAP_DAMAGE_RAMP_HITS;
	}
};

// Crossing a spike or death trap (the player): a bell curve of damage over its hitbox, the offset from its middle in
// half widths, -1 to 1 (gameplay_config.h, SPIKES_CROSS_HP).
namespace TrapBell {
// The share of a whole crossing's damage between the offsets from and to (either way), 0 to 1.
[[nodiscard]] inline double share(float from, float to) {
	const double k = 1.0 / (TRAP_BELL_SIGMA * std::sqrt(2.0));
	const auto at = [k](float u) { return std::erf(k * std::clamp(static_cast<double>(u), -1.0, 1.0)); };
	return std::fabs(at(to) - at(from)) / (2.0 * at(1.f));
}
// The crossing's damage multiplier at that speed (tiles a second): 1 walking (or slower), TRAP_SPRINT_FACTOR at a
// sprint.
[[nodiscard]] inline double speedFactor(float tilesPerSecond) {
	const float sprint = 3.f * WALK_SPEED;
	const float t = std::clamp((tilesPerSecond - WALK_SPEED) / (sprint - WALK_SPEED), 0.f, 1.f);
	return 1.0 + (TRAP_SPRINT_FACTOR - 1.0) * t;
}
// A whole crossing's damage in hundredths of a HP: the floor or the percent of maxHp, whichever is more.
[[nodiscard]] inline int crossHundredths(int floorHp, int percent, int maxHp) {
	return std::max(floorHp * 100, maxHp * percent);
}
} // namespace TrapBell

#endif
