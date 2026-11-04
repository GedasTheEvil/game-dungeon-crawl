#ifndef MONSTER_LEAP_H
#define MONSTER_LEAP_H

// The state of a Monster that leaps; its update: monster_leap.cpp.
// Walk-jumpers: a leap over a gap of pits and traps, x in tile-local map units like Monster::x.
struct Leap {
	float fromX = 0.f, toX = 0.f;
	int startMs = -1; // GameClock time of the take-off; < 0: on the ground
	int readyMs = 0; // no new leap before this GameClock time
	int pounceReadyMs = 0; // no new pounce (PounceRules) before this GameClock time
	int pounceDir = 0; // a pounce: the way to the player (-1 / +1), its landing follows them; 0: a leap over a gap
	float lift = 0.f; // world units from the floor to the model origin
	// Head bump (plan monster-head-bump): the air fraction of the leap at which the rise met a ceiling, < 0: none;
	// the lift it stopped at; the stun it rolled, begun at the landing (a leap is no time to be stunned).
	float bumpAir = -1.f, bumpLift = 0.f;
	int bumpStunMs = 0;
	int bumpMemoryMs = -1; // GameClock time until which the last bump is remembered (no pounce under a ceiling)
	// A leap at a ladder's rungs (plan jump-onto-ladder): over the ladder's column at air share catchAir, the climb
	// to catchToRow begins at the arc's height. catchCol < 0: a plain leap.
	int catchCol = -1, catchToRow = -1;
	float catchAir = 0.f;
};

// What a tick of a leap did to the ceiling (Monster::UpdateJump).
enum class LeapBump : unsigned char { None, Knock, Hard };

#endif
