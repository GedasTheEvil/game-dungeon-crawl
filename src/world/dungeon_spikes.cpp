// Rhythm traps on a fixed cycle (spike_cycle.h). Wall spikes (plan rhythm-traps) come out of the back wall: out, they
// block like a closed gate (isSolidTile); whoever stands in the cell as they come out takes a stab and is shoved out of
// it. A fire vent (plan fire-jet-trap) breathes fire up through its cell: it burns whoever stands in it (updateTraps).
// Drawn in dungeon_render_mechanisms.cpp.
#include "dungeon.h"
#include "../entities/player.h"
#include "rng.h"
#include "spike_cycle.h"
#include "world_events.h"
#include "../core/gameplay_config.h"
#include <cmath>

void Dungeon::updateRhythmTraps() {
	const int now = GameClock::now();
	const ViewWindow v = view();
	bool scrape = false, thunk = false, hiss = false, roar = false;
	for (int cell = 0; cell < MAP_WIDTH * MAP_HEIGHT; cell++) {
		Fixture& t = map[cell].fixture;
		if (!isRhythmTrap(t.type))
			continue;
		const SpikeState was = spikeState(t), is = spikeStateAt(t, now);
		if (was == is)
			continue;
		setSpikeState(t, is);
		const int col = cell % MAP_WIDTH, row = cell / MAP_WIDTH;
		const bool heard = v.contains(static_cast<float>(col) + 0.5f, row);
		const bool fire = t.type == FireVent;
		if (is == SpikeState::Telegraph) {
			scrape = scrape || (heard && !fire);
			hiss = hiss || (heard && fire);
		}
		if (is == SpikeState::Extended) {
			thunk = thunk || (heard && !fire);
			roar = roar || (heard && fire);
			if (!fire)
				stabWallSpikes(col, row);
		}
	}
	if (scrape) // one sound for the traps that move together
		sim.events->Play(WorldSound::SpikeScrape);
	if (thunk)
		sim.events->Play(WorldSound::SpikeThunk);
	if (hiss)
		sim.events->Play(WorldSound::FireHiss);
	if (roar)
		sim.events->Play(WorldSound::FireRoar);
}
//======================================================================================
bool Dungeon::inFireJet(float left, float right, int row) const {
	for (auto col = static_cast<int>(std::floor(left)); static_cast<float>(col) < right; col++)
		if (IsInBounds(col, row) && fireJetOn(MapAt(col, row).fixture))
			return true;
	return false;
}
//======================================================================================
// Map x just out of the cell (col, row) for a body of half width half at centre, on the near side if it is open,
// else the far side; nullopt when both are blocked.
std::optional<float> Dungeon::outOfSpikes(int col, int row, float centre, float half) const {
	const bool leftOpen = IsInBounds(col - 1, row) && !isSolidTile(MapAt(col - 1, row));
	const bool rightOpen = IsInBounds(col + 1, row) && !isSolidTile(MapAt(col + 1, row));
	bool left = centre < static_cast<float>(col) + 0.5f;
	if (left ? !leftOpen : !rightOpen)
		left = !left;
	if (left ? !leftOpen : !rightOpen)
		return std::nullopt;
	constexpr float CLEAR = 0.01f;
	return left ? static_cast<float>(col) - half - CLEAR : static_cast<float>(col + 1) + half + CLEAR;
}
//======================================================================================
void Dungeon::stabWallSpikes(int col, int row) {
	const float left = static_cast<float>(col), right = static_cast<float>(col + 1);
	const float half = sim.player->HalfWidth();
	if (sim.player->Alive() && static_cast<int>(mapY) == row && mapX + half > left && mapX - half < right) {
		sim.events->Status("Spikes shoot out of the wall!");
		if (!sim.player->Dodges(*sim.events, sim.random->gameplay))
			if (const int dmg = sim.player->stats.TrapDamage(SPIKES_STAB_DAMAGE); dmg > 0) {
				sim.player->TakeHit(dmg, SPIKE_ATTACK_MIX, *sim.events);
				sim.player->Poison(PoisonTier::Weak, *sim.events, sim.random->gameplay);
			}
		if (const auto x = outOfSpikes(col, row, mapX, PLAYER_BODY_HALF_WIDTH))
			mapX = *x;
	}
	for (Monster& mon : monsters)
		if (mon.Active() && mon.Alive() && !mon.flies() && !mon.jumping() && !mon.Hidden() && mon.Row() == row &&
			mon.Right() > left && mon.Left() < right) {
			mon.TrapHit(SPIKES_STAB_DAMAGE);
			mon.TakePoison(PoisonTier::Weak, false, sim.random->gameplay); // no XP for a trap's kill
			if (const auto x = outOfSpikes(col, row, mon.CentreX(), mon.HalfWidth()))
				mon.ShoveTo(*x);
		}
}
