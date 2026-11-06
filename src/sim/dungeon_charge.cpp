// Chargers on the level (monster_charge.cpp, Sobek, the hippo, Seth): the rush starts with a clear row, ends at what
// blocks it; the bursts of sand: Seth's storm roar as his stun ends, Rerek's spray as he comes up.
#include "dungeon.h"
#include "player.h"
#include "../level/world_events.h"
#include <algorithm>
#include <cmath>

bool Dungeon::updateCharger(Monster& mon) {
	if (mon.Charging()) {
		const float probe = mon.HeadX() + 0.05f * static_cast<float>(mon.ChargeDir());
		const auto col = static_cast<int>(std::floor(probe));
		// A step or a low opening stops the rush as a wall does (plan subcell-monsters).
		const bool wall = walkerBlocked(col, mon.Row(), true) || stepAt(mon, probe) != StepAhead::Walk;
		if (mon.UpdateCharge(wall, mapX, mapY) && mon.Type()->roar)
			sandBurst(mon, *mon.Type()->roar);
		return true;
	}
	if (monstersStandDown() || !mon.canCharge(mapX, mapY) || !clearRow(mon.HeadX(), mapX, mon.Row()))
		return false;
	mon.StartCharge(mapX);
	return true;
}
//======================================================================================
void Dungeon::sandBurst(const Monster& mon, const RoarRules& roar) {
	if (!sim.player->Alive() || static_cast<int>(mapY) != mon.Row())
		return;
	const float half = sim.player->HalfWidth();
	if (mapX + half < mon.Left() - roar.radius || mapX - half > mon.Right() + roar.radius)
		return;
	hazeStartMs = GameClock::now();
	hazeMs = roar.hazeMs;
	ShovePlayer(mapX < mon.CentreX() ? -1 : 1, roar.shove);
	if (roar.damage <= 0 || sim.player->Dodges(*sim.events, sim.random->gameplay))
		return;
	sim.player->TakeHit(roar.damage, ROAR_MIX, *sim.events);
	sim.journal->HitByCreature(mon.Type()->id, levelNumber);
}
//======================================================================================
void Dungeon::ShovePlayer(int dir, float tiles) {
	if (PlayerHeldBy() != nullptr)
		return;
	shove = {tiles, dir};
}
//======================================================================================
void Dungeon::updateShove() {
	if (shove.left <= 0.f)
		return;
	const float step = std::min(shove.left, SHOVE_SPEED * static_cast<float>(UPDATE_TICK_MS) / 1000.f);
	const float to = mapX + static_cast<float>(shove.dir) * step;
	if (bodyBlocked(to, shove.dir, mapY)) {
		shove = Shove{};
		return;
	}
	mapX = to;
	shove.left -= step;
}
//======================================================================================
float Dungeon::SandHaze() const {
	if (hazeMs <= 0)
		return 0.f;
	const int age = GameClock::now() - hazeStartMs;
	if (age < 0 || age >= hazeMs)
		return 0.f;
	return 1.f - static_cast<float>(age) / static_cast<float>(hazeMs);
}
