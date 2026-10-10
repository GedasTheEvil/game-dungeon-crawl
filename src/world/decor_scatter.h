#ifndef DECOR_SCATTER_H
#define DECOR_SCATTER_H

// The decoration rules without the drawing (Dungeon draws them): which prop, decal, torch, ladder piece and surface
// each cell of a level gets. Every cell rolls from (level file name, cell index), so a level looks the same on every
// load and a cell does not depend on the rest of the map.

#include "decor.h"
#include "level.h"
#include <cstdint>

// One entry per cell, indexed as LevelGrid (row * LEVEL_WIDTH + column).
struct DecorLayout {
	DecorCell decor[LEVEL_CELL_COUNT];
	DecalCell decal[LEVEL_CELL_COUNT];
	SurfaceCell surface[LEVEL_CELL_COUNT];
	bool torch[LEVEL_CELL_COUNT] = {};
	LadderCell ladder[LEVEL_CELL_COUNT];
};

// The fires among the decorations: a brazier's or an oil lamp's flame, a torch's. The drawing gives each its fire
// sprite and its light.
enum class FlameKind : unsigned char { Brazier, OilLamp, Torch };
// A flame in its cell, tile units: x from the cell's centre (a mirrored prop's flipped), y up from the floor, z out
// from the back wall. seed: its flicker.
struct Flame {
	float x, y, z;
	FlameKind kind;
	uint32_t seed;
};
constexpr int MAX_CELL_FLAMES = 2; // a prop's fire and a torch
// The flames in cell (LevelGrid index): how many it wrote to out.
int flamesAt(const DecorLayout& layout, int cell, Flame (&out)[MAX_CELL_FLAMES]);

// What a scatter placed, for the log.
struct DecorCounts {
	int props = 0;
	int torches = 0;
	int decals = 0;
	int ladderShafts = 0;
	int painted = 0, stone = 0, rough = 0; // surface cells by family
};

// An empty floor cell within MINION_SUMMON_REACH of a boss whose minions climb out of coffins (Summon::Coffin) on its
// row, alive or slain (slainMonster): a coffin for its minions stands there. cells: a whole level.
[[nodiscard]] bool bossCoffinCell(const Tile* cells, int col, int row);

// Props, decals, torches, ladders and surfaces, seeded by the level's file name (the part after the last '/'), from
// the decoration tiers its depth unlocks (decorTier). Every mummy's spawn tile and every boss coffin cell gets a
// coffin. Overwrites all of layout.
DecorCounts scatterDecor(const Tile* cells, const char* levelName, int depth, DecorLayout& layout);

// The highest decoration tier among a level's props, decals and surfaces; -1 for a bare level.
[[nodiscard]] int decorTierUsed(const Tile* cells, const DecorLayout& layout);

#endif
