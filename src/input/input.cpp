#include <GL/gl.h>
#include <array>
#include <optional>
#include "../graphics/gl_includes.h"
#include "../state/game_state.h"
#include "../graphics/ink.h"
#include "input.h"
#include "input_actions.h"
#include "../ui/screen_state.h"

int lastMx = 0;
int lastMy = 0;

namespace {
void startJump() {
	if (Game().player->jump.jumping || Game().player->jump.falling || !Game().player->Alive() || Game().dungeon.Won())
		return;
	if (!Game().dungeon.JumpAllowed())
		return;

	if (Game().player->stats.Stamina() < JUMP_STAMINA_COST) {
		Game().player->stats.RefuseStamina(Game().events);
		return;
	}

	Game().player->stats.ConsumeStamina(JUMP_STAMINA_COST);

	float curX, curY;
	Game().dungeon.getC(curX, curY);
	Game().player->jump.start_y = curY;

	Game().player->jump.dir_x = static_cast<float>(Game().camera.Facing());

	Game().player->jump.speed = JUMP_FORWARD_SPEED;
	Game().player->jump.velocity = JUMP_INITIAL_VELOCITY;
	Game().player->jump.jumping = true;
	Game().player->jump.jump_up_timer.Reset();
	Game().player->PlayJumpSound();
}

// The walk keys held down, in GameplayAction order from MoveLeft.
std::array<bool, 4> walkHeld{};

bool isMove(GameplayAction action) {
	return action == GameplayAction::MoveLeft || action == GameplayAction::MoveRight ||
		   action == GameplayAction::MoveDown || action == GameplayAction::MoveUp;
}

class PlayerActionController {
  public:
	static void execute(GameplayAction action) {
		run(action);
		Game().ApplyWorldEvents(); // the riddle opens, a status line shows, before the next tick
	}

	static void run(GameplayAction action) {
		// No sprint while wading.
		float moveMultiplier = Game().dungeon.PlayerWading() ? 1.f : Game().player->stats.SprintMoveMultiplier();
		switch (action) {
		case GameplayAction::MoveLeft:
			walk(-PLAYER_MOVE_STEP * moveMultiplier, 0);
			Game().camera.rotW = -110;
			if (!Game().player->jump.jumping)
				Game().player->setModelState(ModelState::Move);
			break;
		case GameplayAction::MoveRight:
			walk(PLAYER_MOVE_STEP * moveMultiplier, 0);
			Game().camera.rotW = 70;
			if (!Game().player->jump.jumping)
				Game().player->setModelState(ModelState::Move);
			break;
		case GameplayAction::MoveDown:
			walk(0, -PLAYER_MOVE_STEP * moveMultiplier);
			break;
		case GameplayAction::MoveUp:
			walk(0, PLAYER_FORWARD_MOVE_STEP * moveMultiplier);
			break;
		case GameplayAction::Jump:
			startJump();
			break;
		case GameplayAction::Attack:
			tryAttack();
			break;
		case GameplayAction::Interact:
			interact();
			break;
		case GameplayAction::QuickHeal: // allowed during a swing: drinking does not change it
			Game().ui.inventory->QuickDrink(QuickKind::Health);
			break;
		case GameplayAction::QuickStamina:
			Game().ui.inventory->QuickDrink(QuickKind::Stamina);
			break;
		case GameplayAction::EquipClub:
			equip(ItemKind::Club);
			break;
		case GameplayAction::EquipSword:
			equip(ItemKind::Sword);
			break;
		case GameplayAction::EquipSpear:
			equip(ItemKind::Spear);
			break;
		case GameplayAction::EquipBow:
			equip(ItemKind::Bow);
			break;
		case GameplayAction::None:
			break;
		}
	}

	static void applyCameraDelta(float deltaX, float deltaY) {
		Game().camera.rotM += deltaX;
		Game().camera.rotN += deltaY;
		clampCamera();
	}

  private:
	// Only a step that really moved the player counts for the sprint drain, and none in the water: no sprint there.
	// Wading slows it (PlayerWalkFactor).
	static void walk(float dirX, float dirY) {
		const bool wading = Game().dungeon.PlayerWading();
		const float factor = Game().dungeon.PlayerWalkFactor();
		if (Game().dungeon.Move(dirX * factor, dirY * factor) && !wading)
			Game().player->stats.NoteWalked();
	}

	// No switch while a swing or a bow draw is under way: it would hit with the other weapon.
	static void equip(ItemKind weapon) {
		if (Game().player->attackStartMs < 0)
			Game().ui.inventory->Equip(weapon);
	}

	// The swing (or the bow draw) begins; it hits when its hit time comes (updateAttack in draw.cpp).
	static void tryAttack() {
		Player& player = *Game().player;
		if (player.attackStartMs >= 0 || !player.attackTimer.TimePassed())
			return;
		const Item* weapon = Game().ui.inventory->Equipped();
		player.attackTimer.SetInterval(weapon->motion.attackMs);
		player.attackStartMs = GameClock::now();
		player.attackLanded = false;
		weapon->swingSound.Play();
	}

	static void interact() {
		Game().dungeon.PickUp();
		if (Game().dungeon.PullLever())
			return;
		Game().dungeon.Interact();
	}

	static void clampCamera() {
		if (Game().camera.rotM > CAMERA_ROTATE_LIMIT_X)
			Game().camera.rotM = CAMERA_ROTATE_LIMIT_X;

		if (Game().camera.rotM < -CAMERA_ROTATE_LIMIT_X)
			Game().camera.rotM = -CAMERA_ROTATE_LIMIT_X;

		if (Game().camera.rotN > CAMERA_ROTATE_LIMIT_Y)
			Game().camera.rotN = CAMERA_ROTATE_LIMIT_Y;

		if (Game().camera.rotN < -CAMERA_ROTATE_LIMIT_Y)
			Game().camera.rotN = -CAMERA_ROTATE_LIMIT_Y;
	}
};
} // namespace

void executeGameplayAction(GameplayAction action) { PlayerActionController::execute(action); }

void setWalkHeld(GameplayAction move, bool held) {
	if (isMove(move))
		walkHeld[static_cast<size_t>(move) - static_cast<size_t>(GameplayAction::MoveLeft)] = held;
}

void releaseWalk() { walkHeld.fill(false); }

void stepHeldWalk() {
	if (!ScreenState::IsGameplayInteractionAllowed(Game()))
		return;
	for (size_t i = 0; i < walkHeld.size(); i++)
		if (walkHeld[i])
			PlayerActionController::execute(
				static_cast<GameplayAction>(static_cast<size_t>(GameplayAction::MoveLeft) + i));
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
	case BindAction::QuickHeal:
		return GameplayAction::QuickHeal;
	case BindAction::QuickStamina:
		return GameplayAction::QuickStamina;
	case BindAction::EquipClub:
		return GameplayAction::EquipClub;
	case BindAction::EquipSword:
		return GameplayAction::EquipSword;
	case BindAction::EquipSpear:
		return GameplayAction::EquipSpear;
	case BindAction::EquipBow:
		return GameplayAction::EquipBow;
	case BindAction::Sprint:
	case BindAction::LookLeft:
	case BindAction::LookRight:
	case BindAction::LookUp:
	case BindAction::LookDown:
	case BindAction::Inventory:
	case BindAction::Map:
	case BindAction::Journal:
		break;
	}
	return GameplayAction::None;
}

std::string keyCapOf(BindAction action) {
	const InputKey* key = Game().settings.controls.Of(action).First();
	return key ? keyCap(*key) : "";
}

// "1-4" while the four weapons are on four keys in a row, else their first keys one after the other.
std::string equipKeysCap() {
	const BindAction weapons[] = {BindAction::EquipClub, BindAction::EquipSword, BindAction::EquipSpear,
								  BindAction::EquipBow};
	const Bindings& controls = Game().settings.controls;
	const InputKey* first = controls.Of(weapons[0]).First();
	bool run = first != nullptr && first->kind == InputKey::Kind::Char;
	for (int i = 1; i < 4 && run; i++) {
		const InputKey* key = controls.Of(weapons[i]).First();
		run = key != nullptr && key->kind == InputKey::Kind::Char && key->code == first->code + i;
	}
	if (run)
		return keyCap(*first) + "-" + keyCap(*controls.Of(weapons[3]).First());
	std::string caps;
	for (BindAction weapon : weapons)
		caps += keyCapOf(weapon);
	return caps;
}

namespace {
// Held as long as the input is down; the other actions run once a press.
bool isHeld(BindAction action) { return action == BindAction::Sprint || isMove(gameplayActionOf(action)); }

// Look keys turn the camera a step a press (the key repeat turns it on).
bool look(BindAction action) {
	switch (action) {
	case BindAction::LookLeft:
		PlayerActionController::applyCameraDelta(-CAMERA_ROTATE_STEP, 0);
		return true;
	case BindAction::LookRight:
		PlayerActionController::applyCameraDelta(CAMERA_ROTATE_STEP, 0);
		return true;
	case BindAction::LookUp:
		PlayerActionController::applyCameraDelta(0, CAMERA_ROTATE_STEP);
		return true;
	case BindAction::LookDown:
		PlayerActionController::applyCameraDelta(0, -CAMERA_ROTATE_STEP);
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
		screen = screen == target ? Screen::Gameplay : target;
		return true;
	}
	if (action == BindAction::Sprint) { // also on the other screens, so it is down when the game comes back
		Game().player->stats.SetSprintRequested(true);
		return true;
	}
	if (screen != Screen::Gameplay)
		return false;
	if (look(action) || !ScreenState::IsGameplayInteractionAllowed(Game()))
		return true;
	GameplayAction gameplay = gameplayActionOf(action);
	if (isMove(gameplay))
		setWalkHeld(gameplay, true); // the key repeat only sends it again
	else
		PlayerActionController::execute(gameplay);
	return true;
}

// Released on every screen: a walk key let go in the inventory must not keep walking afterwards.
void releaseBinding(BindAction action) {
	if (action == BindAction::Sprint)
		Game().player->stats.SetSprintRequested(false);
	else
		setWalkHeld(gameplayActionOf(action), false);
}

std::optional<BindAction> bound(const InputKey& key) { return Game().settings.controls.ActionFor(key); }
} // namespace

void Idle() { glutPostRedisplay(); }

void keyPressed(unsigned char key, int x, int y) {
	if (ScreenState::ShouldRouteKeyboardToRiddle(Game())) {
		Game().ui.riddle->KeyboardF(key, x, y);
		return;
	}

	MainMenu& menu = Game().ui.menu;
	Screen& screen = Game().ui.screen;
	if (screen == Screen::Menu && menu.Capturing()) { // Options > Controls waits for a key; Esc cancels
		menu.CaptureKey(InputKey::Char(key));
		return;
	}

	if (key == KEY_ESCAPE) // esc
	{
		// Esc backs out of the inventory or the map to the game; only from the game it opens the menu.
		if (screen == Screen::Inventory || screen == Screen::Map || screen == Screen::Journal) {
			screen = Screen::Gameplay;
			return;
		}

		// In the menu Esc backs out of the save / load / options screens, then returns to the game (if there is one).
		if (screen == Screen::Menu && menu.InSubScreen()) {
			menu.ResetSubScreens();
			return;
		}
		if (screen == Screen::Menu && !menu.inGame)
			return;
		menu.ResetSubScreens();
		screen = screen == Screen::Menu ? Screen::Gameplay : Screen::Menu;
		return;
	}

	if (ScreenState::ShouldBlockKeyboardGameplay(Game()))
		return; // while the menu is shown, only [esc] is handled

	std::optional<BindAction> action = bound(InputKey::Char(key));
	if (action && pressBinding(*action))
		return;
	if (screen == Screen::Inventory)
		Game().ui.inventory->KeyPressed(key);
}

void specialKeyPressed(int key, int x, int y) {
	(void)x;
	(void)y;

	if (Game().ui.screen == Screen::Menu && Game().ui.menu.Capturing()) {
		Game().ui.menu.CaptureKey(InputKey::Special(key));
		return;
	}
	if (ScreenState::ShouldBlockKeyboardGameplay(Game()))
		return;

	std::optional<BindAction> action = bound(InputKey::Special(key));
	if (action && pressBinding(*action))
		return;

	if (Game().ui.screen == Screen::Journal)
		Game().ui.journal.SpecialKeyPressed(key);
	if (Game().ui.screen == Screen::Inventory)
		Game().ui.inventory->SpecialKeyPressed(key);
	if (Game().ui.screen != Screen::Gameplay)
		return;

	// Fixed keys (not bindable).
	if (key == SPECIAL_TOGGLE_CARTOON) {
		Game().settings.graphics.toon = !Game().settings.graphics.toon;
		Game().ApplySettings(true);
	}
	if (key == SPECIAL_TOGGLE_HITBOXES)
		Game().render.Hitboxes = !Game().render.Hitboxes;
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
	if (ScreenState::ShouldRouteMouseToMenu(Game())) {
		Game().ui.menu.MouseFunction(button, state, x, y);
		return;
	}

	// A held action (walk, sprint) lets go on every screen, like its key.
	std::optional<BindAction> action = bound(InputKey::Mouse(button));
	if (action && state == GLUT_UP && isHeld(*action))
		releaseBinding(*action);

	if (ScreenTabs::Has(Game().ui.screen) && ScreenTabs::Mouse(button, state, x, y))
		return;

	if (ScreenState::ShouldRouteMouseToInventory(Game())) {
		Game().ui.inventory->MouseFunction(button, state, x, y);
		return;
	}

	if (Game().ui.screen == Screen::Journal) {
		Game().ui.journal.MouseFunction(button, state, x, y);
		return;
	}

	// Held actions start when the button goes down, the others run when it comes up.
	if (action && Game().ui.screen == Screen::Gameplay && (state == GLUT_DOWN) == isHeld(*action))
		pressBinding(*action);
}
void processMousePassiveMotion(int a, int b) {
	if (ScreenState::ShouldRouteMouseToMenu(Game())) {
		Game().ui.menu.MousePassiveMotion(a, b);
		return;
	}
	if (ScreenTabs::Has(Game().ui.screen))
		ScreenTabs::Motion(a, b);

	if (ScreenState::ShouldRouteMouseToInventory(Game())) {
		Game().ui.inventory->MouseMotion(a, b);
		lastMx = a; // no camera jump when the inventory closes
		lastMy = b;
		return;
	}

	if (Game().ui.screen == Screen::Journal)
		Game().ui.journal.MouseMotion(a, b);
	if (Game().ui.screen == Screen::Map || Game().ui.screen == Screen::Journal) {
		lastMx = a;
		lastMy = b;
		return;
	}

	PlayerActionController::applyCameraDelta(-MOUSE_LOOK_SENSITIVITY * static_cast<float>(lastMx - a),
											 -MOUSE_LOOK_SENSITIVITY * static_cast<float>(lastMy - b));

	lastMx = a;
	lastMy = b;
}

void processMouseActiveMotion(int a, int b) {
	if (ScreenState::ShouldRouteMouseToMenu(Game())) {
		Game().ui.menu.MouseDrag(a, b);
		return;
	}
	if (ScreenTabs::Has(Game().ui.screen))
		ScreenTabs::Motion(a, b);
	if (ScreenState::ShouldRouteMouseToInventory(Game()))
		Game().ui.inventory->MouseMotion(a, b);
	if (Game().ui.screen == Screen::Journal)
		Game().ui.journal.MouseMotion(a, b);
}

void processMouseEntry(int a) { (void)a; }
