#include "screen_tabs.h"
#include "../render/gl_includes.h"
#include "../level/input.h"
#include "../state/game_state.h" // RenderSettings
#include "../state/ui_context.h"
#include "../render/ui_draw.h"

namespace {
constexpr Screen TAB_SCREENS[ui::SCREEN_TAB_COUNT] = {Screen::Inventory, Screen::Map, Screen::Journal, Screen::Skills};

int tabOf(Screen s) {
	for (int tab = 0; tab < ui::SCREEN_TAB_COUNT; tab++)
		if (TAB_SCREENS[tab] == s)
			return tab;
	return -1;
}

ui::Rect visibleArea(const RenderSettings& render) {
	return ui::visibleArea(ui::CANVAS_W, ui::CANVAS_H, render.resX, render.resY);
}

int tabAt(const RenderSettings& render, int x, int y) {
	float cx = 0.f;
	float cy = 0.f;
	ui::toCanvas(visibleArea(render), render.resX, render.resY, x, y, cx, cy);
	return ui::screenTabAt(cx, cy);
}
} // namespace

namespace ScreenTabs {

bool Has(Screen s) { return tabOf(s) >= 0; }

void Draw(UIContext& screens) {
	const ui::Pointer<int>& tabs = screens.tabs.pointer;
	const int open = tabOf(screens.screen);
	if (open < 0)
		return;
	if (screens.screen != Screen::Journal) // switched away from the journal by its tabs: what it showed was seen
		screens.journal.SeeShown();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	ui::Rect area = visibleArea(screens.world.render);
	glOrtho(area.x, area.x + area.w, area.y, area.y + area.h, -200, 200);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	ui::screenTabs(
		screens.world.assets.fonts.hudSmall,
		static_cast<ui::ScreenTab>(open),
		tabs.hovered,
		tabs.pressed,
		screens.world.journal.AnyNew()
	);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glColor3f(1, 1, 1);
}

bool Mouse(UIContext& screens, int button, int state, int x, int y) {
	ui::Pointer<int>& tabs = screens.tabs.pointer;
	if (button != MOUSE_LEFT_BUTTON)
		return false;
	tabs.Move(tabAt(screens.world.render, x, y));
	if (state == MOUSE_DOWN) {
		tabs.Down(tabs.hovered, button);
		return tabs.Pressing();
	}
	if (!tabs.Pressing())
		return false;
	if (const int fired = tabs.Up(tabs.hovered, button); fired >= 0) {
		screens.screen = TAB_SCREENS[fired];
		screens.menu.openedJournal = false;
		tabs.Clear(); // the new screen's strip has it under the mouse, but not hovered until it moves
	}
	return true;
}

void Motion(UIContext& screens, int x, int y) { screens.tabs.pointer.Move(tabAt(screens.world.render, x, y)); }

} // namespace ScreenTabs
