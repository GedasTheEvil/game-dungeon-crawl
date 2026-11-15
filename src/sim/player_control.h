#ifndef PLAYER_CONTROL_H
#define PLAYER_CONTROL_H

// The player's controls in the world (plan player-control-in-sim): the attack, the power hit, the walk step and the
// world's per-tick player update, shared by the game (input.cpp, game_loop.cpp) and the unit tests' SimWorld. No
// sound, no key state, no camera: the facing comes in, what began goes out, and the caller plays the sound.

#include "../level/item_bag.h"
#include "../level/world_events.h"

class Dungeon;
class Player;

// The player as the bag's rules see them (ItemBag::Block, Use).
[[nodiscard]] BagUser bagUser(const Player& player);

struct PlayerControl {
	Dungeon& dungeon;
	Player& player;
	ItemBag& bag;
	WorldEvents& events;
	// This press of the attack key was refused (a ladder, a hold, Palsy) and told so: the key repeat does not tell it
	// again. The caller keeps it per press and clears it when the key comes up.
	bool& refusalTold;

	// What a press or a release of the attack key began: the caller plays its sound.
	enum class Began { None, Swing, ChargedSwing, Charge };

	// The attack key went down (not its repeat): a power hit's charge with the skill, the toggle on and a melee weapon
	// (plan skill-power-hit), else the swing or the bow draw.
	Began Press(int facing);
	// The swing or the bow draw begins; it hits when its hit time comes (UpdateAttack). None: not ready (a swing or
	// its recovery runs), not allowed here, Palsy with a ranged weapon, no shot left or a monster too close.
	Began Attack(int facing);
	// The key came up while a power hit charged: the charged swing (a tap's a plain one), from the windup the charge
	// held it at.
	Began ReleaseCharge();
	// The attack's hit time, each tick: a melee hit lands or the shot leaves. handHeight in tiles, the shot's height:
	// the game gives its drawn fist (PlayerView::Fist), SimWorld 0.65 of the player's height, the one difference
	// between them. True: something was struck or a shot left (the strike sound).
	bool UpdateAttack(int facing, float handHeight);

	// The walk keys' speed factor: no sprint while wading, except with the Favour of Hapi skill or the Blessing of Hapi
	// potion, nor in sand (Dungeon::PlayerSprintMultiplier).
	[[nodiscard]] float MoveMultiplier() const;
	// One step of a walk key. Only a step that really moved the player counts for the sprint drain, and none in the
	// water (no sprint there), unless the Favour of Hapi skill or the Blessing of Hapi potion lets the player sprint in
	// it (plan water-blessing-potion), nor in sand (plan desert-chapter). Wading and sand slow the step. True: moved.
	bool Walk(float dx, float dy);

	// After the world's update, each tick: the stamina, the poison and the healing, then every pose.
	void AfterUpdate();

  private:
	// No swing under way, its recovery over, and allowed here (not on a ladder, not held: told once a press).
	bool attackReady();
	// The swing begins; a charged one at its windup.
	void swing(bool charged);
};

#endif
