#ifndef MOVEMENT_H
#define MOVEMENT_H

// The player's jump as the game plays it (Dungeon::UpdateMovementState: every JUMP_TICK_MS the height grows by the
// velocity, which drops by JUMP_GRAVITY_STEP; the jump ends back at the take-off height), computed from the same
// constants, for the level checker's rules. The game compares absolute heights, so float rounding can add one tick
// (19 instead of 18 from rows 1 to 3); the arc here is the intended one.

#include "../core/gameplay_config.h"

namespace Jump {
struct Arc {
	int ticks = 0;	   // until the player is back at the take-off height
	float peak = 0.f;  // tiles above the take-off
	float drift = 0.f; // tiles the jump carries the player forward, without the walk key
	[[nodiscard]] constexpr int ms() const { return ticks * JUMP_TICK_MS; }
};

constexpr Arc arc() {
	Arc a;
	float height = 0.f;
	float velocity = JUMP_INITIAL_VELOCITY;
	do {
		height += velocity;
		velocity -= JUMP_GRAVITY_STEP;
		a.ticks++;
		a.peak = height > a.peak ? height : a.peak;
	} while ((velocity > 0.f || height > 1e-4f) && a.ticks < 1000);
	a.drift = static_cast<float>(a.ticks) * JUMP_FORWARD_SPEED;
	return a;
}

constexpr Arc ARC = arc();
} // namespace Jump

// The checker's walker (level_check.cpp) moves by whole cells and relies on these.
static_assert(Jump::ARC.peak < 1.f, "the checker assumes a jump cannot step up onto a ledge one cell high");
static_assert(Jump::ARC.drift > 0.5f, "the checker assumes a jump clears a one-cell gap");

#endif
