#include <stdio.h>
#include <GL/gl.h>
#include "../state/cashe.h"
#include "input.h"
#include "input_actions.h"
#include "../ui/screen_state.h"

extern Cashe c;

extern float RotW;
extern float rotM, rotN;

extern bool Attacking;

unsigned char last_key;

int lastMx = 0;
int lastMy = 0;

void Draw();

namespace {
void StartJump() {
	if (c.jumping || c.falling || !c.Player->Alive() || c.IHaveWon)
		return;

	float curX, curY;
	c.dungeon.getC(curX, curY);
	c.jump_start_y = curY;

	c.jump_dir_x = 0;
	if (last_key == KEY_MOVE_LEFT)
		c.jump_dir_x = -1;
	else if (last_key == KEY_MOVE_RIGHT)
		c.jump_dir_x = 1;
	else if (RotW > 0)
		c.jump_dir_x = 1;
	else if (RotW < 0)
		c.jump_dir_x = -1;

	c.jump_speed = JUMP_FORWARD_SPEED;
	c.jump_vel = JUMP_INITIAL_VELOCITY;
	c.jumping = 1;
	c.jump_up_timer->Reset();
	c.jump_s.Play();
}

class PlayerActionController {
  public:
	static void Execute(GameplayAction action) {
		switch (action) {
		case GameplayAction::MoveLeft:
			c.dungeon.Move(-PLAYER_MOVE_STEP, 0);
			RotW = -110;
			c.Player->changeMDL(1);
			break;
		case GameplayAction::MoveRight:
			c.dungeon.Move(PLAYER_MOVE_STEP, 0);
			RotW = 70;
			c.Player->changeMDL(1);
			break;
		case GameplayAction::MoveDown:
			c.dungeon.Move(0, -PLAYER_MOVE_STEP);
			break;
		case GameplayAction::MoveUp:
			c.dungeon.Move(0, PLAYER_FORWARD_MOVE_STEP);
			break;
		case GameplayAction::Jump:
			StartJump();
			break;
		case GameplayAction::Attack:
			TryAttack();
			break;
		case GameplayAction::Interact:
			Interact();
			break;
		case GameplayAction::None:
			break;
		}
	}

	static void ApplyCameraDelta(float deltaX, float deltaY) {
		rotM += deltaX;
		rotN += deltaY;
		ClampCamera();
	}

  private:
	static void TryAttack() {
		if (!c.Player->Att_timer->TimePassed())
			return;

		c.dungeon.GetAttack(c.Stats->Damage(), c.invent->Equipped()->range);
		c.Player->att_s.Play();
		Attacking = 1;
	}

	static void Interact() {
		c.dungeon.GetPickUp();
		c.dungeon.GetRiddle();
	}

	static void ClampCamera() {
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
			c.Stats->show = 0;
			c.invent->show = 0;
		}

		c.menu.show = !c.menu.show;
		return;
	}

	if (ScreenState::ShouldBlockKeyboardGameplay(c))
		return; // jei rodomas meniu, tai reaguojam tik i [esc]

	if (ScreenState::IsGameplayInteractionAllowed(c)) {
		PlayerActionController::Execute(MapKeyboardGameplayAction(key));
	} // eo Alive

	if (key == KEY_INVENTORY) {
		c.invent->show = !c.invent->show;
		if (c.invent->show)
			c.Stats->show = 0;
	}

	if (key == KEY_STATS) {
		c.Stats->show = !c.Stats->show;
		if (c.Stats->show)
			c.invent->show = 0;
	}

	last_key = key;
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
		PlayerActionController::Execute(MapSpecialGameplayAction(key));
	}

	if (key == SPECIAL_CAMERA_LEFT) {
		PlayerActionController::ApplyCameraDelta(-CAMERA_ROTATE_STEP, 0);
	}
	if (key == SPECIAL_CAMERA_RIGHT) {
		PlayerActionController::ApplyCameraDelta(CAMERA_ROTATE_STEP, 0);
	}
	if (key == SPECIAL_CAMERA_UP) {
		PlayerActionController::ApplyCameraDelta(0, CAMERA_ROTATE_STEP);
	}
	if (key == SPECIAL_CAMERA_DOWN) {
		PlayerActionController::ApplyCameraDelta(0, -CAMERA_ROTATE_STEP);
	}

	if (key == SPECIAL_INTERACT) {
		PlayerActionController::Execute(GameplayAction::Interact);
	}
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
		PlayerActionController::Execute(MapMouseGameplayAction(button));
	}
}
void processMousePassiveMotion(int a, int b) {
	if (ScreenState::ShouldRouteMouseToMenu(c)) {
		c.menu.MousePassiveMotion(a, b);
		return;
	}

	PlayerActionController::ApplyCameraDelta(-MOUSE_LOOK_SENSITIVITY * (lastMx - a),
											 -MOUSE_LOOK_SENSITIVITY * (lastMy - b));

	lastMx = a;
	lastMy = b;
}

void processMouseActiveMotion(int a, int b) {
	(void)a;
	(void)b;
}

void processMouseEntry(int a) { (void)a; }
