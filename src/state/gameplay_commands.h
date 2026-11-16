#ifndef GAMEPLAY_COMMANDS_H
#define GAMEPLAY_COMMANDS_H

#include "../level/camera_look.h"
#include "../sim/player_control.h"

// The player's gameplay commands (plan game-state-split): what a key, a click or a scenario asks the player to do, and
// the per-tick steps of the keys held. The input module maps the bound keys to them (input/input.cpp).

enum class GameplayAction : unsigned char {
	None,
	MoveLeft,
	MoveRight,
	MoveDown,
	MoveUp,
	Jump,
	Attack,
	Interact,
	Drag, // take hold of a mirror or let go (Dungeon::ToggleDrag, plan mirror-drag)
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
void releaseWalk(); // the look keys too
void stepHeldWalk();
// Once a tick in the game (plan controls-smoothing): the look keys held turn the camera, an idle peek eases back.
void stepCameraLook();
// Power hit (plan skill-power-hit): once a tick, the charge drains; the attack key let go since the last tick swings
// it. Waiting for the tick lets a key repeat's release and press (X sends both) cancel out.
void updatePowerCharge();
// The attack key let go (also the scenario `attack up`).
void releaseAttack();

// The game's player controls (plan player-control-in-sim): the attack's hit time, the per-tick update.
PlayerControl playerControl();

[[nodiscard]] bool isMoveAction(GameplayAction action);
// Insanity (plan curses) locks the screens, the quick drinks and the weapon keys: true, with the status line why.
bool refusedInsane();
// The mouse turned the camera.
void turnCamera(float deltaX, float deltaY);
// A look key went down or up: stepCameraLook turns the camera each tick while one is held.
void holdLook(CameraLook::Dir dir, bool down);
// A new press of the attack key (not the key repeat): a refusal on a ladder is told again.
void newAttackPress();

#endif
