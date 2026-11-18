#include "ending_screen.h"
#include "../render/ui_draw.h"
#include "../level/input.h"
#include "../state/game_state.h"
#include "../state/ui_context.h"
#include <array>
#include "../render/gl_includes.h"

namespace {
using namespace ui;

// ---- layout ----------------------------------------------------------------

constexpr Rect PANEL = {28, 26, 104, 44};
constexpr float TITLE_Y = 61.f;
constexpr float QUESTION_Y = 55.f;
constexpr std::array<Rect, 2> BUTTONS = {{{36, 42, 88, 9}, {36, 31, 88, 9}}};
constexpr std::array<const char*, 2> LABELS = {"Take the ankh: life without end", "Renounce the gods, break the ankh"};
constexpr float DIM = 0.55f; // the game behind the panel
} // namespace

void EndingScreen::Open() {
	pointer.Clear();
	pointer.Move(TAKE);
}

void EndingScreen::choose(int button) {
	if (button != TAKE && button != RENOUNCE)
		return;
	world.dungeon.Win(button == RENOUNCE ? RunEnding::Renounced : RunEnding::Immortal); // final: no second ask
	ui.screen = Screen::Gameplay; // WinEntry takes over
}

void EndingScreen::KeyPressed(unsigned char key) {
	if (key == KEY_ESCAPE)
		ui.screen = Screen::Gameplay; // nothing chosen
	else
		navigate(navKeyOf(InputKey::Char(key), world.settings.controls));
}

void EndingScreen::SpecialKeyPressed(int key) { navigate(navKeyOf(InputKey::Special(key), world.settings.controls)); }

void EndingScreen::navigate(NavKey key) {
	if (key == NavKey::Activate)
		choose(pointer.hovered);
	else if (key != NavKey::None) // two buttons: any direction goes to the other one
		pointer.Move(pointer.hovered == TAKE ? RENOUNCE : TAKE);
}

int EndingScreen::buttonAt(int x, int y) const {
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(
		visibleArea(CANVAS_W, CANVAS_H, world.render.resX, world.render.resY),
		world.render.resX,
		world.render.resY,
		x,
		y,
		cx,
		cy
	);
	for (int i = 0; i < static_cast<int>(BUTTONS.size()); i++)
		if (BUTTONS[static_cast<size_t>(i)].contains(cx, cy))
			return i;
	return NONE;
}

void EndingScreen::MouseMotion(int x, int y) {
	if (const int at = buttonAt(x, y); at != NONE)
		pointer.Move(at);
}

void EndingScreen::MouseFunction(int button, int state, int x, int y) {
	if (button != MOUSE_LEFT_BUTTON)
		return;
	const int at = buttonAt(x, y);
	if (at != NONE)
		pointer.Move(at);
	if (state != MOUSE_UP) {
		pointer.Down(at, button);
		return;
	}
	if (const int fired = pointer.Up(at, button); fired != NONE)
		choose(fired);
}

void EndingScreen::Draw(int resX, int resY) {
	if (!fontsLoaded) {
		fontsLoaded = true;
		loadScreenFonts(title, heading, body, small, 8.f);
	}
	const Rect area = visibleArea(CANVAS_W, CANVAS_H, resX, resY);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(area.x, area.x + area.w, area.y, area.y + area.h, -200, 200);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	beginShapes();
	fillRect(area, BLACK, BLACK, DIM);
	fillRect({PANEL.x + 0.8f, PANEL.y - 1.f, PANEL.w, PANEL.h}, BLACK, BLACK, 0.5f); // drop shadow
	panel(PANEL, 0.94f);
	for (int i = 0; i < static_cast<int>(BUTTONS.size()); i++) {
		const bool focused = pointer.hovered == i;
		const TileStyle style = i == TAKE ? TileStyle::Lapis : TileStyle::Stone;
		const Rect r = tile(BUTTONS[static_cast<size_t>(i)], style, focused, focused && pointer.Held(i));
		beginText();
		textCenteredIn(heading, r, LABELS[static_cast<size_t>(i)], focused ? TEXT_HOVER : i == TAKE ? GOLD : LABEL);
		beginShapes();
	}

	beginText();
	textCentered(title, PANEL.cx(), TITLE_Y, "The Ankh of Life", GOLD);
	textCentered(body, PANEL.cx(), QUESTION_Y, "Life without end, or the gods named for what they are?", LABEL);
	textCentered(small, PANEL.cx(), PANEL.y - 4.5f, "Enter: choose, for good    Esc: not yet", LABEL_DIM);

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glColor3f(1, 1, 1);
}
