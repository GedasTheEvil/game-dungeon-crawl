#include "trap.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../core/logger.h"
#include <cmath>
#include "../input/gameplay_config.h"

trap::trap() {
	Hurt_timer = std::make_unique<timer>(TRAP_HURT_INTERVAL_MS);
	mdl = std::make_unique<AnimatedCartoonModel>();
	tileX = 0;
	tileY = 0;
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

	if (fabs(*dungeonCamX - tileX - 0.5) <= TRAP_HITBOX_X_SCALE * scale &&
		std::fabs(*dungeonCamY - tileY) <= TRAP_HITBOX_Y_SCALE * scale) {
		GAME_STATE.ui.Stats->GetHit(1);
	}
}

void trap::setCords(float nX, float nY) {
	tileX = nX;
	tileY = nY;
}

bool trap::loadModel(const char filename[], Textura& texture, bool compile) {
	tex = texture;

	mdl->Load(filename);
	mdl->BindTexture(texture.ID());
	mdl->Centrify();

	if (compile)
		mdl->Compile();
	return 1;
}

void trap::debugText() {
	LOG_DEBUGF("entities", "trap tileX=%f tileY=%f dungeonCamX=%f dungeonCamY=%f", tileX, tileY, *dungeonCamX,
			   *dungeonCamY);
}
