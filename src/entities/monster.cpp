#include "monster.h"
#include <GL/gl.h>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <string>

#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../core/logger.h"
#include "../graphics/lighting.h"

#ifdef WIN32

#include <cstdlib>

inline int random() { return rand(); }

#endif

namespace {
std::unique_ptr<AnimatedCartoonModel> makeModel(const char* path, GLuint texId, int speed) {
	auto m = std::make_unique<AnimatedCartoonModel>();
	m->Load(path);
	m->BindTexture(texId);
	m->setSpeed(speed);
	return m;
}
} // namespace

void monster::applyModelState(ModelState state) {
	const bool entering = state != currentState;
	selectModel(state);
	if (entering && !model->loop)
		model->Reset(); // one-shot clips (die, jump) play from the start
}
//================================================================================
void monster::selectModel(ModelState state) {
	currentState = state;
	const auto& c = clips[static_cast<int>(state)];
	model = c ? c.get() : clips[static_cast<int>(reference)].get();
}
//================================================================================
void monster::showClimb(float phase) {
	AnimatedCartoonModel* climb = clip(ModelState::Climb);
	if (!climb)
		return;
	selectModel(ModelState::Climb);
	int frames = climb->FrameCount();
	float frame = std::min(phase - std::floor(phase), 1.f) * static_cast<float>(frames);
	climb->SetPlayback({std::min(frame, static_cast<float>(frames) - 0.001f), 0});
}
//================================================================================
monster::monster() {
	mapX = 0;
	mapY = 0;
	speed = 1;
	health = 200;
	maxHealth = 200;
	damage = 1;
	XP = 1000;
	stat = -1;
	facing_dir = 0;
	scale = 1;
	currentState = ModelState::Move;
	model = nullptr;

	Att_timer = std::make_unique<timer>(1000);
	walk_timer = std::make_unique<timer>(40);

	ownBlood = std::make_unique<ParSys>(100);
	blood = ownBlood.get();
}
//================================================================================
monster::monster(float dx, float dy) {
	tileOriginX = dx;
	tileOriginY = dy;

	mapX = 0;
	mapY = 0;
	speed = 1;
	health = 20;
	maxHealth = 20;
	damage = 1;
	XP = 1000;
	stat = -1;
	facing_dir = 0;
	scale = 1;
	currentState = ModelState::Move;
	model = nullptr;
	Att_timer = std::make_unique<timer>(1000);
	walk_timer = std::make_unique<timer>(40);

	ownBlood = std::make_unique<ParSys>(100);
	blood = ownBlood.get();
}
//================================================================================
monster::monster(float nX, float nY, int nSpeed, int nHP, int nDamage, int nXP) {
	mapX = nX;
	mapY = nY;
	speed = nSpeed;
	maxHealth = nHP;
	health = nHP;
	damage = nDamage;
	XP = nXP;
	stat = -1;
	facing_dir = 0;
	currentState = ModelState::Move;
	model = nullptr;
	Att_timer = std::make_unique<timer>(1000);
	walk_timer = std::make_unique<timer>(40);

	ownBlood = std::make_unique<ParSys>(100);
	blood = ownBlood.get();
}
//================================================================================
monster::~monster() {
	stat = -1;
	void* selfPtr = this;
	LOG_DEBUGF("entities", "Deleting monster %p", selfPtr);
}
//================================================================================
bool monster::Draw() // needs to choose animation
{
	glPushMatrix();

	if (this != GAME_STATE.Player.get())
		glTranslatef(40 * mapX - 20, mapY + (flies ? flight.lift : 0.f), -30);
	else
		glTranslatef(0, 0, -30 + depthOffset);

	glPushMatrix(); // will add rotation

	glScalef(scale, scale, scale);

	if (Alive()) {
		if (currentState == ModelState::Die) {
			if (flies)
				applyModelState(flight.phase == FlightPhase::Roost ? ModelState::Idle : ModelState::Move);
			else if (!attackDirection())
				applyModelState(ModelState::Attack);
			else
				applyModelState(ModelState::Move);
		}

		if (this != GAME_STATE.Player.get()) {
			nullTexture.Bind();
			// Above the model; a flyer's bar hangs under it while it roosts (the ceiling is above).
			float barY = 1.1f;
			if (flies)
				barY = flight.phase == FlightPhase::Roost ? idleBottom - 0.2f : referenceTop + 0.1f;
			glPushMatrix();
			glTranslatef(0, barY - 1.1f, 0);

			glColor4f(1, 1, 1, 0.9);
			Lighting::setEmissive(true);

			glBegin(GL_LINE_LOOP);
			glVertex3f(-0.501, 1.101, 0);
			glVertex3f(0.501, 1.101, 0);
			glVertex3f(0.501, 1.201, 0);
			glVertex3f(-0.501, 1.201, 0);
			glEnd();

			glEnable(GL_BLEND);

			float xxx = static_cast<float>(health) / static_cast<float>(maxHealth);
			glColor3f(3 * (1 - xxx), 3 * xxx, 0);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex3f(-0.5, 1.1, 0);
			glTexCoord2f(1, 0);
			glVertex3f(xxx - 0.5, 1.1, 0);
			glTexCoord2f(1, 1);
			glVertex3f(xxx - 0.5, 1.2, 0);
			glTexCoord2f(0, 1);
			glVertex3f(-0.5, 1.2, 0);
			glEnd();

			glDisable(GL_BLEND);
			Lighting::setEmissive(false);
			glPopMatrix();

			glColor3f(1, 1, 1);
		}

		glPushMatrix();
		glScalef(0.5 / scale, 0.5 / scale, 0.5 / scale);

		GAME_STATE.textures.nullTex.Bind();
		blood->Explode();
		blood->Fall();
		blood->Draw();
		glPopMatrix();
	} else
		applyModelState(ModelState::Die);

	// Draw blood particles even when monster is dead
	glPushMatrix();
	glScalef(0.5 / scale, 0.5 / scale, 0.5 / scale);

	GAME_STATE.textures.nullTex.Bind();
	blood->Explode();
	blood->Fall();
	blood->Draw();
	glPopMatrix();

	tex.Bind();

	if (this != GAME_STATE.Player.get()) {
		if (Alive()) {
			if (!flies)
				facing_dir = attackDirection();
			else
				facing_dir = flight.phase == FlightPhase::Roost ? 0 : flight.dir;
			glRotatef(rotA + 90 * facing_dir, 0, 1, 0);
		} else
			glRotatef(rotA + 90 * facing_dir, 0, 1, 0);
	} else
		glRotatef(rotA, 0, 1, 0);

	if (GAME_STATE.render.Cartoon)
		model->ShowC();
	else
		model->Show();

	glPopMatrix();
	glPopMatrix();
	if (currentState != ModelState::Climb) // the climb frame follows the height (showClimb)
		model->Advance_Animation();
	return 1;
}
//================================================================================
bool monster::loadModel(const char filename[], Textura& texture, Textura& nullT, bool compile, const ClipFiles& files) {
	nullTexture = nullT;

	if (stat != -1) {
		LOG_ERRORF("entities", "Object already loaded: error %d", stat);
		return 0;
	}

	reference = files.front().state;
	ModelNormalization norm;
	for (const ClipFile& file : files) {
		const std::string path = std::string("Models/") + filename + file.suffix + ".md3";
		if (!file.required && !std::filesystem::exists(path))
			continue;
		LOG_INFOF("entities", "Loading model: %s", path.c_str());
		auto& c = clips[static_cast<int>(file.state)];
		c = makeModel(path.c_str(), texture.ID(), 35);
		// Every clip uses the reference clip's normalization, so the model doesn't jump between animations.
		if (file.state == reference)
			norm = c->Centrify();
		else
			c->Normalize(norm);
		c->loop = file.loop;
	}
	referenceTop = clip(reference)->YRange(0).second;
	if (const AnimatedCartoonModel* idle = clip(ModelState::Idle)) {
		idleBottom = idle->YRange(0).first;
		idleTop = idle->YRange(0).second;
	}

	// filename is "<category>/<name>" (under Models/); sounds are flat in Sounds/<name>_*.wav.
	const std::string name = std::filesystem::path(filename).filename().string();
	die_s.LoadWAV(("Sounds/" + name + "_die.wav").c_str());
	att_s.LoadWAV(("Sounds/" + name + "_att.wav").c_str());

	if (compile)
		for (auto& c : clips)
			if (c)
				c->Compile();
	stat = 1;

	applyModelState(reference);

	return 1;
}
//================================================================================
void monster::setCords(float nX, float nY) {
	mapX = nX;
	mapY = nY;
}
//================================================================================
bool monster::Alive() {
	if (health > 0)
		return 1;

	return 0;
}
//================================================================================
bool monster::getHit(int dmg) {
	if (Alive()) {
		health -= dmg;
		blood->setCords(random() % static_cast<int>(scale), random() % static_cast<int>(scale), 0);
		blood->Reset();
	}

	if (!Alive() && currentState != ModelState::Die) {
		applyModelState(ModelState::Die);
		sprintf(GAME_STATE.status, "Gained %d XP", XP);
		GAME_STATE.status_timer->Reset();
		GAME_STATE.ui.Stats->GetXP(XP);
		die_s.Play();

		// Death blood effect - 20% more intense than regular hit
		blood->setCords(random() % static_cast<int>(scale), random() % static_cast<int>(scale), 0);
		blood->Reset();

		// Trigger 6 explosion cycles (20% more than the 5 cycles from regular hit + death)
		for (int i = 0; i < 6; i++) {
			blood->Explode();
		}
	}

	return Alive();
}
//================================================================================
void monster::Reanimate() {
	health = maxHealth;
	applyModelState(reference);
	facing_dir = 0;
}
//================================================================================
void monster::setFacingDir(int dir) { facing_dir = dir; }
//================================================================================
int monster::FacingDir() { return facing_dir; }
//================================================================================
void monster::setBloodColor(float r, float g, float b) {
	bloodColour = {r, g, b};
	ownBlood->setBloodColor(r, g, b);
}
//================================================================================
void monster::initBlood(ParSys& tokenBlood) const {
	tokenBlood.setBloodColor(bloodColour.r, bloodColour.g, bloodColour.b);
	tokenBlood.Stop(); // no splash until the first hit
}
//================================================================================
void monster::useBlood(ParSys* tokenBlood) { blood = tokenBlood ? tokenBlood : ownBlood.get(); }
//================================================================================
AnimatedCartoonModel* monster::clip(ModelState state) const { return clips[static_cast<int>(state)].get(); }
//================================================================================
MonsterAnimations monster::spawnAnimations() const {
	MonsterAnimations animations{};
	// Random move / idle phase, so monsters spawned in the same tick don't march in step.
	for (ModelState state : {ModelState::Move, ModelState::Idle})
		if (const AnimatedCartoonModel* c = clip(state); c && c->FrameCount() > 1)
			animations[static_cast<int>(state)].frame = static_cast<float>(rand() % (c->FrameCount() - 1));
	return animations;
}
//================================================================================
void monster::restoreAnimations(int state, const MonsterAnimations& animations) {
	for (int s = 0; s < static_cast<int>(animations.size()); s++)
		if (AnimatedCartoonModel* c = clip(static_cast<ModelState>(s)))
			c->SetPlayback(animations[s]);
	selectModel(static_cast<ModelState>(state));
}
//================================================================================
MonsterAnimations monster::animations() const {
	MonsterAnimations animations{};
	for (int s = 0; s < static_cast<int>(animations.size()); s++)
		if (const AnimatedCartoonModel* c = clip(static_cast<ModelState>(s)))
			animations[s] = c->Playback();
	return animations;
}
//================================================================================
float monster::healthRatio() const {
	if (maxHealth <= 0)
		return 0.0f;

	float ratio = static_cast<float>(health) / static_cast<float>(maxHealth);
	if (ratio < 0.0f)
		return 0.0f;
	if (ratio > 1.0f)
		return 1.0f;

	return ratio;
}
//================================================================================
