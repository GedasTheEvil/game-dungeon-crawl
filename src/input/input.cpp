#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <array>
#include <optional>
#include <cstdlib>
#include "../game/window.h"
#include "input_events.h"
#include "keymap.h"
#include "../state/game_state.h"
#include "../state/ui_context.h"
#include "../render/ink.h"
#include "../level/input/keys.h"
#include "input_actions.h"
#include "../state/gameplay_commands.h"
#include "../level/input/mouse_look.h"
#include "../level/input/camera_look.h"
#include "../ui/screen_state.h"
#include "../render/ui_draw.h"
#include "../level/tuning/gameplay_player.h"
#include "../level/tuning/gameplay_shared.h"

namespace {
MouseLook mouseLook;
bool keyRepeatIgnored = false;

// The game is played, no screen over it: the pointer hidden and held in the window (plan mouse-leaves-window).
bool pointerHidden() {
	return Game().ui.screen == Screen::Gameplay && ScreenState::IsGameplayInteractionAllowed(Game());
}
} // namespace

void followCursor() { Window::holdPointer(pointerHidden()); }

void ignoreKeyRepeat(bool ignore) { keyRepeatIgnored = ignore; }

GameplayAction gameplayActionOf(BindAction action) {
	switch (action) {
	case BindAction::MoveLeft:
		return GameplayAction::MoveLeft;
	case BindAction::MoveRight:
		return GameplayAction::MoveRight;
	case BindAction::ClimbUp:
		return GameplayAction::MoveUp;
	case BindAction::ClimbDown:
		return GameplayAction::MoveDown;
	case BindAction::Jump:
		return GameplayAction::Jump;
	case BindAction::Attack:
		return GameplayAction::Attack;
	case BindAction::Interact:
		return GameplayAction::Interact;
	case BindAction::Drag:
		return GameplayAction::Drag;
	case BindAction::QuickHeal:
		return GameplayAction::QuickHeal;
	case BindAction::QuickStamina:
		return GameplayAction::QuickStamina;
	case BindAction::QuickAntidote:
		return GameplayAction::QuickAntidote;
	case BindAction::StaminaSkills:
		return GameplayAction::StaminaSkills;
	case BindAction::EquipMelee:
		return GameplayAction::EquipMelee;
	case BindAction::EquipRanged:
		return GameplayAction::EquipRanged;
	case BindAction::Sprint:
	case BindAction::LookLeft:
	case BindAction::LookRight:
	case BindAction::LookUp:
	case BindAction::LookDown:
	case BindAction::Inventory:
	case BindAction::Map:
	case BindAction::Journal:
	case BindAction::Skills:
		break;
	}
	return GameplayAction::None;
}

namespace {
// The look keys are held: stepCameraLook turns the camera each tick while one is down (plan controls-smoothing).
// False: not a look key.
bool look(BindAction action, bool down) {
	switch (action) {
	case BindAction::LookLeft:
		holdLook(CameraLook::Dir::Left, down);
		return true;
	case BindAction::LookRight:
		holdLook(CameraLook::Dir::Right, down);
		return true;
	case BindAction::LookUp:
		holdLook(CameraLook::Dir::Up, down);
		return true;
	case BindAction::LookDown:
		holdLook(CameraLook::Dir::Down, down);
		return true;
	default:
		return false;
	}
}

Screen screenOf(BindAction action) {
	switch (action) {
	case BindAction::Inventory:
		return Screen::Inventory;
	case BindAction::Map:
		return Screen::Map;
	case BindAction::Journal:
		return Screen::Journal;
	case BindAction::Skills:
		return Screen::Skills;
	default:
		return Screen::Gameplay;
	}
}

// A bound key or mouse button went down. False when it means nothing on the open screen, which then gets the raw
// input (the inventory and the journal walk their selection with the move keys).
bool pressBinding(BindAction action) {
	// The inventory, map and journal keys open their screen from the game or from each other; the open screen's
	// key closes it. All three pause the game.
	Screen& screen = Game().ui.screen;
	if (Screen target = screenOf(action); target != Screen::Gameplay) {
		if (screen != Screen::Gameplay && !ScreenTabs::Has(screen))
			return false;
		if (screen == Screen::Gameplay && Game().dungeon.PlayerHeldBy() != nullptr)
			return true; // held: no screen, as no drink
		if (screen == Screen::Gameplay && !Game().player->Alive())
			return true; // the death screen comes up
		if (screen == Screen::Gameplay && refusedInsane()) {
			Game().ApplyWorldEvents();
			return true;
		}
		screen = screen == target ? Screen::Gameplay : target;
		Game().ui.menu.openedJournal = false;
		return true;
	}
	if (action == BindAction::Sprint) { // also on the other screens, so it is down when the game comes back
		Game().player->stats.SetSprintRequested(true);
		return true;
	}
	if (screen != Screen::Gameplay)
		return false;
	if (look(action, true) || !ScreenState::IsGameplayInteractionAllowed(Game()))
		return true;
	GameplayAction gameplay = gameplayActionOf(action);
	if (isMoveAction(gameplay))
		setWalkHeld(gameplay, true); // the key repeat only sends it again
	else
		executeGameplayAction(gameplay);
	return true;
}

// Released on every screen: a walk key let go in the inventory must not keep walking afterwards.
void releaseBinding(BindAction action) {
	if (look(action, false))
		return;
	if (action == BindAction::Attack)
		releaseAttack();
	if (action == BindAction::Sprint)
		Game().player->stats.SetSprintRequested(false);
	else
		setWalkHeld(gameplayActionOf(action), false);
}

std::optional<BindAction> bound(const InputKey& key) { return Game().settings.controls.ActionFor(key); }

// Any key or click during a cut-scene skips it and does nothing else (plan cinematic-effects).
bool skipCutScene() {
	if (Game().ui.screen != Screen::Gameplay || !Game().dungeon.CutScenePlaying())
		return false;
	Game().dungeon.SkipCutScene();
	return true;
}
} // namespace

void keyPressed(unsigned char key, int x, int y) {
	ui::hideTooltip();
	if (skipCutScene())
		return;
	if (Game().ui.screen == Screen::Riddle) {
		Game().ui.riddle->KeyboardF(key, x, y);
		return;
	}
	if (Game().ui.screen == Screen::Death) {
		Game().ui.death.KeyPressed(key);
		return;
	}
	if (Game().ui.screen == Screen::Ending) { // plan ending-choice
		Game().ui.ending.KeyPressed(key);
		return;
	}
	if (Game().ui.screen == Screen::Gameplay && Game().ui.winEntry.Typing()) {
		Game().ui.winEntry.KeyPressed(key);
		return;
	}

	MainMenu& menu = Game().ui.menu;
	Screen& screen = Game().ui.screen;
	if (screen == Screen::Menu && menu.Capturing()) { // Options > Controls waits for a key; Esc cancels
		menu.CaptureKey(InputKey::Char(key));
		return;
	}

	if (screen == Screen::Inventory && Game().ui.inventory->Asking()) { // its confirm box takes every key, Esc too
		Game().ui.inventory->KeyPressed(key);
		return;
	}
	if (key == KEY_ESCAPE) // esc
	{
		// Esc backs out of the inventory or the map to the game, of a journal the in-game menu opened back to the menu;
		// only from the game it opens the menu.
		if (ScreenTabs::Has(screen)) {
			screen = screen == Screen::Journal && menu.openedJournal ? Screen::Menu : Screen::Gameplay;
			menu.openedJournal = false;
			return;
		}

		// In the menu Esc backs out of the save / load / options screens, then returns to the game (if there is one).
		if (screen == Screen::Menu && menu.InSubScreen()) {
			menu.BackOut();
			return;
		}
		if (screen == Screen::Menu && !menu.inGame)
			return;
		menu.ResetSubScreens();
		screen = screen == Screen::Menu ? Screen::Gameplay : Screen::Menu;
		return;
	}

	if (screen == Screen::Menu)
		menu.KeyPressed(key);
	if (Game().ui.screen == Screen::Menu)
		return; // while the menu is shown, only [esc] and the sub-screen's keys are handled

	std::optional<BindAction> action = bound(InputKey::Char(key));
	if (action && pressBinding(*action))
		return;
	if (UiScreen* tab = Game().ui.TabScreen())
		tab->KeyPressed(key);
}

void specialKeyPressed(int key, int x, int y) {
	ui::hideTooltip();
	(void)x;
	(void)y;
	if (skipCutScene())
		return;

	if (Game().ui.screen == Screen::Menu && Game().ui.menu.Capturing()) {
		Game().ui.menu.CaptureKey(InputKey::Special(key));
		return;
	}
	if (Game().ui.screen == Screen::Death) {
		if (key == SPECIAL_QUICK_LOAD) // plan quick-save: also from the death screen
			Game().saves.LoadQuicksave();
		else
			Game().ui.death.SpecialKeyPressed(key);
		return;
	}
	if (Game().ui.screen == Screen::Ending) { // no quick save or load either
		Game().ui.ending.SpecialKeyPressed(key);
		return;
	}
	if (Game().ui.screen == Screen::Gameplay && Game().ui.winEntry.Typing()) { // no quick save or load either
		Game().ui.winEntry.SpecialKeyPressed(key);
		return;
	}
	if (Game().ui.screen == Screen::Inventory && Game().ui.inventory->Asking()) {
		Game().ui.inventory->SpecialKeyPressed(key);
		return;
	}
	if (Game().ui.screen == Screen::Menu)
		Game().ui.menu.SpecialKeyPressed(key);
	if (Game().ui.screen == Screen::Menu)
		return;

	std::optional<BindAction> action = bound(InputKey::Special(key));
	if (action && pressBinding(*action))
		return;

	if (UiScreen* tab = Game().ui.TabScreen())
		tab->SpecialKeyPressed(key);
	if (Game().ui.screen != Screen::Gameplay)
		return;

	// Fixed keys (not bindable).
	if (key == SPECIAL_TOGGLE_CARTOON) {
		Game().settings.graphics.toon = !Game().settings.graphics.toon;
		Game().ApplySettings(true);
	}
	if (key == SPECIAL_TOGGLE_HITBOXES && Game().debug) // plan controls-help: a debug key, only with --debug
		Game().render.Hitboxes = !Game().render.Hitboxes;
	if (key == SPECIAL_QUICK_SAVE)
		Game().saves.Quicksave();
	if (key == SPECIAL_QUICK_LOAD)
		Game().saves.LoadQuicksave();
}

void keyReleased(unsigned char key, int x, int y) {
	(void)x;
	(void)y;
	if (std::optional<BindAction> action = bound(InputKey::Char(key)))
		releaseBinding(*action);
}

void specialKeyReleased(int key, int x, int y) {
	(void)x;
	(void)y;
	if (std::optional<BindAction> action = bound(InputKey::Special(key)))
		releaseBinding(*action);
}

void processMouse(int button, int state, int x, int y) {
	if (state == MOUSE_DOWN)
		ui::hideTooltip();
	static int skipButton = -1; // the click that skipped a cut-scene: its release does nothing either
	if (state == MOUSE_DOWN && skipCutScene()) {
		skipButton = button;
		return;
	}
	if (state == MOUSE_UP && button == skipButton) {
		skipButton = -1;
		return;
	}
	if (Game().ui.screen == Screen::Menu) {
		Game().ui.menu.MouseFunction(button, state, x, y);
		return;
	}
	if (Game().ui.screen == Screen::Death) {
		Game().ui.death.MouseFunction(button, state, x, y);
		return;
	}
	if (Game().ui.screen == Screen::Ending) {
		Game().ui.ending.MouseFunction(button, state, x, y);
		return;
	}

	// A bound button comes up on every screen, like its key: a walk or the sprint lets go, a power hit's charge swings.
	std::optional<BindAction> action = bound(InputKey::Mouse(button));
	if (action && state == MOUSE_UP)
		releaseBinding(*action);

	if (ScreenTabs::Has(Game().ui.screen) && ScreenTabs::Mouse(Game().ui, button, state, x, y))
		return;

	if (UiScreen* tab = Game().ui.TabScreen()) {
		tab->MouseFunction(button, state, x, y);
		return;
	}

	// A bound button acts when it goes down, like its key (plan controls-smoothing): the jump, the interact, the
	// attack (with a power hit: the charge starts, the release above swings it). A down on a screen went to it above,
	// and the up only lets go: a click that closes a screen runs nothing in the game.
	if (action && state == MOUSE_DOWN && Game().ui.screen == Screen::Gameplay) {
		if (*action == BindAction::Attack)
			newAttackPress();
		pressBinding(*action);
	}
}

namespace {
// Only the game turns the camera; the screens over it (the riddle too) keep the position fresh (MouseLook).
void lookAt(int x, int y) {
	const MouseLook::Turn turn = mouseLook.Motion(x, y, Game().ui.screen == Screen::Gameplay);
	if (turn.dx == 0 && turn.dy == 0)
		return;
	const MouseTurn perPixel = mouseTurnPerPixel(Game().settings.mouse); // plan settings-extras
	turnCamera(perPixel.x * static_cast<float>(turn.dx), perPixel.y * static_cast<float>(turn.dy));
}
} // namespace

void processMousePassiveMotion(int a, int b) {
	lookAt(a, b);
	if (Game().ui.screen == Screen::Menu) {
		Game().ui.menu.MousePassiveMotion(a, b);
		return;
	}
	if (Game().ui.screen == Screen::Death) {
		Game().ui.death.MouseMotion(a, b);
		return;
	}
	if (Game().ui.screen == Screen::Ending) {
		Game().ui.ending.MouseMotion(a, b);
		return;
	}
	if (ScreenTabs::Has(Game().ui.screen))
		ScreenTabs::Motion(Game().ui, a, b);
	if (UiScreen* tab = Game().ui.TabScreen())
		tab->MouseMotion(a, b); // the map pans only while its button is held
}

// A button held (the attack's power hit charge) still turns the camera: no jump at the release.
void processMouseActiveMotion(int a, int b) {
	lookAt(a, b);
	if (Game().ui.screen == Screen::Menu) {
		Game().ui.menu.MouseDrag(a, b);
		return;
	}
	if (Game().ui.screen == Screen::Death) {
		Game().ui.death.MouseMotion(a, b);
		return;
	}
	if (Game().ui.screen == Screen::Ending) {
		Game().ui.ending.MouseMotion(a, b);
		return;
	}
	if (ScreenTabs::Has(Game().ui.screen))
		ScreenTabs::Motion(Game().ui, a, b);
	if (UiScreen* tab = Game().ui.TabScreen())
		tab->MouseMotion(a, b);
}

// The cursor left or entered the window: the first motion back does not turn the camera by the way it went outside.
void processMouseEntry() { mouseLook.Forget(); }

namespace {
// A motion of the held pointer (relative mode, Window::holdPointer), in pixels: it only turns the look. The next free
// motion starts afresh.
void lookBy(float dx, float dy) {
	mouseLook.Forget();
	if (Game().ui.screen != Screen::Gameplay)
		return;
	const MouseTurn perPixel = mouseTurnPerPixel(Game().settings.mouse); // plan settings-extras
	turnCamera(perPixel.x * dx, perPixel.y * dy);
}

// The text fields take the characters the layout types: the riddle's answer, the Hall of Records name.
bool typing() {
	const Screen screen = Game().ui.screen;
	return screen == Screen::Riddle || (screen == Screen::Gameplay && Game().ui.winEntry.Typing());
}

bool controlChar(int c) { return c < ' ' || c == KEY_DELETE; }

// A key by where it is (Keymap::physical), so the bindings and the menus do not depend on the layout; in a text field
// a character key types what the layout prints. Releases go by place too, so a key let go after Shift still lets go.
void keyEvent(const SDL_KeyboardEvent& event) {
	const std::optional<InputKey> key = Keymap::physical(event.scancode, (event.mod & SDL_KMOD_NUM) != 0);
	if (!event.down) {
		if (key && key->kind == InputKey::Kind::Special)
			specialKeyReleased(key->code, 0, 0);
		else if (key)
			keyReleased(static_cast<unsigned char>(key->code), 0, 0);
		return;
	}
	if (event.repeat && keyRepeatIgnored)
		return;
	if (key && key->kind == InputKey::Kind::Special) {
		specialKeyPressed(key->code, 0, 0);
		return;
	}
	unsigned char c = key ? static_cast<unsigned char>(key->code) : 0;
	if (typing() && !controlChar(c))
		c = Keymap::typed(SDL_GetKeyFromScancode(event.scancode, event.mod, false));
	if (c != 0)
		keyPressed(c, 0, 0);
}

int buttonOf(Uint8 button) {
	switch (button) {
	case SDL_BUTTON_LEFT:
		return MOUSE_LEFT_BUTTON;
	case SDL_BUTTON_MIDDLE:
		return MOUSE_MIDDLE_BUTTON;
	case SDL_BUTTON_RIGHT:
		return MOUSE_RIGHT_BUTTON;
	default:
		return -1;
	}
}

int pixel(float at, float density) { return static_cast<int>(at * density); }
} // namespace

void handleInputEvent(const SDL_Event& event, float density) {
	switch (event.type) {
	case SDL_EVENT_KEY_DOWN:
	case SDL_EVENT_KEY_UP:
		keyEvent(event.key);
		break;
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
	case SDL_EVENT_MOUSE_BUTTON_UP:
		if (const int button = buttonOf(event.button.button); button >= 0)
			processMouse(
				button,
				event.button.down ? MOUSE_DOWN : MOUSE_UP,
				pixel(event.button.x, density),
				pixel(event.button.y, density)
			);
		break;
	case SDL_EVENT_MOUSE_WHEEL: { // a down and an up of button 3 (up) or 4 (down) per notch
		int notches = event.wheel.integer_y;
		if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
			notches = -notches;
		const int button = notches > 0 ? MOUSE_WHEEL_UP : MOUSE_WHEEL_DOWN;
		const int x = pixel(event.wheel.mouse_x, density);
		const int y = pixel(event.wheel.mouse_y, density);
		for (int i = 0; i < std::abs(notches); i++) {
			processMouse(button, MOUSE_DOWN, x, y);
			processMouse(button, MOUSE_UP, x, y);
		}
		break;
	}
	case SDL_EVENT_MOUSE_MOTION:
		if (SDL_GetWindowRelativeMouseMode(SDL_GetWindowFromID(event.motion.windowID)))
			lookBy(event.motion.xrel * density, event.motion.yrel * density);
		else if (event.motion.state != 0) // a button held (the attack's power hit charge)
			processMouseActiveMotion(pixel(event.motion.x, density), pixel(event.motion.y, density));
		else
			processMousePassiveMotion(pixel(event.motion.x, density), pixel(event.motion.y, density));
		break;
	default:
		break;
	}
}
