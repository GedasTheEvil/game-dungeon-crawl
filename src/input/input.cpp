#include <stdio.h>
#include <GL/gl.h>
#include "../state/cashe.h"
#include "input.h"
#include "input_actions.h"
#include "../ui/screen_state.h"
#include "../core/service_locator.h"

extern float rotW;
extern float rotM, rotN;

extern bool attacking;

unsigned char lastKey;

int lastMx = 0;
int lastMy = 0;

void Draw();

namespace {
void startJump() {
	if (GAME_STATE.jumping || GAME_STATE.falling || !GAME_STATE.Player->Alive() || GAME_STATE.IHaveWon)
		return;

	if (GAME_STATE.Player->Stamina() < JUMP_STAMINA_COST)
		return;

	GAME_STATE.Player->ConsumeStamina(JUMP_STAMINA_COST);

	float curX, curY;
	GAME_STATE.dungeon.getC(curX, curY);
	GAME_STATE.jump_start_y = curY;

	GAME_STATE.jump_dir_x = 0;
	if (lastKey == KEY_MOVE_LEFT || rotW < 0)
		GAME_STATE.jump_dir_x = -1;
	else if (lastKey == KEY_MOVE_RIGHT || rotW > 0)
		GAME_STATE.jump_dir_x = 1;

	GAME_STATE.jump_speed = JUMP_FORWARD_SPEED;
	GAME_STATE.jump_vel = JUMP_INITIAL_VELOCITY;
	GAME_STATE.jumping = true;
	GAME_STATE.jump_up_timer->Reset();
	GAME_STATE.jump_s.Play();
}

class PlayerActionController {
  public:
	static void execute(GameplayAction action) {
		float moveMultiplier = GAME_STATE.Stats->SprintMoveMultiplier();
		switch (action) {
		case GameplayAction::MoveLeft:
			GAME_STATE.dungeon.Move(-PLAYER_MOVE_STEP * moveMultiplier, 0);
			rotW = -110;
			GAME_STATE.Player->changeMDL(1);
			break;
		case GameplayAction::MoveRight:
			GAME_STATE.dungeon.Move(PLAYER_MOVE_STEP * moveMultiplier, 0);
			rotW = 70;
			GAME_STATE.Player->changeMDL(1);
			break;
		case GameplayAction::MoveDown:
			GAME_STATE.dungeon.Move(0, -PLAYER_MOVE_STEP * moveMultiplier);
			break;
		case GameplayAction::MoveUp:
			GAME_STATE.dungeon.Move(0, PLAYER_FORWARD_MOVE_STEP * moveMultiplier);
			break;
		case GameplayAction::Jump:
			startJump();
			break;
		case GameplayAction::Attack:
			tryAttack();
			break;
		case GameplayAction::Interact:
			interact();
			break;
		case GameplayAction::None:
			break;
		}
	}

	static void applyCameraDelta(float deltaX, float deltaY) {
		rotM += deltaX;
		rotN += deltaY;
		clampCamera();
	}

  private:
	static void tryAttack() {
		if (!GAME_STATE.Player->Att_timer->TimePassed())
			return;

		GAME_STATE.dungeon.GetAttack(GAME_STATE.Stats->Damage(), GAME_STATE.invent->Equipped()->range);
		GAME_STATE.Player->att_s.Play();
		attacking = true;
	}

	static void interact() {
		GAME_STATE.dungeon.GetPickUp();
		GAME_STATE.dungeon.GetRiddle();
	}

	static void clampCamera() {
		if (rotM > CAMERA_ROTATE_LIMIT_X)
			rotM = CAMERA_ROTATE_LIMIT_X;

		if (rotM < -CAMERA_ROTATE_LIMIT_X)
			rotM = -CAMERA_ROTATE_LIMIT_X;

		if (rotN > CAMERA_ROTATE_LIMIT_Y)
			rotN = CAMERA_ROTATE_LIMIT_Y;

		if (rotN < -CAMERA_ROTATE_LIMIT_Y)
			rotN = -CAMERA_ROTATE_LIMIT_Y;
	}
};
} // namespace

void Idle() { Draw(); }

void keyPressed(unsigned char key, int x, int y) {
	//      printf("You pressed %d\n",key);

	if (ScreenState::ShouldRouteKeyboardToRiddle(GAME_STATE)) {
		GAME_STATE.rid->KeyboardF(key, x, y);
		return;
	}

	if (key == KEY_ESCAPE) // esc
	{
		if (GAME_STATE.Stats->show || GAME_STATE.invent->show) {
			GAME_STATE.Stats->show = false;
			GAME_STATE.invent->show = false;
		}

		GAME_STATE.menu.ResetSubScreens();
		GAME_STATE.menu.show = !GAME_STATE.menu.show;
		return;
	}

	if (ScreenState::ShouldBlockKeyboardGameplay(GAME_STATE))
		return; // jei rodomas meniu, tai reaguojam tik i [esc]

	if (ScreenState::IsGameplayInteractionAllowed(GAME_STATE)) {
		PlayerActionController::execute(MapKeyboardGameplayAction(key));
	} // eo Alive

	if (key == KEY_INVENTORY) {
		GAME_STATE.invent->show = !GAME_STATE.invent->show;
		if (GAME_STATE.invent->show)
			GAME_STATE.Stats->show = false;
	}

	if (key == KEY_STATS) {
		GAME_STATE.Stats->show = !GAME_STATE.Stats->show;
		if (GAME_STATE.Stats->show)
			GAME_STATE.invent->show = false;
	}

	lastKey = key;
}

void specialKeyPressed(int key, int x, int y) {
	//      printf("Special key %d pressed\n",key);
	(void)x;
	(void)y;

	if (ScreenState::ShouldBlockKeyboardGameplay(GAME_STATE))
		return;

	if (key == SPECIAL_TOGGLE_CARTOON)
		GAME_STATE.Cartoon = !GAME_STATE.Cartoon;

	if (key == SPECIAL_TOGGLE_ORIGINAL_MODEL)
		GAME_STATE.Orig_model = !GAME_STATE.Orig_model;

	if (ScreenState::IsGameplayInteractionAllowed(GAME_STATE)) {
		PlayerActionController::execute(MapSpecialGameplayAction(key));
	}

	if (key == SPECIAL_CAMERA_LEFT) {
		PlayerActionController::applyCameraDelta(-CAMERA_ROTATE_STEP, 0);
	}
	if (key == SPECIAL_CAMERA_RIGHT) {
		PlayerActionController::applyCameraDelta(CAMERA_ROTATE_STEP, 0);
	}
	if (key == SPECIAL_CAMERA_UP) {
		PlayerActionController::applyCameraDelta(0, CAMERA_ROTATE_STEP);
	}
	if (key == SPECIAL_CAMERA_DOWN) {
		PlayerActionController::applyCameraDelta(0, -CAMERA_ROTATE_STEP);
	}

	if (key == SPECIAL_INTERACT) {
		PlayerActionController::execute(GameplayAction::Interact);
	}

	if (key == SPECIAL_SHIFT_LEFT || key == SPECIAL_SHIFT_RIGHT)
		GAME_STATE.Stats->SetSprintRequested(true);
}

void specialKeyReleased(int key, int x, int y) {
	(void)x;
	(void)y;

	if (key == SPECIAL_SHIFT_LEFT || key == SPECIAL_SHIFT_RIGHT)
		GAME_STATE.Stats->SetSprintRequested(false);
}

void processMouse(int button, int state, int x, int y) {
	if (ScreenState::ShouldRouteMouseToMenu(GAME_STATE)) {
		GAME_STATE.menu.MouseFunction(button, state, x, y);
		return;
	}

	if (ScreenState::ShouldRouteMouseToInventory(GAME_STATE)) {
		GAME_STATE.invent->MouseFunction(button, state, x, y);
		return;
	}

	if (state && ScreenState::IsGameplayInteractionAllowed(GAME_STATE)) {
		PlayerActionController::execute(MapMouseGameplayAction(button));
	}
}
void processMousePassiveMotion(int a, int b) {
	if (ScreenState::ShouldRouteMouseToMenu(GAME_STATE)) {
		GAME_STATE.menu.MousePassiveMotion(a, b);
		return;
	}

	PlayerActionController::applyCameraDelta(-MOUSE_LOOK_SENSITIVITY * (lastMx - a),
											 -MOUSE_LOOK_SENSITIVITY * (lastMy - b));

	lastMx = a;
	lastMy = b;
}

void processMouseActiveMotion(int a, int b) {
	(void)a;
	(void)b;
}

void processMouseEntry(int a) { (void)a; }
