#include "../input/input.h"
#include <GL/gl.h>
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../ui/screen_state.h"
#include "hud.h"
#include "gl_includes.h"

int weaponRot = 0;

void Update() {
	if (!GAME_STATE.Cache_loaded) {
		GAME_STATE.Load();
		glutPostRedisplay();
		return;
	}

	if (ScreenState::GetDrawScreen(GAME_STATE) != ScreenState::DrawScreen::Gameplay) {
		glutPostRedisplay();
		return;
	}

	GAME_STATE.dungeon.Update();
	GAME_STATE.Player->rotA = GAME_STATE.camera.rotW;

	if (GAME_STATE.Player->Alive()) {
		if (GAME_STATE.timers.mdlChange->TimePassed())
			GAME_STATE.Player->changeMDL(1);

		GAME_STATE.ui.invent->Equipped()->rotA++;

		if (GAME_STATE.Player->attacking) {
			if (GAME_STATE.Player->Att_timer->TimePassed() || weaponRot <= -40) {
				weaponRot = 70;
				GAME_STATE.Player->attacking = false;
			} else
				weaponRot -= 4;
		} else if (GAME_STATE.timers.AttTimer->TimePassed())
			weaponRot = 0;
	}

	GAME_STATE.ui.Stats->UpdateStamina();
	glutPostRedisplay();
}

void Draw() {
	if (!GAME_STATE.Cache_loaded)
		return;

	switch (ScreenState::GetDrawScreen(GAME_STATE)) {
	case ScreenState::DrawScreen::Menu:
		GAME_STATE.ui.menu.Draw();
		return;
	case ScreenState::DrawScreen::Inventory:
		GAME_STATE.ui.invent->Draw();
		return;
	case ScreenState::DrawScreen::Stats:
		GAME_STATE.ui.Stats->Draw();
		return;
	case ScreenState::DrawScreen::Riddle:
		GAME_STATE.ui.rid->Draw();
		return;
	case ScreenState::DrawScreen::Gameplay:
		break;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, static_cast<float>(GAME_STATE.render.resX) / static_cast<float>(GAME_STATE.render.resY),
				   10.0f, 300.0f);
	glMatrixMode(GL_MODELVIEW);

	glTranslatef(0, -20, -70);

	glRotatef(GAME_STATE.camera.rotM, 0, 1, 0);
	glRotatef(GAME_STATE.camera.rotN, 1, 0, 0);

	GAME_STATE.textures.Dt[0].Bind();

	glPushMatrix();
	glTranslatef(-202, 0.0, -10);

	GAME_STATE.dungeon.Draw();

	glPopMatrix();

	GAME_STATE.Player->Draw();

	if (GAME_STATE.IHaveWon)
		GAME_STATE.ui.wlc->DrawWin();

	if (GAME_STATE.Player->Alive()) {
		glPushMatrix(); // weapon

		if (GAME_STATE.Player->rotA > 0) {
			glTranslatef(GAME_STATE.Player->scale / 20, GAME_STATE.Player->scale / 4 * 3 + 0.27, 2);
			glRotatef(-45 - weaponRot, 0, 0, 1);
		} else {
			glTranslatef(-GAME_STATE.Player->scale / 20, GAME_STATE.Player->scale / 4 * 3 + 0.27, 2);
			glRotatef(45 + weaponRot, 0, 0, 1);
		}
		GAME_STATE.ui.invent->Equipped()->Draw();

		glPopMatrix();
	} else
		GAME_STATE.ui.wlc->DrawLoose();

	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 100, 0, 100, -21, 21);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glEnable(GL_BLEND);

	GAME_STATE.textures.nullTex.Bind();
	Hud::drawPlayerBars(GAME_STATE.Player->healthRatio(), GAME_STATE.Player->staminaRatio());

	glColor3f(1, 1, 1);

	if (!GAME_STATE.status_timer->TimePassed(true))
		GAME_STATE.fonts.load_font.print(25, 72, GAME_STATE.status);

	glFlush();

	glutSwapBuffers();
}
