#ifndef MONSTER_GRAB_H
#define MONSTER_GRAB_H

// The state of a Monster whose bite grabs (HoldRules); its update: monster_grab.cpp.
// Grabbers (the crocodile, Sobek, the giant scorpion, the scorpion queen): a bite may open its jaws or claws wide
// (Tell), then lunge; in reach the player is grabbed and held (Hold) until it lets go.
enum class GrabPhase : unsigned char { Ready, Tell, Hold };
struct Grab {
	GrabPhase phase = GrabPhase::Ready;
	int startMs = 0; // GameClock time the phase began
	int endMs = 0; // Hold: it lets go then; the player's hits on a scorpion bring it nearer
	int ticks = 0; // Hold: the death roll's ticks dealt
};

#endif
