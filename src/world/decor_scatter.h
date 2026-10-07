#ifndef DECOR_SCATTER_H
#define DECOR_SCATTER_H

// The decoration rules without the drawing (Dungeon draws them): which prop, decal, torch, ladder piece and surface
// each cell of a level gets. Every cell rolls from (level file name, cell index), so a level looks the same on every
// load and a cell does not depend on the rest of the map.

#include "decor.h"
#include "level.h"
#include <functional>

// One entry per cell, indexed as LevelGrid (row * LEVEL_WIDTH + column).
struct DecorLayout {
	DecorCell decor[LEVEL_CELL_COUNT];
	DecalCell decal[LEVEL_CELL_COUNT];
	SurfaceCell surface[LEVEL_CELL_COUNT];
	bool torch[LEVEL_CELL_COUNT] = {};
	LadderCell ladder[LEVEL_CELL_COUNT];
};

// What a scatter placed, for the log.
struct DecorCounts {
	int props = 0;
	int torches = 0;
	int decals = 0;
	int ladderShafts = 0;
	int painted = 0, stone = 0, rough = 0; // surface cells by family
};

// True for a boss monster type whose minions climb out of coffins (Summon::Coffin): the boss table is the app's.
using CoffinBoss = std::function<bool(int monsterType)>;

// An empty floor cell within MINION_SUMMON_REACH of such a boss on its row, alive or slain (slainBoss): a coffin for
// its minions stands there. cells: a whole level.
[[nodiscard]] bool bossCoffinCell(const Tile* cells, int col, int row, const CoffinBoss& coffinBoss);

// Props, decals, torches, ladders and surfaces, seeded by the level's file name (the part after the last '/'), from
// the decoration tiers its depth unlocks (decorTier). Every mummy's spawn tile and every boss coffin cell gets a
// coffin. Overwrites all of layout.
DecorCounts scatterDecor(const Tile* cells, const char* levelName, int depth, const CoffinBoss& coffinBoss,
						 DecorLayout& layout);

// The highest decoration tier among a level's props, decals and surfaces; -1 for a bare level.
[[nodiscard]] int decorTierUsed(const Tile* cells, const DecorLayout& layout);

#endif
