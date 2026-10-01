#ifndef TRAP_HURT_H
#define TRAP_HURT_H
#include "../core/gameplay_config.h"
#include "../core/timer.h"

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

#endif
