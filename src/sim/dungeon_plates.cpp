// Pressure plates (plan pressure-plate): a plate in the floor carries the weight of whoever and whatever stands on it;
// while every plate of a colour carries the weight it needs, the plate gates of that colour stand open, under it they
// come down again. Drawn in dungeon_render_mechanisms.cpp; dragging the weights there: dungeon_drag.cpp.
#include "dungeon.h"
#include "player.h"
#include "dungeon_rules.h"
#include "../level/containers.h"
#include "../level/gameplay_traps.h"
#include "../level/gameplay_world.h"
#include "../level/world_events.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

int Dungeon::WeightOn(int col, int row) const {
	if (!IsInBounds(col, row))
		return 0;
	const int cell = MapIndex(col, row);
	const float centre = static_cast<float>(col) + 0.5f;
	int weight = 0;
	const JumpState& jump = sim.player->jump;
	if (!jump.jumping && !jump.falling && static_cast<int>(std::floor(mapY)) == row &&
		std::fabs(mapX - centre) <= PRESSURE_PLATE_HALF_WIDTH)
		weight += PLAYER_WEIGHT;
	for (const Monster& mon : monsters) {
		if (!mon.Active() || mon.Row() != row || std::fabs(mon.CentreX() - centre) > PRESSURE_PLATE_HALF_WIDTH)
			continue;
		if (!mon.Alive())
			weight += std::max(1, mon.Type()->weight); // a body lies on it, a bat's too
		else if (!mon.flies() && !mon.jumping() && !mon.Hidden())
			weight += mon.Type()->weight;
	}
	const Tile& t = map[cell];
	if (t.type == Stone)
		weight += STONE_WEIGHT;
	else if (t.type == Mirror)
		weight += MIRROR_WEIGHT;
	else if (!hasObject(t) && containers[cell].emptied && treasureLook(cell).kind == Container::Chest)
		weight += CHEST_WEIGHT;
	return weight;
}
bool Dungeon::gateCellFree(int cell) const {
	const int col = cell % MAP_WIDTH, row = cell / MAP_WIDTH;
	const float centre = static_cast<float>(col) + 0.5f;
	if (static_cast<int>(std::floor(mapY)) == row && std::fabs(mapX - centre) < 0.5f + sim.player->HalfWidth())
		return false;
	for (const Monster& mon : monsters)
		if (mon.Active() && mon.Row() == row && std::fabs(mon.CentreX() - centre) < 0.5f + mon.HalfWidth())
			return false;
	return true;
}
void Dungeon::updatePlates() {
	const PressurePlates::Weighed w = plates.Weigh(map, [this](int col, int row) { return WeightOn(col, row); });
	if (w.clicked)
		sim.events->Play(WorldSound::PlateClick);

	const int now = GameClock::now();
	std::vector<int> opened;
	bool moved = false;
	for (int cell = 0; cell < MAP_WIDTH * MAP_HEIGHT; cell++) {
		const Tile& gate = map[cell];
		if (gate.type != Gate || !isPlateGateColour(gate.attr))
			continue;
		const int lock = plateLockOf(gate.attr);
		const GateState before = gateState(gate);
		if (w.plates[lock] && w.held[lock]) {
			gates.OpenPlate(map, cell, now);
			if (before == GateState::Closed && gateState(gate) == GateState::Opening && !plates.OpenedOnce(lock))
				opened.push_back(cell);
		} else if (before == GateState::Opening || (before == GateState::Open && gateCellFree(cell)))
			gates.ClosePlate(map, cell, now);
		moved = moved || gateState(gate) != before;
	}
	gates.FinishClosed(map);
	if (moved && w.settled)
		sim.events->Play(WorldSound::GateOpen);
	if (opened.empty())
		return;
	for (int cell : opened)
		plates.MarkOpened(plateLockOf(map[cell].attr));
	if (!w.settled)
		return;
	char text[80];
	snprintf(
		text,
		sizeof(text),
		"The plate sinks. Somewhere a %s silver gate rises",
		lockColour(map[opened[0]].attr).gem
	);
	sim.events->Status("%s", text);
	sim.events->Note(FieldNote::PressurePlates);
	startCutScene(opened);
}
