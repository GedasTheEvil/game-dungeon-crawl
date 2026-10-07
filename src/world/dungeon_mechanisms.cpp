#include "dungeon.h"
#include "dungeon_rules.h"
#include "../state/assets.h"
#include "../entities/player.h"
#include "item_bag.h"
#include "journal.h"
#include "rng.h"
#include "world_events.h"
#include "../core/gameplay_config.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
int lockBit(int colour) { return 1 << (colour - 1); }
} // namespace

void Dungeon::resetMechanisms() {
	keysHeld = 0;
	levelKeys = 0;
	for (const Tile& t : map)
		if (t.type == Key && isLockColour(t.attr))
			levelKeys |= lockBit(t.attr);
	openingGates.clear();
	fallingRocks.clear();
	// A save taken mid-motion finishes it on load.
	for (Tile& t : map)
		if (t.type == Gate && gateState(t) == GateState::Opening)
			setGateState(t, GateState::Open);
		else if (t.type == RockFall && rockState(t) == RockState::Falling)
			setRockState(t, RockState::Fallen);
}
//======================================================================================
void Dungeon::updateMechanisms() {
	int col = static_cast<int>(mapX);
	int row = static_cast<int>(mapY);
	Tile here = MapAt(col, row);

	if (here.type == Key && isLockColour(here.attr) && sim.player->Alive()) {
		keysHeld |= lockBit(here.attr);
		clearObject(map[MapIndex(col, row)]);
		char text[64];
		snprintf(text, sizeof(text), "Found the %s key", lockColour(here.attr).gem);
		sim.events->Status("%s", text);
		sim.events->Play(WorldSound::KeyPickup);
		sim.journal->LearnNote(FieldNote::Keys);
	}

	if (here.type == RockFall && sim.player->Alive())
		startRockFall(MapIndex(col, row));
	// A reckless walker sets one off too (cowards keep out, flyers pass over, a leaper is in the air).
	for (const Monster& mon : monsters)
		if (mon.Active() && mon.Alive() && !mon.flies() && !mon.jumping()) {
			const auto monCol = static_cast<int>(std::floor(mon.CentreX()));
			if (IsInBounds(monCol, mon.Row()) && MapAt(monCol, mon.Row()).type == RockFall)
				startRockFall(MapIndex(monCol, mon.Row()));
		}

	// A gate the player holds the key for opens as they come up to it.
	for (int dir : {-1, 1}) {
		Tile next = MapAt(col + dir, row);
		float gap = dir > 0 ? static_cast<float>(col + 1) - mapX : mapX - static_cast<float>(col);
		if (next.type == Gate && gateState(next) == GateState::Closed && isLockColour(next.attr) &&
			(keysHeld & lockBit(next.attr)) != 0 && gap < GATE_APPROACH) {
			startOpeningGate(MapIndex(col + dir, row));
			sim.events->Play(WorldSound::GateOpen);
		}
	}

	for (auto it = openingGates.begin(); it != openingGates.end();) {
		if (motionProgress(it->startMs, GATE_OPEN_MS) >= 1.f) {
			setGateState(map[it->cell], GateState::Open);
			it = openingGates.erase(it);
		} else
			++it;
	}

	updateRocks();
}
//======================================================================================
void Dungeon::startRockFall(int cell) {
	if (map[cell].type != RockFall || rockState(map[cell]) != RockState::Armed)
		return;
	setRockState(map[cell], RockState::Falling);
	fallingRocks.push_back({cell, GameClock::now()});
	sim.events->Play(WorldSound::RockRumble);
}
//======================================================================================
void Dungeon::updateRocks() {
	for (auto it = fallingRocks.begin(); it != fallingRocks.end();) {
		if (GameClock::now() - it->startMs < ROCK_WARN_MS + ROCK_FALL_MS) {
			++it;
			continue;
		}

		// Landed: hits the player if they are still under it.
		int col = it->cell % MAP_WIDTH;
		int row = it->cell / MAP_WIDTH;
		float centreX = static_cast<float>(col) + 0.5f;
		auto floorY = static_cast<float>(row);
		sim.events->Play(WorldSound::RockCrash);
		float dx = std::fabs(mapX - centreX);
		if (dx < ROCK_GRAZE_HALF_WIDTH && mapY >= floorY - 0.2f && mapY < floorY + ROCK_HIT_HEIGHT &&
			sim.player->Alive()) {
			bool crushed = dx < ROCK_CRUSH_HALF_WIDTH;
			sim.player->TakeHit(crushed ? ROCK_CRUSH_DAMAGE : ROCK_GRAZE_DAMAGE, ROCK_ATTACK_MIX, *sim.events, true);
			sim.events->Status("%s", crushed ? "Crushed by a falling rock!" : "The rock clips your leg!");
		}
		for (Monster& mon : monsters) { // walkers under it, a leaper in the air too (a jump does not dodge it)
			const float monDx = std::fabs(mon.CentreX() - centreX);
			if (mon.Active() && mon.Alive() && !mon.flies() && mon.Row() == row && monDx < ROCK_GRAZE_HALF_WIDTH)
				mon.TrapHit(monDx < ROCK_CRUSH_HALF_WIDTH ? ROCK_CRUSH_DAMAGE : ROCK_GRAZE_DAMAGE);
		}
		setRockState(map[it->cell], RockState::Fallen);
		it = fallingRocks.erase(it);
	}
}
//======================================================================================
void Dungeon::startOpeningGate(int cell) {
	if (map[cell].type != Gate || gateState(map[cell]) != GateState::Closed)
		return;
	setGateState(map[cell], GateState::Opening);
	openingGates.push_back({cell, GameClock::now()});
}
//======================================================================================
void Dungeon::openGates(int colour) {
	bool any = false;
	for (int cell = 0; cell < MAP_WIDTH * MAP_HEIGHT; cell++)
		if (map[cell].type == Gate && map[cell].attr == colour && gateState(map[cell]) == GateState::Closed) {
			startOpeningGate(cell);
			any = true;
		}
	if (any)
		sim.events->Play(WorldSound::GateOpen);
}
//======================================================================================
void Dungeon::bumpGate(int col, int row) {
	Tile gate = MapAt(col, row);
	if (gate.type != Gate || gateState(gate) != GateState::Closed || !isGateColour(gate.attr))
		return;

	if (gate.attr != BOSS_LOCK && (keysHeld & lockBit(gate.attr)) != 0) {
		startOpeningGate(MapIndex(col, row));
		sim.events->Play(WorldSound::GateOpen);
		return;
	}

	// Locked: say why, at most every LOCKED_HINT_INTERVAL_MS while the player keeps pushing.
	int now = GameClock::now();
	if (now - lockedHintMs < LOCKED_HINT_INTERVAL_MS)
		return;
	lockedHintMs = now;
	char text[96];
	if (gate.attr == BOSS_LOCK)
		snprintf(text, sizeof(text), "%s", "Sealed. It opens when its guardian falls.");
	else
		snprintf(text, sizeof(text), "Sealed. It needs the %s key or a %s lever.", lockColour(gate.attr).gem,
				 lockColour(gate.attr).gem);
	sim.events->Status("%s", text);
	sim.events->Play(WorldSound::GateLocked);
}
//======================================================================================
bool Dungeon::PullLever() {
	int col = static_cast<int>(mapX);
	int row = static_cast<int>(mapY);
	Tile lever = MapAt(col, row);
	if (lever.type != Lever || !isLockColour(lever.attr))
		return false;
	if (leverPulled(lever))
		return true; // pulled already: the gates stay open

	pullLever(map[MapIndex(col, row)]);
	sim.events->Play(WorldSound::Lever);
	openGates(lever.attr);
	char text[64];
	snprintf(text, sizeof(text), "Somewhere a %s gate grinds open", lockColour(lever.attr).gem);
	sim.events->Status("%s", text);
	return true;
}
