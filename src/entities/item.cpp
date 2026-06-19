#include "item.h"
#include <cmath>
#include <stdio.h>
#include "../state/game_state.h"
#include "../core/service_locator.h"

void item::Draw() {
	if (!loaded)
		return;

	glPushMatrix();

	if (!in_inventory && std::fabs(itemX) >= 0.1)
		glTranslatef(40 * itemX - 20, itemY, -30);
	else
		glTranslatef(0, 0, -30);

	glPushMatrix(); // will add rotation

	glScalef(scale, scale, scale);

	tex.Bind();
	glRotatef(rotA, 0, 1, 0);

	if (GAME_STATE.render.Cartoon)
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
	itemX = 0;
	itemY = 0;
	scale = 0;
	rotA = 0;
	loaded = 0;
	heal = 1;
	damage = 1;
	range = 1;
	type = 0;
}
item::~item() { loaded = 0; }

bool item::loadModel(const char filename[], Textura& texture, bool compile) {
	tex = texture;
	mdl = std::make_unique<AnimatedCartoonModel>();
	mdl->Load(filename);
	mdl->Centrify();
	mdl->BindTexture(tex.ID());
	if (compile)
		mdl->Compile();

	loaded = 1;
	return 1;
}
