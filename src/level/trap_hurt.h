#ifndef TRAP_HURT_H
#define TRAP_HURT_H
#include "gameplay_config.h"
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
	float lastX = 0.f; // the body's map x on the tick before: the way it covered in a trap (TrapBell)
	bool venomed = false; // a death trap's poison took on this entry (once per entry, reset on leaving it)
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
	// What hits() TrapHurt hits in a row deal together, the ramp from the start (plan ceiling-spikes: a monster's
	// whole crossing at once).
	[[nodiscard]] static constexpr int rampTotal(int hits) {
		int total = 0;
		for (int k = 0; k < hits; k++)
			total += 1 + k / TRAP_DAMAGE_RAMP_HITS;
		return total;
	}
};

// Crossing a spike or death trap on foot (the player and a reckless monster, plan same-hazards-for-all): a bell curve
// of damage over its hitbox, the offset from its middle in half widths, -1 to 1 (gameplay_config.h, SPIKES_CROSS_HP).
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
// The same for a spike (death false) or a death trap.
[[nodiscard]] inline int trapCrossHundredths(bool death, int maxHp) {
	return death ? crossHundredths(DEATH_TRAP_CROSS_HP, DEATH_TRAP_CROSS_PERCENT, maxHp)
				 : crossHundredths(SPIKES_CROSS_HP, SPIKES_CROSS_PERCENT, maxHp);
}
// TrapHurt hits a walk across a trap's tile takes a body of that half width at that pace (tiles a second): its box,
// TRAP_BOX_FORGIVE in, across the tile.
[[nodiscard]] inline int crossHits(float tilesPerSecond, float halfWidth) {
	const float way = 1.f + 2.f * halfWidth * (1.f - TRAP_BOX_FORGIVE);
	return static_cast<int>(
		std::ceil(way / std::max(0.01f, tilesPerSecond) * 1000.f / static_cast<float>(TRAP_HURT_INTERVAL_MS))
	);
}
// A whole walk through the trap at that pace, before the body's share, in hundredths of a HP: the bell's crossing at
// the speed and TrapHurt's ramp over the time it takes (a jump into ceiling spikes, plan ceiling-spikes).
[[nodiscard]] inline int walkHundredths(bool death, int maxHp, float tilesPerSecond, float halfWidth) {
	const double bell = trapCrossHundredths(death, maxHp) * speedFactor(tilesPerSecond);
	return static_cast<int>(std::lround(bell)) + 100 * TrapHurt::rampTotal(crossHits(tilesPerSecond, halfWidth));
}
} // namespace TrapBell

// A fire vent's jet (plan fire-jet-trap), the same for every body (plan same-hazards-for-all): hundredths of a HP a
// second, FIRE_JET_PERCENT_PER_SECOND of its max HP or FIRE_JET_HP_PER_SECOND, whichever is more; dealt each
// TRAP_HURT_INTERVAL_MS, before its trap share and its fire resistance.
[[nodiscard]] inline int fireJetHundredthsPerSecond(int maxHp) {
	return std::max(FIRE_JET_HP_PER_SECOND * 100, maxHp * FIRE_JET_PERCENT_PER_SECOND);
}

#endif
