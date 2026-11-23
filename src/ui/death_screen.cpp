#include "death_screen.h"
#include "../render/ui_draw.h"
#include "../level/gameplay_world.h"
#include "../level/input.h"
#include "../state/game_state.h"
#include "../state/ui_context.h"
#include <array>
#include "../render/gl_includes.h"

namespace {
using namespace ui;

// ---- layout ----------------------------------------------------------------

constexpr Rect PANEL = {46, 28, 68, 38};
constexpr float TITLE_Y = 56.f;
constexpr std::array<Rect, 2> BUTTONS = {{{52, 42, 56, 9}, {52, 31, 56, 9}}};
constexpr float DIM = 0.55f; // the game behind the panel

Rect visibleArea(const RenderSettings& render) { return ui::visibleArea(CANVAS_W, CANVAS_H, render.resX, render.resY); }
} // namespace

void DeathScreen::Follow() {
	if (world.player.Alive()) {
		state = State::Alive;
		return;
	}
	const int now = GameClock::now();
	if (state == State::Alive) {
		state = State::Dying;
		diedMs = now;
		world.saves.CountDeath();
	}
	if (state != State::Dying || now - diedMs < DEATH_SCREEN_DELAY_MS || ui.screen != Screen::Gameplay)
		return;
	state = State::Shown;
	ui.screen = Screen::Death;
	const auto latest = world.saves.folder.Latest();
	retryEnabled = latest.has_value();
	retryLabel = latest ? "Retry from " + latest->Label() : "Retry";
	pointer.Clear();
	pointer.Move(retryEnabled ? RETRY : MAIN_MENU);
}

void DeathScreen::activate(int button) {
	if (const auto latest = world.saves.folder.Latest(); button == RETRY && retryEnabled && latest) {
		if (!world.saves.LoadIntoGame(latest->path)) { // a broken file (plan save-load-safety): no second try
			retryEnabled = false;
			retryLabel = "Could not load " + latest->Label();
			pointer.Move(MAIN_MENU);
		}
		return;
	}
	if (button != MAIN_MENU)
		return;
	MainMenu& menu = ui.menu;
	menu.ResetSubScreens();
	menu.inGame = false;
	ui.screen = Screen::Menu;
}

void DeathScreen::KeyPressed(unsigned char key) {
	if (key == KEY_ESCAPE)
		activate(MAIN_MENU);
	else
		Navigate(navKeyOf(InputKey::Char(key), world.settings.controls));
}

void DeathScreen::SpecialKeyPressed(int key) { Navigate(navKeyOf(InputKey::Special(key), world.settings.controls)); }

void DeathScreen::Navigate(NavKey key) {
	if (key == NavKey::Activate)
		activate(pointer.hovered);
	else if (key != NavKey::None && retryEnabled) // two buttons: any direction goes to the other one
		pointer.Move(pointer.hovered == RETRY ? MAIN_MENU : RETRY);
}

int DeathScreen::buttonAt(int x, int y) const {
	float cx = 0.f;
	float cy = 0.f;
	toCanvas(visibleArea(world.render), world.render.resX, world.render.resY, x, y, cx, cy);
	for (int i = 0; i < static_cast<int>(BUTTONS.size()); i++)
		if (BUTTONS[static_cast<size_t>(i)].contains(cx, cy))
			return i == RETRY && !retryEnabled ? NONE : i;
	return NONE;
}

void DeathScreen::MouseMotion(int x, int y) {
	if (const int at = buttonAt(x, y); at != NONE)
		pointer.Move(at);
}

void DeathScreen::MouseFunction(int button, int state, int x, int y) {
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
		activate(fired);
}

void DeathScreen::Draw(int resX, int resY) {
	ensureScreenFonts(title, heading, body, small, 8.f, fontGeneration);
	const Rect area = ui::visibleArea(CANVAS_W, CANVAS_H, resX, resY);
	beginCanvas(area);
	glDisable(GL_DEPTH_TEST);

	beginShapes();
	fillRect(area, BLACK, BLACK, DIM);
	fillRect({PANEL.x + 0.8f, PANEL.y - 1.f, PANEL.w, PANEL.h}, BLACK, BLACK, 0.5f); // drop shadow
	panel(PANEL, 0.94f);
	for (int i = 0; i < static_cast<int>(BUTTONS.size()); i++) {
		const bool enabled = i != RETRY || retryEnabled;
		const bool focused = enabled && pointer.hovered == i;
		const bool primary = i == RETRY ? retryEnabled : !retryEnabled;
		const TileStyle style = !enabled ? TileStyle::Disabled : primary ? TileStyle::Lapis : TileStyle::Stone;
		const Rect r = tile(BUTTONS[static_cast<size_t>(i)], style, focused, focused && pointer.Held(i));
		beginText();
		const Color c = !enabled ? LABEL_DIM : focused ? TEXT_HOVER : primary ? GOLD : LABEL;
		textCenteredIn(heading, r, i == RETRY ? retryLabel.c_str() : "Main Menu", c);
		beginShapes();
	}

	beginText();
	textCentered(title, PANEL.cx(), TITLE_Y, "You died", GOLD);
	textCentered(
		small,
		PANEL.cx(),
		PANEL.y - 4.5f,
		retryEnabled ? "Enter: pick    Esc: main menu" : "No save yet    Esc: main menu",
		LABEL_DIM
	);

	endScreen();
}
