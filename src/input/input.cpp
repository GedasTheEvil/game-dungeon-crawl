#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_keyboard.h>
#include <array>
#include <optional>
#include <algorithm>
#include <cstdlib>
#include <utility>
#include <vector>
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
#include "../level/input/pad_stick.h"
#include "../core/logger.h"
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
	Game().caps = Caps::Keys;
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
	Game().caps = Caps::Keys;
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
	if (state == MOUSE_DOWN) {
		ui::hideTooltip();
		Game().caps = Caps::Keys;
	}
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
PadStick leftStick;
float rightX = 0.f; // the right stick, -1..1, y down
float rightY = 0.f;
std::array<bool, PadButton::COUNT> padHeld{}; // the buttons down, so a pad pulled out lets them go

// The pad's caps show from now on (the HUD, the hints), and its family's in Options > Controls.
void showPadCaps(Caps caps) {
	Game().caps = caps;
	Game().padFamily = caps;
}

float deadZone() { return static_cast<float>(Game().settings.gamepad.deadZone) / 100.f; }

// The game takes the pad's bindings: no screen over it, no name being typed.
bool padPlays() { return Game().ui.screen == Screen::Gameplay && !Game().ui.winEntry.Typing(); }

// On a screen a pad button is a key (plan sdl3-gamepad): the d-pad the arrows, South Enter, East Esc. False: none.
bool padAsKey(int button) {
	switch (button) {
	case PadButton::DPAD_UP:
		specialKeyPressed(SpecialKey::UP, 0, 0);
		return true;
	case PadButton::DPAD_DOWN:
		specialKeyPressed(SpecialKey::DOWN, 0, 0);
		return true;
	case PadButton::DPAD_LEFT:
		specialKeyPressed(SpecialKey::LEFT, 0, 0);
		return true;
	case PadButton::DPAD_RIGHT:
		specialKeyPressed(SpecialKey::RIGHT, 0, 0);
		return true;
	case PadButton::SOUTH:
		keyPressed(KEY_ENTER, 0, 0);
		return true;
	case PadButton::EAST:
		keyPressed(KEY_ESCAPE, 0, 0);
		return true;
	default:
		return false;
	}
}

constexpr std::array<BindAction, PadStick::DIRS> STICK_ACTIONS = {
	BindAction::MoveLeft,
	BindAction::MoveRight,
	BindAction::ClimbUp,
	BindAction::ClimbDown,
};
constexpr std::array<int, PadStick::DIRS> STICK_ARROWS = {
	SpecialKey::LEFT,
	SpecialKey::RIGHT,
	SpecialKey::UP,
	SpecialKey::DOWN,
};
} // namespace

void padPressed(int button, Caps caps) {
	if (button < 0 || button >= PadButton::COUNT)
		return;
	padHeld[static_cast<size_t>(button)] = true;
	ui::hideTooltip();
	MainMenu& menu = Game().ui.menu;
	if (Game().ui.screen == Screen::Menu && menu.Capturing()) { // Options > Controls waits; Start cancels as Esc
		if (button == PadButton::START)
			keyPressed(KEY_ESCAPE, 0, 0);
		else
			menu.CaptureKey(InputKey::Pad(button));
		showPadCaps(caps);
		return;
	}
	if (skipCutScene()) {
		showPadCaps(caps);
		return;
	}
	std::optional<BindAction> action = bound(InputKey::Pad(button));
	if (button == PadButton::START)
		keyPressed(KEY_ESCAPE, 0, 0);
	else if (padPlays()) {
		if (action == BindAction::Attack)
			newAttackPress();
		if (action)
			pressBinding(*action);
	} else if (action && (*action == BindAction::Sprint ||
						  (ScreenTabs::Has(Game().ui.screen) && screenOf(*action) != Screen::Gameplay)))
		pressBinding(*action); // a tab screen's button switches or closes it, as its key; the sprint stays held
	else
		padAsKey(button);
	showPadCaps(caps); // after the keys it sent, which show the keyboard's
}

void padReleased(int button) {
	if (button < 0 || button >= PadButton::COUNT || !std::exchange(padHeld[static_cast<size_t>(button)], false))
		return;
	if (std::optional<BindAction> action = bound(InputKey::Pad(button)))
		releaseBinding(*action);
}

void padStickMoved(bool right, float x, float y, Caps caps) {
	const float zone = deadZone();
	if (right) {
		rightX = x;
		rightY = y;
		if (padStickReach(x, zone) != 0.f || padStickReach(y, zone) != 0.f)
			showPadCaps(caps);
		return;
	}
	const PadStick before = leftStick;
	leftStick.Move(x, y, zone);
	for (int i = 0; i < PadStick::DIRS; i++) {
		const auto dir = static_cast<PadStick::Dir>(i);
		if (before.Held(dir) == leftStick.Held(dir))
			continue;
		const BindAction action = STICK_ACTIONS[static_cast<size_t>(i)];
		if (!leftStick.Held(dir)) {
			releaseBinding(action);
			continue;
		}
		if (padPlays())
			pressBinding(action);
		else if (Game().ui.screen != Screen::Menu || !Game().ui.menu.Capturing())
			specialKeyPressed(STICK_ARROWS[static_cast<size_t>(i)], 0, 0);
		showPadCaps(caps);
	}
}

void stepPadLook() {
	if (!padPlays() || !ScreenState::IsGameplayInteractionAllowed(Game()))
		return;
	const Settings::Gamepad& pad = Game().settings.gamepad;
	const PadLookTurn turn = padLookTurn(rightX, rightY, deadZone(), pad.lookSpeed, pad.invertY, UPDATE_TICK_MS);
	if (turn.m != 0.f || turn.n != 0.f)
		peekCamera(turn.m, turn.n);
}

void padLetGo() {
	for (int button = 0; button < PadButton::COUNT; button++)
		padReleased(button);
	padStickMoved(false, 0.f, 0.f, Game().padFamily);
	rightX = 0.f;
	rightY = 0.f;
}

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

// The pads plugged in (plan sdl3-gamepad), opened as they come (SDL sends an added event for each one there at the
// start too) and closed as they go.
std::vector<SDL_Gamepad*> pads;

Caps capsOf(SDL_JoystickID id) {
	SDL_Gamepad* pad = SDL_GetGamepadFromID(id);
	return Keymap::padCaps(pad != nullptr ? SDL_GetGamepadType(pad) : SDL_GAMEPAD_TYPE_STANDARD);
}

float axisValue(Sint16 value) { return std::clamp(static_cast<float>(value) / 32767.f, -1.f, 1.f); }

// A trigger is a button: down once pulled past half way, up again under 40 %.
constexpr float TRIGGER_DOWN = 0.5f;
constexpr float TRIGGER_UP = 0.4f;
std::array<bool, 2> triggerDown{};

void padAxis(const SDL_GamepadAxisEvent& event) {
	const Caps caps = capsOf(event.which);
	SDL_Gamepad* pad = SDL_GetGamepadFromID(event.which);
	if (pad == nullptr)
		return;
	auto axis = [pad](SDL_GamepadAxis a) { return axisValue(SDL_GetGamepadAxis(pad, a)); };
	switch (event.axis) {
	case SDL_GAMEPAD_AXIS_LEFTX:
	case SDL_GAMEPAD_AXIS_LEFTY:
		padStickMoved(false, axis(SDL_GAMEPAD_AXIS_LEFTX), axis(SDL_GAMEPAD_AXIS_LEFTY), caps);
		break;
	case SDL_GAMEPAD_AXIS_RIGHTX:
	case SDL_GAMEPAD_AXIS_RIGHTY:
		padStickMoved(true, axis(SDL_GAMEPAD_AXIS_RIGHTX), axis(SDL_GAMEPAD_AXIS_RIGHTY), caps);
		break;
	case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
	case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: {
		const bool left = event.axis == SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
		bool& down = triggerDown[left ? 0 : 1];
		const float pull = axisValue(event.value);
		const int button = left ? PadButton::LEFT_TRIGGER : PadButton::RIGHT_TRIGGER;
		if (!down && pull > TRIGGER_DOWN) {
			down = true;
			padPressed(button, caps);
		} else if (down && pull < TRIGGER_UP) {
			down = false;
			padReleased(button);
		}
		break;
	}
	default:
		break;
	}
}

void padAdded(SDL_JoystickID id) {
	if (SDL_GetGamepadFromID(id) != nullptr)
		return;
	if (SDL_Gamepad* pad = SDL_OpenGamepad(id)) {
		pads.push_back(pad);
		LOG_INFOF(
			"input",
			"Gamepad: %s (%s)",
			SDL_GetGamepadName(pad),
			SDL_GetGamepadStringForType(SDL_GetGamepadType(pad))
		);
	}
}

void padRemoved(SDL_JoystickID id) {
	SDL_Gamepad* pad = SDL_GetGamepadFromID(id);
	if (pad == nullptr)
		return;
	std::erase(pads, pad);
	SDL_CloseGamepad(pad);
	padLetGo();
	triggerDown.fill(false);
}
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
	case SDL_EVENT_GAMEPAD_ADDED:
		padAdded(event.gdevice.which);
		break;
	case SDL_EVENT_GAMEPAD_REMOVED:
		padRemoved(event.gdevice.which);
		break;
	case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
	case SDL_EVENT_GAMEPAD_BUTTON_UP:
		if (const std::optional<int> button = Keymap::padButton(static_cast<SDL_GamepadButton>(event.gbutton.button))) {
			if (event.gbutton.down)
				padPressed(*button, capsOf(event.gbutton.which));
			else
				padReleased(*button);
		}
		break;
	case SDL_EVENT_GAMEPAD_AXIS_MOTION:
		padAxis(event.gaxis);
		break;
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
