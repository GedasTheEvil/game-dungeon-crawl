#ifndef MONSTER_CHARGE_H
#define MONSTER_CHARGE_H

// The state of a Monster that charges; its update: monster_charge.cpp.
// Chargers (Sobek, the hippo): from afar on its row it lowers its head (Windup), rushes along the row (Rush) through
// the player, and on into a wall that stuns it (Stunned: it takes double damage). The player jumps over it or leaves
// the row.
enum class ChargePhase : unsigned char { Ready, Windup, Rush, Stunned };
struct Charge {
	ChargePhase phase = ChargePhase::Ready;
	int startMs = 0;
	int nextMs = 0; // no charge before this GameClock time
	int dir = 1; // -1 / +1 along the row
	bool hit = false; // this rush has hit the player
};

#endif
