#include "inventory.h"
#include "../entities/item.h"
#include "../state/cashe.h"
#include <GL/gl.h>
#ifndef WIN32
#include <GL/glut.h>
#endif
#ifdef WIN32
#include <GL/freeglut.h>
#endif
#include <stdio.h>

namespace {
constexpr int MELEE_WEAPON_COUNT = 3;
constexpr float SLOT_ITEM_SCALE = 17.0f;
constexpr float VIEW_ITEM_SCALE = 34.0f;
constexpr float POTION_SLOT_SCALE = 10.0f;

constexpr int SLOT_Z = -39;
constexpr int VIEW_BG_Z = -40;
} // namespace

item* viewed; // only pointer

extern Cashe c;

inventory::inventory() {
	inv_ani = new timer(10);
	Impact.Load("Fonts/papyrus_i.bmp", 5, -0.6);
	small.Load("Fonts/impact_i.bmp", 3.5, -0.3);

	equipped.type = ItemType::MELEE_WEAPON;
	equipped.id = WeaponId::CLUB;
	show = false;

	view.type = ItemType::MELEE_WEAPON;
	view.id = WeaponId::CLUB;
	view.count = 1;

	mw[WeaponId::CLUB].count = 1;
	mw[WeaponId::CLUB].realScale = c.club->scale;
	mw[WeaponId::SWORD].count = 0;
	mw[WeaponId::SWORD].realScale = c.sword->scale;
	mw[WeaponId::SPEAR].count = 0;
	mw[WeaponId::SPEAR].realScale = c.spear->scale;

	rw.count = 0;
	rw.realScale = c.bow->scale;

	viewed = c.club.get();

	for (int i = 0; i < PotionId::COUNT; i++) {
		potions[i].count = 0;
		potions[i].realScale = c.potion->scale;
	}
}

inventory::~inventory() {
	printf("Deleting inventory %p \n", (void*)this);
	delete inv_ani;
}

void inventory::UsePotion() {
	if (!c.Player->Alive()) {
		printf("Player dead, cannot drink anymore \n");
		return;
	}

	if (view.type != ItemType::POTION)
		return;

	if (potions[view.id].count <= 0)
		return;

	c.drink_s.Play();
	potions[view.id].count--;

	switch (view.id) {
	case PotionId::SMALL_HEALTH:
		c.Stats->Heal(25);
		break;
	case PotionId::LARGE_HEALTH:
		c.Stats->Heal(50);
		break;
	case PotionId::STRENGTH:
		c.Stats->GetStronger(2);
		break;
	case PotionId::ARMOR:
		c.Stats->GetArmored(2);
		break;
	case PotionId::LIFE:
		c.Stats->GetTougher(5);
		break;
	}
}

void inventory::GetItem(int type, int id) {
	switch (type) {
	case ItemType::MELEE_WEAPON:
		if (id >= MELEE_WEAPON_COUNT) {
			printf("Error: Unknown weapon, id=%d\n", id);
			return;
		}
		mw[id].count++;
		break;
	case ItemType::RANGED_WEAPON:
		if (id >= 1) {
			printf("Error: Unknown bow, id=%d\n", id);
			return;
		}
		rw.count++;
		break;
	case ItemType::POTION:
		if (id >= PotionId::COUNT) {
			printf("Error: Unknown potion, id=%d\n", id);
			return;
		}
		potions[id].count++;
		break;
	}
}

void inventory::Draw() {

	bool rot_items;
	if (inv_ani->TimePassed())
		rot_items = 1;
	else
		rot_items = 0;

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

	if (view.type == ItemType::POTION)
		glColor3f(1 - 0.3f * (view.id % 3), 0.6f * (view.id / 3), 0.3f * (view.id % 3));

	glPushMatrix(); // drawModel of view
	glTranslatef(71, 40, 0);
	viewed->scale = 34;
	viewed->Draw();
	if (rot_items)
		viewed->rotA++;
	glPopMatrix();
	c.progBar.Bind();

	glColor3f(0, 0, 0);
	glBegin(GL_LINE_LOOP); // weapon slot1
	glVertex3i(4, 92, -39);
	glVertex3i(14, 92, -39);
	glVertex3i(14, 75, -39);
	glVertex3i(4, 75, -39);
	glEnd();

	glColor3f(1, 1, 1);
	glPushMatrix(); // drawModel1
	glTranslatef(9, 75, 0);
	c.club->scale = 17;
	c.club->Draw();
	if (rot_items)
		c.club->rotA++;
	c.club->scale = mw[0].realScale;
	glPopMatrix();
	c.progBar.Bind();

	glColor3f(0, 0, 0);
	glBegin(GL_LINE_LOOP); // weapon slot2
	glVertex3i(17, 92, -39);
	glVertex3i(27, 92, -39);
	glVertex3i(27, 75, -39);
	glVertex3i(17, 75, -39);
	glEnd();

	glColor3f(1, 1, 1);
	glPushMatrix(); // drawModel2
	glTranslatef(22, 75, 0);
	c.sword->scale = 17;
	c.sword->Draw();
	if (rot_items)
		c.sword->rotA++;
	c.sword->scale = mw[1].realScale;
	glPopMatrix();
	c.progBar.Bind();

	glColor3f(0, 0, 0);
	glBegin(GL_LINE_LOOP); // weapon slot3
	glVertex3i(30, 92, -39);
	glVertex3i(40, 92, -39);
	glVertex3i(40, 75, -39);
	glVertex3i(30, 75, -39);
	glEnd();

	glColor3f(1, 1, 1);
	glPushMatrix(); // drawModel2
	glTranslatef(35, 75, 0);
	c.spear->scale = 17;
	c.spear->Draw();
	if (rot_items)
		c.spear->rotA++;
	c.spear->scale = mw[2].realScale;
	glPopMatrix();
	c.progBar.Bind();

	glColor3f(0, 0, 0);
	glBegin(GL_LINE); // underline for weapons text1
	glVertex3i(4, 94, -39);
	glVertex3i(45, 94, -39);
	glEnd();

	glBegin(GL_LINE); // underline for weapons text2
	glVertex3i(4, 69, -39);
	glVertex3i(45, 69, -39);
	glEnd();

	glBegin(GL_LINE_LOOP); // weapon2 slot1
	glVertex3i(4, 65, -39);
	glVertex3i(14, 65, -39);
	glVertex3i(14, 48, -39);
	glVertex3i(4, 48, -39);
	glEnd();

	glColor3f(1, 1, 1);
	glPushMatrix(); // drawModel2.2
	glTranslatef(9, 48, 0);
	c.bow->scale = 17;
	c.bow->Draw();
	if (rot_items)
		c.bow->rotA++;
	c.bow->scale = rw.realScale;
	glPopMatrix();
	c.progBar.Bind();

	glColor3f(0, 0, 0);
	glBegin(GL_LINE); // underline forpotions text
	glVertex3i(4, 42, -39);
	glVertex3i(45, 42, -39);
	glEnd();

	// potions slots
	for (int j = 0; j < 2; j++)
		for (int i = 0; i < 3 - j; i++) {
			glColor3f(0, 0, 0);
			glBegin(GL_LINE_LOOP); // sloti.j
			glVertex3i(4 + 13 * i, 40 - 20 * j, -39);
			glVertex3i(14 + 13 * i, 40 - 20 * j, -39);
			glVertex3i(14 + 13 * i, 25 - 20 * j, -39);
			glVertex3i(4 + 13 * i, 25 - 20 * j, -39);
			glEnd();

			glColor3f(1 - 0.3 * i, 0 + 0.6 * j, 0 + 0.3 * i);

			glPushMatrix(); // drawModel1
			glTranslatef(9 + 13 * i, 25 - 20 * j, 0);
			c.potion->scale = 10;
			c.potion->Draw();
			c.potion->scale = potions[0].realScale;
			glPopMatrix();
			glColor3f(1, 1, 1);
			c.progBar.Bind();
		}

	glColor3f(1, 1, 1);

	if (rot_items)
		c.potion->rotA++;

	/// selected item slot
	glColor3f(0, 0, 0);
	glBegin(GL_LINE_LOOP); // frame
	glVertex3i(52, 92, -39);
	glVertex3i(92, 92, -39);
	glVertex3i(92, 20, -39);
	glVertex3i(52, 20, -39);
	glEnd();

	glBegin(GL_LINE_LOOP); // model slot
	glVertex3i(54, 80, -39);
	glVertex3i(88, 80, -39);
	glVertex3i(88, 40, -39);
	glVertex3i(54, 40, -39);
	glEnd();

	glBegin(GL_LINE_LOOP); // equip / use button
	glVertex3i(54, 14, -30);
	glVertex3i(88, 14, -30);
	glVertex3i(88, 8, -30);
	glVertex3i(54, 8, -30);
	glEnd();

	glColor3f(1, 1, 1);

	// all text comes from this point on
	glBlendFunc(GL_ONE_MINUS_SRC_COLOR, GL_SRC_COLOR);
	glEnable(GL_BLEND);
	Impact.print(54, 8, "EQUIP / USE"); // equip / use button

	Impact.print(2, 93, "Close combat weapons:");

	Impact.print(2, 68, "Ranged weapons:");

	Impact.print(2, 41, "Potions:");

	if (view.type == ItemType::MELEE_WEAPON) {
		DrawWeaponInfo(view.id);
	} else if (view.type == ItemType::RANGED_WEAPON) {
		Impact.print(53, 86, "Type: Ranged Weapon");
		Impact.print(53, 82, "Name: Bow");
		Impact.print(53, 34, "Damage: %d hp", c.bow->damage);
		Impact.print(53, 31, "Range: %d ", c.bow->range);
		Impact.print(53, 26, "The simple bow.");
		Impact.print(53, 23, "For slow monsters");
	} else if (view.type == ItemType::POTION) {
		DrawPotionInfo(view.id);
	}

	// item quantities
	small.print(4, 75, "%d", mw[0].count);
	small.print(17, 75, "%d", mw[1].count);
	small.print(30, 75, "%d", mw[2].count);
	small.print(4, 48, "%d", rw.count);

	glColor3f(1, 1, 1);

	// potions
	for (int j = 0; j < 2; j++)
		for (int i = 0; i < 3 - j; i++)
			small.print(4 + 13 * i, 25 - 20 * j, "%d", potions[3 * j + i].count);

	glDisable(GL_BLEND);

	glFlush();

	glutSwapBuffers();
}

void inventory::MouseFunction(int button, int state, int x, int y) {
	(void)button;

	if (!state)
		return;

	extern int resX, resY;
	float inv_x = 100 * ((float)x / (float)resX);
	float inv_y = 100 - 100 * ((float)y / (float)resY);

	if (inv_x > 4 && inv_x < 84) {
		// Melee weapon slots (row 1)
		if (inv_y > 74 && inv_y < 92) {
			if (inv_x <= 15)
				SelectItem(c.club.get(), ItemType::MELEE_WEAPON, WeaponId::CLUB, mw[WeaponId::CLUB].count);
			else if (inv_x >= 30)
				SelectItem(c.spear.get(), ItemType::MELEE_WEAPON, WeaponId::SPEAR, mw[WeaponId::SPEAR].count);
			else
				SelectItem(c.sword.get(), ItemType::MELEE_WEAPON, WeaponId::SWORD, mw[WeaponId::SWORD].count);
		}

		// Ranged weapon slot
		if (inv_y > 48 && inv_y < 65 && inv_x <= 15)
			SelectItem(c.bow.get(), ItemType::RANGED_WEAPON, WeaponId::BOW, rw.count);

		// Potion slots (row 1)
		if (inv_y > 24 && inv_y < 40) {
			if (inv_x <= 15)
				SelectItem(c.potion.get(), ItemType::POTION, PotionId::SMALL_HEALTH,
						   potions[PotionId::SMALL_HEALTH].count);
			else if (inv_x >= 30)
				SelectItem(c.potion.get(), ItemType::POTION, PotionId::STRENGTH, potions[PotionId::STRENGTH].count);
			else
				SelectItem(c.potion.get(), ItemType::POTION, PotionId::LARGE_HEALTH,
						   potions[PotionId::LARGE_HEALTH].count);
		}

		// Potion slots (row 2)
		if (inv_y > 5 && inv_y < 20) {
			if (inv_x <= 15)
				SelectItem(c.potion.get(), ItemType::POTION, PotionId::ARMOR, potions[PotionId::ARMOR].count);
			else if (inv_x >= 30) {
				// Empty slot
			} else
				SelectItem(c.potion.get(), ItemType::POTION, PotionId::LIFE, potions[PotionId::LIFE].count);
		}
	}

	// Equip / use button
	if (inv_y >= 8 && inv_y <= 14 && inv_x >= 54 && inv_x <= 88 && view.count > 0) {
		if (!c.Player->Alive()) {
			printf("Player dead. Deal with it. No more actions\n");
			return;
		}
		if (view.type != ItemType::POTION)
			equipped = view;
		else
			UsePotion();
	}
}

item* inventory::Equipped() {
	if (equipped.type == ItemType::MELEE_WEAPON) {
		switch (equipped.id) {
		case WeaponId::CLUB:
			return c.club.get();
		case WeaponId::SWORD:
			return c.sword.get();
		case WeaponId::SPEAR:
			return c.spear.get();
		}
	} else if (equipped.type == ItemType::RANGED_WEAPON) {
		return c.bow.get();
	}
	return c.club.get();
}

void inventory::SelectItem(item* itemPtr, int type, int id, int count) {
	viewed = itemPtr;
	view.type = type;
	view.id = id;
	view.count = count;
}

void inventory::DrawItemSlot(int x1, int y1, int x2, int y2) {
	glColor3f(0, 0, 0);
	glBegin(GL_LINE_LOOP);
	glVertex3i(x1, y1, SLOT_Z);
	glVertex3i(x2, y1, SLOT_Z);
	glVertex3i(x2, y2, SLOT_Z);
	glVertex3i(x1, y2, SLOT_Z);
	glEnd();
}

void inventory::DrawItemModel(item* itemPtr, float posX, float posY, float scale, float& realScale, bool rotate) {
	glColor3f(1, 1, 1);
	glPushMatrix();
	glTranslatef(posX, posY, 0);
	itemPtr->scale = scale;
	itemPtr->Draw();
	if (rotate)
		itemPtr->rotA++;
	itemPtr->scale = realScale;
	glPopMatrix();
	c.progBar.Bind();
}

void inventory::DrawWeaponInfo(int weaponId) {
	Impact.print(53, 86, "Type: Close Weapon");
	switch (weaponId) {
	case WeaponId::CLUB:
		Impact.print(53, 82, "Name: Club");
		Impact.print(53, 34, "Damage: %d hp", c.club->damage);
		Impact.print(53, 31, "Range: %d ", c.club->range);
		Impact.print(53, 26, "Good old club.");
		Impact.print(53, 23, "Now with spikes.");
		break;
	case WeaponId::SWORD:
		Impact.print(53, 82, "Name: Sword");
		Impact.print(53, 34, "Damage: %d hp", c.sword->damage);
		Impact.print(53, 31, "Range: %d ", c.sword->range);
		Impact.print(53, 26, "Shiny sword.");
		Impact.print(53, 23, "Hahah!");
		break;
	case WeaponId::SPEAR:
		Impact.print(53, 82, "Name: Spear");
		Impact.print(53, 34, "Damage: %d hp", c.spear->damage);
		Impact.print(53, 31, "Range: %d ", c.spear->range);
		Impact.print(53, 26, "Long spear.");
		Impact.print(53, 23, "None shall pass!");
		break;
	}
}

void inventory::DrawPotionInfo(int potionId) {
	Impact.print(53, 86, "Type: Magic Potion");
	switch (potionId) {
	case PotionId::SMALL_HEALTH:
		Impact.print(53, 82, "Name: Small health");
		Impact.print(53, 26, "Heals 25%% HP");
		break;
	case PotionId::LARGE_HEALTH:
		Impact.print(53, 82, "Name: Large health");
		Impact.print(53, 26, "Heals 50%% HP");
		break;
	case PotionId::STRENGTH:
		Impact.print(53, 82, "Name: Aphethamine");
		Impact.print(53, 26, "Adds 2 strength");
		break;
	case PotionId::ARMOR:
		Impact.print(53, 82, "Name: Stone skin");
		Impact.print(53, 26, "Adds 2 armor");
		break;
	case PotionId::LIFE:
		Impact.print(53, 82, "Name: Elixir of life");
		Impact.print(53, 26, "Adds 5%% HP");
		break;
	}
}

void inventory::Dump(std::ofstream& f) {
	for (int i = 0; i < MELEE_WEAPON_COUNT; i++)
		f << mw[i].count << " ";
	f << rw.count << " ";
	for (int i = 0; i < PotionId::COUNT; i++)
		f << potions[i].count << " ";
	f << "\n";
}

void inventory::LoadDump(std::ifstream& f) {
	for (int i = 0; i < MELEE_WEAPON_COUNT; i++)
		f >> mw[i].count;
	f >> rw.count;
	for (int i = 0; i < PotionId::COUNT; i++)
		f >> potions[i].count;
}
