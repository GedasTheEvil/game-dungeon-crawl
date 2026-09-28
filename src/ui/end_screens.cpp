#include "end_screens.h"
#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"

EndScreens::EndScreens() {
	win.LoadPNG("textures/ui/win.png", TexFilter::Flat);
	lose.LoadPNG("textures/ui/dead.png", TexFilter::Flat);
	credits.LoadPNG("textures/ui/credits.png", TexFilter::Flat);
}

void EndScreens::DrawQuad(float sx, float sy) {
	glBegin(GL_QUADS);
	glNormal3f(0, 0, -1);
	glTexCoord2f(0, 0);
	glVertex3f(-sx / static_cast<float>(2.0), -sy / static_cast<float>(2.0), static_cast<float>(15));
	glTexCoord2f(1, 0);
	glVertex3f(sx / static_cast<float>(2.0), -sy / static_cast<float>(2.0), static_cast<float>(15));
	glTexCoord2f(1, 1);
	glVertex3f(sx / static_cast<float>(2.0), sy / static_cast<float>(2.0), static_cast<float>(15));
	glTexCoord2f(0, 1);
	glVertex3f(-sx / static_cast<float>(2.0), sy / static_cast<float>(2.0), static_cast<float>(15));
	glEnd();
}

void EndScreens::DrawWin() {
	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glEnable(GL_BLEND);

	glPushMatrix();

	glRotatef(-GAME_STATE.camera.rotN, 1, 0, 0);
	glRotatef(-GAME_STATE.camera.rotM, 0, 1, 0);

	glTranslatef(0, 20, 0);

	win.Bind();
	DrawQuad(60, 40);
	glPopMatrix();

	glDisable(GL_BLEND);
}

void EndScreens::DrawLose() {
	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glEnable(GL_BLEND);

	glPushMatrix();

	glRotatef(-GAME_STATE.camera.rotN, 1, 0, 0);
	glRotatef(-GAME_STATE.camera.rotM, 0, 1, 0);

	glTranslatef(0, 20, 0);

	lose.Bind();
	DrawQuad(60, 40);
	glPopMatrix();

	glDisable(GL_BLEND);
}

void EndScreens::DrawCredits() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 100, 0, 100, -21, 21);
	glMatrixMode(GL_MODELVIEW);

	glColor3f(1, 1, 1);

	credits.Bind();

	glPushMatrix();
	glTranslatef(45, 45, 0);
	DrawQuad(90, 90);
	glPopMatrix();

	glFlush();

	glutSwapBuffers();
}
