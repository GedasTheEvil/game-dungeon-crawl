#include <stdio.h>
#include <GL/gl.h>
#include "../state/cashe.h"
#include "input.h"
#include "input_actions.h"
#include "../ui/screen_state.h"

extern Cashe c;

extern float rotW;
extern float rotM, rotN;

extern bool attacking;

unsigned char lastKey;

int lastMx = 0;
int lastMy = 0;

void Draw();

namespace {
void startJump() {
	if (c.jumping || c.falling || !c.Player->Alive() || c.IHaveWon)
		return;

	if (c.Player->Stamina() < JUMP_STAMINA_COST)
		return;

	c.Player->ConsumeStamina(JUMP_STAMINA_COST);

	float curX, curY;
	c.dungeon.getC(curX, curY);
	c.jump_start_y = curY;

	c.jump_dir_x = 0;
	if (lastKey == KEY_MOVE_LEFT || rotW < 0)
		c.jump_dir_x = -1;
	else if (lastKey == KEY_MOVE_RIGHT || rotW > 0)
		c.jump_dir_x = 1;

	c.jump_speed = JUMP_FORWARD_SPEED;
	c.jump_vel = JUMP_INITIAL_VELOCITY;
	c.jumping = true;
	c.jump_up_timer->Reset();
	c.jump_s.Play();
}

class PlayerActionController {
  public:
	static void execute(GameplayAction action) {
		float moveMultiplier = c.Stats->SprintMoveMultiplier();
		switch (action) {
		case GameplayAction::MoveLeft:
			c.dungeon.Move(-PLAYER_MOVE_STEP * moveMultiplier, 0);
			rotW = -110;
			c.Player->changeMDL(1);
			break;
		case GameplayAction::MoveRight:
			c.dungeon.Move(PLAYER_MOVE_STEP * moveMultiplier, 0);
			rotW = 70;
			c.Player->changeMDL(1);
			break;
		case GameplayAction::MoveDown:
			c.dungeon.Move(0, -PLAYER_MOVE_STEP * moveMultiplier);
			break;
		case GameplayAction::MoveUp:
			c.dungeon.Move(0, PLAYER_FORWARD_MOVE_STEP * moveMultiplier);
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
		if (!c.Player->Att_timer->TimePassed())
			return;

		c.dungeon.GetAttack(c.Stats->Damage(), c.invent->Equipped()->range);
		c.Player->att_s.Play();
		attacking = true;
	}

	static void interact() {
		c.dungeon.GetPickUp();
		c.dungeon.GetRiddle();
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

	if (ScreenState::ShouldRouteKeyboardToRiddle(c)) {
		c.rid->KeyboardF(key, x, y);
		return;
	}

	if (key == KEY_ESCAPE) // esc
	{
		if (c.Stats->show || c.invent->show) {
			c.Stats->show = false;
			c.invent->show = false;
		}

		c.menu.ResetSubScreens();
		c.menu.show = !c.menu.show;
		return;
	}

	if (ScreenState::ShouldBlockKeyboardGameplay(c))
		return; // jei rodomas meniu, tai reaguojam tik i [esc]

	if (ScreenState::IsGameplayInteractionAllowed(c)) {
		PlayerActionController::execute(MapKeyboardGameplayAction(key));
	} // eo Alive

	if (key == KEY_INVENTORY) {
		c.invent->show = !c.invent->show;
		if (c.invent->show)
			c.Stats->show = false;
	}

	if (key == KEY_STATS) {
		c.Stats->show = !c.Stats->show;
		if (c.Stats->show)
			c.invent->show = false;
	}

	lastKey = key;
}

void specialKeyPressed(int key, int x, int y) {
	//      printf("Special key %d pressed\n",key);
	(void)x;
	(void)y;

	if (ScreenState::ShouldBlockKeyboardGameplay(c))
		return;

	if (key == SPECIAL_TOGGLE_CARTOON)
		c.Cartoon = !c.Cartoon;

	if (key == SPECIAL_TOGGLE_ORIGINAL_MODEL)
		c.Orig_model = !c.Orig_model;

	if (ScreenState::IsGameplayInteractionAllowed(c)) {
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
		c.Stats->SetSprintRequested(true);
}

void specialKeyReleased(int key, int x, int y) {
	(void)x;
	(void)y;

	if (key == SPECIAL_SHIFT_LEFT || key == SPECIAL_SHIFT_RIGHT)
		c.Stats->SetSprintRequested(false);
}

void processMouse(int button, int state, int x, int y) {
	if (ScreenState::ShouldRouteMouseToMenu(c)) {
		c.menu.MouseFunction(button, state, x, y);
		return;
	}

	if (ScreenState::ShouldRouteMouseToInventory(c)) {
		c.invent->MouseFunction(button, state, x, y);
		return;
	}

	if (state && ScreenState::IsGameplayInteractionAllowed(c)) {
		PlayerActionController::execute(MapMouseGameplayAction(button));
	}
}
void processMousePassiveMotion(int a, int b) {
	if (ScreenState::ShouldRouteMouseToMenu(c)) {
		c.menu.MousePassiveMotion(a, b);
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
