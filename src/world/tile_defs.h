#ifndef TILE_DEFS_H
#define TILE_DEFS_H

// One row per tile type (the object layer) and per structure: what the game, the editor, the checker and the ASCII
// level sources know about it. A new tile type gets its row here; the compiler then points at the switches that must
// handle it.

#include "level.h"
#include <vector>

constexpr int TILE_TYPE_COUNT = RockFall + 1; // type numbers 0 and 7 are free (level.h)
[[nodiscard]] constexpr bool isTileType(int type) {
	constexpr int AREA3D = 7; // dropped in the level format v2
	return type >= NoObject && type < TILE_TYPE_COUNT && type != AREA3D;
}

struct TileDef {
	const char* name;
	const char* description; // the editor's hint panel
	bool decals;			 // the back wall shows and is free: wall decals may go there (not in a wall cell)
	bool torch;				 // a wall torch may hang there (not in a wall cell)
};
// Unknown types get a row that says so (the game treats them as open space).
[[nodiscard]] const TileDef& tileDef(int type);

struct StructureDef {
	const char* name;
	const char* description; // the editor's hint panel
};
[[nodiscard]] const StructureDef& structureDef(Structure s);

// The level as text (levelcheck --map, the ASCII campaign sources): one character per cell, its object; a cell
// without one is '#' in a wall, '.' anywhere else. The structure has its own drawing (structureGlyph, level.h).
[[nodiscard]] char tileGlyph(const Tile& t);

// The characters an ASCII level source may use, each with the tile it stands for. Details the character does not say
// (which item a chest holds, a lever's colour, a teleporter pair other than 1) come from 'def' / 'set' lines in the
// source (tools/level/ascii2level.py).
struct GlyphDef {
	char glyph;
	Tile tile;
	const char* meaning;
};
[[nodiscard]] const std::vector<GlyphDef>& glyphLegend();

#endif
