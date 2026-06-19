#include "trap.h"
#include <stdio.h>
#include "../state/cashe.h"
#include "../core/service_locator.h"
#include <cmath>
#include "../input/gameplay_config.h"

trap::trap() {
	Hurt_timer = std::make_unique<timer>(TRAP_HURT_INTERVAL_MS);
	mdl = std::make_unique<AnimatedCartoonModel>();
	x = 0;
	y = 0;
	scale = 3;
}

trap::~trap() {}

void trap::Show() {
	glPushMatrix();
	glTranslatef(0, 0, -30);
	glPushMatrix();
	glScalef(scale, scale, scale);
	tex.Bind();
	if (GAME_STATE.render.Cartoon)
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

	if (fabs(*DX - x - 0.5) <= TRAP_HITBOX_X_SCALE * scale && std::fabs(*DY - y) <= TRAP_HITBOX_Y_SCALE * scale) {
		GAME_STATE.ui.Stats->GetHit(1);
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