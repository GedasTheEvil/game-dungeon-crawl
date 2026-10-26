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
};

#endif
