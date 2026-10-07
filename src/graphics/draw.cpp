#include "draw.h"
#include "../test/scenario.h"
#include <GL/gl.h>
#include "../state/game_state.h"
#include "../ui/screen_state.h"
#include "../ui/player_hud.h"
#include "../ui/player_hud_view.h"
#include "../ui/inventory.h"
#include "../ui/level_gem.h"
#include "../ui/boss_bar.h"
#include "../ui/status_box.h"
#include "lighting.h"
#include "ink.h"
#include "motion_fx.h"
#include "fire.h"
#include "gl_includes.h"
#include "render_config.h"
#include <algorithm>
#include <optional>
#include <string>

namespace {
constexpr float SCENE_NEAR = 10.f;
constexpr float SCENE_FAR = 300.f;
// Of a swing's time to its hit: raising the weapon back, the rest bringing it down (WeaponMotion).
constexpr float WINDUP_SHARE = 0.6f;
constexpr float WINDUP_PULL = 0.3f; // a thrust draws back this share of its reach first

// A point of the current model view on the window, as a share of its size (y up).
struct ScreenPoint {
	float x, y;
};
ScreenPoint toScreen(double x, double y, double z) {
	GLdouble model[16], projection[16];
	GLint viewport[4];
	glGetDoublev(GL_MODELVIEW_MATRIX, model);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glGetIntegerv(GL_VIEWPORT, viewport);
	GLdouble wx = 0, wy = 0, wz = 0;
	if (gluProject(x, y, z, model, projection, viewport, &wx, &wy, &wz) == GL_FALSE || viewport[2] <= 0 ||
		viewport[3] <= 0)
		return {0.5f, 0.5f};
	return {static_cast<float>((wx - viewport[0]) / viewport[2]), static_cast<float>((wy - viewport[1]) / viewport[3])};
}

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

float smooth(float k) { return k * k * (3.f - 2.f * k); }

float mix(float a, float b, float k) { return a + (b - a) * k; }

// Where the swing has the weapon (WeaponMotion): its tilt, its thrust in lengths and the bow's draw 0..1.
struct SwingPose {
	float tilt, thrust, draw;
};

SwingPose swingPose(const WeaponMotion& m) {
	const int start = Game().player->attackStartMs;
	if (start < 0)
		return {m.restTilt, 0.f, 0.f};
	const auto t = static_cast<float>(GameClock::now() - start);
	const auto hit = static_cast<float>(m.hitMs);
	const float windup = hit * WINDUP_SHARE;
	const float pull = -WINDUP_PULL * m.thrust;
	if (t < windup) {
		const float k = smooth(t / windup);
		return {mix(m.restTilt, m.windupTilt, k), mix(0.f, pull, k), t / hit};
	}
	if (t < hit) {
		float k = (t - windup) / (hit - windup);
		k *= k; // gathering speed down to the strike
		return {mix(m.windupTilt, m.strikeTilt, k), mix(pull, m.thrust, k), t / hit};
	}
	const auto recover = static_cast<float>(m.swingMs - m.hitMs);
	const float k = recover > 0.f ? smooth(std::min(1.f, (t - hit) / recover)) : 1.f;
	return {mix(m.strikeTilt, m.restTilt, k), mix(m.thrust, 0.f, k), 0.f};
}

void drawWeapon() { // in the fist nearer the camera
	const int facing = Game().camera.Facing();
	const auto dir = static_cast<float>(facing);
	Item* weapon = Game().ui.inventory->Equipped();
	const Player& player = *Game().player;
	if (isThrown(Game().ui.inventory->EquippedKind()) && player.attackStartMs >= 0 && player.attackLanded)
		return; // in the air: the hand is empty until the swing ends
	const SwingPose pose = swingPose(weapon->motion);
	const std::array<float, 3> fist = Game().player->Fist(facing);
	const float length = weapon->scale * Ink::heldWeaponScale(); // Centrify: the largest dimension is 1
	// The pickups spin (rotA, shared model); held, the flat side faces the camera, the bow's back the enemy.
	const float spin = weapon->rotA;
	weapon->rotA = facing > 0 ? 0.f : 180.f;
	glPushMatrix();
	glTranslatef(fist[0], fist[1], fist[2] + Item::DRAW_DEPTH);
	glRotatef(-dir * pose.tilt, 0, 0, 1);
	glTranslatef(0, (pose.thrust - weapon->motion.grip) * length, 0);
	weapon->DrawHeld(pose.draw);
	glPopMatrix();
	weapon->rotA = spin;
}

} // namespace

namespace {
void drawGameplay();
} // namespace

// Whatever the screen, the frame ends here: the scenario runner takes its screenshot, then the buffers swap.
void Draw() {
	if (!Game().cacheLoaded)
		return;

	switch (ScreenState::GetDrawScreen(Game())) {
	case Screen::Menu:
		Game().ui.menu.Draw();
		break;
	case Screen::Inventory:
		Game().ui.inventory->Draw();
		break;
	case Screen::Riddle:
		Game().ui.riddle->Draw();
		break;
	case Screen::Map:
		Game().ui.map.Draw();
		break;
	case Screen::Journal:
		Game().ui.journal.Draw();
		break;
	case Screen::Gameplay:
		drawGameplay();
		break;
	}
	glFlush();
	Scenario::onFrameRendered();
	glutSwapBuffers();
}

namespace {
void drawGameplay() {

	const int resX = Game().render.resX;
	const int resY = Game().render.resY;
	MotionFx::update(Game().settings.graphics.motionEffects && Game().player->stats.IsSprinting(), GameClock::now());
	MotionFx::begin(resX, resY, !Ink::toon()); // toon mode: no blur over the ink lines (yet), the rest stays
	Ink::begin(SCENE_NEAR, SCENE_FAR, resX, resY);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(MotionFx::fov(45.0f), static_cast<float>(resX) / static_cast<float>(resY), SCENE_NEAR, SCENE_FAR);
	glMatrixMode(GL_MODELVIEW);

	glTranslatef(0, -20, -70);

	glRotatef(Game().camera.rotM, 0, 1, 0);
	glRotatef(Game().camera.rotN, 1, 0, 0);
	const ScreenPoint chest = toScreen(0, 16, -22); // the player stays sharp in the blur

	Lighting::begin();
	if (Game().player->Alive())
		Lighting::add(0, 16, -22, Lighting::PLAYER, 0); // just in front of the player's chest
	const std::optional<float> sunBeam = sunBeamProgress();
	if (sunBeam) {
		const float k = SunBeam::strength(*sunBeam);
		Lighting::add(0, 45, -30, {1.f * k, 0.83f * k, 0.45f * k, 150.f, 0.f}, 0); // the beam lights the room
	}

	glPushMatrix();
	glTranslatef(-200, 0.0, -10); // the player (drawn at x 0) at mapX

	std::optional<HitboxView> hitboxes;
	if (Game().render.Hitboxes) {
		const Item* weapon = Game().ui.inventory->Equipped();
		hitboxes = HitboxView{weapon->Reach(), !isRanged(Game().ui.inventory->EquippedKind()), Game().camera.Facing()};
	}
	Game().dungeon.Draw(hitboxes ? &*hitboxes : nullptr);

	glPopMatrix();

	const float sink = Game().dungeon.PlayerSink(); // wading: down in the water's basin, the weapon with them
	glPushMatrix();
	glTranslatef(0, -sink, 0);
	Game().player->Draw(Game().assets.textures);

	Lighting::setEmissive(true);
	if (Game().dungeon.Won())
		Game().ui.endScreens->DrawWin();
	Lighting::setEmissive(false);

	if (Game().player->Alive() && !Game().player->climbing()) // both hands on the ladder: no weapon
		drawWeapon();
	glPopMatrix();
	glPushMatrix(); // the water over the player and the monsters in it, as the level is drawn
	glTranslatef(-200, 0.0, -10);
	Game().dungeon.DrawWater();
	glPopMatrix();

	if (Game().player->Alive()) {
		if (sunBeam)
			SunBeam::draw(0, 0, -30 + Game().player->depthOffset, *sunBeam);
	} else {
		Lighting::setEmissive(true);
		Game().ui.endScreens->DrawLose();
	}
	Lighting::end();
	Ink::end();
	MotionFx::end(chest.x, chest.y);

	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 100, 0, 100, -21, 21);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glEnable(GL_BLEND);

	Game().assets.textures.nullTex.Bind();
	PlayerHud::draw(playerHudView(), Game().render.resX, Game().render.resY, Game().assets.fonts.hudBody,
					Game().assets.fonts.hudSmall, Game().assets.textures.hudIcons.ID());
	LevelGem::draw(Game().dungeon.LevelNumber(), Game().render.resX, Game().render.resY, Game().assets.fonts.hud);
	if (const Monster* boss = Game().dungeon.Boss())
		BossBar::draw(boss->Type()->name, static_cast<float>(boss->Health()) / static_cast<float>(boss->MaxHealth()),
					  boss->Poisoned(), Game().render.resX, Game().render.resY, Game().assets.fonts.status);

	glColor3f(1, 1, 1);

	if (!Game().statusTimer.TimePassed(true))
		StatusBox::draw(Game().status, GameClock::now() - Game().statusTimer.StartTime(), GameState::STATUS_MS,
						Game().render.resX, Game().render.resY, Game().assets.fonts.status);
}
} // namespace
