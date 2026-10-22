#ifndef MONSTER_ROLL_H
#define MONSTER_ROLL_H

// The state of a Monster that rolls dung balls (RollRules, plan scarab-dung-ball); its update: monster_roll.cpp.
// Its back to the player, it shapes a ball for windupMs, kicks it off towards them (Dungeon::DungBall) and stands up
// again for DUNG_BALL_RECOVER_MS.
struct Roll {
	int startMs = -1; // GameClock time the windup began; < 0: none under way
	int nextMs = 0; // no windup before this GameClock time
	int dir = 1; // the way the ball goes, to the player: -1 / +1
	bool kicked = false; // this windup's ball is off
};

#endif
