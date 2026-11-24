#ifndef MONSTER_BURROW_H
#define MONSTER_BURROW_H

// The state of a Monster that burrows; its update: monster_burrow.cpp.
// Burrowers (Apep): dive into the floor, stay under it, come up elsewhere on the row (Dungeon::burrowTarget picks
// where). Hidden while diving and under: nothing hits it, it bites nothing.
// Sand burrowers (the worm, plan desert-chapter): only in sand. Spawned in it, it lies under it (lurk) until the player
// comes near; under the sand it moves to where it comes up (fromX to toX, a ripple), under the player's feet. With a
// rise clip (the worm's) it bursts up out of the sand (Surface) and dives head first (Dive: the clip backwards).
enum class BurrowPhase : unsigned char { Up, Dive, Under, Surface };
struct Burrow {
	BurrowPhase phase = BurrowPhase::Up;
	int startMs = 0; // GameClock time the phase began
	int nextMs = 0; // no dive before this GameClock time
	float fromX = 0.f; // tile-local x it went under at
	float toX = 0.f; // tile-local x it comes up at
	bool lurk = false; // a sand burrower Under since its spawn: it stays there until Surge
	int dives = 0; // since its spawn: a sand burrower that comes up behind does so every other dive
};

#endif
