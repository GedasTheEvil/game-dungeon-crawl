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
#include "../ui/interact_prompt.h"
#include "../input/input_actions.h"
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
// The burning player's flames and their light, a little in front of the body (plan fire-damage).
constexpr float PLAYER_FLAMES_Z = -22.f;
constexpr Lighting::LightDef PLAYER_BURNING_LIGHT = {1.f, 0.55f, 0.2f, 80.f, 0.7f};

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

void drawWeapon() { // in the fist nearer the camera
	const int facing = Game().camera.Facing();
	const auto dir = static_cast<float>(facing);
	Item* weapon = Game().ui.inventory->Equipped();
	const Player& player = *Game().player;
	if (isThrown(Game().ui.inventory->EquippedKind()) && player.attackStartMs >= 0 && player.attackLanded)
		return; // in the air: the hand is empty until the swing ends
	const WeaponMotion& motion = weaponDef(Game().ui.inventory->EquippedKind()).motion;
	const SwingPose pose = swingPose(motion, player.attackStartMs < 0 ? -1 : GameClock::now() - player.attackStartMs);
	const std::array<float, 3> fist = Game().playerView.Fist(player, facing);
	const float length = weapon->scale * Ink::heldWeaponScale(); // Centrify: the largest dimension is 1
	// The pickups spin (rotA, shared model); held, the flat side faces the camera, the bow's back the enemy.
	const float spin = weapon->rotA;
	weapon->rotA = facing > 0 ? 0.f : 180.f;
	glPushMatrix();
	glTranslatef(fist[0], fist[1], fist[2] + Item::DRAW_DEPTH);
	glRotatef(-dir * pose.tilt, 0, 0, 1);
	glTranslatef(0, (pose.thrust - motion.grip) * length, 0);
	weapon->DrawHeld(pose.draw);
	glPopMatrix();
	weapon->rotA = spin;
}

void drawStatusLine() {
	if (!Game().statusTimer.TimePassed(true))
		StatusBox::draw(Game().status, GameClock::now() - Game().statusTimer.StartTime(), GameState::STATUS_MS,
						Game().render.resX, Game().render.resY, Game().assets.fonts.status);
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
	case Screen::Skills:
		Game().ui.skills.Draw();
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
	// A cut-scene (plan cinematic-effects): the camera is away from the player, the look comes in, the HUD goes.
	const CutShot shot = Game().dungeon.CutSceneShot();
	const bool cutScene = Game().dungeon.CutScenePlaying();
	MotionFx::cinema(shot.look);
	const float playerX = -shot.dx * RenderConfig::TILE_SIZE, playerY = -shot.dy * RenderConfig::TILE_SIZE;
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
	// The player stays sharp in the blur; the vignette of a cut-scene stays on the middle.
	const ScreenPoint chest = cutScene ? ScreenPoint{0.5f, 0.5f} : toScreen(0, 16, -22);

	Lighting::begin();
	if (Game().player->Alive())
		Lighting::add(playerX, playerY + 16, -22, Lighting::PLAYER, 0); // just in front of the player's chest
	if (cutScene) // the far gate is lit as the player would light it
		Lighting::add(0, 16, -22, Lighting::PLAYER, 0);
	if (Game().player->burning.Any())
		Lighting::add(playerX, playerY + 16, PLAYER_FLAMES_Z, PLAYER_BURNING_LIGHT, 1);
	const std::optional<float> sunBeam = sunBeamProgress();
	if (sunBeam) {
		const float k = SunBeam::strength(*sunBeam);
		Lighting::add(playerX, playerY + 45, -30, {1.f * k, 0.83f * k, 0.45f * k, 150.f, 0.f},
					  0); // the beam lights the room
	}

	glPushMatrix();
	glTranslatef(-200, 0.0, -10); // the player (drawn at x 0) at mapX

	std::optional<HitboxView> hitboxes;
	if (Game().render.Hitboxes) {
		const ItemKind kind = Game().ui.inventory->EquippedKind();
		hitboxes = HitboxView{weaponReach(kind, Game().player->stats.heka), !isRanged(kind), Game().camera.Facing()};
	}
	Game().dungeon.Draw(hitboxes ? &*hitboxes : nullptr);

	glPopMatrix();

	const float sink = Game().dungeon.PlayerSink(); // wading: down in the water's basin, the weapon with them
	glPushMatrix();
	glTranslatef(playerX, playerY - sink, 0);
	Game().playerView.Draw(*Game().player, Game().assets.textures);

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

	if (Game().player->burning.Any()) // plan fire-damage
		Fire::body(playerX, playerY - sink, PLAYER_FLAMES_Z + Game().player->depthOffset,
				   Game().player->HalfWidth() * RenderConfig::TILE_SIZE,
				   Game().player->Height() * RenderConfig::TILE_SIZE, 1);
	if (Game().player->Alive()) {
		if (sunBeam)
			SunBeam::draw(playerX, playerY, -30 + Game().player->depthOffset, *sunBeam);
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
	if (const float fade = Game().dungeon.TravelFade(); fade > 0.f) { // gate travel: the view fades into the plasma
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if (Game().dungeon.TravelToExit())
			glColor4f(1.f, 0.35f, 0.25f, fade);
		else
			glColor4f(0.35f, 0.65f, 1.f, fade);
		glBegin(GL_QUADS);
		glVertex2f(0, 0);
		glVertex2f(100, 0);
		glVertex2f(100, 100);
		glVertex2f(0, 100);
		glEnd();
		glColor3f(1, 1, 1);
		glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	}
	if (cutScene) { // the HUD is hidden; the status line stays, a caption
		glColor3f(1, 1, 1);
		drawStatusLine();
		return;
	}
	if (!Game().ui.winEntry.Typing()) // the win's name entry sits there
		PlayerHud::draw(playerHudView(), Game().render.resX, Game().render.resY, Game().assets.fonts.hudBody,
						Game().assets.fonts.hudSmall, Game().assets.textures.hudIcons.ID(),
						Game().assets.textures.skillIcons.ID());
	LevelGem::draw(Game().dungeon.LevelNumber(), Game().render.resX, Game().render.resY, Game().assets.fonts.hud);
	const bool prompt = Game().player->Alive() && !Game().dungeon.Won() && !Game().dungeon.Travelling();
	InteractPrompt::draw(prompt ? Game().dungeon.InteractVerb() : Interaction::None, keyCapOf(BindAction::Interact),
						 Game().render.resX, Game().render.resY, Game().assets.fonts.hudBody,
						 Game().assets.fonts.hudSmall);
	if (const Monster* boss = Game().dungeon.Boss())
		BossBar::draw(boss->Type()->name, static_cast<float>(boss->Health()) / static_cast<float>(boss->MaxHealth()),
					  boss->Poisoned(), Game().render.resX, Game().render.resY, Game().assets.fonts.status);

	glColor3f(1, 1, 1);

	drawStatusLine();
	Game().ui.winEntry.Draw(Game().render.resX, Game().render.resY);
}
} // namespace
