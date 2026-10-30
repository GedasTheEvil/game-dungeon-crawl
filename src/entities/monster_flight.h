#ifndef MONSTER_FLIGHT_H
#define MONSTER_FLIGHT_H

// The state of a Monster that flies; its update: monster_flight.cpp.
// Flying monsters (bats): hang on the ceiling until the player comes near, then swoop through them,
// biting on the way, fly on, turn and come back.
enum class FlightPhase : unsigned char { Roost, Swoop, Return };
struct Flight {
	FlightPhase phase = FlightPhase::Roost;
	int dir = 1; // +1 flying right, -1 left
	float lift = -1.f; // world units from the floor to the model origin; < 0: not placed yet (on the ceiling)
	float fall = 0.f; // falling speed after death, world units per second
	bool bitten = false; // this pass has bitten already
	int attackUntilMs = 0;
	int lastMs = -1; // GameClock time of the last update
	// The screech (plan giant-bat-screech, ScreechRules): the tell's end, < 0: none under way; no new tell before
	// screechReadyMs (GameClock).
	int tellUntilMs = -1;
	int screechReadyMs = 0;
};
// What a flyer knows of the level this tick (Dungeon::flightGoal): it follows the player to another row through open
// cells (plan bats-high-ceiling).
struct FlightGoal {
	bool sees = false; // the player is in sight: on its row as before, or in reach on another row with a way there
	int toRow = -1; // the row it heads for: the player's while it sees them, else its roost's
	float shaftX = 0.f; // tile-local x where it goes up or down a row next, on the way to toRow
	bool openBelow = false; // the cell under it is open: a dead flyer falls on through it
	bool inCone = false; // the player is in its screech's cone, no wall between (ScreechRules)
	// Provoked, with no way to the player (plan hit-wakes-monster): it keeps off its roost, away from them.
	bool flee = false;
	int fleeDir = 0; // the way it flies off while the player is near along the row, -1 / +1; 0: far enough, it hovers
};

#endif
