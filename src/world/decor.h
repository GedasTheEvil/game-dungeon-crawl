#ifndef DECOR_H
#define DECOR_H

#include <cstdint>

// Static props and wall decals scattered on empty floor cells when a level loads (see Dungeon::scatterDecorations).
// Models and textures: Models/decor_<name>.md3, Textures/decor_<name>.png, built by
// tools/blender/models/decor.py in tile units (origin = floor centre of the tile on the back wall).
constexpr int DECOR_COUNT = 10;
constexpr int DECOR_WEB = 0; // modelled in the upper left corner, needs a ceiling
constexpr const char* DECOR_NAMES[DECOR_COUNT] = {"web",	  "pottery", "canopic", "rubble",  "sand",
												  "skeleton", "brazier", "lamp",	"scrolls", "ushabti"};

struct DecorCell {
	int8_t type = -1; // -1 = none, else index into DECOR_NAMES
	bool mirror = false;
	float offsetX = 0.f; // tile units
};

// Wall decals: quads on the back wall textured from the Textures/decals.png atlas (4 x 4 cells), generated
// by tools/textures/decals.py in this order.
enum class DecalAnchor : int8_t {
	Free,	 // anywhere on the wall
	Ceiling, // hangs from the ceiling, needs a wall above
	Floor,	 // grows from the floor, needs a wall below
};

struct DecalDef {
	DecalAnchor anchor;
	float size; // quad side, tile units
};

constexpr int DECAL_ATLAS_GRID = 4;
constexpr int DECAL_COUNT = 16;
constexpr DecalDef DECAL_DEFS[DECAL_COUNT] = {
	{DecalAnchor::Free, 0.6f},	   // crack
	{DecalAnchor::Free, 0.55f},	   // impact crack
	{DecalAnchor::Floor, 0.7f},	   // crack rising from the floor
	{DecalAnchor::Ceiling, 0.8f},  // hanging vines
	{DecalAnchor::Ceiling, 0.7f},  // flowering vines
	{DecalAnchor::Ceiling, 0.75f}, // roots
	{DecalAnchor::Ceiling, 0.85f}, // damp seepage with moss
	{DecalAnchor::Free, 0.75f},	   // hieroglyph column
	{DecalAnchor::Free, 0.8f},	   // hieroglyph row
	{DecalAnchor::Free, 0.6f},	   // cartouche
	{DecalAnchor::Free, 0.5f},	   // eye of Horus
	{DecalAnchor::Free, 0.75f},	   // winged sun
	{DecalAnchor::Floor, 0.7f},	   // papyrus
	{DecalAnchor::Floor, 0.45f},   // dry grass
	{DecalAnchor::Floor, 0.75f},   // creeper
	{DecalAnchor::Free, 0.5f},	   // moss
};

struct DecalCell {
	int8_t type = -1; // -1 = none, else index into DECAL_DEFS
	bool mirror = false;
	float x = 0.f, y = 0.f; // quad centre on the back wall, tile units
};

#endif
