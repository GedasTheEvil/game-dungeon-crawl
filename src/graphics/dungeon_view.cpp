// The dungeon's drawing state over the game clock (plan dungeon-view-split); the draw members are in
// dungeon_render*.cpp.
#include "dungeon_view.h"
#include "../core/timer.h"
#include "../level/tuning/gameplay_shared.h"

namespace {
constexpr int PORTAL_STEP_MS = 50; // the plasma texture moves a step this often
} // namespace

float DungeonView::portalScroll() {
	const int steps = GameClock::now() / PORTAL_STEP_MS;
	return -0.022f * static_cast<float>(steps);
}

float DungeonView::riddleMarkYaw() { return static_cast<float>(GameClock::now()) / UPDATE_TICK_MS; }

float DungeonView::treasureSpin() { return static_cast<float>(GameClock::now()) / UPDATE_TICK_MS; }
