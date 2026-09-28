#include "item.h"
#include <cmath>
#include <stdio.h>
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../graphics/ink.h"

void item::Draw() {
	if (!loaded)
		return;

	glPushMatrix();

	if (!in_inventory && std::fabs(itemX) >= 0.1)
		glTranslatef(40 * itemX - 20, itemY, -30);
	else
		glTranslatef(0, 0, -30);

	glPushMatrix(); // will add rotation

	const float drawScale = scale * Ink::figureScale();
	glScalef(drawScale, drawScale, drawScale);

	tex.Bind();
	glRotatef(rotA, 0, 1, 0);

	mdl->Show();
	glPopMatrix();
	glPopMatrix();
	mdl->Advance_Animation();
}

bool item::getPickedUp() {
	if (!loaded)
		return false;

	in_inventory = true;

	return true;
}

item::item() {
	itemX = 0;
	itemY = 0;
	scale = 0;
	rotA = 0;
	loaded = false;
	heal = 1;
	damage = 1;
	range = 1;
	type = 0;
}
item::~item() { loaded = false; }

bool item::loadModel(const char filename[], Textura& texture, bool compile) {
	tex = texture;
	mdl = std::make_unique<AnimatedModel>();
	mdl->Load(filename);
	mdl->Centrify();
	mdl->BindTexture(tex.ID());
	if (compile)
		mdl->Compile();

	loaded = true;
	return true;
}
