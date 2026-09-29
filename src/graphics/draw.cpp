#include "../input/input.h"
#include "../test/scenario.h"
#include <GL/gl.h>
#include "../state/game_state.h"
#include "../ui/screen_state.h"
#include "hud.h"
#include "../ui/level_gem.h"
#include "../ui/status_box.h"
#include "lighting.h"
#include "ink.h"
#include "fire.h"
#include "gl_includes.h"
#include "render_config.h"
#include <algorithm>
#include <optional>
#include <string>

int weaponRot = 0;

namespace {
constexpr float SCENE_NEAR = 10.f;
constexpr float SCENE_FAR = 300.f;
constexpr float PLAYER_CLIMB_ROT = 180.f; // back to the camera
// Towards the back wall so the fists close round the rungs: the ladder's rungs are 2.35 in front of the wall
// (tools/blender/models/ladder.py), the fists ~1.7 in front of the model's centre (CLIMB_GRIP_Y in archeologist.py).
constexpr float PLAYER_CLIMB_DEPTH = -16.f;
constexpr float BOW_REACH = 0.25f; // player heights in front of the chest: the drawn bow

// Progress 0..1 of the level-up sun beam, or none when it is not showing.
std::optional<float> sunBeamProgress() {
	std::optional<int> at = Game().player->stats.LevelUpMs();
	if (!at || !Game().player->Alive())
		return std::nullopt;
	float p = static_cast<float>(GameClock::now() - *at) / static_cast<float>(SunBeam::DURATION_MS);
	if (p < 0.f || p >= 1.f)
		return std::nullopt;
	return p;
}

// Fists height above the feet, world units: the weapon is held here.
float handHeight() { return Game().player->scale * Ink::figureScale() / 4 * 3 + 0.27f; }

// 0..1 through the bow draw, or none when the bow is not being drawn.
std::optional<float> bowDraw() {
	if (Game().player->bowDrawMs < 0)
		return std::nullopt;
	return std::min(1.f,
					static_cast<float>(GameClock::now() - Game().player->bowDrawMs) / static_cast<float>(BOW_DRAW_MS));
}

bool holdingBow() { return Game().ui.inventory->EquippedType() == ItemType::RANGED_WEAPON; }

void drawWeapon() { // floats in front of the chest
	const float playerScale = Game().player->scale * Ink::figureScale();
	const auto facing = static_cast<float>(Game().camera.Facing());
	Item* weapon = Game().ui.inventory->Equipped();
	// The pickups spin (rotA, shared model); held, the flat side faces the camera, the bow's back the enemy.
	const float spin = weapon->rotA;
	weapon->rotA = facing > 0 ? 0.f : 180.f;
	glPushMatrix();
	if (holdingBow()) { // upright at arm's length, the grip in the fist; drawn on attack
		glTranslatef(facing * playerScale * BOW_REACH, handHeight() - weapon->scale * Ink::figureScale() / 2, 2);
		weapon->Draw(bowDraw().value_or(0.f));
	} else {
		glTranslatef(facing * playerScale / 20, handHeight(), 2);
		glRotatef(-facing * (45.f + static_cast<float>(weaponRot)), 0, 0, 1);
		weapon->Draw();
	}
	glPopMatrix();
	weapon->rotA = spin;
}

// The bow is drawn: the arrow leaves. Climbing or a weapon change on the way puts it down.
void releaseArrow(bool climbing) {
	if (bowDraw().value_or(0.f) < 1.f)
		return;
	Game().player->bowDrawMs = -1;
	if (climbing || !holdingBow())
		return;
	Game().dungeon.ShootArrow(Game().player->stats.Damage(), Game().camera.Facing(),
							  handHeight() / RenderConfig::TILE_SIZE);
	Game().player->PlayAttackSound();
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

		releaseArrow(climbing);

		if (Game().player->attacking) {
			if (Game().player->attackTimer.TimePassed() || weaponRot <= -40) {
				weaponRot = 70;
				Game().player->attacking = false;
			} else
				weaponRot -= 4;
		} else if (Game().timers.weaponRest.TimePassed())
			weaponRot = 0;
	}

	Game().player->stats.UpdateStamina();
	glutPostRedisplay();
}

void Draw() {
	if (!Game().cacheLoaded)
		return;

	switch (ScreenState::GetDrawScreen(Game())) {
	case ScreenState::DrawScreen::Menu:
		Game().ui.menu.Draw();
		return;
	case ScreenState::DrawScreen::Inventory:
		Game().ui.inventory->Draw();
		return;
	case ScreenState::DrawScreen::Riddle:
		Game().ui.riddle->Draw();
		return;
	case ScreenState::DrawScreen::Map:
		Game().ui.map.Draw();
		return;
	case ScreenState::DrawScreen::Gameplay:
		break;
	}

	Ink::begin(SCENE_NEAR, SCENE_FAR);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, static_cast<float>(Game().render.resX) / static_cast<float>(Game().render.resY), SCENE_NEAR,
				   SCENE_FAR);
	glMatrixMode(GL_MODELVIEW);

	glTranslatef(0, -20, -70);

	glRotatef(Game().camera.rotM, 0, 1, 0);
	glRotatef(Game().camera.rotN, 1, 0, 0);

	Lighting::begin();
	if (Game().player->Alive())
		Lighting::add(0, 16, -22, Lighting::PLAYER, 0); // just in front of the player's chest
	const std::optional<float> sunBeam = sunBeamProgress();
	if (sunBeam) {
		const float k = SunBeam::strength(*sunBeam);
		Lighting::add(0, 45, -30, {1.f * k, 0.83f * k, 0.45f * k, 150.f, 0.f}, 0); // the beam lights the room
	}

	glPushMatrix();
	glTranslatef(-202, 0.0, -10);

	Game().dungeon.Draw();

	glPopMatrix();

	Game().player->Draw();

	Lighting::setEmissive(true);
	if (Game().hasWon)
		Game().ui.endScreens->DrawWin();
	Lighting::setEmissive(false);

	if (Game().player->Alive()) {
		if (!Game().player->climbing()) // both hands on the ladder: no weapon
			drawWeapon();
		if (sunBeam)
			SunBeam::draw(0, 0, -30 + Game().player->depthOffset, *sunBeam);
	} else {
		Lighting::setEmissive(true);
		Game().ui.endScreens->DrawLose();
	}
	Lighting::end();
	Ink::end();

	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 100, 0, 100, -21, 21);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glEnable(GL_BLEND);

	Game().assets.textures.nullTex.Bind();
	Hud::drawPlayerBars(Game().player->stats.HealthRatio(), Game().player->stats.StaminaRatio());
	Hud::drawKeys(Game().dungeon.KeysHeld());
	LevelGem::draw(Game().curMap, Game().render.resX, Game().render.resY, Game().assets.fonts.hud);

	glColor3f(1, 1, 1);

	if (!Game().statusTimer.TimePassed(true))
		StatusBox::draw(Game().status, GameClock::now() - Game().statusTimer.StartTime(), GameState::STATUS_MS,
						Game().render.resX, Game().render.resY, Game().assets.fonts.status);

	glFlush();

	Scenario::onFrameRendered();
	glutSwapBuffers();
}
