#include "item.h"
#include <cmath>
#include <stdio.h>
#include "../state/cashe.h"
#include "../core/service_locator.h"

void item::Draw() {
	if (!loaded)
		return;

	glPushMatrix();

	if (!in_inventory && std::fabs(x) >= 0.1)
		glTranslatef(40 * x - 20, y, -30);
	else
		glTranslatef(0, 0, -30);

	glPushMatrix(); // will add rotation

	glScalef(scale, scale, scale);

	tex.Bind();
	glRotatef(rotA, 0, 1, 0);

	if (GAME_STATE.Cartoon)
		mdl->ShowC();
	else
		mdl->Show();
	glPopMatrix();
	glPopMatrix();
	mdl->Advance_Animation();
}

bool item::getPickedUp() {
	if (!loaded)
		return 0;

	in_inventory = 1;

	return 1;
}

item::item() {
	mdl = nullptr;
	x = 0;
	y = 0;
	scale = 0;
	rotA = 0;
	loaded = 0;
	heal = 1;
	damage = 1;
	range = 1;
	type = 0;
}
item::~item() {

	delete mdl;
	mdl = nullptr;
	loaded = 0;
	printf("Deleting item\n");
}

bool item::LoadMDL(const char filename[], Textura& texture, bool compile) {
	tex = texture;
	mdl = new AnimatedCartoonModel();
	mdl->Load(filename);
	mdl->Centrify();
	mdl->BindTexture(tex.ID());
	if (compile)
		mdl->Compile();

	loaded = 1;
	return 1;
}
