#include "stats.h"
#include <cmath>
#include <GL/gl.h>
#ifndef WIN32
#include <GL/glut.h>
#endif
#ifdef WIN32
#include <GL/freeglut.h>
#endif
#include <stdio.h>
#include "../state/cashe.h"

extern Cashe c;

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
	c.bg.Bind();

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

	c.progBar.Bind();
	glColor3f(1, 1, 1);

	realPscale = c.Player->scale;
	realIscale = c.invent->Equipped()->scale;

	glBegin(GL_LINE_LOOP); // player slot
	glVertex3i(4, 4, -38);
	glVertex3i(4, 96, -38);
	glVertex3i(48, 96, -38);
	glVertex3i(48, 4, -38);
	glEnd();

	glPushMatrix();
	glTranslatef(22, 54, 20);
	c.Player->scale = 32;
	c.Player->Draw();
	if (rotItems)
		c.Player->rotA--;
	glPopMatrix();
	c.Player->scale = realPscale;

	glBegin(GL_LINE_LOOP); // item
	glVertex3i(5, 5, -38);
	glVertex3i(5, 50, -38);
	glVertex3i(47, 50, -38);
	glVertex3i(47, 5, -38);
	glEnd();

	glPushMatrix();
	c.invent->Equipped()->scale = 40;
	glTranslatef(26, 5, 25);
	c.invent->Equipped()->Draw();
	if (rotItems)
		c.invent->Equipped()->rotA++;
	glPopMatrix();
	c.invent->Equipped()->scale = realIscale;

	glColor3f(1, 1, 1);

	// all text comes from this point on
	glBlendFunc(GL_ONE_MINUS_SRC_COLOR, GL_SRC_COLOR);
	glEnable(GL_BLEND);

	Impact.print(54, 90, "Level: %d ", level);
	Impact.print(54, 80, "XP: %d ", (int)XP);
	Impact.print(54, 70, "Next level:%d ", (int)(1000 * (pow(level, 1.4))));
	Impact.print(54, 60, "HP %d/%d", HP, MaxHP);
	Impact.print(54, 50, "Might: %d", Might);
	Impact.print(54, 40, "Armor: %d", Armor);
	Impact.print(54, 30, "Damage: %d", Damage());
	Impact.print(54, 20, "Range: %d", c.invent->Equipped()->range);

	glDisable(GL_BLEND);

	glFlush();

	glutSwapBuffers();
}

void stats::GetXP(int xp) {
	//      printf("getXP called with %d \n",xp);
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
	if (!c.Player->Alive()) {
		sprinting = false;
		sprint_requested = false;
		return;
	}

	if (!sprint_requested || c.Player->Stamina() <= 0) {
		sprinting = false;
		RegenerateStamina();
		return;
	}

	sprinting = true;

	if (!stamina_sprint_drain_timer->TimePassed())
		return;

	stamina_sprint_drain_carry += 0.05f * (float)MaxStamina();

	int staminaDrain = (int)stamina_sprint_drain_carry;
	if (staminaDrain <= 0)
		return;

	stamina_sprint_drain_carry -= static_cast<float>(staminaDrain);
	if (!c.Player->ConsumeStamina(staminaDrain))
		c.Player->SetStamina(0);

	if (c.Player->Stamina() <= 0) {
		sprinting = false;
	}
}

void stats::RegenerateStamina() {
	if (!stamina_regen_timer->TimePassed())
		return;

	stamina_regen_carry += 0.02f * (float)MaxStamina();

	int staminaGain = (int)stamina_regen_carry;
	if (staminaGain <= 0)
		return;

	stamina_regen_carry -= static_cast<float>(staminaGain);
	c.Player->AddStamina(staminaGain);
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

	c.Player->HP = HP;
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
	c.Player->MaxHP = MaxHP;
	c.Player->HP = MaxHP;
	c.Player->SetMaxStamina(MaxStamina());
	c.Player->SetStamina(c.Player->MaxStamina());
	stats_ani = new timer(10);
	stamina_regen_timer = new timer(1000);
	stamina_sprint_drain_timer = new timer(1000);
}

stats::~stats() { printf("Deleting stats %p \n", (void*)this); }

void stats::GetArmored(int na) { Armor += na; }

void stats::GetHit(int dmg) {
	int damage = 1;

	if (dmg - Armor > 0)
		damage = dmg - Armor;

	c.Player->getHit(damage);
	HP = c.Player->HP;
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

	c.Player->MaxHP = MaxHP;
	c.Player->HP = MaxHP;
	c.Player->SetMaxStamina(MaxStamina());

	sprintf(c.status, "Now you are level %d\n", level);
	c.status_timer->Reset();

	return true;
}

int stats::Damage() {
	return Might + c.invent->Equipped()->damage; // + weapon dmg
}

void stats::GetTougher(int hpPart) {
	float more = static_cast<float>(1) + static_cast<float>(hpPart) / static_cast<float>(100.0);
	MaxHP = static_cast<int>(static_cast<float>(MaxHP) * more);
	HP = MaxHP;
	c.Player->MaxHP = MaxHP;
	c.Player->HP = MaxHP;
}

void stats::Dump(std::ofstream& f) {
	f << level << " " << XP << " " << Armor << " " << MaxHP << " " << HP << " " << Might << " " << c.Player->Stamina()
	  << "\n";
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
	c.Player->MaxHP = MaxHP;
	c.Player->HP = HP;
	c.Player->SetMaxStamina(MaxStamina());
	c.Player->SetStamina(loadedStamina);
}
