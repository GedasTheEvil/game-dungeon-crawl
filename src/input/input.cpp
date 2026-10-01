#include <GL/gl.h>
#include <array>
#include "../graphics/gl_includes.h"
#include "../state/game_state.h"
#include "../graphics/ink.h"
#include "input.h"
#include "input_actions.h"
#include "../ui/screen_state.h"

unsigned char lastKey;

int lastMx = 0;
int lastMy = 0;

namespace {
void startJump() {
	if (Game().player->jump.jumping || Game().player->jump.falling || !Game().player->Alive() || Game().hasWon)
		return;

	if (Game().player->stats.Stamina() < JUMP_STAMINA_COST) {
		Game().player->stats.RefuseStamina();
		return;
	}

	Game().player->stats.ConsumeStamina(JUMP_STAMINA_COST);

	float curX, curY;
	Game().dungeon.getC(curX, curY);
	Game().player->jump.start_y = curY;

	Game().player->jump.dir_x = 0;
	if (lastKey == KEY_MOVE_LEFT || Game().camera.rotW < 0)
		Game().player->jump.dir_x = -1;
	else if (lastKey == KEY_MOVE_RIGHT || Game().camera.rotW > 0)
		Game().player->jump.dir_x = 1;

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
		float moveMultiplier = Game().player->stats.SprintMoveMultiplier();
		switch (action) {
		case GameplayAction::MoveLeft:
			Game().dungeon.Move(-PLAYER_MOVE_STEP * moveMultiplier, 0);
			Game().camera.rotW = -110;
			if (!Game().player->jump.jumping)
				Game().player->setModelState(ModelState::Move);
			break;
		case GameplayAction::MoveRight:
			Game().dungeon.Move(PLAYER_MOVE_STEP * moveMultiplier, 0);
			Game().camera.rotW = 70;
			if (!Game().player->jump.jumping)
				Game().player->setModelState(ModelState::Move);
			break;
		case GameplayAction::MoveDown:
			Game().dungeon.Move(0, -PLAYER_MOVE_STEP * moveMultiplier);
			break;
		case GameplayAction::MoveUp:
			Game().dungeon.Move(0, PLAYER_FORWARD_MOVE_STEP * moveMultiplier);
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

void Idle() { glutPostRedisplay(); }

void keyPressed(unsigned char key, int x, int y) {
	if (ScreenState::ShouldRouteKeyboardToRiddle(Game())) {
		Game().ui.riddle->KeyboardF(key, x, y);
		return;
	}

	if (key == KEY_ESCAPE) // esc
	{
		// Esc backs out of the inventory or the map to the game; only from the game it opens the menu.
		Screen& screen = Game().ui.screen;
		if (screen == Screen::Inventory || screen == Screen::Map || screen == Screen::Journal) {
			screen = Screen::Gameplay;
			return;
		}

		// In the menu Esc backs out of the save / load / options screens, then returns to the game (if there is one).
		MainMenu& menu = Game().ui.menu;
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

	if (Game().ui.screen == Screen::Inventory && key != KEY_INVENTORY) {
		Game().ui.inventory->KeyPressed(key);
		return;
	}

	// The map pauses the game: only M (or Esc, above) closes it.
	if (key == KEY_MAP || key == KEY_MAP_UPPER) {
		if (Game().ui.screen != Screen::Inventory)
			Game().ui.screen = Game().ui.screen == Screen::Map ? Screen::Gameplay : Screen::Map;
		return;
	}
	if (Game().ui.screen == Screen::Map)
		return;

	// The journal pauses the game too: J (or Esc) closes it.
	if (key == KEY_JOURNAL || key == KEY_JOURNAL_UPPER) {
		if (Game().ui.screen != Screen::Inventory)
			Game().ui.screen = Game().ui.screen == Screen::Journal ? Screen::Gameplay : Screen::Journal;
		return;
	}
	if (Game().ui.screen == Screen::Journal)
		return;

	if (ScreenState::IsGameplayInteractionAllowed(Game())) {
		GameplayAction action = MapKeyboardGameplayAction(key);
		if (isMove(action))
			setWalkHeld(action, true); // the key repeat only sends it again
		else
			PlayerActionController::execute(action);
	} // eo Alive

	if (key == KEY_INVENTORY)
		Game().ui.screen = Game().ui.screen == Screen::Inventory ? Screen::Gameplay : Screen::Inventory;

	lastKey = key;
}

void specialKeyPressed(int key, int x, int y) {
	(void)x;
	(void)y;

	if (ScreenState::ShouldBlockKeyboardGameplay(Game()) || Game().ui.screen == Screen::Map)
		return;

	if (Game().ui.screen == Screen::Journal) {
		Game().ui.journal.SpecialKeyPressed(key);
		return;
	}

	if (Game().ui.screen == Screen::Inventory && key != SPECIAL_SHIFT_LEFT && key != SPECIAL_SHIFT_RIGHT) {
		Game().ui.inventory->SpecialKeyPressed(key);
		return;
	}

	if (key == SPECIAL_TOGGLE_CARTOON)
		Ink::setToon(!Ink::toon());
	if (key == SPECIAL_TOGGLE_HITBOXES)
		Game().render.Hitboxes = !Game().render.Hitboxes;

	if (ScreenState::IsGameplayInteractionAllowed(Game()))
		setWalkHeld(MapSpecialGameplayAction(key), true);

	if (key == SPECIAL_CAMERA_LEFT) {
		PlayerActionController::applyCameraDelta(-CAMERA_ROTATE_STEP, 0);
	}
	if (key == SPECIAL_CAMERA_RIGHT) {
		PlayerActionController::applyCameraDelta(CAMERA_ROTATE_STEP, 0);
	}
	if (key == SPECIAL_CAMERA_UP) {
		PlayerActionController::applyCameraDelta(0, CAMERA_ROTATE_STEP);
	}
	if (key == SPECIAL_CAMERA_DOWN) {
		PlayerActionController::applyCameraDelta(0, -CAMERA_ROTATE_STEP);
	}

	if (key == SPECIAL_INTERACT) {
		PlayerActionController::execute(GameplayAction::Interact);
	}

	if (key == SPECIAL_SHIFT_LEFT || key == SPECIAL_SHIFT_RIGHT)
		Game().player->stats.SetSprintRequested(true);
}

// Released on every screen: a walk key let go in the inventory must not keep walking afterwards.
void keyReleased(unsigned char key, int x, int y) {
	(void)x;
	(void)y;
	setWalkHeld(MapKeyboardGameplayAction(key), false);
}

void specialKeyReleased(int key, int x, int y) {
	(void)x;
	(void)y;
	setWalkHeld(MapSpecialGameplayAction(key), false);

	if (key == SPECIAL_SHIFT_LEFT || key == SPECIAL_SHIFT_RIGHT)
		Game().player->stats.SetSprintRequested(false);
}

void processMouse(int button, int state, int x, int y) {
	if (ScreenState::ShouldRouteMouseToMenu(Game())) {
		Game().ui.menu.MouseFunction(button, state, x, y);
		return;
	}

	if (ScreenState::ShouldRouteMouseToInventory(Game())) {
		Game().ui.inventory->MouseFunction(button, state, x, y);
		return;
	}

	if (Game().ui.screen == Screen::Journal) {
		Game().ui.journal.MouseFunction(button, state, x, y);
		return;
	}

	if (state && Game().ui.screen != Screen::Map && ScreenState::IsGameplayInteractionAllowed(Game())) {
		PlayerActionController::execute(MapMouseGameplayAction(button));
	}
}
void processMousePassiveMotion(int a, int b) {
	if (ScreenState::ShouldRouteMouseToMenu(Game())) {
		Game().ui.menu.MousePassiveMotion(a, b);
		return;
	}

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
	if (ScreenState::ShouldRouteMouseToInventory(Game()))
		Game().ui.inventory->MouseMotion(a, b);
	if (Game().ui.screen == Screen::Journal)
		Game().ui.journal.MouseMotion(a, b);
}

void processMouseEntry(int a) { (void)a; }
