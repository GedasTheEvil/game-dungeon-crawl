#include "../input/input.h"
#include <GL/gl.h>
#include "../state/cashe.h"
#include "../ui/screen_state.h"
#include "hud.h"
#ifndef WIN32
#include <GL/glut.h>
#endif
#ifdef WIN32
#include <GL/freeglut.h>
#endif

float rotW = -110;
float rotM = 0;
float rotN = 0;

int weaponRot = 0;

Cashe c;

bool attacking = 0;

extern float resX, resY;

void drawLoad(float xxx, char text[]);

void Draw() {

	if (!c.Cache_loaded) {
		c.Load();
		return;
	}

	switch (ScreenState::GetDrawScreen(c)) {
	case ScreenState::DrawScreen::Menu:
		c.menu.Draw();
		return;
	case ScreenState::DrawScreen::Inventory:
		c.invent->Draw();
		return;
	case ScreenState::DrawScreen::Stats:
		c.Stats->Draw();
		return;
	case ScreenState::DrawScreen::Riddle:
		c.rid->Draw();
		return;
	case ScreenState::DrawScreen::Gameplay:
		break;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear The Screen And The Depth Buffer
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION); // Select The Projection Matrix
	glLoadIdentity();			 // Reset The Projection Matrix
	gluPerspective(45.0f, resX / resY, 10.0f, 300.0f);
	glMatrixMode(GL_MODELVIEW); // Select The Modelview Matrix

	glTranslatef(0, -20, -70); // for perspective, bet skaiciai is lempos

	glRotatef(rotM, 0, 1, 0);
	glRotatef(rotN, 1, 0, 0);

	c.Dt[0].Bind();

	glPushMatrix();				  // for perspective
	glTranslatef(-202, 0.0, -10); // for perspective, tarkim bus tiek :)

	c.dungeon.Update();
	c.dungeon.Draw();

	glPopMatrix(); // for perspective

	c.Player->rotA = rotW;

	c.Player->Draw();

	if (c.IHaveWon)
		c.wlc->DrawWin();

	if (c.Player->Alive()) {
		if (c.mdlChange->TimePassed())
			c.Player->changeMDL(0);

		glPushMatrix(); // weapon

		if (c.Player->rotA > 0) {
			glTranslatef(c.Player->scale / 20, c.Player->scale / 4 * 3 + 0.27, 2);
			glRotatef(-45 - weaponRot, 0, 0, 1);

		} else {
			glTranslatef(-c.Player->scale / 20, c.Player->scale / 4 * 3 + 0.27, 2);
			glRotatef(45 + weaponRot, 0, 0, 1);
		}
		c.invent->Equipped()->Draw();
		c.invent->Equipped()->rotA++; // just for the heck of it

		if (attacking) {
			if (c.Player->Att_timer->TimePassed() || weaponRot <= -40) {
				weaponRot = 70;
				attacking = 0;
			} else
				weaponRot -= 4;
		} else if (c.AttTimer->TimePassed())
			weaponRot = 0;

		glPopMatrix();

	} // end of alive
	else
		c.wlc->DrawLoose();

	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 100, 0, 100, -21, 21);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glEnable(GL_BLEND);
	c.Stats->UpdateStamina();

	c.nullTex.Bind();
	Hud::drawPlayerBars(c.Player->healthRatio(), c.Player->staminaRatio());

	glColor3f(1, 1, 1);

	if (!c.status_timer->TimePassed(true))
		c.load_font.print(25, 72, c.status);

	glFlush();

	glutSwapBuffers();
}
