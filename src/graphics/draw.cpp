#include "../input/input.h"
#include <GL/gl.h>
#include "../state/cashe.h"
#include "../core/service_locator.h"
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

bool attacking = 0;

extern float resX, resY;

void drawLoad(float xxx, char text[]);

void Draw() {

	if (!GAME_STATE.Cache_loaded) {
		GAME_STATE.Load();
		return;
	}

	switch (ScreenState::GetDrawScreen(GAME_STATE)) {
	case ScreenState::DrawScreen::Menu:
		GAME_STATE.menu.Draw();
		return;
	case ScreenState::DrawScreen::Inventory:
		GAME_STATE.invent->Draw();
		return;
	case ScreenState::DrawScreen::Stats:
		GAME_STATE.Stats->Draw();
		return;
	case ScreenState::DrawScreen::Riddle:
		GAME_STATE.rid->Draw();
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

	GAME_STATE.Dt[0].Bind();

	glPushMatrix();				  // for perspective
	glTranslatef(-202, 0.0, -10); // for perspective, tarkim bus tiek :)

	GAME_STATE.dungeon.Update();
	GAME_STATE.dungeon.Draw();

	glPopMatrix(); // for perspective

	GAME_STATE.Player->rotA = rotW;

	GAME_STATE.Player->Draw();

	if (GAME_STATE.IHaveWon)
		GAME_STATE.wlc->DrawWin();

	if (GAME_STATE.Player->Alive()) {
		if (GAME_STATE.mdlChange->TimePassed())
			GAME_STATE.Player->changeMDL(0);

		glPushMatrix(); // weapon

		if (GAME_STATE.Player->rotA > 0) {
			glTranslatef(GAME_STATE.Player->scale / 20, GAME_STATE.Player->scale / 4 * 3 + 0.27, 2);
			glRotatef(-45 - weaponRot, 0, 0, 1);

		} else {
			glTranslatef(-GAME_STATE.Player->scale / 20, GAME_STATE.Player->scale / 4 * 3 + 0.27, 2);
			glRotatef(45 + weaponRot, 0, 0, 1);
		}
		GAME_STATE.invent->Equipped()->Draw();
		GAME_STATE.invent->Equipped()->rotA++; // just for the heck of it

		if (attacking) {
			if (GAME_STATE.Player->Att_timer->TimePassed() || weaponRot <= -40) {
				weaponRot = 70;
				attacking = 0;
			} else
				weaponRot -= 4;
		} else if (GAME_STATE.AttTimer->TimePassed())
			weaponRot = 0;

		glPopMatrix();

	} // end of alive
	else
		GAME_STATE.wlc->DrawLoose();

	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 100, 0, 100, -21, 21);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glEnable(GL_BLEND);
	GAME_STATE.Stats->UpdateStamina();

	GAME_STATE.nullTex.Bind();
	Hud::drawPlayerBars(GAME_STATE.Player->healthRatio(), GAME_STATE.Player->staminaRatio());

	glColor3f(1, 1, 1);

	if (!GAME_STATE.status_timer->TimePassed(true))
		GAME_STATE.load_font.print(25, 72, GAME_STATE.status);

	glFlush();

	glutSwapBuffers();
}
