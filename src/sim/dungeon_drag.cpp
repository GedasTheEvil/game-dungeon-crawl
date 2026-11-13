// Dragging (plan mirror-drag): the player takes hold of a mirror in the next cell and drags it along the flat
// corridor, crouched, facing it: walking away pulls it, walking at it pushes it, a cell at a time.
#include "dungeon.h"
#include "player.h"
#include "dungeon_rules.h"
#include "../level/decor.h"
#include "../level/gameplay_config.h"
#include "../level/shrines.h"
#include "../level/world_events.h"
#include <cmath>
#include <cstdlib>

namespace {
constexpr int DRAG_SETTLE_MS = 150; // let go, the thing slides from where it was drawn to its cell's centre
float cellCentre(int col) { return static_cast<float>(col) + 0.5f; }
int colOf(int cell) { return cell % LEVEL_WIDTH; }
int rowOf(int cell) { return cell / LEVEL_WIDTH; }
} // namespace

bool Dungeon::dragGround(int col, int row, bool thing) const {
	if (!IsInBounds(col, row) || !IsInBounds(col, row - 1))
		return false;
	const Tile& t = MapAt(col, row);
	if (t.structure != Structure::Empty || hasShape(t) || isSolidTile(t) || !isWall(MapAt(col, row - 1)))
		return false;
	if (!thing)
		return true;
	if (hasObject(t) || hasFixture(t))
		return false;
	// A prop the player uses there would be under the thing, its prompt lost (InteractVerb asks the mirror first).
	const int8_t prop = decoration.decor[MapIndex(col, row)].type;
	return prop != DECOR_STARGATE && prop != DECOR_DIAL && prop != DECOR_BRAZIER && !shrineGod(prop);
}
//======================================================================================
int Dungeon::draggableAt(int facing) const {
	const auto col = static_cast<int>(std::floor(mapX)), row = static_cast<int>(std::floor(mapY));
	if (IsInBounds(col, row) && isDraggable(MapAt(col, row)))
		return MapIndex(col, row); // standing at it, where it is turned
	const int next = col + (facing < 0 ? -1 : 1);
	if (!IsInBounds(next, row) || !isDraggable(MapAt(next, row)) || std::fabs(cellCentre(next) - mapX) > DRAG_REACH)
		return -1;
	return MapIndex(next, row);
}
//======================================================================================
bool Dungeon::ToggleDrag(int facing) {
	if (Dragging()) {
		LetGo();
		return false;
	}
	const Player& player = *sim.player;
	if (!player.Alive() || won || player.jump.jumping || player.jump.falling || PlayerHeldBy() != nullptr ||
		PlayerOnLadder())
		return false;
	const int cell = draggableAt(facing);
	if (cell < 0) {
		sim.events->Status("%s", "Nothing here to drag");
		return false;
	}
	const auto col = static_cast<int>(std::floor(mapX)), row = static_cast<int>(std::floor(mapY));
	const int thingCol = colOf(cell);
	if (!dragGround(col, row, false) || !dragGround(thingCol, row, false)) {
		sim.events->Status("%s", "It will not slide here: only on bare, flat floor");
		return false;
	}
	const float offset = static_cast<float>(facing < 0 ? -1 : 1) * DRAG_HOLD; // the thing the way they face
	mapX = cellCentre(thingCol) - offset; // a step to it (or back from it, standing at it): in its cell
	drag = {cell, offset};
	sim.events->Status("%s", "You take hold of the mirror");
	return true;
}
//======================================================================================
void Dungeon::LetGo() {
	if (!Dragging())
		return;
	dragSettle = {drag.cell, dragShift(drag.cell), GameClock::now()};
	drag = {};
}
//======================================================================================
int Dungeon::DragFacing() const {
	if (!Dragging())
		return 0;
	return drag.offset < 0.f ? -1 : 1;
}
//======================================================================================
bool Dungeon::dragTo(float x) {
	const auto row = static_cast<int>(std::floor(mapY));
	if (!dragGround(static_cast<int>(std::floor(x)), row, false))
		return false;
	const int from = colOf(drag.cell);
	const auto to = static_cast<int>(std::floor(x + drag.offset));
	if (to == from)
		return true;
	if (std::abs(to - from) != 1 || !dragGround(to, row, true))
		return false;
	const int cell = MapIndex(to, row);
	setObject(map[cell], map[drag.cell]);
	clearObject(map[drag.cell]);
	for (MirrorSwing& m : mirrorSwings)
		if (m.cell == drag.cell)
			m.cell = cell;
	drag.cell = cell;
	traceSunBeams(); // the beams follow the mirror at once
	sim.events->Play(WorldSound::MirrorTurn); // the bronze stand scrapes over the stone
	return true;
}
//======================================================================================
void Dungeon::updateDrag() {
	if (!Dragging())
		return;
	const Player& player = *sim.player;
	const auto row = static_cast<int>(std::floor(mapY));
	const float apart = std::fabs(cellCentre(colOf(drag.cell)) - (mapX + drag.offset));
	// A teleport, a fall, a load: the thing is no longer in reach on the player's row.
	const bool inReach = rowOf(drag.cell) == row && apart <= 0.5f;
	if (!player.Alive() || won || player.jump.jumping || player.jump.falling || PlayerHeldBy() != nullptr ||
		!isDraggable(map[drag.cell]) || !inReach)
		LetGo();
}
//======================================================================================
float Dungeon::dragShift(int cell) const {
	if (Dragging() && cell == drag.cell)
		return mapX + drag.offset - cellCentre(colOf(cell));
	if (cell != dragSettle.cell)
		return 0.f;
	const float p = motionProgress(dragSettle.startMs, DRAG_SETTLE_MS);
	return p >= 1.f ? 0.f : dragSettle.from * (1.f - p * p * (3.f - 2.f * p));
}
