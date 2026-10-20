// Sand and quicksand (plan desert-chapter): the walk in sand, sinking in quicksand, the sand fall's drift.
#include "dungeon.h"
#include "../entities/player.h"
#include "world_events.h"
#include "../core/gameplay_config.h"
#include "../graphics/render_config.h"
#include <algorithm>
#include <cmath>

bool Dungeon::PlayerInSand() const {
	const JumpState& jump = sim.player->jump;
	return !jump.jumping && !jump.falling && inSand(Map(mapX, mapY));
}
//======================================================================================
// Sinking: the longer the player stands in quicksand, the slower the walk (PlayerWalkFactor), stuck at
// QUICKSAND_STUCK_MS, hurt from QUICKSAND_HURT_MS. Out of it (a jump, a step onto firm ground) it starts over.
void Dungeon::updateQuicksand() {
	if (!PlayerInQuicksand() || !sim.player->Alive()) {
		sunkMs = 0;
		return;
	}
	if (sunkMs == 0) {
		sim.events->Status("%s", "The sand gives way under your feet!");
		sim.events->Note(FieldNote::Quicksand);
		nextSinkHurtMs = QUICKSAND_HURT_MS;
	}
	sunkMs += UPDATE_TICK_MS;
	if (sunkMs < nextSinkHurtMs)
		return;
	nextSinkHurtMs += QUICKSAND_HURT_EVERY_MS;
	const int dmg = std::max(1, sim.player->stats.CurrentMaxHP() * QUICKSAND_HURT_PERCENT / 100);
	sim.player->TakeHit(dmg, ROCK_ATTACK_MIX, *sim.events, true);
	sim.events->Status("%s", "The sand squeezes you! Jump!");
}
//======================================================================================
float Dungeon::QuicksandSink() const {
	const float p = std::min(static_cast<float>(sunkMs) / static_cast<float>(QUICKSAND_STUCK_MS), 1.f);
	return QUICKSAND_SINK_DEPTH * RenderConfig::TILE_SIZE * p;
}
//======================================================================================
void Dungeon::landSandFall(int cell) {
	const int col = cell % MAP_WIDTH;
	const int row = cell / MAP_WIDTH;
	const float centreX = static_cast<float>(col) + 0.5f;
	sim.events->Play(WorldSound::SandPour);
	if (map[cell].structure == Structure::Empty) // buried under a drift
		map[cell].structure = Structure::Sand;
	const float dx = std::fabs(mapX - centreX);
	if (dx < ROCK_GRAZE_HALF_WIDTH && mapY >= static_cast<float>(row) - 0.2f &&
		mapY < static_cast<float>(row) + ROCK_HIT_HEIGHT && sim.player->Alive()) {
		sim.player->TakeHit(SAND_FALL_DAMAGE, ROCK_ATTACK_MIX, *sim.events, true);
		sandSlowUntilMs = GameClock::now() + SAND_SLOW_MS;
		sim.events->Status("%s", "Buried in sand!");
	}
	for (Monster& mon : monsters)
		if (mon.Active() && mon.Alive() && !mon.flies() && mon.Row() == row &&
			std::fabs(mon.CentreX() - centreX) < ROCK_GRAZE_HALF_WIDTH)
			mon.TrapHit(SAND_FALL_DAMAGE);
}
