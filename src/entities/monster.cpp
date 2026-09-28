#include "monster.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../graphics/lighting.h"
#include "../graphics/ink.h"

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
	state = flies() ? ModelState::Idle : ModelState::Move;
	facing = 0;
	flight = Flight{};
	leap = Leap{};
	playback = kind.model.SpawnPlayback();
	if (!stepTimer)
		stepTimer.emplace(70);
	if (!attackTimer)
		attackTimer.emplace(800);
}

void Monster::Clear() {
	type = nullptr;
	col = -1;
	row = -1;
	health = 0;
}

bool Monster::sameRow(float py) const { return std::fabs(static_cast<float>(row) - py) < 0.8f; }

bool Monster::Nearby(float px, float py, int range) const {
	return std::fabs(CentreX() - px) <= 0.1f * static_cast<float>(range) &&
		   std::fabs(static_cast<float>(row) - py) < 0.7f;
}

bool Monster::takeHit(int dmg) {
	const int scale = static_cast<int>(type->scale);
	if (Alive()) {
		health -= dmg;
		blood->setCords(static_cast<float>(random() % scale), static_cast<float>(random() % scale), 0);
		blood->Reset();
	}

	if (!Alive() && state != ModelState::Die) {
		enter(ModelState::Die);
		sprintf(GAME_STATE.status, "Gained %d XP", type->xp);
		GAME_STATE.status_timer->Reset();
		GAME_STATE.ui.stats->AddXP(type->xp);
		type->model.dieSound.Play();

		// Death blood effect, stronger than a hit.
		blood->setCords(static_cast<float>(random() % scale), static_cast<float>(random() % scale), 0);
		blood->Reset();
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

	GAME_STATE.textures.progBar.Bind();
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

void Monster::Draw(float px, float py) {
	const float scale = type->scale;
	glPushMatrix();
	glTranslatef(40 * x - 20, flies() ? flight.lift : leap.lift, -30);
	glPushMatrix(); // will add rotation

	if (Alive())
		drawHealthBar();

	glScalef(scale, scale, scale);

	auto drawBlood = [&] {
		glPushMatrix();
		glScalef(0.5f / scale, 0.5f / scale, 0.5f / scale);
		GAME_STATE.textures.nullTex.Bind();
		blood->Explode();
		blood->Fall();
		blood->Draw();
		glPopMatrix();
	};
	if (Alive()) {
		if (state == ModelState::Die) {
			if (flies())
				enter(flight.phase == FlightPhase::Roost ? ModelState::Idle : ModelState::Move);
			else if (!attackDirection(px, py))
				enter(ModelState::Attack);
			else
				enter(ModelState::Move);
		}
		drawBlood();
	} else
		enter(ModelState::Die);
	drawBlood(); // even when dead

	type->model.BindTexture();
	if (Alive()) {
		if (jumping())
			facing = leap.toX > leap.fromX ? 1 : -1;
		else if (!flies())
			facing = attackDirection(px, py);
		else
			facing = flight.phase == FlightPhase::Roost ? 0 : flight.dir;
	}
	glRotatef(type->rotA + 90.f * static_cast<float>(facing), 0, 1, 0);

	const float figure = Ink::figureScale();
	glScalef(figure, figure, figure);
	type->model.Show(state, playback);

	glPopMatrix();
	glPopMatrix();
	type->model.Advance(state, playback);
}
