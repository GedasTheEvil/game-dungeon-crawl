// Sandstorm halls (plan sandstorm-halls): sand gusts on a cycle (spike_cycle.h, Dungeon::updateRhythmTraps) blow along
// the row through their cells. A gust pushes the player and the light monsters in it downwind, walls stop them; a cell
// of the hall without a gust is a shelter. Near a hall the haze closes in (StormHaze) and in a gust the wind drowns the
// other sounds (SoundMuffle); arrows and spit drift (dungeon_arrows.cpp). The blown sand is drawn in
// dungeon_render_sand.cpp.
#include "dungeon.h"
#include "../entities/player.h"
#include "spike_cycle.h"
#include "world_events.h"
#include "../core/gameplay_config.h"
#include <algorithm>
#include <cmath>

int Dungeon::gustAt(float x, int row) const {
	const auto col = static_cast<int>(std::floor(x));
	if (!IsInBounds(col, row))
		return 0;
	const Fixture& f = MapAt(col, row).fixture;
	return gustBlowing(f) ? gustDir(f) : 0;
}
//======================================================================================
namespace {
// A light monster a gust takes along: the small walkers and the flyers (MonsterKind::weight), awake and in the open; no
// boss, nothing rooted, lying in wait, burrowed or in a leap.
bool blownAway(const Monster& mon) {
	if (!mon.Active() || !mon.Alive() || mon.Hidden() || mon.jumping() || mon.Roosting())
		return false;
	const MonsterType& kind = *mon.Type();
	const bool moves = kind.locomotion == Locomotion::Walk || leaps(kind.locomotion) || mon.flies();
	return moves && kind.weight <= GUST_LIGHT_WEIGHT && !isBossMonster(kind.id) && !ambushes(kind.locomotion);
}
} // namespace
//======================================================================================
void Dungeon::stormTarget(float& haze, float& blow) const {
	haze = 0.f;
	blow = 0.f;
	const int now = GameClock::now();
	const auto row = static_cast<int>(mapY);
	const auto reach = static_cast<int>(std::ceil(GUST_HAZE_REACH));
	const auto here = static_cast<int>(std::floor(mapX));
	for (int col = here - reach; col <= here + reach; col++) {
		if (!IsInBounds(col, row) || MapAt(col, row).fixture.type != SandGust)
			continue;
		const Fixture& f = MapAt(col, row).fixture;
		const float off = std::max(0.f, std::fabs(static_cast<float>(col) + 0.5f - mapX) - 0.5f);
		const float near = std::max(0.f, 1.f - off / GUST_HAZE_REACH);
		haze = std::max(haze, near * gustStrength(f, now));
		if (spikeStateAt(f, now) == SpikeState::Extended)
			blow = std::max(blow, near);
	}
}
//======================================================================================
void Dungeon::updateGusts() {
	float haze = 0.f, blow = 0.f;
	stormTarget(haze, blow);
	const float k = std::min(1.f, static_cast<float>(UPDATE_TICK_MS) / static_cast<float>(GUST_HAZE_FADE_MS));
	storm += (haze - storm) * k;
	muffle += (GUST_MUFFLE * blow - muffle) * k;

	const float step = GUST_PUSH_SPEED * static_cast<float>(UPDATE_TICK_MS) / 1000.f;
	const int dir = sim.player->Alive() && PlayerHeldBy() == nullptr && !PlayerOnLadder()
						? gustAt(mapX, static_cast<int>(mapY))
						: 0;
	if (dir != 0) {
		const float to = mapX + static_cast<float>(dir) * step;
		if (!bodyBlocked(to, dir, mapY))
			mapX = to;
		if (!playerInGust) {
			sim.events->Status("%s", "The wind throws sand in your face!");
			sim.events->Note(FieldNote::Sandstorm);
		}
	}
	playerInGust = dir != 0;

	const float monsterStep = GUST_MONSTER_PUSH_SPEED * static_cast<float>(UPDATE_TICK_MS) / 1000.f;
	for (Monster& mon : monsters) {
		if (!blownAway(mon))
			continue;
		const int way = gustAt(mon.CentreX(), mon.Row());
		if (way == 0)
			continue;
		const float to = mon.CentreX() + static_cast<float>(way) * monsterStep;
		const float front = to + static_cast<float>(way) * mon.HalfWidth();
		const auto edge = static_cast<int>(std::floor(front));
		// A walker stops where it would not walk (a wall, a drop, a trap, a step), a flyer at a wall.
		if (mon.flies() ? !isSolidTile(MapAt(edge, mon.Row()))
						: !walkerBlocked(edge, mon.Row()) && stepAt(mon, front) == StepAhead::Walk)
			mon.ShoveTo(to);
	}
}
