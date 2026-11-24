#ifndef MONSTER_CLIMB_H
#define MONSTER_CLIMB_H

// The state of a Monster that climbs; its update: monster_climb.cpp.
// Climbers on a ladder (plan monster-climbers): from its row up or down to toRow, y map units off the row's floor on
// the way. A climb once started runs to its end.
struct Climb {
	int toRow = -1; // < 0: not climbing
	float y = 0.f;
	bool falls = false; // killed on the rungs: it drops down the shaft to the floor (CLIMBER_FALL_STEP)
};

#endif
