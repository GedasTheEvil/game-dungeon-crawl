#include "menu.h"
#include <string>
#include <time.h>
#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include <stdlib.h>
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include <string.h>
#include <fstream>
namespace {
void getMenuMouseCoords(int mouseX, int mouseY, float& outMenuX, float& outMenuY) {
	outMenuX = 100 * (static_cast<float>(mouseX) / static_cast<float>(GAME_STATE.render.resX));
	outMenuY = 100 - 100 * (static_cast<float>(mouseY) / static_cast<float>(GAME_STATE.render.resY));
}

int getSaveSlotFromCoords(float mx, float my) {
	if (mx >= 6 && mx <= 43) {
		if (my >= 71 && my <= 84)
			return 0;
		if (my >= 51 && my <= 64)
			return 1;
		if (my >= 31 && my <= 44)
			return 2;
	} else if (mx >= 54 && mx <= 93) {
		if (my >= 71 && my <= 84)
			return 3;
		if (my >= 51 && my <= 64)
			return 4;
		if (my >= 31 && my <= 44)
			return 5;
	}

	return -1;
}

bool isSaveMenuExitFromCoords(float mx, float my) { return mx >= 6 && mx <= 43 && my >= 10 && my <= 23; }

void persistSaveNameList() {
	std::ofstream f("Saves/gamelist.dat");
	for (int a = 0; a < 6; a++)
		f << GAME_STATE.saveNames[a].name << "\n";
}

static std::string formatSaveLabel() {
	time_t t = time(nullptr);
	struct tm* lt = localtime(&t);
	char buf[25];
	std::snprintf(buf, sizeof(buf), "%02d_%02d-%02d_%02d:%02d", GAME_STATE.curMap, lt->tm_mon + 1, lt->tm_mday,
				  lt->tm_hour, lt->tm_min);
	return std::string(buf);
}

class SaveSlotService {
  public:
	static std::string slotFilename(int slot) { return "Saves/save" + std::to_string(slot) + ".sav"; }

	static bool fileExists(int slot) {
		std::ifstream f(slotFilename(slot));
		return f.good();
	}

	static void saveToSlot(int slot) {
		GAME_STATE.Save(slotFilename(slot).c_str());

		const std::string label = formatSaveLabel();
		strncpy(GAME_STATE.saveNames[slot].name, label.c_str(), sizeof(GAME_STATE.saveNames[slot].name) - 1);
		GAME_STATE.saveNames[slot].name[sizeof(GAME_STATE.saveNames[slot].name) - 1] = '\0';
		persistSaveNameList();
	}

	static void loadFromSlot(int slot) {
		if (!fileExists(slot))
			return;
		GAME_STATE.LoadSave(slotFilename(slot).c_str());
	}
};
} // namespace

MainMenu::MainMenu() {
	show = true;
	inGame = false;
	credits = false;
	h1 = false;
	h2 = false;
	h3 = false;
	h4 = false;
	h5 = false;
	h6 = false;
	h7 = false;
	saveD = false;
	loadD = false;
	creditsTimer = std::make_unique<timer>(10000);
}

MainMenu::~MainMenu() {}

void MainMenu::Draw() {
	if (credits) {
		GAME_STATE.ui.wlc->DrawCredits();
		if (creditsTimer->TimePassed())
			credits = false;
		return;
	}

	if (saveD || loadD) {
		DrawSave();
		return;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 100, 0, 100, -21, 21);
	glMatrixMode(GL_MODELVIEW);

	glColor3f(1, 1, 1);

	GAME_STATE.textures.menu_bg.Bind();

	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1);
	glTexCoord2f(0, 0);
	glVertex3i(0, 0, -20);
	glTexCoord2f(0, 1);
	glVertex3i(0, 100, -20);
	glTexCoord2f(1, 1);
	glVertex3i(100, 100, -20);
	glTexCoord2f(1, 0);
	glVertex3i(100, 0, -20);
	glEnd();

	glColor3f(1, 1, 1);

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glEnable(GL_BLEND);

	if (inGame) {
		InGameDraw();
		return;
	}

	if (h1)
		glColor3f(0, 1, 0);
	else
		glColor3f(1, 1, 1);
	GAME_STATE.fonts.load_font.print(25, 72, "New Game");

	if (h2)
		glColor3f(0, 1, 0);
	else
		glColor3f(1, 1, 1);
	GAME_STATE.fonts.load_font.print(25, 54, "Load Game");

	if (h3)
		glColor3f(0, 1, 0);
	else
		glColor3f(1, 1, 1);
	GAME_STATE.fonts.load_font.print(25, 36, "Credits");

	if (h4)
		glColor3f(0, 1, 0);
	else
		glColor3f(1, 1, 1);
	GAME_STATE.fonts.load_font.print(25, 18, "Exit");

	glDisable(GL_BLEND);

	glFlush();

	glutSwapBuffers();
}

void MainMenu::InGameDraw() {

	if (h1)
		glColor3f(0, 1, 0);
	else
		glColor3f(1, 1, 1);
	GAME_STATE.fonts.load_font.print(25, 72, "Return to game");

	if (h2)
		glColor3f(0, 1, 0);
	else
		glColor3f(1, 1, 1);
	GAME_STATE.fonts.load_font.print(25, 54, "Save Game");

	if (h3)
		glColor3f(0, 1, 0);
	else
		glColor3f(1, 1, 1);
	GAME_STATE.fonts.load_font.print(25, 36, "Load Game");

	if (h4)
		glColor3f(0, 1, 0);
	else
		glColor3f(1, 1, 1);
	GAME_STATE.fonts.load_font.print(25, 18, "Exit to MainMenu");

	glDisable(GL_BLEND);

	glFlush();

	glutSwapBuffers();
}

void MainMenu::MouseFunction(int button, int state, int x, int y) {
	if (!state)
		return;

	if (credits)
		return;

	if (saveD || loadD) {
		SaveMouseFunction(button, state, x, y);
		return;
	}

	if (inGame) {
		InGameMouseFunction(button, state, x, y);
		return;
	}

	float mX, mY;
	getMenuMouseCoords(x, y, mX, mY);

	if (mX >= 23 && mX <= 75) {
		if (mY <= 82 && mY >= 71) // button1
		{
			show = false;
			// 		 GAME_STATE.LoadSave("Saves/new.sav");
			GAME_STATE.dungeon.Load("Levels/lvl1");
			inGame = true;
			GAME_STATE.IHaveWon = false;
			GAME_STATE.Player->Reanimate();
		}

		if (mY <= 63 && mY >= 53) // button2
		{
			loadD = true;
		}

		if (mY <= 46 && mY >= 35) // button3
		{
			credits = true;
		}

		if (mY <= 27 && mY >= 17) // button4
		{
			exit(666);
		}
	}
}

void MainMenu::InGameMouseFunction(int mouseButton, int buttonState, int mouseX, int mouseY) {
	(void)mouseButton;
	(void)buttonState;

	if (credits)
		return;

	float mX, mY;
	getMenuMouseCoords(mouseX, mouseY, mX, mY);

	if (mX >= 23 && mX <= 75) {
		if (mY <= 82 && mY >= 71) // button1
		{
			show = false;
		}

		if (mY <= 63 && mY >= 53) // button2
		{
			saveD = true;
		}

		if (mY <= 46 && mY >= 35) // button3
		{
			loadD = true;
		}

		if (mY <= 27 && mY >= 17) // button4
		{
			inGame = false;
		}
	}
}

void MainMenu::MousePassiveMotion(int x, int y) {
	if (credits)
		return;

	if (saveD || loadD) {
		MousePassiveMotionSave(x, y);
		return;
	}

	float mX, mY;
	getMenuMouseCoords(x, y, mX, mY);

	if (mX >= 23 && mX <= 75) {
		if (mY <= 82 && mY >= 71) // button1
		{
			NoHover();
			h1 = true;
		}

		if (mY <= 63 && mY >= 53) // button2
		{
			NoHover();
			h2 = true;
		}

		if (mY <= 46 && mY >= 35) // button3
		{
			NoHover();
			h3 = true;
		}

		if (mY <= 27 && mY >= 17) // button4
		{
			NoHover();
			h4 = true;
		}
	} else
		NoHover();
}

void MainMenu::NoHover() {
	h1 = false;
	h2 = false;
	h3 = false;
	h4 = false;
	h5 = false;
	h6 = false;
	h7 = false;
}

void MainMenu::DrawSave() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 100, 0, 100, -21, 21);
	glMatrixMode(GL_MODELVIEW);

	glColor3f(1, 1, 1);

	GAME_STATE.textures.menu_save_bg.Bind();

	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1);
	glTexCoord2f(0, 0);
	glVertex3i(0, 0, -20);
	glTexCoord2f(0, 1);
	glVertex3i(0, 100, -20);
	glTexCoord2f(1, 1);
	glVertex3i(100, 100, -20);
	glTexCoord2f(1, 0);
	glVertex3i(100, 0, -20);
	glEnd();

	glColor3f(1, 1, 1);

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glEnable(GL_BLEND);

	auto slotLabel = [&](int slot) -> const char* {
		if (loadD && !SaveSlotService::fileExists(slot))
			return "-- empty --";
		return GAME_STATE.saveNames[slot].name;
	};

	if (h1)
		glColor3f(0, 1, 0);
	else
		glColor3f(1, 1, 1);
	GAME_STATE.fonts.load_font.print(6, 72, slotLabel(0));

	if (h2)
		glColor3f(0, 1, 0);
	else
		glColor3f(1, 1, 1);
	GAME_STATE.fonts.load_font.print(6, 52, slotLabel(1));

	if (h3)
		glColor3f(0, 1, 0);
	else
		glColor3f(1, 1, 1);
	GAME_STATE.fonts.load_font.print(6, 33, slotLabel(2));

	if (h4)
		glColor3f(0, 1, 0);
	else
		glColor3f(1, 1, 1);
	GAME_STATE.fonts.load_font.print(56, 72, slotLabel(3));

	if (h5)
		glColor3f(0, 1, 0);
	else
		glColor3f(1, 1, 1);
	GAME_STATE.fonts.load_font.print(56, 52, slotLabel(4));

	if (h6)
		glColor3f(0, 1, 0);
	else
		glColor3f(1, 1, 1);
	GAME_STATE.fonts.load_font.print(56, 33, slotLabel(5));

	if (h7)
		glColor3f(0, 1, 0);
	else
		glColor3f(1, 1, 1);
	GAME_STATE.fonts.load_font.print(6, 10, "Exit");

	if (loadD) {
		DrawLoad();
		return;
	}

	GAME_STATE.fonts.load_font.print(15, 85, "Select slot to save ");

	glDisable(GL_BLEND);

	glFlush();

	glutSwapBuffers();
}

void MainMenu::DrawLoad() {
	GAME_STATE.fonts.load_font.print(15, 85, "Select slot to Load ");

	glDisable(GL_BLEND);

	glFlush();

	glutSwapBuffers();
}

void MainMenu::SaveMouseFunction(int button, int state, int x, int y) {
	if (loadD) {
		LoadMouseFunction(button, state, x, y);
		return;
	}

	float mx, my;
	getMenuMouseCoords(x, y, mx, my);

	int slot = getSaveSlotFromCoords(mx, my);
	if (slot != -1) {
		SaveSlotService::saveToSlot(slot);
		saveD = false;
		return;
	}

	if (isSaveMenuExitFromCoords(mx, my))
		saveD = false;
}

void MainMenu::LoadMouseFunction(int mouseButton, int buttonState, int mouseX, int mouseY) {
	(void)mouseButton;
	(void)buttonState;

	// GAME_STATE.LoadSave("Saves/dump.tmp");
	float mx, my;
	getMenuMouseCoords(mouseX, mouseY, mx, my);

	int slot = getSaveSlotFromCoords(mx, my);
	if (slot != -1) {
		SaveSlotService::loadFromSlot(slot);
		loadD = false;
		show = false;
		inGame = true;
		return;
	}

	if (isSaveMenuExitFromCoords(mx, my))
		loadD = false;
}

void MainMenu::MousePassiveMotionSave(int x, int y) {
	float mx, my;
	getMenuMouseCoords(x, y, mx, my);

	NoHover();

	int slot = getSaveSlotFromCoords(mx, my);
	switch (slot) {
	case 0:
		h1 = true;
		return;
	case 1:
		h2 = true;
		return;
	case 2:
		h3 = true;
		return;
	case 3:
		h4 = true;
		return;
	case 4:
		h5 = true;
		return;
	case 5:
		h6 = true;
		return;
	default:
		break;
	}

	if (isSaveMenuExitFromCoords(mx, my))
		h7 = true;
}

void MainMenu::ResetSubScreens() {
	credits = false;
	saveD = false;
	loadD = false;
}
