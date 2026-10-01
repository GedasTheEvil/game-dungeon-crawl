#include "screen_tabs.h"
#include "../graphics/gl_includes.h"
#include "../input/input.h"
#include "../state/game_state.h"
#include "ui_draw.h"
#include <GL/gl.h>

namespace {
constexpr Screen TAB_SCREENS[ui::SCREEN_TAB_COUNT] = {Screen::Inventory, Screen::Map, Screen::Journal};

int tabOf(Screen s) {
	for (int tab = 0; tab < ui::SCREEN_TAB_COUNT; tab++)
		if (TAB_SCREENS[tab] == s)
			return tab;
	return -1;
}

ui::Rect visibleArea() { return ui::visibleArea(ui::CANVAS_W, ui::CANVAS_H, Game().render.resX, Game().render.resY); }

int tabAt(int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	ui::toCanvas(visibleArea(), Game().render.resX, Game().render.resY, x, y, cx, cy);
	return ui::screenTabAt(cx, cy);
}
} // namespace

namespace ScreenTabs {

bool Has(Screen s) { return tabOf(s) >= 0; }

Screen ForKey(unsigned char key) {
	if (key == KEY_INVENTORY || key == KEY_INVENTORY_UPPER)
		return Screen::Inventory;
	if (key == KEY_MAP || key == KEY_MAP_UPPER)
		return Screen::Map;
	if (key == KEY_JOURNAL || key == KEY_JOURNAL_UPPER)
		return Screen::Journal;
	return Screen::Gameplay;
}

void Draw() {
	const ScreenTabsState& tabs = Game().ui.tabs;
	const int open = tabOf(Game().ui.screen);
	if (open < 0)
		return;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	ui::Rect area = visibleArea();
	glOrtho(area.x, area.x + area.w, area.y, area.y + area.h, -200, 200);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	ui::screenTabs(Game().assets.fonts.hudSmall, static_cast<ui::ScreenTab>(open), tabs.hovered, tabs.pressed);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glColor3f(1, 1, 1);
}

bool Mouse(int button, int state, int x, int y) {
	ScreenTabsState& tabs = Game().ui.tabs;
	if (button != MOUSE_LEFT_BUTTON)
		return false;
	tabs.hovered = tabAt(x, y);
	if (state == GLUT_DOWN) {
		tabs.pressed = tabs.hovered;
		return tabs.pressed >= 0;
	}
	const int was = tabs.pressed;
	tabs.pressed = -1;
	if (was < 0)
		return false;
	if (was == tabs.hovered) {
		Game().ui.screen = TAB_SCREENS[was];
		tabs.hovered = -1; // the new screen's strip has it under the mouse, but not hovered until it moves
	}
	return true;
}

void Motion(int x, int y) { Game().ui.tabs.hovered = tabAt(x, y); }

} // namespace ScreenTabs
