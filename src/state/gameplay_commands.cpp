#include "gameplay_commands.h"
#include "game_state.h"
#include "ui_context.h"
#include "../level/camera_look.h"
#include "../level/gameplay_player.h"
#include "../level/gameplay_shared.h"
#include "../ui/screen_state.h"
#include <array>
#include <utility>

// The player's gameplay commands (plan game-state-split): what a key, a click or a scenario asks the player to do, and
// the per-tick steps of the keys held. The input module (input/input.cpp) maps keys to them.
namespace {
CameraLook cameraLook; // the look keys held, the time since the last look input
// The walk keys held down, in GameplayAction order from MoveLeft.
std::array<bool, 4> walkHeld{};
// This press of the attack key was refused on a ladder and told so: the key repeat does not tell it again.
bool attackRefusalTold = false;
// The attack key came up while a power hit charged: the next tick swings it, unless the key is down again by then.
bool chargeReleased = false;

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
		const float moveMultiplier = playerControl().MoveMultiplier();
		switch (action) {
		case GameplayAction::MoveLeft:
			walk(-PLAYER_MOVE_STEP * moveMultiplier, 0);
			face(-1);
			if (!Game().player->jump.jumping)
				Game().player->setModelState(ModelState::Move);
			break;
		case GameplayAction::MoveRight:
			walk(PLAYER_MOVE_STEP * moveMultiplier, 0);
			face(1);
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
			else
				played(playerControl().Press(Game().camera.Facing()));
			break;
		case GameplayAction::Interact:
			Game().dungeon.Interact();
			break;
		case GameplayAction::Drag:
			Game().dungeon.ToggleDrag(Game().camera.Facing());
			break;
		case GameplayAction::QuickHeal: // allowed during a swing: drinking does not change it
			if (!insane())
				Game().itemCommands.QuickDrink(QuickKind::Health);
			break;
		case GameplayAction::QuickStamina:
			if (!insane())
				Game().itemCommands.QuickDrink(QuickKind::Stamina);
			break;
		case GameplayAction::StaminaSkills:
			toggleStaminaSkills();
			break;
		case GameplayAction::QuickAntidote:
			if (!insane())
				Game().itemCommands.QuickAntidote();
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

	// The mouse turned the camera.
	static void applyCameraDelta(float deltaX, float deltaY) {
		const CameraLook::Angles turned = CameraLook::Clamp({Game().camera.rotM + deltaX, Game().camera.rotN + deltaY});
		Game().camera.rotM = turned.m;
		Game().camera.rotN = turned.n;
		cameraLook.Looked();
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
	// The player turns to walk dir (-1 / +1); dragging, they keep looking at the thing held (plan mirror-drag).
	static void face(int dir) {
		const int drag = Game().dungeon.DragFacing();
		Game().camera.rotW = (drag != 0 ? drag : dir) > 0 ? 70.f : -110.f;
	}

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

	static void walk(float dirX, float dirY) { playerControl().Walk(dirX, dirY); }

	// No switch while a swing, a bow draw or a power hit's charge is under way: it would hit with the other weapon.
	static void equipNext(bool ranged) {
		if (Game().player->attackStartMs < 0 && !Game().player->Charging())
			Game().itemCommands.EquipNext(ranged);
	}

  public:
	// The key came up while a power hit charged: the charged swing (a tap's a plain one).
	static void releaseCharge() {
		Sound::Stop(std::exchange(chargeChannel, -1));
		played(playerControl().ReleaseCharge());
	}

  private:
	inline static int chargeChannel = -1; // the charge's rising sound, stopped at the release

	// The sound of what the attack key began: the charge's rising sound, the power hit's heavy whoosh or the weapon's.
	static void played(PlayerControl::Began began) {
		switch (began) {
		case PlayerControl::Began::None:
			return;
		case PlayerControl::Began::Charge:
			chargeReleased = false;
			chargeChannel = Game().assets.sounds.powerCharge.Play();
			return;
		case PlayerControl::Began::ChargedSwing:
			Game().assets.sounds.powerSwing.Play();
			return;
		case PlayerControl::Began::Swing:
			Game().assets.items.Of(Game().bag.Equipped())->swingSound.Play();
			return;
		}
	}
};
} // namespace

bool isMoveAction(GameplayAction action) {
	return action == GameplayAction::MoveLeft || action == GameplayAction::MoveRight ||
		   action == GameplayAction::MoveDown || action == GameplayAction::MoveUp;
}

// The player's controls in the world (plan player-control-in-sim), shared with the unit tests' SimWorld.
PlayerControl playerControl() { return {Game().dungeon, *Game().player, Game().bag, Game().events, attackRefusalTold}; }

void executeGameplayAction(GameplayAction action) { PlayerActionController::execute(action); }

// The down key held is the crouch key too (plan crouch): Dungeon::updateCrouch crouches the player off a ladder.
void setWalkHeld(GameplayAction move, bool held) {
	if (isMoveAction(move))
		walkHeld[std::to_underlying(move) - std::to_underlying(GameplayAction::MoveLeft)] = held;
	if (move == GameplayAction::MoveDown)
		Game().player->crouchHeld = held;
}

void releaseWalk() {
	walkHeld.fill(false);
	cameraLook.Release();
	Game().player->crouchHeld = false;
}

void stepHeldWalk() {
	if (!ScreenState::IsGameplayInteractionAllowed(Game()))
		return;
	for (size_t i = 0; i < walkHeld.size(); i++)
		if (walkHeld[i])
			PlayerActionController::execute(
				static_cast<GameplayAction>(std::to_underlying(GameplayAction::MoveLeft) + i)
			);
}

void stepCameraLook() {
	Camera& camera = Game().camera;
	const CameraLook::Angles angles = cameraLook.Step(
		{camera.rotM, camera.rotN},
		UPDATE_TICK_MS,
		Game().settings.graphics.cameraRecenter
	);
	camera.rotM = angles.m;
	camera.rotN = angles.n;
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

bool refusedInsane() { return PlayerActionController::insane(); }

void turnCamera(float deltaX, float deltaY) { PlayerActionController::applyCameraDelta(deltaX, deltaY); }

void holdLook(CameraLook::Dir dir, bool down) { cameraLook.Hold(dir, down); }

void newAttackPress() { attackRefusalTold = false; }
