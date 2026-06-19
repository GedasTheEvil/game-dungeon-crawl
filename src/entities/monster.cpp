#include "monster.h"
#include <GL/gl.h>
#include <cmath>

#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../core/logger.h"

#ifdef WIN32

#include <cstdlib>

inline int random() { return rand(); }

#endif

namespace {
std::unique_ptr<AnimatedCartoonModel> makeModel(const char* path, GLuint texId, int speed) {
	auto m = std::make_unique<AnimatedCartoonModel>();
	m->Load(path);
	m->BindTexture(texId);
	m->Centrify();
	m->setSpeed(speed);
	return m;
}
} // namespace

void monster::applyModelState(ModelState state) {
	currentState = state;
	switch (state) {
	case ModelState::Walk:
		model = walk.get();
		break;
	case ModelState::Attack:
		model = attack.get();
		break;
	case ModelState::Die:
		model = die.get();
		break;
	}
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
	currentState = ModelState::Walk;
	model = nullptr;

	Att_timer = std::make_unique<timer>(1000);
	walk_timer = std::make_unique<timer>(40);

	blood = std::make_unique<ParSys>(100);
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
	currentState = ModelState::Walk;
	model = nullptr;
	Att_timer = std::make_unique<timer>(1000);
	walk_timer = std::make_unique<timer>(40);

	blood = std::make_unique<ParSys>(100);
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
	currentState = ModelState::Walk;
	model = nullptr;
	Att_timer = std::make_unique<timer>(1000);
	walk_timer = std::make_unique<timer>(40);

	blood = std::make_unique<ParSys>(100);
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
		glTranslatef(40 * mapX - 20, mapY, -30);
	else
		glTranslatef(0, 0, -30);

	glPushMatrix(); // will add rotation

	glScalef(scale, scale, scale);

	if (Alive()) {
		if (currentState == ModelState::Die) {
			if (!attackDirection())
				applyModelState(ModelState::Attack);
			else
				applyModelState(ModelState::Walk);
		}

		if (this != GAME_STATE.Player.get()) {
			nullTexture.Bind();

			glColor4f(1, 1, 1, 0.9);

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
			facing_dir = attackDirection();
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
	model->Advance_Animation();
	return 1;
}
//================================================================================
bool monster::loadModel(const char filename[], Textura& texture, Textura& nullT, bool compile) {
	nullTexture = nullT;

	if (stat != -1) {
		LOG_ERRORF("entities", "Object already loaded: error %d", stat);
		return 0;
	}

	char tmp1[255], tmp2[255], tmp3[255], tmp4[255], tmp5[255];

	sprintf(tmp1, "Models/%s.mdl", filename);
	sprintf(tmp2, "Models/%s_att.mdl", filename);
	sprintf(tmp3, "Models/%s_die.mdl", filename);
	sprintf(tmp4, "Sounds/%s_att.wav", filename);
	sprintf(tmp5, "Sounds/%s_die.wav", filename);

	LOG_INFOF("entities", "Loading model: %s", tmp1);
	walk = makeModel(tmp1, texture.ID(), 35);

	LOG_INFOF("entities", "Loading model: %s", tmp2);
	attack = makeModel(tmp2, texture.ID(), 35);

	LOG_INFOF("entities", "Loading model: %s", tmp3);
	die = makeModel(tmp3, texture.ID(), 35);
	die->loop = 0;

	die_s.LoadWAV(tmp5);
	att_s.LoadWAV(tmp4);

	if (compile) {
		walk->Compile();
		attack->Compile();
		die->Compile();
	}
	stat = 1;

	applyModelState(ModelState::Walk);

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
	applyModelState(ModelState::Walk);
	facing_dir = 0;
}
//================================================================================
void monster::setFacingDir(int dir) { facing_dir = dir; }
//================================================================================
int monster::FacingDir() { return facing_dir; }
//================================================================================
void monster::setBloodColor(float r, float g, float b) { blood->setBloodColor(r, g, b); }
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
