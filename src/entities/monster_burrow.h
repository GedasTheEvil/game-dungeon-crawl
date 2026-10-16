#ifndef MONSTER_BURROW_H
#define MONSTER_BURROW_H

// The state of a Monster that burrows; its update: monster_burrow.cpp.
// Burrowers (Apep): dive into the floor, stay under it, come up elsewhere on the row (Dungeon::burrowTarget picks
// where). Hidden while diving and under: nothing hits it, it bites nothing.
enum class BurrowPhase : unsigned char { Up, Dive, Under, Surface };
struct Burrow {
	BurrowPhase phase = BurrowPhase::Up;
	int startMs = 0; // GameClock time the phase began
	int nextMs = 0; // no dive before this GameClock time
	float toX = 0.f; // tile-local x it comes up at
};

#endif
