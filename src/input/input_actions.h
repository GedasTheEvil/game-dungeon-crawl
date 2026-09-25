#ifndef INPUT_ACTIONS_H
#define INPUT_ACTIONS_H

#include "input.h"

enum class GameplayAction {
	None,
	MoveLeft,
	MoveRight,
	MoveDown,
	MoveUp,
	Jump,
	Attack,
	Interact,
};

// Runs an action as if the player pressed its key (used by scenario tests).
void executeGameplayAction(GameplayAction action);

inline GameplayAction MapKeyboardGameplayAction(unsigned char key) {
	switch (key) {
	case KEY_MOVE_LEFT:
	case KEY_MOVE_LEFT_UPPER:
		return GameplayAction::MoveLeft;
	case KEY_MOVE_RIGHT:
	case KEY_MOVE_RIGHT_UPPER:
		return GameplayAction::MoveRight;
	case KEY_MOVE_DOWN:
	case KEY_MOVE_DOWN_UPPER:
		return GameplayAction::MoveDown;
	case KEY_MOVE_UP:
	case KEY_MOVE_UP_UPPER:
		return GameplayAction::MoveUp;
	case KEY_SPACE:
		return GameplayAction::Jump;
	case KEY_ENTER:
	case KEY_ATTACK:
	case KEY_ATTACK_UPPER:
		return GameplayAction::Attack;
	case KEY_INTERACT:
	case KEY_INTERACT_UPPER:
		return GameplayAction::Interact;
	default:
		return GameplayAction::None;
	}
}

inline GameplayAction MapSpecialGameplayAction(int key) {
	switch (key) {
	case SPECIAL_MOVE_LEFT:
		return GameplayAction::MoveLeft;
	case SPECIAL_MOVE_RIGHT:
		return GameplayAction::MoveRight;
	case SPECIAL_MOVE_DOWN:
		return GameplayAction::MoveDown;
	case SPECIAL_MOVE_UP:
		return GameplayAction::MoveUp;
	default:
		return GameplayAction::None;
	}
}

inline GameplayAction MapMouseGameplayAction(int button) {
	switch (button) {
	case MOUSE_LEFT_BUTTON:
		return GameplayAction::Attack;
	case MOUSE_MIDDLE_BUTTON:
		return GameplayAction::Interact;
	case MOUSE_RIGHT_BUTTON:
		return GameplayAction::Jump;
	default:
		return GameplayAction::None;
	}
}

#endif
