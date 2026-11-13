// Dragging (plans mirror-drag, pressure-plate): the player takes hold of a mirror, a stone, an emptied chest or a
// monster's body in the next cell and drags it along the flat corridor, crouched, facing it: walking away pulls it,
// walking at it pushes it. An object or a chest moves a cell at a time, a body follows the player smoothly.
#include "dungeon.h"
#include "player.h"
#include "dungeon_rules.h"
#include "../level/containers.h"
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
	// A pressure plate is where things go (plan pressure-plate); any other fixture keeps them off.
	if (hasObject(t) || (hasFixture(t) && t.fixture.type != PressurePlate) || containers[MapIndex(col, row)].opened)
		return false;
	// A prop the player uses there would be under the thing, its prompt lost (InteractVerb asks the mirror first).
	const int8_t prop = decoration.decor[MapIndex(col, row)].type;
	return prop != DECOR_STARGATE && prop != DECOR_DIAL && prop != DECOR_BRAZIER && !shrineGod(prop);
}
//======================================================================================
Dungeon::Drag Dungeon::thingIn(int col, int row) const {
	if (!IsInBounds(col, row))
		return {};
	const int cell = MapIndex(col, row);
	if (isDraggable(map[cell]))
		return {DragKind::Object, cell};
	if (!hasObject(map[cell]) && containers[cell].emptied && treasureLook(cell).kind == Container::Chest)
		return {DragKind::Chest, cell};
	for (int i = 0; i < MAX_MONSTERS; i++) {
		const Monster& mon = monsters[i];
		if (mon.Active() && !mon.Alive() && mon.Row() == row && static_cast<int>(std::floor(mon.CentreX())) == col)
			return {DragKind::Body, cell, i};
	}
	return {};
}
//======================================================================================
Dungeon::Drag Dungeon::findDraggable(int facing) const {
	const auto col = static_cast<int>(std::floor(mapX)), row = static_cast<int>(std::floor(mapY));
	if (const Drag here = thingIn(col, row); here.cell >= 0)
		return here; // standing at it (where a mirror is turned)
	const Drag next = thingIn(col + (facing < 0 ? -1 : 1), row);
	if (next.cell < 0)
		return next;
	const float centre = next.kind == DragKind::Body ? monsters[next.slot].CentreX() : cellCentre(colOf(next.cell));
	return std::fabs(centre - mapX) <= DRAG_REACH ? next : Drag{};
}
//======================================================================================
float Dungeon::heldCentre() const {
	if (drag.kind == DragKind::Body)
		return monsters[drag.slot].CentreX();
	return cellCentre(colOf(drag.cell));
}
//======================================================================================
bool Dungeon::stillHeld() const {
	switch (drag.kind) {
	case DragKind::Object:
		return isDraggable(map[drag.cell]);
	case DragKind::Chest:
		return containers[drag.cell].emptied && !hasObject(map[drag.cell]);
	case DragKind::Body: {
		const Monster& mon = monsters[drag.slot];
		return mon.Active() && !mon.Alive();
	}
	}
	return false;
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
	Drag held = findDraggable(facing);
	if (held.cell < 0) {
		sim.events->Status("%s", "Nothing here to drag");
		return false;
	}
	const auto col = static_cast<int>(std::floor(mapX)), row = static_cast<int>(std::floor(mapY));
	if (!dragGround(col, row, false) || !dragGround(colOf(held.cell), row, false)) {
		sim.events->Status("%s", "It will not slide here: only on bare, flat floor");
		return false;
	}
	held.offset = static_cast<float>(facing < 0 ? -1 : 1) * DRAG_HOLD; // the thing the way they face
	drag = held;
	mapX = heldCentre() - held.offset; // a step to it (or back from it, standing at it): in its cell
	const char* what = drag.kind == DragKind::Chest	  ? "the chest"
					   : drag.kind == DragKind::Body  ? "the body"
					   : map[drag.cell].type == Stone ? "the stone"
													  : "the mirror";
	sim.events->Status("You take hold of %s", what);
	return true;
}
//======================================================================================
void Dungeon::LetGo() {
	if (!Dragging())
		return;
	if (drag.kind != DragKind::Body)
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
	if (to != from && (std::abs(to - from) != 1 || !dragGround(to, row, true)))
		return false;
	const int cell = MapIndex(to, row);
	switch (drag.kind) {
	case DragKind::Body: // it slides with the player, not a cell at a time
		monsters[drag.slot].DragBodyTo(x + drag.offset);
		drag.cell = cell;
		return true;
	case DragKind::Object:
		if (to == from)
			return true;
		setObject(map[cell], map[drag.cell]);
		clearObject(map[drag.cell]);
		for (MirrorSwing& m : mirrorSwings)
			if (m.cell == drag.cell)
				m.cell = cell;
		traceSunBeams(); // the beams follow a mirror at once
		break;
	case DragKind::Chest:
		if (to == from)
			return true;
		containers[cell] = containers[drag.cell];
		containers[cell].dropped = true; // its look goes with it, and into a save (a dropped chest's)
		containers[cell].droppedKind = Container::Chest;
		containers[drag.cell] = {};
		break;
	}
	drag.cell = cell;
	sim.events->Play(WorldSound::MirrorTurn); // the thing scrapes over the stone
	return true;
}
//======================================================================================
void Dungeon::updateDrag() {
	if (!Dragging())
		return;
	const Player& player = *sim.player;
	const auto row = static_cast<int>(std::floor(mapY));
	const bool held = stillHeld();
	const float apart = held ? std::fabs(heldCentre() - (mapX + drag.offset)) : 0.f;
	// A teleport, a fall, a load: the thing is no longer in reach on the player's row.
	const bool inReach = rowOf(drag.cell) == row && apart <= 0.5f;
	if (!player.Alive() || won || player.jump.jumping || player.jump.falling || PlayerHeldBy() != nullptr || !held ||
		!inReach)
		LetGo();
}
//======================================================================================
float Dungeon::dragShift(int cell) const {
	if (Dragging() && cell == drag.cell && drag.kind != DragKind::Body)
		return mapX + drag.offset - cellCentre(colOf(cell));
	if (cell != dragSettle.cell)
		return 0.f;
	const float p = motionProgress(dragSettle.startMs, DRAG_SETTLE_MS);
	return p >= 1.f ? 0.f : dragSettle.from * (1.f - p * p * (3.f - 2.f * p));
}
