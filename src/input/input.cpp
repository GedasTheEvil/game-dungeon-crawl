#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include "../state/game_state.h"
#include "input.h"
#include "input_actions.h"
#include "../ui/screen_state.h"
#include "../core/service_locator.h"

unsigned char lastKey;

int lastMx = 0;
int lastMy = 0;

namespace {
void startJump() {
	if (GAME_STATE.Player->jump.jumping || GAME_STATE.Player->jump.falling || !GAME_STATE.Player->Alive() ||
		GAME_STATE.IHaveWon)
		return;

	if (GAME_STATE.Player->Stamina() < JUMP_STAMINA_COST)
		return;

	GAME_STATE.Player->ConsumeStamina(JUMP_STAMINA_COST);

	float curX, curY;
	GAME_STATE.dungeon.getC(curX, curY);
	GAME_STATE.Player->jump.start_y = curY;

	GAME_STATE.Player->jump.dir_x = 0;
	if (lastKey == KEY_MOVE_LEFT || GAME_STATE.camera.rotW < 0)
		GAME_STATE.Player->jump.dir_x = -1;
	else if (lastKey == KEY_MOVE_RIGHT || GAME_STATE.camera.rotW > 0)
		GAME_STATE.Player->jump.dir_x = 1;

	GAME_STATE.Player->jump.speed = JUMP_FORWARD_SPEED;
	GAME_STATE.Player->jump.velocity = JUMP_INITIAL_VELOCITY;
	GAME_STATE.Player->jump.jumping = true;
	GAME_STATE.Player->jump.jump_up_timer->Reset();
	GAME_STATE.sounds.jump_s.Play();
}

class PlayerActionController {
  public:
	static void execute(GameplayAction action) {
		float moveMultiplier = GAME_STATE.ui.Stats->SprintMoveMultiplier();
		switch (action) {
		case GameplayAction::MoveLeft:
			GAME_STATE.dungeon.Move(-PLAYER_MOVE_STEP * moveMultiplier, 0);
			GAME_STATE.camera.rotW = -110;
			GAME_STATE.Player->changeMDL(2);
			break;
		case GameplayAction::MoveRight:
			GAME_STATE.dungeon.Move(PLAYER_MOVE_STEP * moveMultiplier, 0);
			GAME_STATE.camera.rotW = 70;
			GAME_STATE.Player->changeMDL(2);
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
		GAME_STATE.camera.rotM += deltaX;
		GAME_STATE.camera.rotN += deltaY;
		clampCamera();
	}

  private:
	static void tryAttack() {
		if (!GAME_STATE.Player->Att_timer->TimePassed())
			return;

		GAME_STATE.dungeon.GetAttack(GAME_STATE.ui.Stats->Damage(), GAME_STATE.ui.invent->Equipped()->range);
		GAME_STATE.Player->att_s.Play();
		GAME_STATE.Player->attacking = true;
	}

	static void interact() {
		GAME_STATE.dungeon.GetPickUp();
		GAME_STATE.dungeon.GetRiddle();
	}

	static void clampCamera() {
		if (GAME_STATE.camera.rotM > CAMERA_ROTATE_LIMIT_X)
			GAME_STATE.camera.rotM = CAMERA_ROTATE_LIMIT_X;

		if (GAME_STATE.camera.rotM < -CAMERA_ROTATE_LIMIT_X)
			GAME_STATE.camera.rotM = -CAMERA_ROTATE_LIMIT_X;

		if (GAME_STATE.camera.rotN > CAMERA_ROTATE_LIMIT_Y)
			GAME_STATE.camera.rotN = CAMERA_ROTATE_LIMIT_Y;

		if (GAME_STATE.camera.rotN < -CAMERA_ROTATE_LIMIT_Y)
			GAME_STATE.camera.rotN = -CAMERA_ROTATE_LIMIT_Y;
	}
};
} // namespace

void Idle() { glutPostRedisplay(); }

void keyPressed(unsigned char key, int x, int y) {
	if (ScreenState::ShouldRouteKeyboardToRiddle(GAME_STATE)) {
		GAME_STATE.ui.rid->KeyboardF(key, x, y);
		return;
	}

	if (key == KEY_ESCAPE) // esc
	{
		if (GAME_STATE.ui.Stats->show || GAME_STATE.ui.invent->show) {
			GAME_STATE.ui.Stats->show = false;
			GAME_STATE.ui.invent->show = false;
		}

		GAME_STATE.ui.menu.ResetSubScreens();
		GAME_STATE.ui.menu.show = !GAME_STATE.ui.menu.show;
		return;
	}

	if (ScreenState::ShouldBlockKeyboardGameplay(GAME_STATE))
		return; // jei rodomas meniu, tai reaguojam tik i [esc]

	if (ScreenState::IsGameplayInteractionAllowed(GAME_STATE)) {
		PlayerActionController::execute(MapKeyboardGameplayAction(key));
	} // eo Alive

	if (key == KEY_INVENTORY) {
		GAME_STATE.ui.invent->show = !GAME_STATE.ui.invent->show;
		if (GAME_STATE.ui.invent->show)
			GAME_STATE.ui.Stats->show = false;
	}

	if (key == KEY_STATS) {
		GAME_STATE.ui.Stats->show = !GAME_STATE.ui.Stats->show;
		if (GAME_STATE.ui.Stats->show)
			GAME_STATE.ui.invent->show = false;
	}

	lastKey = key;
}

void specialKeyPressed(int key, int x, int y) {
	(void)x;
	(void)y;

	if (ScreenState::ShouldBlockKeyboardGameplay(GAME_STATE))
		return;

	if (key == SPECIAL_TOGGLE_CARTOON)
		GAME_STATE.render.Cartoon = !GAME_STATE.render.Cartoon;

	if (key == SPECIAL_TOGGLE_ORIGINAL_MODEL)
		GAME_STATE.render.Orig_model = !GAME_STATE.render.Orig_model;

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
		GAME_STATE.ui.Stats->SetSprintRequested(true);
}

void specialKeyReleased(int key, int x, int y) {
	(void)x;
	(void)y;

	if (key == SPECIAL_SHIFT_LEFT || key == SPECIAL_SHIFT_RIGHT)
		GAME_STATE.ui.Stats->SetSprintRequested(false);
}

void processMouse(int button, int state, int x, int y) {
	if (ScreenState::ShouldRouteMouseToMenu(GAME_STATE)) {
		GAME_STATE.ui.menu.MouseFunction(button, state, x, y);
		return;
	}

	if (ScreenState::ShouldRouteMouseToInventory(GAME_STATE)) {
		GAME_STATE.ui.invent->MouseFunction(button, state, x, y);
		return;
	}

	if (state && ScreenState::IsGameplayInteractionAllowed(GAME_STATE)) {
		PlayerActionController::execute(MapMouseGameplayAction(button));
	}
}
void processMousePassiveMotion(int a, int b) {
	if (ScreenState::ShouldRouteMouseToMenu(GAME_STATE)) {
		GAME_STATE.ui.menu.MousePassiveMotion(a, b);
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
