#ifndef TILE_DEFS_H
#define TILE_DEFS_H

// One row per tile type: what the game, the editor, the checker and the ASCII level sources know about it.
// A new tile type gets its row here; the compiler then points at the switches that must handle it.

#include "level.h"
#include <vector>

constexpr int TILE_TYPE_COUNT = RockFall + 1;
[[nodiscard]] constexpr bool isTileType(int type) { return type >= 0 && type < TILE_TYPE_COUNT; }

struct TileDef {
	const char* name;
	const char* description; // the editor's hint panel
	bool decals;			 // the back wall shows and is free: wall decals may go there
	bool torch;				 // a wall torch may hang there
};
// Unknown types get a row that says so (the game treats them as open space).
[[nodiscard]] const TileDef& tileDef(int type);

// The level as text (levelcheck --map, the ASCII campaign sources): one character per cell.
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
