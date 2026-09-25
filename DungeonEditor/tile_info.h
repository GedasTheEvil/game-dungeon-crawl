#ifndef TILE_INFO_H
#define TILE_INFO_H

// What the tile types, attributes and values mean (DungeonEditor/readme.md), as text for the editor's hint panel.
// No GL here.

#include "../src/world/level.h"
#include <string>

constexpr int TILE_COUNT = 14; // DungeonTileType Wall .. RockFall

struct TileInfo {
	const char* name;
	const char* icon; // PNG under DungeonEditor/Textures, nullptr: drawn as a flat colour
	const char* description;
};

[[nodiscard]] const TileInfo& tileInfo(int type);
[[nodiscard]] bool isTileType(int type);

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

[[nodiscard]] CellHint describeCell(const Tint& cell);

#endif
