#include "win_entry.h"
#include "ui_draw.h"
#include "../input/input.h"
#include "../state/game_state.h"
#include "../world/leaderboard.h"
#include <GL/gl.h>
#include <cmath>
#include <ctime>

#ifndef GAME_VERSION // the makefile passes the git hash
#define GAME_VERSION "unknown"
#endif

namespace {
using namespace ui;

// ---- layout ----------------------------------------------------------------

constexpr Rect PANEL = {36, 3, 88, 27}; // under the win picture's text; the HUD is hidden meanwhile
constexpr Rect FIELD = {PANEL.x + 8, PANEL.y + 7.5f, PANEL.w - 16, 7.f};

std::string today() {
	const time_t t = time(nullptr);
	char date[16];
	std::strftime(date, sizeof(date), "%Y-%m-%d", std::localtime(&t));
	return date;
}
} // namespace

void WinEntry::Follow() {
	if (!Game().dungeon.Won()) {
		state = State::Hidden;
		return;
	}
	if (state != State::Hidden)
		return;
	if (!Game().player->stats.run.campaign) {
		state = State::Done;
		return;
	}
	state = State::Typing;
	name = Game().leaderboard.LastName();
}

void WinEntry::KeyPressed(unsigned char key) {
	if (key == KEY_ESCAPE)
		state = State::Done;
	else if (key == KEY_ENTER)
		record();
	else if (key == KEY_BACKSPACE || key == KEY_DELETE) {
		if (!name.empty())
			name.pop_back();
	} else if (key >= 32 && key <= 126 && name.size() < MAX_NAME) // the papyrus font's printable ASCII
		name += static_cast<char>(key);
}

void WinEntry::record() {
	const size_t start = name.find_first_not_of(' ');
	if (start == std::string::npos)
		return; // a name is needed; Esc skips
	const std::string trimmed = name.substr(start, name.find_last_not_of(' ') - start + 1);
	Leaderboard& board = Game().leaderboard;
	const bool written = board.Add(recordOf(Game().player->stats, trimmed, today(), GAME_VERSION));
	state = State::Done;
	MainMenu& menu = Game().ui.menu;
	menu.inGame = false; // the run is over
	menu.OpenRecords(written ? static_cast<int>(board.Records().size()) - 1 : RecordsView::NONE);
	Game().ui.screen = Screen::Menu;
}

void WinEntry::Draw(int resX, int resY) {
	if (state != State::Typing)
		return;
	if (!fontsLoaded) {
		fontsLoaded = true;
		loadScreenFonts(title, heading, body, small, 7.f);
	}
	const Rect area = visibleArea(CANVAS_W, CANVAS_H, resX, resY);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(area.x, area.x + area.w, area.y, area.y + area.h, -200, 200);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	beginShapes();
	fillRect({PANEL.x + 0.8f, PANEL.y - 1.f, PANEL.w, PANEL.h}, BLACK, BLACK, 0.5f); // drop shadow
	panel(PANEL, 0.94f);

	const RunStats& run = Game().player->stats.run;
	const std::string summary = "Time " + formatPlayTime(run.PlayMs()) + "    Slain " +
								std::to_string(run.SlainTotal()) + "    Potions " + std::to_string(run.DrunkTotal()) +
								"    Level " + std::to_string(Game().player->stats.CurrentLevel());
	beginText();
	textCentered(body, PANEL.cx(), PANEL.y + PANEL.h - 7.f, summary.c_str(), GOLD);
	text(small, FIELD.x, FIELD.y + FIELD.h + 1.5f, "Your name for the Hall of Records", LABEL);

	beginShapes();
	fillRect(FIELD, WELL, WELL, 1.f);
	strokeRect(FIELD, GOLD, 1.f, 1.5f);
	const float nameW = heading.TextWidth(name.c_str());
	const float caretX = FIELD.x + 2.f + nameW + 0.4f;
	if (std::fmod(static_cast<float>(GameClock::now()) / 500.f, 2.f) < 1.f) // a blinking caret
		fillRect({caretX, FIELD.y + 1.3f, 0.5f, FIELD.h - 2.6f}, GOLD_BRIGHT, GOLD_BRIGHT, 1.f);
	beginText();
	textMiddle(heading, FIELD.x + 2.f, FIELD.cy(), name.c_str(), GOLD_BRIGHT);
	textCentered(small, PANEL.cx(), PANEL.y + 2.8f, "Enter: record    Esc: skip", LABEL_DIM);

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glColor3f(1, 1, 1);
}
