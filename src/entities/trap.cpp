#include "trap.h"
#include <stdio.h>
#include "../state/cashe.h"
#include <cmath>
#include "../input/gameplay_config.h"

extern Cashe c;

trap::trap() {
	Hurt_timer = new timer(TRAP_HURT_INTERVAL_MS);
	mdl = new CartoonANI();
	x = 0;
	y = 0;
	scale = 3;
}

trap::~trap() {
	delete mdl;
	delete Hurt_timer;
	printf("Deleting trap\n");
}

void trap::Show() {
	glPushMatrix();
	glTranslatef(0, 0, -30);
	glPushMatrix();
	glScalef(scale, scale, scale);
	tex.Bind();
	if (c.Cartoon)
		mdl->ShowC();
	else
		mdl->Show();
	glPopMatrix();
	glPopMatrix();
	Hurt();
}
void trap::Hurt() {
	if (!Hurt_timer->TimePassed())
		return;

	if (fabs(*DX - x - 0.5) <= TRAP_HITBOX_X_SCALE * scale && fabs(*DY - y) <= TRAP_HITBOX_Y_SCALE * scale) {
		c.Stats->GetHit(1);
	}
}

void trap::setCords(float nX, float nY) {
	x = nX;
	y = nY;
}

bool trap::LoadMDL(const char filename[], Textura& texture, bool compile) {
	tex = texture;

	mdl->Load(filename);
	mdl->BindTexture(texture.ID());
	mdl->Centrify();

	if (compile)
		mdl->Compile();
	return 1;
}

void trap::debugText() { printf("I am trap, my x=%f, y=%f\n Dungeon x=%f, y=%f\n\n", x, y, *DX, *DY); }