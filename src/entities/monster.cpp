#include "monster.h"
#include <GL/gl.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "player.h"
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

void Monster::Spawn(const MonsterType& kind, int spawnCol, int spawnRow, const MonsterLinks& world, Rng& effects) {
	links = world;
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
	summonMs = -1;
	state = flies() || lurking() ? ModelState::Idle : ModelState::Move;
	tomb = entombed() ? MUMMY_COFFIN_DEPTH : 0.f;
	inWater = false;
	headInWater = false;
	sink = 0.f;
	swim = 0.f;
	swimPlaced = false;
	facing = 0;
	flight = Flight{};
	leap = Leap{};
	burrow = Burrow{};
	charge = Charge{};
	trapHurt = TrapHurt{};
	trapDamageCarry = 0;
	spitReadyMs = 0;
	spitReleased = true;
	drop.reset();
	playback = kind.model.SpawnPlayback(effects);
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
	drop.reset();
}

void Monster::MakeMinion(Summon how) {
	minion = true;
	alerted = true;
	summonMs = GameClock::now();
	summonedBy = how;
	if (entombed() && how == Summon::Coffin) // lies in the coffin it was summoned into, climbs out (tomb, Rising)
		wake();
	else if (rises()) // no coffin to climb out of; a cobra summoned comes out reared up
		enter(ModelState::Move);
	if (flies() && how == Summon::Drop) { // falls out of the ceiling to where bats turn, flapping (emergeLift)
		flight.lift = BAT_HIGH_LIFT;
		enter(ModelState::Move);
	} else if (flies())
		flight.lift = roostLift();
}

bool Monster::Emerging() const {
	return summonMs >= 0 && summonedBy != Summon::Coffin && GameClock::now() - summonMs < MINION_EMERGE_MS;
}

// Digging out it rises from its full height under the floor, slowing at the top. Dropping it falls from the
// ceiling, where a roosting flyer hangs out of sight, speeding up.
float Monster::emergeLift() const {
	if (!Emerging())
		return 0.f;
	const float p = static_cast<float>(GameClock::now() - summonMs) / static_cast<float>(MINION_EMERGE_MS);
	if (summonedBy == Summon::Drop)
		return (BAT_CEILING - flight.lift) * (1.f - p * p);
	return -type->model.referenceTop * type->scale * Ink::figureScale() * (1.f - p) * (1.f - p);
}

float Monster::swimLift() const {
	if (!inWater || !headInWater || type->wading != Wading::Swimmer || !Alive())
		return 0.f;
	const float drawScale = type->scale * Ink::figureScale();
	const float top = lurking() ? type->model.idleTop * drawScale - SUBMERGED_SHOW
								: type->model.referenceTop * drawScale - 2.f * SUBMERGED_SHOW;
	return std::max(0.f, RenderConfig::WATER_DEPTH - top);
}

float Monster::burrowLift() const {
	const float depth = -type->model.referenceTop * type->scale * Ink::figureScale();
	const int age = GameClock::now() - burrow.startMs;
	switch (burrow.phase) {
	case BurrowPhase::Up:
		return 0.f;
	case BurrowPhase::Dive: {
		const float p = std::min(static_cast<float>(age) / static_cast<float>(BURROW_DIVE_MS), 1.f);
		return depth * p * p;
	}
	case BurrowPhase::Under:
		return depth;
	case BurrowPhase::Surface: {
		const float p = std::min(static_cast<float>(age) / static_cast<float>(BURROW_SURFACE_MS), 1.f);
		return depth * (1.f - p) * (1.f - p);
	}
	}
	return 0.f;
}

float Monster::lift() const {
	return (flies() ? std::max(flight.lift, 0.f) : leap.lift + swim - sink) + emergeLift() + burrowLift();
}

bool Monster::LeavesChest() const {
	return type->locomotion == Locomotion::Ambush && !Alive() && state == ModelState::Die &&
		   type->model.Finished(state, playback);
}

std::optional<ItemKind> Monster::TakeDrop() {
	if (!drop || Alive() || state != ModelState::Die || !type->model.Finished(state, playback))
		return std::nullopt;
	std::optional<ItemKind> d = drop;
	drop.reset();
	return d;
}

bool Monster::Rising() const {
	return rises() && Alive() && state == ModelState::Rise && !type->model.Finished(state, playback);
}

bool Monster::sameRow(float py) const { return std::fabs(static_cast<float>(row) - py) < 0.8f; }

float Monster::HalfWidth() const {
	return type->model.HalfWidth() * type->scale * Ink::figureScale() / RenderConfig::TILE_SIZE;
}

float Monster::MeleeGap(float px, int dir) const {
	return (NearEdge(dir) - px) * static_cast<float>(dir) - links.player->HalfWidth();
}

bool Monster::Nearby(float px, float py, float reach, int dir) const {
	const float behind = (FarEdge(dir) - px) * static_cast<float>(dir); // < 0: the far edge is behind the player
	return MeleeGap(px, dir) <= reach && behind >= -MELEE_REACH_BEHIND &&
		   std::fabs(static_cast<float>(row) - py) < 0.7f;
}

// A roosting flyer, a lurker under the water and a coiled one show their idle clip.
float Monster::BottomY() const {
	const bool idle = (flies() && flight.phase == FlightPhase::Roost) || ((submerged() || coiled()) && lurking());
	const float bottom = idle ? type->model.idleBottom * type->scale * Ink::figureScale() : 0.f;
	return static_cast<float>(row) + (lift() + bottom) / RenderConfig::TILE_SIZE;
}

float Monster::TopY() const {
	const bool idle = (flies() && flight.phase == FlightPhase::Roost) || ((submerged() || coiled()) && lurking());
	const float top = (idle ? type->model.idleTop : type->model.referenceTop) * type->scale * Ink::figureScale();
	return static_cast<float>(row) + (lift() + top) / RenderConfig::TILE_SIZE;
}

bool Monster::takeHit(int dmg) {
	const int scale = static_cast<int>(type->scale);
	if (lurking())
		wake();
	alerted = true;
	if (Alive()) {
		health -= dmg;
		blood->Splash(scale);
	}

	if (Alive() || state == ModelState::Die)
		return false;
	enter(ModelState::Die);
	type->model.dieSound.Play();

	// Death blood effect, stronger than a hit.
	blood->Splash(scale);
	for (int i = 0; i < 6; i++)
		blood->Explode();
	return true;
}

bool Monster::TakeWeaponHit(int dmg, const DamageMix& mix) {
	const int hit = resistedDamage(dmg, mix, type->resist);
	return takeHit(Stunned() ? hit * CHARGE_STUN_DAMAGE_FACTOR : hit);
}

void Monster::StandInTrap() {
	if (const int dmg = trapHurt.hit(); dmg > 0)
		TrapHit(dmg);
}

void Monster::TrapHit(int dmg) {
	const int hundredths = dmg * type->trapDamagePct + trapDamageCarry;
	trapDamageCarry = hundredths % 100;
	if (hundredths >= 100)
		takeHit(hundredths / 100); // no reward: the player must not farm kills with traps
}

void Monster::drawHealthBar(const Texture& bar) {
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

	bar.Bind();
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
		else if (Charging())
			facing = charge.dir; // runs on past the player
		else if ((submerged() && lurking()) || (coiled() && (lurking() || Rising())))
			facing = px < CentreX() ? -1 : 1; // lies along the row (in its coil), watching the player
		else if (lurking() || Rising())
			facing = 0; // a chest doesn't turn to look at the player, a mummy lies along its coffin
		else if (!flies()) {
			facing = attackDirection(px, py);
			if (facing == 0 && sameRow(py) && !rooted()) // biting: turned to the player, the jaws at them (the box)
				facing = px < CentreX() ? -1 : 1;
		} else
			facing = flight.phase == FlightPhase::Roost ? 0 : flight.dir;
	}
	type->model.Advance(state, playback);
	// A swimmer floats up as it wades in and sinks back to the floor on the bank; placed at once when it spawns.
	const float swimTarget = swimLift();
	if (!swimPlaced)
		swim = swimTarget;
	swimPlaced = true;
	swim += (swimTarget - swim) * std::min(1.f, SWIM_LIFT_RATE * static_cast<float>(UPDATE_TICK_MS) / 1000.f);
	if (Alive() && entombed() && !lurking()) { // climbing out; a mummy killed on the way stays where it fell
		const float t = state == ModelState::Rise ? type->model.Progress(state, playback) : 1.f;
		const float k = std::clamp((t - MUMMY_CLIMB_FROM) / (MUMMY_CLIMB_TO - MUMMY_CLIMB_FROM), 0.f, 1.f);
		tomb = MUMMY_COFFIN_DEPTH * (1.f - k * k * (3.f - 2.f * k));
	}
}

void Monster::Draw(const TextureRegistry& textures) {
	const float scale = type->scale;
	glPushMatrix();
	glTranslatef(RenderConfig::TILE_SIZE * x - RenderConfig::TILE_HALF,
				 (flies() ? flight.lift : leap.lift + swim - sink) + emergeLift() + burrowLift(), -30.f - tomb);
	glPushMatrix(); // will add rotation

	if (Alive() && alerted && !type->isBoss()) // idle monsters keep up the disguise; the boss's bar is on the HUD
		drawHealthBar(textures.loadingBar);

	glScalef(scale, scale, scale);

	auto drawBlood = [&] {
		glPushMatrix();
		glScalef(0.5f / scale, 0.5f / scale, 0.5f / scale);
		textures.nullTex.Bind();
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
