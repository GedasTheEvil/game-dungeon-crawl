#include "stats.h"
#include <cmath>
#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../core/logger.h"

void stats::GetStronger(int ns) { Might += ns; }

void stats::Draw() {

	bool rotItems = stats_ani->TimePassed();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear The Screen And The Depth Buffer
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);		// Select The Projection Matrix
	glLoadIdentity();					// Reset The Projection Matrix
	glOrtho(0, 100, 0, 100, -200, 200); // Set Up An Ortho Screen
	glMatrixMode(GL_MODELVIEW);			// Select The Modelview Matrix

	// Background image
	GAME_STATE.textures.bg.Bind();

	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1);
	glTexCoord2f(0, 0);
	glVertex3i(0, 0, -40);
	glTexCoord2f(0, 1);
	glVertex3i(0, 100, -40);
	glTexCoord2f(1, 1);
	glVertex3i(100, 100, -40);
	glTexCoord2f(1, 0);
	glVertex3i(100, 0, -40);
	glEnd();

	GAME_STATE.textures.progBar.Bind();
	glColor3f(1, 1, 1);

	realPscale = GAME_STATE.Player->scale;
	realIscale = GAME_STATE.ui.invent->Equipped()->scale;

	glBegin(GL_LINE_LOOP); // player slot
	glVertex3i(4, 4, -38);
	glVertex3i(4, 96, -38);
	glVertex3i(48, 96, -38);
	glVertex3i(48, 4, -38);
	glEnd();

	glPushMatrix();
	glTranslatef(22, 54, 20);
	GAME_STATE.Player->scale = 32;
	GAME_STATE.Player->Draw();
	if (rotItems)
		GAME_STATE.Player->rotA--;
	glPopMatrix();
	GAME_STATE.Player->scale = realPscale;

	glBegin(GL_LINE_LOOP); // item
	glVertex3i(5, 5, -38);
	glVertex3i(5, 50, -38);
	glVertex3i(47, 50, -38);
	glVertex3i(47, 5, -38);
	glEnd();

	glPushMatrix();
	GAME_STATE.ui.invent->Equipped()->scale = 40;
	glTranslatef(26, 5, 25);
	GAME_STATE.ui.invent->Equipped()->Draw();
	if (rotItems)
		GAME_STATE.ui.invent->Equipped()->rotA++;
	glPopMatrix();
	GAME_STATE.ui.invent->Equipped()->scale = realIscale;

	glColor3f(1, 1, 1);

	// all text comes from this point on
	glBlendFunc(GL_ONE_MINUS_SRC_COLOR, GL_SRC_COLOR);
	glEnable(GL_BLEND);

	Impact.print(54, 90, "Level: %d ", level);
	Impact.print(54, 80, "XP: %d ", static_cast<int>(XP));
	Impact.print(54, 70, "Next level:%d ", static_cast<int>(1000 * (pow(level, 1.4))));
	Impact.print(54, 60, "HP %d/%d", HP, MaxHP);
	Impact.print(54, 50, "Might: %d", Might);
	Impact.print(54, 40, "Armor: %d", Armor);
	Impact.print(54, 30, "Damage: %d", Damage());
	Impact.print(54, 20, "Range: %d", GAME_STATE.ui.invent->Equipped()->range);

	glDisable(GL_BLEND);

	glFlush();

	glutSwapBuffers();
}

void stats::GetXP(int xp) {
	XP += xp;
	while (AdvanceLevel())
		;
}

void stats::SetSprintRequested(bool requested) { sprint_requested = requested; }

bool stats::IsSprinting() const { return sprinting; }

float stats::SprintMoveMultiplier() const {
	if (sprinting)
		return 3.0f;

	return 1.0f;
}

void stats::UpdateStamina() {
	if (!GAME_STATE.Player->Alive()) {
		sprinting = false;
		sprint_requested = false;
		return;
	}

	if (!sprint_requested || GAME_STATE.Player->Stamina() <= 0) {
		sprinting = false;
		RegenerateStamina();
		return;
	}

	sprinting = true;

	if (!stamina_sprint_drain_timer->TimePassed())
		return;

	stamina_sprint_drain_carry += 0.05f * static_cast<float>(MaxStamina());

	int staminaDrain = static_cast<int>(stamina_sprint_drain_carry);
	if (staminaDrain <= 0)
		return;

	stamina_sprint_drain_carry -= static_cast<float>(staminaDrain);
	if (!GAME_STATE.Player->ConsumeStamina(staminaDrain))
		GAME_STATE.Player->SetStamina(0);

	if (GAME_STATE.Player->Stamina() <= 0) {
		sprinting = false;
	}
}

void stats::RegenerateStamina() {
	if (!stamina_regen_timer->TimePassed())
		return;

	stamina_regen_carry += 0.02f * static_cast<float>(MaxStamina());

	int staminaGain = static_cast<int>(stamina_regen_carry);
	if (staminaGain <= 0)
		return;

	stamina_regen_carry -= static_cast<float>(staminaGain);
	GAME_STATE.Player->AddStamina(staminaGain);
}

int stats::MaxStamina() const { return 100 + (level - 1) * 10; }

void stats::Heal(int hpPart) {
	if (HP == MaxHP)
		return;

	float heal = static_cast<float>(hpPart * MaxHP) / static_cast<float>(100.0);
	if (static_cast<float>(HP) + heal > static_cast<float>(MaxHP))
		HP = MaxHP;
	else
		HP = static_cast<int>(heal + static_cast<float>(HP));

	GAME_STATE.Player->health = HP;
}

stats::stats() {
	MaxHP = 50;
	HP = MaxHP;
	Might = 0;
	Armor = 0;
	level = 1;
	stamina_regen_carry = 0.0f;
	stamina_sprint_drain_carry = 0.0f;
	sprint_requested = false;
	sprinting = false;
	Impact.Load("Fonts/papyrus_i.bmp", 5, -0.6);
	show = false;
	GAME_STATE.Player->maxHealth = MaxHP;
	GAME_STATE.Player->health = MaxHP;
	GAME_STATE.Player->SetMaxStamina(MaxStamina());
	GAME_STATE.Player->SetStamina(GAME_STATE.Player->MaxStamina());
	stats_ani = std::make_unique<timer>(10);
	stamina_regen_timer = std::make_unique<timer>(1000);
	stamina_sprint_drain_timer = std::make_unique<timer>(1000);
}

stats::~stats() {
	void* selfPtr = this;
	LOG_DEBUGF("ui", "Deleting stats %p", selfPtr);
}

void stats::GetArmored(int na) { Armor += na; }

void stats::GetHit(int dmg) {
	int damage = 1;

	if (dmg - Armor > 0)
		damage = dmg - Armor;

	GAME_STATE.Player->getHit(damage);
	HP = GAME_STATE.Player->health;
}

void stats::MouseFunction(int mouseButton, int buttonState, int mouseX, int mouseY) {
	(void)mouseButton;
	(void)buttonState;
	(void)mouseX;
	(void)mouseY;
}

bool stats::AdvanceLevel() {
	if (XP >= 1000 * (pow(level, 1.4)))
		level++;
	else
		return false;

	if (level % 5 == 0)
		Armor++;

	if (level % 8 == 0)
		Might++;

	MaxHP += 20;
	HP = MaxHP;

	GAME_STATE.Player->maxHealth = MaxHP;
	GAME_STATE.Player->health = MaxHP;
	GAME_STATE.Player->SetMaxStamina(MaxStamina());

	sprintf(GAME_STATE.status, "Now you are level %d\n", level);
	GAME_STATE.status_timer->Reset();

	return true;
}

int stats::Damage() const {
	return Might + GAME_STATE.ui.invent->Equipped()->damage; // + weapon dmg
}

void stats::GetTougher(int hpPart) {
	float more = static_cast<float>(1) + static_cast<float>(hpPart) / static_cast<float>(100.0);
	MaxHP = static_cast<int>(static_cast<float>(MaxHP) * more);
	HP = MaxHP;
	GAME_STATE.Player->maxHealth = MaxHP;
	GAME_STATE.Player->health = MaxHP;
}

void stats::Dump(std::ofstream& f) const {
	f << level << " " << XP << " " << Armor << " " << MaxHP << " " << HP << " " << Might << " "
	  << GAME_STATE.Player->Stamina() << "\n";
}

void stats::LoadDump(std::ifstream& f) {
	f >> level >> XP >> Armor >> MaxHP >> HP >> Might;

	int loadedStamina = 100;

	if (!(f >> loadedStamina))
		f.clear();

	stamina_regen_carry = 0.0f;
	stamina_sprint_drain_carry = 0.0f;
	sprint_requested = false;
	sprinting = false;
	stamina_regen_timer->Reset();
	stamina_sprint_drain_timer->Reset();
	GAME_STATE.Player->maxHealth = MaxHP;
	GAME_STATE.Player->health = HP;
	GAME_STATE.Player->SetMaxStamina(MaxStamina());
	GAME_STATE.Player->SetStamina(loadedStamina);
}
