// The checker's dressing checks (plan split-large-sources, moved from level_check.cpp): placed props, decals and
// containers where they fit.
#include "level_check_internal.h"
#include "climb_path.h"
#include "decor_scatter.h"
#include "movement.h"
#include "inscriptions.h"
#include "items.h"
#include "monster_kinds.h"
#include "spike_cycle.h"
#include "sun_beam.h"
#include "tile_defs.h"
#include <algorithm>
#include <cstdio>
#include <map>
#include <numeric>
#include <optional>
#include <queue>
#include <set>
#include "level_check.h"

namespace levelcheck {
// Things a placed prop would stand in front of or in the way of, on screen: a web hangs in the corner over them all.
bool hidesBehindProp(const Tile& t) {
	switch (t.type) {
	case Treasure:
	case TrappedTreasure:
	case Door:
	case Gate:
	case Ankh:
	case Ladder:
	case Mirror:
	case MonsterSpawn:
		return true;
	default:
		break;
	}
	return hasFixture(t);
}

// The scatter's own prop for the cell, which beats a placed one: a coffin, the skeleton under a page, an ushabti.
bool scatterOwnsProp(const LevelGrid& grid, int col, int row) {
	const Tile t = grid.at(col, row);
	const MonsterKind* kind = monsterKind(spawnType(t));
	return t.type == Page || bossCoffinCell(grid.cells, col, row) ||
		   (kind != nullptr && (kind->locomotion == Locomotion::Entombed || kind->locomotion == Locomotion::Figurine));
}

// A shaped cell's floor for a placed prop (plan subcell-checker): the same height in every sub-cell column, in
// quarters, with nothing solid over it; -1 when it is not flat. A cell with no shape: 0.
int flatFloor(const Tile& t) {
	const int floor = nextSubcellRow(t, 0, 0, false);
	for (int s = 0; s < SUBCELLS; s++)
		if (nextSubcellRow(t, s, 0, false) != floor || nextSubcellRow(t, s, floor, true) != SUBCELLS)
			return -1;
	return floor;
}

// The sub-cells of the back wall a decal or a torch is drawn on, by its anchor; a solid sub-cell there hides it.
std::uint16_t decalSubcells(DecalAnchor anchor) {
	constexpr std::uint16_t BOTTOM_ROW = 0x000f;
	constexpr std::uint16_t TOP_ROW = 0xf000;
	constexpr std::uint16_t MIDDLE = 0x0660; // the middle two columns of the middle two rows
	return anchor == DecalAnchor::Floor ? BOTTOM_ROW : (anchor == DecalAnchor::Ceiling ? TOP_ROW : MIDDLE);
}

void checkPlacedProp(const LevelGrid& grid, int cell, LevelReport& r) {
	const int col = cell % LEVEL_WIDTH;
	const int row = cell / LEVEL_WIDTH;
	const Tile t = grid.cells[cell];
	const int prop = t.dressing.prop;
	if (prop < 0)
		return;
	const std::string what = std::string("the ") + DECOR_NAMES[prop] + " at " + at(cell);
	if (isWall(t)) {
		r.warnings.push_back(what + " is in rock");
		return;
	}
	if (scatterOwnsProp(grid, col, row))
		r.warnings.push_back(what + " is not shown: the cell has its own (a coffin, a skeleton, an ushabti)");
	if (isWaterProp(prop) && !inHalfWater(t))
		r.warnings.push_back(what + " is a water prop out of half water");
	if (!isWaterProp(prop) && (t.structure == Structure::HalfWater || t.structure == Structure::DeepWater))
		r.warnings.push_back(what + " stands in water");
	const int step = flatFloor(t);
	if (step < 0)
		r.warnings.push_back(what + " does not fit the cell's shape: its floor is not flat, or rock is over it");
	if (!isWaterProp(prop) && prop != DECOR_WEB && !isWall(grid.at(col, row - 1)) && step <= 0)
		r.warnings.push_back(what + " has no floor under it");
	if (prop == DECOR_WEB && !isWall(grid.at(col, row + 1)))
		r.warnings.push_back(what + " has no ceiling");
	if (prop != DECOR_WEB && hidesBehindProp(t))
		r.warnings.push_back(
			what + " stands in front of the " + (hasObject(t) ? tileDef(t.type).name : tileDef(t.fixture.type).name)
		);
}

void checkPlacedDecal(const LevelGrid& grid, int cell, LevelReport& r) {
	const int col = cell % LEVEL_WIDTH;
	const int row = cell / LEVEL_WIDTH;
	const Tile t = grid.cells[cell];
	const Dressing& d = t.dressing;
	const bool torch = d.torch == TorchPlacing::Torch;
	if (d.decal >= 0) {
		const DecalAnchor anchor = DECAL_DEFS[d.decal].anchor;
		const std::string what = "decal " + std::to_string(d.decal) + " at " + at(cell);
		if (isWall(t))
			r.warnings.push_back(what + " is in rock");
		else if (!tileDef(t.type).decals || !tileDef(t.fixture.type).decals)
			r.warnings.push_back(what + " is on a cell that takes no decals");
		if (anchor == DecalAnchor::Ceiling && !isWall(grid.at(col, row + 1)))
			r.warnings.push_back(what + " hangs from the ceiling, there is none");
		if (anchor == DecalAnchor::Floor &&
			(!isWall(grid.at(col, row - 1)) || t.structure != Structure::Empty || d.prop >= 0))
			r.warnings.push_back(what + " grows from the floor: no floor, in water or sand, or behind a prop");
		if (anchor == DecalAnchor::Free && torch)
			r.warnings.push_back(what + " is behind the torch");
		if (hasShape(t) && (t.shape & decalSubcells(anchor)) != 0)
			r.warnings.push_back(what + " is hidden by the cell's shape");
	}
	if (!torch)
		return;
	const std::string what = "the torch at " + at(cell);
	if (isWall(t))
		r.warnings.push_back(what + " is in rock");
	else if (t.structure == Structure::DeepWater)
		r.warnings.push_back(what + " is in deep water");
	else if (!tileDef(t.type).torch || !tileDef(t.fixture.type).torch)
		r.warnings.push_back(what + " is on a cell that takes no torch");
	if (hasShape(t) && (t.shape & decalSubcells(DecalAnchor::Free)) != 0)
		r.warnings.push_back(what + " is hidden by the cell's shape");
	if (d.prop == DECOR_BRAZIER || d.prop == DECOR_LAMP || isThoth(d.prop) || d.prop == DECOR_STARGATE)
		r.warnings.push_back(what + " is over a " + DECOR_NAMES[d.prop]);
}

// The looks layer (plan placed-container-looks): a look on a treasure, of a kind that has a stance there.
void checkPlacedContainer(const LevelGrid& grid, int cell, LevelReport& r) {
	const Dressing& d = grid.cells[cell].dressing;
	if (!hasPlacedContainer(d))
		return;
	const std::string what = std::string("the ") + CONTAINER_NAMES[d.container] + " look at " + at(cell);
	if (!isTreasure(grid.cells[cell])) {
		r.warnings.push_back(what + " is on a cell without a treasure");
		return;
	}
	if (d.lid == CoffinLid::Plain && grid.cells[cell].type == TrappedTreasure)
		r.warnings.push_back(what + " has a plain lid on a trapped treasure: a trapped coffin always shows the text");
	uint32_t weights[STANCE_COUNT];
	if (containerStances(
			grid.cells,
			cell % LEVEL_WIDTH,
			cell / LEVEL_WIDTH,
			static_cast<Container>(d.container),
			weights
		) == 0)
		r.warnings.push_back(what + " has no stance that fits the cell");
}

// The decor, decal and looks layers (plan placed-decor): a placed prop, decal, torch or container where it fits.
void checkDressing(const LevelGrid& grid, LevelReport& r) {
	for (int cell = 0; cell < CELLS; cell++) {
		checkPlacedProp(grid, cell, r);
		checkPlacedDecal(grid, cell, r);
		checkPlacedContainer(grid, cell, r);
	}
}

} // namespace levelcheck
