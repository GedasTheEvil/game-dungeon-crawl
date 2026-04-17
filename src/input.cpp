#include <stdio.h>
#include <GL/gl.h>
#include "cashe.h"
#include "input.h"

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
} // namespace

void Idle() { Draw(); }

void keyPressed(unsigned char key, int x, int y) {
	//      printf("You pressed %d\n",key);

	if (c.rid->show) {
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

	if (c.menu.show)
		return; // jei rodomas meniu, tai reaguojam tik i [esc]

	if (c.Player->Alive() && !c.IHaveWon) {

		if (key == KEY_MOVE_LEFT) // a
		{
			c.dungeon.Move(-PLAYER_MOVE_STEP, 0);
			RotW = -110;
			c.Player->changeMDL(1);
		}
		if (key == KEY_MOVE_RIGHT) // d
		{
			c.dungeon.Move(PLAYER_MOVE_STEP, 0);
			RotW = 70;
			c.Player->changeMDL(1);
		}

		if (key == KEY_MOVE_DOWN) // s
			c.dungeon.Move(0, -PLAYER_MOVE_STEP);
		if (key == KEY_MOVE_UP) // w
			c.dungeon.Move(0, PLAYER_MOVE_STEP);

		if (key == KEY_SPACE) // spacebar, jump
		{
			StartJump();
		}

		if (key == KEY_ENTER && c.Player->Att_timer->TimePassed()) // enter, attack
		{
			c.dungeon.GetAttack(c.Stats->Damage(), c.invent->Equipped()->range);
			c.Player->att_s.Play();
			Attacking = 1;
		}

	} // eo Alive

	if (key == KEY_INVENTORY)
		c.invent->show = !c.invent->show;

	if (key == KEY_STATS)
		c.Stats->show = !c.Stats->show;

	last_key = key;
}

void specialKeyPressed(int key, int x, int y) {
	//      printf("Special key %d pressed\n",key);

	if (key == SPECIAL_TOGGLE_CARTOON)
		c.Cartoon = !c.Cartoon;

	if (key == SPECIAL_TOGGLE_ORIGINAL_MODEL)
		c.Orig_model = !c.Orig_model;

	if (key == SPECIAL_CAMERA_LEFT) {
		rotM -= CAMERA_ROTATE_STEP;
		if (rotM < -CAMERA_ROTATE_LIMIT_X)
			rotM = -CAMERA_ROTATE_LIMIT_X;
	}
	if (key == SPECIAL_CAMERA_RIGHT) {
		rotM += CAMERA_ROTATE_STEP;
		if (rotM > CAMERA_ROTATE_LIMIT_X)
			rotM = CAMERA_ROTATE_LIMIT_X;
	}
	if (key == SPECIAL_CAMERA_UP) {
		rotN += CAMERA_ROTATE_STEP;
		if (rotN > CAMERA_ROTATE_LIMIT_Y)
			rotN = CAMERA_ROTATE_LIMIT_Y;
	}
	if (key == SPECIAL_CAMERA_DOWN) {
		rotN -= CAMERA_ROTATE_STEP;
		if (rotN < -CAMERA_ROTATE_LIMIT_Y)
			rotN = -CAMERA_ROTATE_LIMIT_Y;
	}

	if (key == SPECIAL_INTERACT) {
		c.dungeon.GetPickUp();
		c.dungeon.GetRiddle();
	}
}

void processMouse(int button, int state, int x, int y) {
	if (c.menu.show) {
		c.menu.MouseFunction(button, state, x, y);
		return;
	}

	if (c.invent->show) {
		c.invent->MouseFunction(button, state, x, y);
		return;
	}

	if (state && c.Player->Alive() && !c.IHaveWon) {
		if (button == MOUSE_LEFT_BUTTON && c.Player->Att_timer->TimePassed()) {
			c.dungeon.GetAttack(c.Stats->Damage(), c.invent->Equipped()->range);
			c.Player->att_s.Play();
			Attacking = 1;
		}

		if (button == MOUSE_MIDDLE_BUTTON) {
			c.dungeon.GetPickUp();
			c.dungeon.GetRiddle();
		}

		if (button == MOUSE_RIGHT_BUTTON) {
			StartJump();
		}
	}
}

void processMouseActiveMotion(int a, int b) {}

void processMousePassiveMotion(int a, int b) {
	if (c.menu.show) {
		c.menu.MousePassiveMotion(a, b);
		return;
	}

	rotM += -MOUSE_LOOK_SENSITIVITY * (lastMx - a);
	rotN += -MOUSE_LOOK_SENSITIVITY * (lastMy - b);

	if (rotM > CAMERA_ROTATE_LIMIT_X)
		rotM = CAMERA_ROTATE_LIMIT_X;

	if (rotM < -CAMERA_ROTATE_LIMIT_X)
		rotM = -CAMERA_ROTATE_LIMIT_X;

	if (rotN > CAMERA_ROTATE_LIMIT_Y)
		rotN = CAMERA_ROTATE_LIMIT_Y;

	if (rotN < -CAMERA_ROTATE_LIMIT_Y)
		rotN = -CAMERA_ROTATE_LIMIT_Y;

	lastMx = a;
	lastMy = b;
}

void processMouseEntry(int a) {}
