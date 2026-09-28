#include "trap.h"
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../core/logger.h"
#include <cmath>
#include "../input/gameplay_config.h"

namespace {
// Shared by all trap instances: the streak belongs to the player, not to one trap model.
int gHitStreak = 0;
int gLastHitMs = 0;
} // namespace

Trap::Trap() {
	Hurt_timer = std::make_unique<Timer>(TRAP_HURT_INTERVAL_MS);
	mdl = std::make_unique<AnimatedModel>();
	tileX = 0;
	tileY = 0;
	scale = 3;
}

Trap::~Trap() {}

void Trap::Show() {
	glPushMatrix();
	glTranslatef(0, 0, -30);
	glPushMatrix();
	glScalef(scale, scale, scale);
	tex.Bind();
	mdl->Show();
	glPopMatrix();
	glPopMatrix();
	Hurt();
}
void Trap::Hurt() {
	if (fabs(*dungeonCamX - tileX - 0.5) > TRAP_HITBOX_X_SCALE * scale ||
		std::fabs(*dungeonCamY - tileY) > TRAP_HITBOX_Y_SCALE * scale)
		return;
	if (!Hurt_timer->TimePassed())
		return;

	// Damage ramps up while the player stays in a trap; any gap resets it.
	int now = GameClock::now();
	if (now - gLastHitMs > TRAP_STREAK_RESET_MS)
		gHitStreak = 0;
	gLastHitMs = now;
	GAME_STATE.player->TakeHit(1 + gHitStreak / TRAP_DAMAGE_RAMP_HITS);
	gHitStreak++;
}

void Trap::setCords(float nX, float nY) {
	tileX = nX;
	tileY = nY;
}

bool Trap::loadModel(const char filename[], Texture& texture, bool compile) {
	tex = texture;

	mdl->Load(filename);
	mdl->BindTexture(texture.ID());
	mdl->Centrify();

	if (compile)
		mdl->Compile();
	return true;
}

void Trap::debugText() {
	LOG_DEBUGF("entities", "trap tileX=%f tileY=%f dungeonCamX=%f dungeonCamY=%f", tileX, tileY, *dungeonCamX,
			   *dungeonCamY);
}
