#ifndef TILE_INFO_H
#define TILE_INFO_H

// What the tile types, attributes and values mean (readme.md), as text for the editor's hint panel.
// No GL here.

#include "../../src/world/tile_defs.h"
#include <string>

struct TileInfo {
	const char* name;
	const char* icon; // PNG under tools/editor/icons/, nullptr: drawn as a flat colour
	const char* description;
};

[[nodiscard]] TileInfo tileInfo(int type); // isTileType: tile_defs.h

// One field (attribute or value) of a cell, explained.
struct FieldHint {
	bool used = false;	 // false: the tile ignores the field, keep it at 0
	bool valid = true;	 // the current number means something
	std::string label;	 // "Attribute: gate type"
	std::string current; // "2 = Exit, loads the next level"
	std::string choices; // "1 entrance, 2 exit, ..."
};

struct CellHint {
	std::string title; // "Door (type 2)"
	std::string description;
	FieldHint attribute;
	FieldHint value;
};

[[nodiscard]] CellHint describeCell(const Tile& cell);

#endif
