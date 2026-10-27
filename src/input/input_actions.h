#ifndef INPUT_ACTIONS_H
#define INPUT_ACTIONS_H

#include "input.h"
#include "bindings.h"
#include <string>

enum class GameplayAction : unsigned char {
	None,
	MoveLeft,
	MoveRight,
	MoveDown,
	MoveUp,
	Jump,
	Attack,
	Interact,
	QuickHeal, // drink the best fitting healing potion (Inventory::QuickDrink)
	QuickStamina, // the same for stamina
	StaminaSkills, // switch the learned stamina skills on or off (Player::staminaSkills)
	QuickAntidote, // drink an antidote while poisoned (Inventory::QuickAntidote)
	EquipMelee, // the next melee weapon held (Inventory::EquipNext)
	EquipRanged, // the next ranged one
};

// Runs an action as if the player pressed its key (used by scenario tests). A move action takes one step.
void executeGameplayAction(GameplayAction action);

// The walk keys held down (MoveLeft, MoveRight, MoveDown, MoveUp; other actions are ignored). Update() moves the
// player one step a tick while one is held (stepHeldWalk).
void setWalkHeld(GameplayAction move, bool held);
void releaseWalk();
void stepHeldWalk();

// Power hit (plan skill-power-hit): once a tick, the charge drains; the attack key let go since the last tick swings
// it. Waiting for the tick lets a key repeat's release and press (X sends both) cancel out.
void updatePowerCharge();
// The attack key let go (also the scenario `attack up`).
void releaseAttack();

// The gameplay action a binding runs; None for the ones handled outside it (sprint, look, the screens).
GameplayAction gameplayActionOf(BindAction action);

// Key cap labels for the HUD, from the bindings: "H", "1-2"; empty when unbound.
std::string keyCapOf(BindAction action);
std::string equipKeysCap();

#endif
