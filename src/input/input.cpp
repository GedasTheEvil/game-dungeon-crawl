#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include "../state/game_state.h"
#include "input.h"
#include "input_actions.h"
#include "../ui/screen_state.h"

unsigned char lastKey;

int lastMx = 0;
int lastMy = 0;

namespace {
void startJump() {
	if (Game().player->jump.jumping || Game().player->jump.falling || !Game().player->Alive() || Game().hasWon)
		return;

	if (Game().player->stats.Stamina() < JUMP_STAMINA_COST)
		return;

	Game().player->stats.ConsumeStamina(JUMP_STAMINA_COST);

	float curX, curY;
	Game().dungeon.getC(curX, curY);
	Game().player->jump.start_y = curY;

	Game().player->jump.dir_x = 0;
	if (lastKey == KEY_MOVE_LEFT || Game().camera.rotW < 0)
		Game().player->jump.dir_x = -1;
	else if (lastKey == KEY_MOVE_RIGHT || Game().camera.rotW > 0)
		Game().player->jump.dir_x = 1;

	Game().player->jump.speed = JUMP_FORWARD_SPEED;
	Game().player->jump.velocity = JUMP_INITIAL_VELOCITY;
	Game().player->jump.jumping = true;
	Game().player->jump.jump_up_timer.Reset();
	Game().player->PlayJumpSound();
}

class PlayerActionController {
  public:
	static void execute(GameplayAction action) {
		float moveMultiplier = Game().player->stats.SprintMoveMultiplier();
		switch (action) {
		case GameplayAction::MoveLeft:
			Game().dungeon.Move(-PLAYER_MOVE_STEP * moveMultiplier, 0);
			Game().camera.rotW = -110;
			if (!Game().player->jump.jumping)
				Game().player->setModelState(ModelState::Move);
			break;
		case GameplayAction::MoveRight:
			Game().dungeon.Move(PLAYER_MOVE_STEP * moveMultiplier, 0);
			Game().camera.rotW = 70;
			if (!Game().player->jump.jumping)
				Game().player->setModelState(ModelState::Move);
			break;
		case GameplayAction::MoveDown:
			Game().dungeon.Move(0, -PLAYER_MOVE_STEP * moveMultiplier);
			break;
		case GameplayAction::MoveUp:
			Game().dungeon.Move(0, PLAYER_FORWARD_MOVE_STEP * moveMultiplier);
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
		Game().camera.rotM += deltaX;
		Game().camera.rotN += deltaY;
		clampCamera();
	}

  private:
	static void tryAttack() {
		if (!Game().player->attackTimer.TimePassed())
			return;

		Game().dungeon.AttackNearest(Game().player->stats.Damage(), Game().ui.inventory->Equipped()->range);
		Game().player->PlayAttackSound();
		Game().player->attacking = true;
	}

	static void interact() {
		Game().dungeon.PickUp();
		if (Game().dungeon.PullLever())
			return;
		Game().dungeon.Interact();
	}

	static void clampCamera() {
		if (Game().camera.rotM > CAMERA_ROTATE_LIMIT_X)
			Game().camera.rotM = CAMERA_ROTATE_LIMIT_X;

		if (Game().camera.rotM < -CAMERA_ROTATE_LIMIT_X)
			Game().camera.rotM = -CAMERA_ROTATE_LIMIT_X;

		if (Game().camera.rotN > CAMERA_ROTATE_LIMIT_Y)
			Game().camera.rotN = CAMERA_ROTATE_LIMIT_Y;

		if (Game().camera.rotN < -CAMERA_ROTATE_LIMIT_Y)
			Game().camera.rotN = -CAMERA_ROTATE_LIMIT_Y;
	}
};
} // namespace

void executeGameplayAction(GameplayAction action) { PlayerActionController::execute(action); }

void Idle() { glutPostRedisplay(); }

void keyPressed(unsigned char key, int x, int y) {
	if (ScreenState::ShouldRouteKeyboardToRiddle(Game())) {
		Game().ui.riddle->KeyboardF(key, x, y);
		return;
	}

	if (key == KEY_ESCAPE) // esc
	{
		// Esc backs out of the inventory or the map to the game; only from the game it opens the menu.
		if (!Game().ui.menu.show && Game().ui.inventory->show) {
			Game().ui.inventory->show = false;
			return;
		}
		if (!Game().ui.menu.show && Game().ui.map.show) {
			Game().ui.map.show = false;
			return;
		}

		// In the menu Esc backs out of the save / load / options screens, then returns to the game (if there is one).
		MainMenu& menu = Game().ui.menu;
		if (menu.show && menu.InSubScreen()) {
			menu.ResetSubScreens();
			return;
		}
		if (menu.show && !menu.inGame)
			return;
		menu.ResetSubScreens();
		menu.show = !menu.show;
		return;
	}

	if (ScreenState::ShouldBlockKeyboardGameplay(Game()))
		return; // while the menu is shown, only [esc] is handled

	if (Game().ui.inventory->show && key != KEY_INVENTORY) {
		Game().ui.inventory->KeyPressed(key);
		return;
	}

	// The map pauses the game: only M (or Esc, above) closes it.
	if (key == KEY_MAP || key == KEY_MAP_UPPER) {
		if (!Game().ui.inventory->show)
			Game().ui.map.show = !Game().ui.map.show;
		return;
	}
	if (Game().ui.map.show)
		return;

	if (ScreenState::IsGameplayInteractionAllowed(Game())) {
		PlayerActionController::execute(MapKeyboardGameplayAction(key));
	} // eo Alive

	if (key == KEY_INVENTORY)
		Game().ui.inventory->show = !Game().ui.inventory->show;

	lastKey = key;
}

void specialKeyPressed(int key, int x, int y) {
	(void)x;
	(void)y;

	if (ScreenState::ShouldBlockKeyboardGameplay(Game()) || Game().ui.map.show)
		return;

	if (Game().ui.inventory->show && key != SPECIAL_SHIFT_LEFT && key != SPECIAL_SHIFT_RIGHT) {
		Game().ui.inventory->SpecialKeyPressed(key);
		return;
	}

	if (key == SPECIAL_TOGGLE_CARTOON)
		Game().render.Cartoon = !Game().render.Cartoon;

	if (ScreenState::IsGameplayInteractionAllowed(Game())) {
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
		Game().player->stats.SetSprintRequested(true);
}

void specialKeyReleased(int key, int x, int y) {
	(void)x;
	(void)y;

	if (key == SPECIAL_SHIFT_LEFT || key == SPECIAL_SHIFT_RIGHT)
		Game().player->stats.SetSprintRequested(false);
}

void processMouse(int button, int state, int x, int y) {
	if (ScreenState::ShouldRouteMouseToMenu(Game())) {
		Game().ui.menu.MouseFunction(button, state, x, y);
		return;
	}

	if (ScreenState::ShouldRouteMouseToInventory(Game())) {
		Game().ui.inventory->MouseFunction(button, state, x, y);
		return;
	}

	if (state && !Game().ui.map.show && ScreenState::IsGameplayInteractionAllowed(Game())) {
		PlayerActionController::execute(MapMouseGameplayAction(button));
	}
}
void processMousePassiveMotion(int a, int b) {
	if (ScreenState::ShouldRouteMouseToMenu(Game())) {
		Game().ui.menu.MousePassiveMotion(a, b);
		return;
	}

	if (ScreenState::ShouldRouteMouseToInventory(Game())) {
		Game().ui.inventory->MouseMotion(a, b);
		lastMx = a; // no camera jump when the inventory closes
		lastMy = b;
		return;
	}

	if (Game().ui.map.show) {
		lastMx = a;
		lastMy = b;
		return;
	}

	PlayerActionController::applyCameraDelta(-MOUSE_LOOK_SENSITIVITY * static_cast<float>(lastMx - a),
											 -MOUSE_LOOK_SENSITIVITY * static_cast<float>(lastMy - b));

	lastMx = a;
	lastMy = b;
}

void processMouseActiveMotion(int a, int b) {
	if (ScreenState::ShouldRouteMouseToInventory(Game()))
		Game().ui.inventory->MouseMotion(a, b);
}

void processMouseEntry(int a) { (void)a; }
