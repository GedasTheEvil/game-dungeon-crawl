#include "game_loop.h"
#include "game_state.h"
#include "../graphics/gl_includes.h"
#include "../graphics/render_config.h"
#include "../ui/inventory.h"
#include "../ui/player_hud.h"
#include "../ui/screen_state.h"

namespace {
constexpr float PLAYER_CLIMB_ROT = 180.f; // back to the camera
// Towards the back wall so the fists close round the rungs: the ladder's rungs are 2.35 in front of the wall
// (tools/blender/models/ladder.py), the fists ~1.7 in front of the model's centre (CLIMB_GRIP_Y in archeologist.py).
constexpr float PLAYER_CLIMB_DEPTH = -16.f;

bool holdingBow() { return isRanged(Game().ui.inventory->EquippedKind()); }

// The attack's hit time: a melee hit lands (its sound only if it hit something), or the arrow leaves.
void updateAttack() {
	Player& player = *Game().player;
	if (player.attackStartMs < 0)
		return;
	Item* weapon = Game().ui.inventory->Equipped();
	const int t = GameClock::now() - player.attackStartMs;
	if (!player.attackLanded && t >= weapon->motion.hitMs) {
		player.attackLanded = true;
		const int damage = player.stats.Damage(Game().ui.inventory->EquippedDamage());
		const float aimRange = weapon->Reach();
		if (holdingBow()) {
			Game().dungeon.ShootArrow(damage, Game().camera.Facing(),
									  player.Fist(Game().camera.Facing())[1] / RenderConfig::TILE_SIZE, aimRange);
			weapon->strikeSound.Play();
		} else if (Game().dungeon.AttackNearest(damage, weapon->Reach(), Game().camera.Facing()))
			weapon->strikeSound.Play();
	}
	if (t >= weapon->motion.swingMs)
		player.attackStartMs = -1;
}

} // namespace

void Update() {
	if (!Game().cacheLoaded) {
		Game().Load();
		glutPostRedisplay();
		return;
	}

	if (ScreenState::GetDrawScreen(Game()) != ScreenState::DrawScreen::Gameplay) {
		glutPostRedisplay();
		return;
	}

	Game().dungeon.Update();
	Game().player->rotA = Game().camera.rotW;

	if (Game().player->Alive()) {
		bool climbing = !Game().player->jump.jumping && Game().dungeon.PlayerOnLadder();
		Game().player->depthOffset = climbing ? PLAYER_CLIMB_DEPTH : 0.f;
		if (Game().player->jump.jumping)
			Game().player->setModelState(ModelState::Jump);
		else if (climbing) {
			Game().player->showClimb(Game().dungeon.ClimbPhase());
			Game().player->rotA = PLAYER_CLIMB_ROT;
		} else if (Game().timers.idleModel.TimePassed())
			Game().player->setModelState(ModelState::Idle);

		updateAttack();
	}

	Game().player->stats.UpdateStamina();
	Game().dungeon.AnimateMonsters();
	Game().player->Animate();
	PlayerHud::tick(Game().player->stats.CurrentHP(), Game().player->stats.CurrentMaxHP());
	glutPostRedisplay();
}
