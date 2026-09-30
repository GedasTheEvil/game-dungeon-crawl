#include "../input/input.h"
#include "../test/scenario.h"
#include <GL/gl.h>
#include "../state/game_state.h"
#include "../ui/screen_state.h"
#include "../ui/player_hud.h"
#include "../ui/inventory.h"
#include "../ui/level_gem.h"
#include "../ui/boss_bar.h"
#include "../ui/status_box.h"
#include "lighting.h"
#include "ink.h"
#include "fire.h"
#include "gl_includes.h"
#include "render_config.h"
#include <algorithm>
#include <optional>
#include <string>

namespace {
constexpr float SCENE_NEAR = 10.f;
constexpr float SCENE_FAR = 300.f;
constexpr float PLAYER_CLIMB_ROT = 180.f; // back to the camera
// Towards the back wall so the fists close round the rungs: the ladder's rungs are 2.35 in front of the wall
// (tools/blender/models/ladder.py), the fists ~1.7 in front of the model's centre (CLIMB_GRIP_Y in archeologist.py).
constexpr float PLAYER_CLIMB_DEPTH = -16.f;
// Of a swing's time to its hit: raising the weapon back, the rest bringing it down (WeaponMotion).
constexpr float WINDUP_SHARE = 0.6f;
constexpr float WINDUP_PULL = 0.3f; // a thrust draws back this share of its reach first

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

bool holdingBow() { return Game().ui.inventory->EquippedType() == ItemType::RANGED_WEAPON; }

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
	const SwingPose pose = swingPose(weapon->motion);
	const std::array<float, 3> fist = Game().player->Fist(facing);
	const float length = weapon->scale * Ink::figureScale(); // Centrify: the largest dimension is 1
	// The pickups spin (rotA, shared model); held, the flat side faces the camera, the bow's back the enemy.
	const float spin = weapon->rotA;
	weapon->rotA = facing > 0 ? 0.f : 180.f;
	glPushMatrix();
	glTranslatef(fist[0], fist[1], fist[2] + Item::DRAW_DEPTH);
	glRotatef(-dir * pose.tilt, 0, 0, 1);
	glTranslatef(0, (pose.thrust - weapon->motion.grip) * length, 0);
	weapon->Draw(pose.draw);
	glPopMatrix();
	weapon->rotA = spin;
}

// The attack's hit time: a melee hit lands (its sound only if it hit something), or the arrow leaves.
void updateAttack() {
	Player& player = *Game().player;
	if (player.attackStartMs < 0)
		return;
	Item* weapon = Game().ui.inventory->Equipped();
	const int t = GameClock::now() - player.attackStartMs;
	if (!player.attackLanded && t >= weapon->motion.hitMs) {
		player.attackLanded = true;
		const int damage = player.stats.Damage();
		const float aimRange = 0.1f * static_cast<float>(weapon->range);
		if (holdingBow()) {
			Game().dungeon.ShootArrow(damage, Game().camera.Facing(),
									  player.Fist(Game().camera.Facing())[1] / RenderConfig::TILE_SIZE, aimRange);
			weapon->strikeSound.Play();
		} else if (Game().dungeon.AttackNearest(damage, weapon->range, Game().camera.Facing()))
			weapon->strikeSound.Play();
	}
	if (t >= weapon->motion.swingMs)
		player.attackStartMs = -1;
}

// A quick slot: the potion H / 0 would drink now, empty when none of that kind is left.
PlayerHud::Slot quickSlot(QuickKind kind, const char* key) {
	const Inventory& inventory = *Game().ui.inventory;
	PlayerHud::Slot slot;
	slot.key = key;
	int potion = inventory.QuickChoice(kind);
	if (potion != NO_POTION) {
		slot.icon = PlayerHud::Icon::Potion;
		slot.tint = Inventory::PotionColor(potion);
		slot.count = inventory.Count(ItemType::POTION, potion);
	}
	if (std::optional<int> drunk = inventory.QuickDrinkMs(kind))
		slot.flashAgeMs = GameClock::now() - *drunk;
	return slot;
}

PlayerHud::Icon weaponIcon(const Inventory& inventory) {
	if (inventory.EquippedType() == ItemType::RANGED_WEAPON)
		return PlayerHud::Icon::Bow;
	switch (inventory.EquippedId()) {
	case WeaponId::SWORD:
		return PlayerHud::Icon::Sword;
	case WeaponId::SPEAR:
		return PlayerHud::Icon::Spear;
	default:
		return PlayerHud::Icon::Club;
	}
}

PlayerHud::View playerHudView() {
	const PlayerStats& stats = Game().player->stats;
	PlayerHud::View view;
	view.hp = stats.CurrentHP();
	view.maxHp = stats.CurrentMaxHP();
	view.stamina = stats.Stamina();
	view.maxStamina = stats.MaxStamina();
	if (std::optional<int> refused = stats.StaminaRefusedMs())
		view.staminaRefusedAgeMs = GameClock::now() - *refused;
	double levelStart = PlayerStats::LevelXP(stats.CurrentLevel());
	double levelEnd = PlayerStats::LevelXP(stats.CurrentLevel() + 1);
	view.xpRatio = static_cast<float>((stats.CurrentXP() - levelStart) / (levelEnd - levelStart));
	view.keysHeld = Game().dungeon.KeysHeld();
	view.levelKeys = Game().dungeon.LevelKeys();
	view.slots[0].icon = weaponIcon(*Game().ui.inventory);
	view.slots[0].key = "1-4";
	view.slots[1] = quickSlot(QuickKind::Health, "H");
	view.slots[2] = quickSlot(QuickKind::Stamina, "0");
	return view;
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
	glTranslatef(-200, 0.0, -10); // the player (drawn at x 0) at mapX

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
	PlayerHud::draw(playerHudView(), Game().render.resX, Game().render.resY, Game().assets.fonts.hudBody,
					Game().assets.fonts.hudSmall, Game().assets.textures.hudIcons.ID());
	LevelGem::draw(Game().curMap, Game().render.resX, Game().render.resY, Game().assets.fonts.hud);
	if (const Monster* boss = Game().dungeon.Boss())
		BossBar::draw(boss->Type()->name, static_cast<float>(boss->Health()) / static_cast<float>(boss->MaxHealth()),
					  Game().render.resX, Game().render.resY, Game().assets.fonts.status);

	glColor3f(1, 1, 1);

	if (!Game().statusTimer.TimePassed(true))
		StatusBox::draw(Game().status, GameClock::now() - Game().statusTimer.StartTime(), GameState::STATUS_MS,
						Game().render.resX, Game().render.resY, Game().assets.fonts.status);

	glFlush();

	Scenario::onFrameRendered();
	glutSwapBuffers();
}
