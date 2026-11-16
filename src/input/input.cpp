#include <GL/gl.h>
#include <array>
#include <optional>
#include <utility>
#include "../render/gl_includes.h"
#include "../state/game_state.h"
#include "../state/ui_context.h"
#include "../render/ink.h"
#include "../graphics/frame_rate.h"
#include "../level/input.h"
#include "input_actions.h"
#include "../state/gameplay_commands.h"
#include "../level/mouse_look.h"
#include "../level/camera_look.h"
#include "../ui/screen_state.h"
#include "../render/ui_draw.h"
#include "../level/gameplay_player.h"
#include "../level/gameplay_shared.h"

namespace {
MouseLook mouseLook;
bool pointerInside = true; // the pointer is over the window (GLUT entry): the pointer hold may warp it

// The game is played, no screen over it: the cursor hidden and held in the window (plan mouse-leaves-window).
bool pointerHidden() {
	return Game().ui.screen == Screen::Gameplay && ScreenState::IsGameplayInteractionAllowed(Game());
}
} // namespace

void followCursor() {
	static int shown = -1;
	const int cursor = pointerHidden() ? GLUT_CURSOR_NONE : GLUT_CURSOR_LEFT_ARROW;
	if (cursor != std::exchange(shown, cursor))
		glutSetCursor(cursor);
}

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

void Idle() { FrameRate::idle(); }

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
	if (state == GLUT_DOWN)
		ui::hideTooltip();
	static int skipButton = -1; // the click that skipped a cut-scene: its release does nothing either
	if (state == GLUT_DOWN && skipCutScene()) {
		skipButton = button;
		return;
	}
	if (state == GLUT_UP && button == skipButton) {
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

	// A bound button comes up on every screen, like its key: a walk or the sprint lets go, a power hit's charge swings.
	std::optional<BindAction> action = bound(InputKey::Mouse(button));
	if (action && state == GLUT_UP)
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
	if (action && state == GLUT_DOWN && Game().ui.screen == Screen::Gameplay) {
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
	if (ScreenTabs::Has(Game().ui.screen))
		ScreenTabs::Motion(Game().ui, a, b);
	if (UiScreen* tab = Game().ui.TabScreen())
		tab->MouseMotion(a, b);
}

// The cursor left or entered the window: the first motion back does not turn the camera by the way it went outside.
// Gone (another window over it, Alt+Tab): no warp until it is back.
void processMouseEntry(int state) {
	pointerInside = state == GLUT_ENTERED;
	mouseLook.Forget();
}

namespace {
// Plan mouse-leaves-window: while the game looks, the hidden pointer goes back to the window's centre before it
// reaches an edge, so the look keeps turning and a click never lands outside. Not in a scenario (no GLUT callbacks).
void holdPointer() {
	if (!pointerInside || !pointerHidden())
		return;
	if (const std::optional<MouseLook::Point> to = mouseLook.Hold(Game().render.resX, Game().render.resY))
		glutWarpPointer(to->x, to->y);
}
} // namespace

void windowPassiveMotion(int x, int y) {
	processMousePassiveMotion(x, y);
	holdPointer();
}

void windowActiveMotion(int x, int y) {
	processMouseActiveMotion(x, y);
	holdPointer();
}
