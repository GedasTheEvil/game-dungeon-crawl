#include "monster.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "../state/game_state.h"
#include "../graphics/lighting.h"
#include "../graphics/ink.h"
#include "../graphics/render_config.h"
#include "../core/gameplay_config.h"

namespace {
// Health bar in world units (a tile is 40), the same for every monster.
constexpr float HEALTH_BAR_WIDTH = 14.f;
constexpr float HEALTH_BAR_HEIGHT = 1.5f;
constexpr float HEALTH_BAR_GAP = 3.f; // between the model and the bar
} // namespace

void Monster::Spawn(const MonsterType& kind, int spawnCol, int spawnRow) {
	type = &kind;
	if (!blood)
		blood = std::make_unique<ParticleSystem>();
	blood->setBloodColor(kind.blood.r, kind.blood.g, kind.blood.b);
	blood->Stop(); // no splash until the first hit
	col = spawnCol;
	row = spawnRow;
	health = kind.maxHealth;
	x = 0.f;
	alerted = false;
	minion = false;
	state = flies() || type->locomotion == Locomotion::Ambush ? ModelState::Idle : ModelState::Move;
	facing = 0;
	flight = Flight{};
	leap = Leap{};
	playback = kind.model.SpawnPlayback(Game().random.effects);
	attackTimer.SetInterval(kind.attackMs); // a slot can respawn another kind
	if (!spawned) {
		stepTimer.Reset();
		attackTimer.Reset();
		spawned = true;
	}
}

void Monster::Clear() {
	type = nullptr;
	col = -1;
	row = -1;
	health = 0;
}

bool Monster::LeavesChest() const {
	return type->locomotion == Locomotion::Ambush && !Alive() && state == ModelState::Die &&
		   type->model.Finished(state, playback);
}

bool Monster::sameRow(float py) const { return std::fabs(static_cast<float>(row) - py) < 0.8f; }

float Monster::HalfWidth() const {
	return type->model.HalfWidth() * type->scale * Ink::figureScale() / RenderConfig::TILE_SIZE;
}

float Monster::MeleeGap(float px, int dir) const {
	return (NearEdge(dir) - px) * static_cast<float>(dir) - Game().player->HalfWidth();
}

bool Monster::Nearby(float px, float py, float reach, int dir) const {
	const float behind = (FarEdge(dir) - px) * static_cast<float>(dir); // < 0: the far edge is behind the player
	return MeleeGap(px, dir) <= reach && behind >= -MELEE_REACH_BEHIND &&
		   std::fabs(static_cast<float>(row) - py) < 0.7f;
}

float Monster::BottomY() const {
	const bool roosting = flies() && flight.phase == FlightPhase::Roost;
	const float lift = flies() ? std::max(flight.lift, 0.f) : leap.lift;
	const float bottom = roosting ? type->model.idleBottom * type->scale * Ink::figureScale() : 0.f;
	return static_cast<float>(row) + (lift + bottom) / RenderConfig::TILE_SIZE;
}

float Monster::TopY() const {
	const bool roosting = flies() && flight.phase == FlightPhase::Roost;
	const float lift = flies() ? std::max(flight.lift, 0.f) : leap.lift;
	const float top = (roosting ? type->model.idleTop : type->model.referenceTop) * type->scale * Ink::figureScale();
	return static_cast<float>(row) + (lift + top) / RenderConfig::TILE_SIZE;
}

bool Monster::takeHit(int dmg) {
	const int scale = static_cast<int>(type->scale);
	alerted = true;
	if (Alive()) {
		health -= dmg;
		blood->Splash(scale);
	}

	if (!Alive() && state != ModelState::Die) {
		enter(ModelState::Die);
		const int xp = minion ? Game().dungeon.MinionXP(type->xp) : type->xp;
		Game().ShowStatus("Gained %d XP", xp);
		Game().player->stats.AddXP(xp);
		type->model.dieSound.Play();

		// Death blood effect, stronger than a hit.
		blood->Splash(scale);
		for (int i = 0; i < 6; i++)
			blood->Explode();
	}

	return Alive();
}

void Monster::drawHealthBar() {
	// Above the model's frame 0 top; a roosting flyer's bar hangs under it (the ceiling is above).
	const float drawScale = type->scale * Ink::figureScale();
	float y = type->model.referenceTop * drawScale + HEALTH_BAR_GAP;
	if (flies() && flight.phase == FlightPhase::Roost)
		y = type->model.idleBottom * drawScale - HEALTH_BAR_GAP - HEALTH_BAR_HEIGHT;
	glPushMatrix();
	glTranslatef(0, y, 0);
	// Billboard: keep where the anchor is, drop the camera and model rotation, keep the scene's scale.
	GLfloat mv[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, mv);
	float s = std::sqrt(mv[0] * mv[0] + mv[1] * mv[1] + mv[2] * mv[2]);
	for (int c = 0; c < 3; c++)
		for (int r = 0; r < 3; r++)
			mv[c * 4 + r] = c == r ? s : 0.f;
	glLoadMatrixf(mv);

	Game().assets.textures.loadingBar.Bind();
	Lighting::setEmissive(true);
	float w = HEALTH_BAR_WIDTH / 2;
	float h = HEALTH_BAR_HEIGHT;
	float o = 0.1f; // outline just outside the bar
	glColor4f(1, 1, 1, 0.9);
	glBegin(GL_LINE_LOOP);
	glVertex3f(-w - o, -o, 0);
	glVertex3f(w + o, -o, 0);
	glVertex3f(w + o, h + o, 0);
	glVertex3f(-w - o, h + o, 0);
	glEnd();

	glEnable(GL_BLEND);
	float ratio = type->maxHealth <= 0
					  ? 0.f
					  : std::clamp(static_cast<float>(health) / static_cast<float>(type->maxHealth), 0.f, 1.f);
	float right = -w + 2 * w * ratio;
	glColor3f(3 * (1 - ratio), 3 * ratio, 0);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex3f(-w, 0, 0);
	glTexCoord2f(1, 0);
	glVertex3f(right, 0, 0);
	glTexCoord2f(1, 1);
	glVertex3f(right, h, 0);
	glTexCoord2f(0, 1);
	glVertex3f(-w, h, 0);
	glEnd();
	glDisable(GL_BLEND);

	Lighting::setEmissive(false);
	glColor3f(1, 1, 1);
	glPopMatrix();
}

// The blood runs twice a tick while alive (and is drawn twice), once when dead: as it always has.
void Monster::Animate(float px, float py) {
	if (Alive()) {
		if (state == ModelState::Die) {
			if (flies())
				enter(flight.phase == FlightPhase::Roost ? ModelState::Idle : ModelState::Move);
			else if (lurking())
				enter(ModelState::Idle);
			else if (!attackDirection(px, py))
				enter(ModelState::Attack);
			else
				enter(ModelState::Move);
		}
		blood->Explode();
		blood->Fall();
	} else
		enter(ModelState::Die);
	blood->Explode();
	blood->Fall();

	if (Alive()) {
		if (jumping())
			facing = leap.toX > leap.fromX ? 1 : -1;
		else if (lurking())
			facing = 0; // a chest doesn't turn to look at the player
		else if (!flies()) {
			facing = attackDirection(px, py);
			if (facing == 0 && sameRow(py) && !rooted()) // biting: turned to the player, the jaws at them (the box)
				facing = px < CentreX() ? -1 : 1;
		} else
			facing = flight.phase == FlightPhase::Roost ? 0 : flight.dir;
	}
	type->model.Advance(state, playback);
}

void Monster::Draw() {
	const float scale = type->scale;
	glPushMatrix();
	glTranslatef(40 * x - 20, flies() ? flight.lift : leap.lift, -30);
	glPushMatrix(); // will add rotation

	if (Alive() && alerted && !type->isBoss()) // idle monsters keep up the disguise; the boss's bar is on the HUD
		drawHealthBar();

	glScalef(scale, scale, scale);

	auto drawBlood = [&] {
		glPushMatrix();
		glScalef(0.5f / scale, 0.5f / scale, 0.5f / scale);
		Game().assets.textures.nullTex.Bind();
		blood->Draw();
		glPopMatrix();
	};
	if (Alive())
		drawBlood();
	drawBlood(); // even when dead

	type->model.BindTexture();
	glRotatef(type->rotA + 90.f * static_cast<float>(facing), 0, 1, 0);

	const float figure = Ink::figureScale();
	glScalef(figure, figure, figure);
	type->model.Show(state, playback);

	glPopMatrix();
	glPopMatrix();
}
