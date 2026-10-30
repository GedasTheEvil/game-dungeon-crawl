#include <GL/gl.h>
#include <array>
#include <optional>
#include <utility>
#include "../graphics/gl_includes.h"
#include "../state/game_state.h"
#include "../graphics/ink.h"
#include "../graphics/frame_rate.h"
#include "input.h"
#include "input_actions.h"
#include "../ui/screen_state.h"

int lastMx = 0;
int lastMy = 0;

namespace {
// The walk keys held down, in GameplayAction order from MoveLeft.
std::array<bool, 4> walkHeld{};
// This press of the attack key was refused on a ladder and told so: the key repeat does not tell it again.
bool attackRefusalTold = false;
// The attack key came up while a power hit charged: the next tick swings it, unless the key is down again by then.
bool chargeReleased = false;

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
		if (held(action))
			return;
		// No sprint while wading, except with the Favour of Hapi skill or the Blessing of Hapi potion, nor in sand
		// (Dungeon::PlayerSprintMultiplier).
		const float moveMultiplier = Game().dungeon.PlayerSprintMultiplier(Game().player->stats.SprintMoveMultiplier());
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
			Game().dungeon.StartJump(Game().camera.Facing());
			break;
		case GameplayAction::Attack:
			if (Game().player->Charging())
				chargeReleased = false; // the key repeat: still held
			else if (!tryCharge())
				tryAttack();
			break;
		case GameplayAction::Interact:
			Game().dungeon.Interact();
			break;
		case GameplayAction::QuickHeal: // allowed during a swing: drinking does not change it
			if (!insane())
				Game().ui.inventory->QuickDrink(QuickKind::Health);
			break;
		case GameplayAction::QuickStamina:
			if (!insane())
				Game().ui.inventory->QuickDrink(QuickKind::Stamina);
			break;
		case GameplayAction::StaminaSkills:
			toggleStaminaSkills();
			break;
		case GameplayAction::QuickAntidote:
			if (!insane())
				Game().ui.inventory->QuickAntidote();
			break;
		case GameplayAction::EquipMelee:
			if (!insane())
				equipNext(false);
			break;
		case GameplayAction::EquipRanged:
			if (!insane())
				equipNext(true);
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

	static void toggleStaminaSkills() {
		Player& player = *Game().player;
		if (!player.stats.heka.HasStaminaSkill()) {
			Game().events.Status("No stamina skill learnt");
			return;
		}
		player.staminaSkills = !player.staminaSkills;
		Game().events.Status("Stamina skills %s", player.staminaSkills ? "on" : "off");
	}

	// Insanity (plan curses) locks the screens, the quick drinks and the weapon keys: true, with the status line why.
	static bool insane() {
		if (!Game().player->stats.curses.Has(Curse::Insanity))
			return false;
		Game().events.Status("Your mind is clouded");
		return true;
	}

  private:
	// Held by a crocodile or a scorpion (Dungeon::PlayerHeldBy; plans crocodile-hold-bite, giant-scorpion-grab): the
	// walk keys only turn the player, no drink, no interact. The jump and the attack refuse in the dungeon. True: the
	// action is refused.
	static bool held(GameplayAction action) {
		if (Game().dungeon.PlayerHeldBy() == nullptr)
			return false;
		switch (action) {
		case GameplayAction::MoveLeft:
			Game().camera.rotW = -110;
			return true;
		case GameplayAction::MoveRight:
			Game().camera.rotW = 70;
			return true;
		case GameplayAction::MoveUp:
		case GameplayAction::MoveDown:
		case GameplayAction::Interact:
		case GameplayAction::QuickHeal:
		case GameplayAction::QuickStamina:
		case GameplayAction::QuickAntidote:
			return true;
		default:
			return false;
		}
	}

	// Only a step that really moved the player counts for the sprint drain, and none in the water (no sprint there),
	// unless the Favour of Hapi skill or the Blessing of Hapi potion lets the player sprint in it (plan
	// water-blessing-potion), nor in sand (plan desert-chapter). Wading and sand slow the step (PlayerWalkFactor).
	static void walk(float dirX, float dirY) {
		const bool sprints = Game().dungeon.PlayerCanSprint();
		const JumpState& jump = Game().player->jump;
		const float factor = Game().dungeon.PlayerWalkFactor() *
							 Game().player->stats.WalkFactor(jump.jumping || jump.falling);
		if (Game().dungeon.Move(dirX * factor, dirY * factor) && sprints)
			Game().player->stats.NoteWalked(Game().dungeon.PlayerHalvedWadeDrain());
	}

	// No switch while a swing, a bow draw or a power hit's charge is under way: it would hit with the other weapon.
	static void equipNext(bool ranged) {
		if (Game().player->attackStartMs < 0 && !Game().player->Charging())
			Game().ui.inventory->EquipNext(ranged);
	}

	// The swing (or the bow draw) begins; it hits when its hit time comes (updateAttack in draw.cpp).
	static void tryAttack() {
		if (!attackReady())
			return;
		Player& player = *Game().player;
		if (player.stats.curses.Has(Curse::Palsy) && isRanged(Game().ui.inventory->EquippedKind())) {
			if (!attackRefusalTold)
				Game().events.Status("Your hands shake too much to aim");
			attackRefusalTold = true; // no shot, no ammo, no cooldown
			return;
		}
		// No shot left, or a monster too close (plan ranged-weaker-than-melee): no draw, nothing spent.
		const Dungeon::ShotRefusal refused = Game().dungeon.ShotRefused(
			Game().ui.inventory->EquippedKind(),
			Game().camera.Facing()
		);
		if (refused != Dungeon::ShotRefusal::None) {
			Game().dungeon.RefuseShot(refused);
			return;
		}
		player.swingPower = 0;
		swing(false);
	}

  public:
	// Power hit (plan skill-power-hit): with the skill, the toggle on and a melee weapon in hand, the attack key down
	// starts the charge instead of the swing. False: no charge (too little stamina for one: the HUD sign blinks), the
	// attack runs as before.
	static bool tryCharge() {
		Player& player = *Game().player;
		if (!player.CanCharge(isRanged(Game().ui.inventory->EquippedKind())) || player.attackStartMs >= 0 ||
			!player.attackTimer.TimePassed(true))
			return false;
		if (!attackReady())
			return true; // refused (a ladder, a hold), told: nothing more
		if (!player.StartCharge())
			return false;
		chargeReleased = false;
		chargeChannel = Game().assets.sounds.powerCharge.Play();
		return true;
	}

	// The key came up: the charged swing (a tap's a plain one), from the windup the charge held it at.
	static void releaseCharge() {
		Player& player = *Game().player;
		const bool charged = player.ReleaseCharge() > 0;
		Sound::Stop(std::exchange(chargeChannel, -1));
		if (!player.Alive() || !attackReady())
			return;
		swing(charged);
	}

  private:
	inline static int chargeChannel = -1; // the charge's rising sound, stopped at the release

	// No swing under way, its recovery over, and allowed here (not on a ladder, not held: told once a press).
	static bool attackReady() {
		Player& player = *Game().player;
		if (player.attackStartMs >= 0 || !player.attackTimer.TimePassed(true))
			return false;
		if (!Game().dungeon.AttackAllowed(!attackRefusalTold)) {
			attackRefusalTold = true;
			return false;
		}
		return true;
	}

	// charged: a power hit, already wound up (it starts at the windup), its heavy whoosh.
	static void swing(bool charged) {
		Player& player = *Game().player;
		const Item* weapon = Game().ui.inventory->Equipped();
		const WeaponMotion& motion = weaponDef(Game().ui.inventory->EquippedKind()).motion;
		player.attackTimer.Reset();
		player.attackTimer.SetInterval(motion.AttackMs());
		player.attackStartMs = GameClock::now() - (charged ? windupMs(motion) : 0);
		player.attackLanded = false;
		if (charged)
			Game().assets.sounds.powerSwing.Play();
		else
			weapon->swingSound.Play();
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
				static_cast<GameplayAction>(static_cast<size_t>(GameplayAction::MoveLeft) + i)
			);
}

void releaseAttack() {
	attackRefusalTold = false;
	chargeReleased = Game().player->Charging();
}

void updatePowerCharge() {
	Player& player = *Game().player;
	if (!player.Charging())
		return;
	if (!player.Alive())
		chargeReleased = true; // no swing (releaseCharge)
	player.UpdateCharge();
	if (std::exchange(chargeReleased, false))
		PlayerActionController::releaseCharge();
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

std::string keyCapOf(BindAction action) {
	const InputKey* key = Game().settings.controls.Of(action).First();
	return key ? keyCap(*key) : "";
}

// "1-2" while the melee and ranged keys are two keys in a row, else their first keys one after the other.
std::string equipKeysCap() {
	const Bindings& controls = Game().settings.controls;
	const InputKey* melee = controls.Of(BindAction::EquipMelee).First();
	const InputKey* ranged = controls.Of(BindAction::EquipRanged).First();
	if (melee != nullptr && ranged != nullptr && melee->kind == InputKey::Kind::Char &&
		ranged->kind == InputKey::Kind::Char && ranged->code == melee->code + 1)
		return keyCap(*melee) + "-" + keyCap(*ranged);
	return keyCapOf(BindAction::EquipMelee) + keyCapOf(BindAction::EquipRanged);
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
		if (screen == Screen::Gameplay && PlayerActionController::insane()) {
			Game().ApplyWorldEvents();
			return true;
		}
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
	if (skipCutScene())
		return;
	if (ScreenState::ShouldRouteKeyboardToRiddle(Game())) {
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

	if (key == KEY_ESCAPE) // esc
	{
		// Esc backs out of the inventory or the map to the game; only from the game it opens the menu.
		if (ScreenTabs::Has(screen)) {
			screen = Screen::Gameplay;
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
	if (ScreenState::ShouldBlockKeyboardGameplay(Game()))
		return; // while the menu is shown, only [esc] and the sub-screen's keys are handled

	std::optional<BindAction> action = bound(InputKey::Char(key));
	if (action && pressBinding(*action))
		return;
	if (screen == Screen::Inventory)
		Game().ui.inventory->KeyPressed(key);
	if (screen == Screen::Skills)
		Game().ui.skills.KeyPressed(key);
}

void specialKeyPressed(int key, int x, int y) {
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
			Game().LoadQuicksave();
		else
			Game().ui.death.SpecialKeyPressed(key);
		return;
	}
	if (Game().ui.screen == Screen::Menu)
		Game().ui.menu.SpecialKeyPressed(key);
	if (ScreenState::ShouldBlockKeyboardGameplay(Game()))
		return;

	std::optional<BindAction> action = bound(InputKey::Special(key));
	if (action && pressBinding(*action))
		return;

	if (Game().ui.screen == Screen::Journal)
		Game().ui.journal.SpecialKeyPressed(key);
	if (Game().ui.screen == Screen::Inventory)
		Game().ui.inventory->SpecialKeyPressed(key);
	if (Game().ui.screen == Screen::Skills)
		Game().ui.skills.SpecialKeyPressed(key);
	if (Game().ui.screen != Screen::Gameplay)
		return;

	// Fixed keys (not bindable).
	if (key == SPECIAL_TOGGLE_CARTOON) {
		Game().settings.graphics.toon = !Game().settings.graphics.toon;
		Game().ApplySettings(true);
	}
	if (key == SPECIAL_TOGGLE_HITBOXES)
		Game().render.Hitboxes = !Game().render.Hitboxes;
	if (key == SPECIAL_QUICK_SAVE && !Game().ui.winEntry.Typing())
		Game().Quicksave();
	if (key == SPECIAL_QUICK_LOAD && !Game().ui.winEntry.Typing())
		Game().LoadQuicksave();
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
	static int skipButton = -1; // the click that skipped a cut-scene: its release does nothing either
	if (state == GLUT_DOWN && skipCutScene()) {
		skipButton = button;
		return;
	}
	if (state == GLUT_UP && button == skipButton) {
		skipButton = -1;
		return;
	}
	if (ScreenState::ShouldRouteMouseToMenu(Game())) {
		Game().ui.menu.MouseFunction(button, state, x, y);
		return;
	}
	if (Game().ui.screen == Screen::Death) {
		Game().ui.death.MouseFunction(button, state, x, y);
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
	if (Game().ui.screen == Screen::Skills) {
		Game().ui.skills.MouseFunction(button, state, x, y);
		return;
	}

	// The attack button down starts a power hit's charge (plan skill-power-hit), up swings it; without one the click
	// attacks when it comes up, as below.
	if (action == BindAction::Attack && Game().ui.screen == Screen::Gameplay) {
		if (state == GLUT_DOWN) {
			attackRefusalTold = false;
			if (ScreenState::IsGameplayInteractionAllowed(Game()) && PlayerActionController::tryCharge())
				Game().ApplyWorldEvents();
			return;
		}
		if (Game().player->Charging()) {
			releaseBinding(*action);
			return;
		}
	}
	// Held actions start when the button goes down, the others run when it comes up: each click is a press.
	if (action && Game().ui.screen == Screen::Gameplay && (state == GLUT_DOWN) == isHeld(*action)) {
		if (*action == BindAction::Attack)
			attackRefusalTold = false;
		pressBinding(*action);
	}
}
void processMousePassiveMotion(int a, int b) {
	if (ScreenState::ShouldRouteMouseToMenu(Game())) {
		Game().ui.menu.MousePassiveMotion(a, b);
		return;
	}
	if (Game().ui.screen == Screen::Death) {
		Game().ui.death.MouseMotion(a, b);
		lastMx = a; // no camera jump on the retry
		lastMy = b;
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
	if (Game().ui.screen == Screen::Skills)
		Game().ui.skills.MouseMotion(a, b);
	if (Game().ui.screen == Screen::Map || Game().ui.screen == Screen::Journal || Game().ui.screen == Screen::Skills) {
		lastMx = a;
		lastMy = b;
		return;
	}

	PlayerActionController::applyCameraDelta(
		-MOUSE_LOOK_SENSITIVITY * static_cast<float>(lastMx - a),
		-MOUSE_LOOK_SENSITIVITY * static_cast<float>(lastMy - b)
	);

	lastMx = a;
	lastMy = b;
}

void processMouseActiveMotion(int a, int b) {
	if (ScreenState::ShouldRouteMouseToMenu(Game())) {
		Game().ui.menu.MouseDrag(a, b);
		return;
	}
	if (Game().ui.screen == Screen::Death) {
		Game().ui.death.MouseMotion(a, b);
		return;
	}
	if (ScreenTabs::Has(Game().ui.screen))
		ScreenTabs::Motion(a, b);
	if (ScreenState::ShouldRouteMouseToInventory(Game()))
		Game().ui.inventory->MouseMotion(a, b);
	if (Game().ui.screen == Screen::Journal)
		Game().ui.journal.MouseMotion(a, b);
	if (Game().ui.screen == Screen::Skills)
		Game().ui.skills.MouseMotion(a, b);
}

void processMouseEntry(int a) { (void)a; }
