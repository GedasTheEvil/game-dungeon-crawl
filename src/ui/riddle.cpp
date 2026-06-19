#include "riddle.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../core/logger.h"
#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include <cmath>
#include <ctime>

#ifdef WIN32

#include <cstdlib>

inline int random() { return rand(); }

#endif

Riddle::Riddle() {
	RiddleCount = 5;
	rid = new riddle[RiddleCount];
	selected = 0;

	show = false;

	rid[0].question_l1 = "How many hounds guard this gate?";
	rid[0].question_l2 = "";
	rid[0].question_l3 = "";
	rid[0].question_l4 = "";
	rid[0].answer = "2";

	rid[1].question_l1 = "What is the answer to the";
	rid[1].question_l2 = "Ultimate question in the universe?";
	rid[1].question_l3 = "";
	rid[1].question_l4 = "";
	rid[1].answer = "42";

	rid[2].question_l1 = "God lived as a _____ dog";
	rid[2].question_l2 = "";
	rid[2].question_l3 = "";
	rid[2].question_l4 = "";
	rid[2].answer = "devil";

	rid[3].question_l1 = "Maps, ___, and spam.";
	rid[3].question_l2 = "";
	rid[3].question_l3 = "";
	rid[3].question_l4 = "";
	rid[3].answer = "DNA";

	rid[4].question_l1 = "Devil never ____ lived.";
	rid[4].question_l2 = "";
	rid[4].question_l3 = "";
	rid[4].question_l4 = "";
	rid[4].answer = "even";

	YourAnswer[0] = 0;
	ans_l = 0;
}

Riddle::~Riddle() {
	delete[] rid;
	LOG_DEBUGF("ui", "Deleting Riddle %p", static_cast<void*>(this));
}

void Riddle::GetRiddle() {
	srand(static_cast<unsigned>(time(nullptr)));

	selected = static_cast<int>(random() % RiddleCount);
	YourAnswer[0] = '_';
	YourAnswer[1] = 0;
}

void Riddle::Draw() {

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear The Screen And The Depth Buffer
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);	// Select The Projection Matrix
	glLoadIdentity();				// Reset The Projection Matrix
	glOrtho(0, 50, 0, 50, -20, 20); // Set Up An Ortho Screen
	glMatrixMode(GL_MODELVIEW);		// Select The Modelview Matrix

	GAME_STATE.textures.riddle_bg.Bind();
	glColor3f(1, 1, 1);

	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1);
	glTexCoord2f(0, 0);
	glVertex3i(0, 0, -19);
	glTexCoord2f(0, 1);
	glVertex3i(0, 50, -19);
	glTexCoord2f(1, 1);
	glVertex3i(50, 50, -19);
	glTexCoord2f(1, 0);
	glVertex3i(50, 0, -19);
	glEnd();

	// text goes now
	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR /*GL_ONE_MINUS_SRC_ALPHA*/);
	glEnable(GL_BLEND);

	glColor3f(1, 1, 1);
	GAME_STATE.fonts.font.print(static_cast<int>(5), static_cast<int>(12.5), rid[selected].question_l1);
	GAME_STATE.fonts.font.print(static_cast<int>(5), static_cast<int>(10), rid[selected].question_l2);
	GAME_STATE.fonts.font.print(static_cast<int>(5), static_cast<int>(7.5), rid[selected].question_l3);
	glColor3f(0, 1, 0);
	GAME_STATE.fonts.font.print(5, 5, YourAnswer);

	glDisable(GL_BLEND);

	glFlush();

	glutSwapBuffers();
}

void Riddle::KeyboardF(unsigned char key, int mouseX, int mouseY) {
	(void)mouseX;
	(void)mouseY;

	if (key != 8) // backspace,enter
	{
		if (key == 13) // enter
		{
			if (CheckAnswer()) {
				show = false;
				sprintf(GAME_STATE.status, "Riddle answered, got 500 XP \n");
				GAME_STATE.status_timer->Reset();
				GAME_STATE.ui.Stats->GetXP(500);
			} else {
				YourAnswer[0] = 'W';
				YourAnswer[1] = 'r';
				YourAnswer[2] = 'o';
				YourAnswer[3] = 'n';
				YourAnswer[4] = 'g';
				ans_l = 4;
			}
		}

		YourAnswer[ans_l] = static_cast<char>(key);
		if (ans_l < 23)
			ans_l++;
		YourAnswer[ans_l] = 0;
	}

	else if (ans_l > 0) {
		ans_l--;
		YourAnswer[ans_l] = 0;
	}
}

bool Riddle::CheckAnswer() {
	for (int i = 0; i < ans_l; i++) {
		if (!rid[selected].answer[i])
			return true;
		if (rid[selected].answer[i] != YourAnswer[i])
			return false;
	}

	return true; // nevykdomas
}
