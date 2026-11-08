#include "first_hints.h"
#include "game_state.h"
#include "../level/controls_help.h"
#include "../ui/inventory.h"
#include "../core/timer.h"
#include <cmath>

namespace {
constexpr int WALK_BEFORE_SPRINT_MS = 4000; // walked this long without a sprint: the sprint hint
constexpr float WALK_STEP = 0.001f; // tiles: a tick that moved the player at least this far walked

struct Watch {
	HintPacer pacer;
	int walkedMs = 0;
	float lastX = 0.f;
	float lastY = 0.f;
};
Watch watch;

bool pending(FirstHint hint, bool done) { return HintPacer::Pending(Game().player->stats.run.hints, hint, done); }

// Shows it now if the pacer lets it; true when shown.
bool offer(FirstHint hint, bool possible, int now) {
	if (!watch.pacer.Offer(Game().player->stats.run.hints, hint, possible, now))
		return false;
	Game().events.Status(StatusTone::Hint, "%s", hintText(hint, Game().settings.controls).c_str());
	return true;
}
} // namespace

void updateFirstHints() {
	Player& player = *Game().player;
	if (!Game().settings.gameplay.hints || !(player.stats.run.campaign || Game().hintsAnyRun) || !player.Alive() ||
		Game().dungeon.Won())
		return;
	const int now = GameClock::now();
	float x = 0.f;
	float y = 0.f;
	Game().dungeon.getC(x, y);
	const bool moved = std::fabs(x - watch.lastX) + std::fabs(y - watch.lastY) >= WALK_STEP;
	watch.lastX = x;
	watch.lastY = y;
	if (moved)
		watch.walkedMs += UPDATE_TICK_MS;

	const Inventory& inventory = *Game().ui.inventory;
	const Dungeon& dungeon = Game().dungeon;
	if (pending(FirstHint::Attack, player.attackStartMs >= 0) && offer(FirstHint::Attack, !dungeon.PlayerSafe(), now))
		return;
	if (pending(FirstHint::Jump, player.jump.jumping) &&
		offer(FirstHint::Jump, dungeon.PlayerFacesJump(Game().camera.Facing()), now))
		return;
	if (pending(FirstHint::Climb, dungeon.PlayerOnLadder()) && offer(FirstHint::Climb, dungeon.PlayerAtLadder(), now))
		return;
	if (pending(FirstHint::PowerHit, player.Charging()) &&
		offer(FirstHint::PowerHit, player.stats.heka.Has(SkillId::PowerHit), now))
		return;
	if (pending(FirstHint::QuickHeal, false) &&
		offer(FirstHint::QuickHeal, inventory.QuickChoice(QuickKind::Health).has_value(), now))
		return;
	if (pending(FirstHint::QuickStamina, false) &&
		offer(FirstHint::QuickStamina, inventory.QuickChoice(QuickKind::Stamina).has_value(), now))
		return;
	if (pending(FirstHint::QuickAntidote, false) &&
		offer(FirstHint::QuickAntidote, inventory.Count(ItemKind::Antidote) > 0, now))
		return;
	if (pending(FirstHint::Sprint, player.stats.IsSprinting()))
		offer(FirstHint::Sprint, watch.walkedMs >= WALK_BEFORE_SPRINT_MS, now);
}
