// What only the tests do to the dungeon (plan dungeon-subsystems): the scenario commands (src/test/scenario.cpp) and
// the unit tests hurt, kill, poison or stun a monster, set its health, lay a web or a burning floor, point a mirror.
// The game never calls these. Use: DungeonCheats(dungeon).HurtBoss(10).
#ifndef DUNGEON_CHEATS_H
#define DUNGEON_CHEATS_H
#include "dungeon.h"

class DungeonCheats {
  public:
	explicit DungeonCheats(Dungeon& dungeon) : d(dungeon) {}
	bool KillNearestMonster(); // as if the player killed it (XP, note, drop); false if none lives (plan
							   // kill-monster-command)
	bool SetNearestMonsterHealth(int hp); // no hit, no reward; false if none (plan monster-hp-command)
	bool SetBossHealth(int hp); // ... the boss in play's; false if none
	void HurtBoss(int dmg); // the boss in play takes a hit, as from the player
	// The living monster nearest the player, or the boss in play, is poisoned, as by the player (it may shrug it off).
	void PoisonNearestMonster(PoisonTier tier);
	void PoisonBoss(PoisonTier tier);
	void BleedNearestMonster(BleedTier tier); // a sure cut by the player (plan bleeding): bloodless, none
	// Scenario `stun`: the boss of the level while it lives, else the living monster nearest the player, is stunned for
	// STUN_MS, as by a blunt hit with the Stun skill (Monster::StunHit; a boss for less). False: it cannot be (lurking,
	// hidden, ...), or there is none.
	bool StunNearest();
	// Scenario `grab`: the living monster nearest the player starts a grab's tell now (Monster::StartGrab). False: it
	// does not grab, or is grabbing already.
	bool GrabNearest();
	// A web over the floor of (col, row). False: no floor for one there (webFloor), or one lies there.
	bool PlaceWeb(int col, int row);
	// The floor of (col, row) burns for ms. False: no dry floor there (blazeFloor).
	bool PlaceBlaze(int col, int row, int ms);
	// Scenario `mirror`: the mirror in (col, row) points dir (BeamDir), at once. False: no mirror there.
	bool SetMirror(int col, int row, int dir);

  private:
	Dungeon& d;
	[[nodiscard]] Monster* nearest() const; // the living monster nearest the player; null if none
	[[nodiscard]] Monster* boss() const; // the boss in play (alive or not); null if none
};

#endif
