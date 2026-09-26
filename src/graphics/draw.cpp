#include "../input/input.h"
#include "../test/scenario.h"
#include <GL/gl.h>
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../ui/screen_state.h"
#include "hud.h"
#include "lighting.h"
#include "gl_includes.h"
#include <string>

int weaponRot = 0;

namespace {
constexpr float PLAYER_CLIMB_ROT = 180.f; // back to the camera
// Towards the back wall so the fists close round the rungs: the ladder's rungs are 2.35 in front of the wall
// (tools/blender/models/ladder.py), the fists ~1.7 in front of the model's centre (CLIMB_GRIP_Y in archeologist.py).
constexpr float PLAYER_CLIMB_DEPTH = -16.f;

void drawWeapon() { // floats in front of the chest
	glPushMatrix();
	if (GAME_STATE.Player->rotA > 0) {
		glTranslatef(GAME_STATE.Player->scale / 20, GAME_STATE.Player->scale / 4 * 3 + 0.27, 2);
		glRotatef(-45 - weaponRot, 0, 0, 1);
	} else {
		glTranslatef(-GAME_STATE.Player->scale / 20, GAME_STATE.Player->scale / 4 * 3 + 0.27, 2);
		glRotatef(45 + weaponRot, 0, 0, 1);
	}
	GAME_STATE.ui.invent->Equipped()->Draw();

	glPopMatrix();
}
// The status message, centred, one line per '\n', in a projection that keeps the glyphs square.
void drawStatus(const char* status) {
	constexpr float TOP_LINE_Y = 74.f;
	constexpr float LINE_H = 6.f;
	float width = 100.f * static_cast<float>(GAME_STATE.render.resX) / static_cast<float>(GAME_STATE.render.resY);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, 0, 100, -21, 21);
	glMatrixMode(GL_MODELVIEW);

	std::string lines = status;
	float y = TOP_LINE_Y;
	for (size_t start = 0; start < lines.size(); y -= LINE_H) {
		size_t end = lines.find('\n', start);
		if (end == std::string::npos)
			end = lines.size();
		std::string line = lines.substr(start, end - start);
		if (!line.empty())
			GAME_STATE.fonts.status.print((width - GAME_STATE.fonts.status.TextWidth(line.c_str())) / 2, y, "%s",
										  line.c_str());
		start = end + 1;
	}
}
} // namespace

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
		bool climbing = !GAME_STATE.Player->jump.jumping && GAME_STATE.dungeon.PlayerOnLadder();
		GAME_STATE.Player->depthOffset = climbing ? PLAYER_CLIMB_DEPTH : 0.f;
		if (GAME_STATE.Player->jump.jumping)
			GAME_STATE.Player->setModelState(ModelState::Jump);
		else if (climbing) {
			GAME_STATE.Player->showClimb(GAME_STATE.dungeon.ClimbPhase());
			GAME_STATE.Player->rotA = PLAYER_CLIMB_ROT;
		} else if (GAME_STATE.timers.mdlChange->TimePassed())
			GAME_STATE.Player->setModelState(ModelState::Idle);

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

	Lighting::begin();
	if (GAME_STATE.Player->Alive())
		Lighting::add(0, 16, -22, Lighting::PLAYER, 0); // just in front of the player's chest

	GAME_STATE.textures.Dt[0].Bind();

	glPushMatrix();
	glTranslatef(-202, 0.0, -10);

	GAME_STATE.dungeon.Draw();

	glPopMatrix();

	GAME_STATE.Player->Draw();

	Lighting::setEmissive(true);
	if (GAME_STATE.IHaveWon)
		GAME_STATE.ui.wlc->DrawWin();
	Lighting::setEmissive(false);

	if (GAME_STATE.Player->Alive()) {
		if (!GAME_STATE.Player->climbing()) // both hands on the ladder: no weapon
			drawWeapon();
	} else {
		Lighting::setEmissive(true);
		GAME_STATE.ui.wlc->DrawLoose();
	}
	Lighting::end();

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
	Hud::drawKeys(GAME_STATE.dungeon.KeysHeld());

	glColor3f(1, 1, 1);

	if (!GAME_STATE.status_timer->TimePassed(true))
		drawStatus(GAME_STATE.status);

	glFlush();

	Scenario::onFrameRendered();
	glutSwapBuffers();
}
