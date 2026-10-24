#ifndef DECOR_H
#define DECOR_H

#include <cstdint>
#include "../core/paths.h"

// Static props and wall decals scattered on empty floor cells when a level loads (see scatterDecor, decor_scatter.h).
// Models and textures: media/models/decorations/decor_<name>.md3, media/textures/decorations/decor_<name>.png, built by
// tools/blender/models/decor.py in tile units (origin = floor centre of the tile on the back wall).
constexpr int DECOR_COUNT = 25;
constexpr int DECOR_SCATTERED =
	18; // the first ones, on dry floor; the coffin is placed on purpose, the water props after it
constexpr int DECOR_WEB = 0; // modelled in the upper left corner, needs a ceiling
constexpr int DECOR_BRAZIER = 6;
constexpr int DECOR_SKELETON = 5; // also under every page of the earlier expedition (plan expedition-notes)
constexpr int DECOR_LAMP = 7;
constexpr int DECOR_USHABTI = 9; // also on every living ushabti's spawn tile: it stands among them until it wakes
// Thoth's four statues: one pick among the props, then a variant at even odds (scatterDecor, decor_scatter.h). The
// crowns reach up to a wall torch.
constexpr int DECOR_THOTH = 13;
constexpr int DECOR_THOTH_VARIANTS = 4;
constexpr bool isThoth(int decor) { return decor >= DECOR_THOTH && decor < DECOR_THOTH + DECOR_THOTH_VARIANTS; }
constexpr int DECOR_COFFIN = 18; // empty, on every mummy's spawn tile: it lies in there until it wakes
constexpr int DECOR_COFFIN_LARGE = 23; // the mummy priest's, a head longer and gilded (plan mummy-boss)
// The decoy (plan placed-decor): the coffin with a mummy asleep in it for good, its model in the entombed pose
// (Dungeon::drawDecoys), never a monster. Placed only (the decor layer), no model file of its own: the coffin's.
constexpr int DECOR_COFFIN_MUMMY = 24;
[[nodiscard]] constexpr bool isCoffin(int decor) {
	return decor == DECOR_COFFIN || decor == DECOR_COFFIN_LARGE || decor == DECOR_COFFIN_MUMMY;
}
// The prop whose model and texture a prop is drawn with.
[[nodiscard]] constexpr int decorModel(int decor) { return decor == DECOR_COFFIN_MUMMY ? DECOR_COFFIN : decor; }
// Water props (plan water-decorations): plants scattered in half water cells only, modelled at their height in the
// basin. The reeds and the weed sway, the leaves afloat bob (drawDecorTile).
constexpr int DECOR_WATER_REEDS = 19; // papyrus, prefers the bank
constexpr int DECOR_WATER_WEED = 20; // under the surface
constexpr int DECOR_WATER_PADS = 21; // lotus leaves afloat, prefer the middle of a pool
constexpr int DECOR_WATER_LOTUS = 22; // the same with a blue lotus in bloom
constexpr int DECOR_WATER_FIRST = DECOR_WATER_REEDS;
constexpr int DECOR_WATER_COUNT = 4;
constexpr bool isWaterProp(int decor) {
	return decor >= DECOR_WATER_FIRST && decor < DECOR_WATER_FIRST + DECOR_WATER_COUNT;
}
constexpr const char* DECOR_NAMES[DECOR_COUNT] = {
	"web",
	"pottery",
	"canopic",
	"rubble",
	"sand",
	"skeleton",
	"brazier",
	"lamp",
	"scrolls",
	"ushabti",
	"cat",
	"jackal",
	"osiris",
	"thoth_ibis_standing",
	"thoth_ibis_seated",
	"thoth_baboon_seated",
	"thoth_baboon_standing",
	"sarcophagus",
	"coffin",
	"water_reeds",
	"water_weed",
	"water_pads",
	"water_lotus",
	"coffin_large",
	"coffin_mummy"
};

// Decorations by depth (plan decor-by-depth): the tiers add up, a level draws from every tier unlocked at its depth,
// the newest one counting double. The first levels are a natural cave, the deepest a painted temple. Depth is the
// campaign level; a generated level's comes from its difficulty (level_gen.h), any other level gets them all.
constexpr int DECOR_TIER_COUNT = 4;
constexpr int DECOR_TIER_FROM[DECOR_TIER_COUNT] = {1, 4, 8, 13}; // cave, worked tunnel, tomb, temple / necropolis
constexpr int DECOR_DEPTH_ALL = 1000; // a level outside the campaign
constexpr int decorTier(int depth) {
	int tier = 0;
	while (tier + 1 < DECOR_TIER_COUNT && depth >= DECOR_TIER_FROM[tier + 1])
		tier++;
	return tier;
}
// The tier each prop comes in with; the coffins (-1) stand wherever a mummy or the mummy priest lies, the decoy only
// where a level places it. The lotus comes
// in with the worked tunnels, before the tombs: the water lotus monster (plan water-lotus) hides among the leaves.
constexpr int8_t DECOR_TIERS[DECOR_COUNT] = {0,	 0, 1, 0, 0, 0, // web, pottery, canopic, rubble, sand, skeleton
											 1,	 1, 1, 2, 2, 3, // brazier, lamp, scrolls, ushabti, cat, jackal
											 3,	 2, 2, 2, 2, 3, // osiris, Thoth x 4, sarcophagus
											 -1, 0, 0, 1, 1, // coffin, reeds, weed, pads, lotus
											 -1, -1}; // the large coffin, the decoy
// Rows of rough rock (the rest dressed stone), and of the dressed stretches the painted ones, per tier, in percent:
// all cave at first, less and less deeper down.
constexpr uint32_t ROUGH_PERCENT[DECOR_TIER_COUNT] = {100, 55, 30, 15};
constexpr uint32_t PAINTED_PERCENT[DECOR_TIER_COUNT] = {0, 0, 40, 55};
constexpr int STAR_CEILING_TIER = 3; // painted stretches get the starry ceiling from here, slabs before
constexpr int CRACKED_FLOOR_TIER = 1;
constexpr int SLAB_FLOOR_TIER = 2;

// The wall torch (media/models/decorations/decor_torch.md3, also built by decor.py) is not in this list:
// scatterTorches in decor_scatter.cpp places it on its own.

// Surfaces of the cells: media/textures/dungeon/<name>.png, generated by tools/textures/surfaces.py in this order.
// Any two variants of a kind tile side by side, except that a painted wall and a stone one meet through
// WALL_PLASTER_BROKEN (plaster on the left, stone on the right; mirrored for the other way round).
// scatterSurfaces (decor_scatter.cpp) picks one per cell.
constexpr int WALL_PLASTER = 0;
constexpr int WALL_PLASTER_WORN = 1;
constexpr int WALL_PLASTER_BROKEN = 2;
constexpr int WALL_STONE = 3;
constexpr int WALL_STONE_CRACKED = 4;
constexpr int WALL_STONE_SAND = 5;
constexpr int WALL_ROUGH = 6;
constexpr int WALL_ROUGH_STRATA = 7;
// The desert chapter's sandstone (plan desert-chapter, its zone's stone: campaign.h): every row of it, no plaster.
constexpr int WALL_SANDSTONE = 8;
constexpr int WALL_SANDSTONE_WORN = 9;
// The other zones' stones (plan zone-looks): the dressed rows, no plaster.
constexpr int WALL_WETSTONE = 10; // the cisterns
constexpr int WALL_WETSTONE_MOSS = 11;
constexpr int WALL_BASALT = 12; // the Lake of Fire
constexpr int WALL_BASALT_GLOW = 13; // ember cracks
constexpr int WALL_GRANITE = 14; // the necropolis, the Hall of Judgment
constexpr int WALL_GRANITE_WORN = 15;
constexpr int WALL_STYLE_COUNT = 16;
constexpr const char* WALL_STYLE_NAMES[WALL_STYLE_COUNT] = {
	"wall_plaster",
	"wall_plaster_worn",
	"wall_plaster_broken",
	"wall_stone",
	"wall_stone_cracked",
	"wall_stone_sand",
	"wall_rough",
	"wall_rough_strata",
	"wall_sandstone",
	"wall_sandstone_worn",
	"wall_wetstone",
	"wall_wetstone_moss",
	"wall_basalt",
	"wall_basalt_glow",
	"wall_granite",
	"wall_granite_worn"
};

constexpr int FLOOR_SLABS = 0;
constexpr int FLOOR_SAND = 1;
constexpr int FLOOR_CRACKED = 2;
constexpr int FLOOR_SANDSTONE = 3; // the desert's
constexpr int FLOOR_WETSTONE = 4; // the other zones' stones (plan zone-looks)
constexpr int FLOOR_BASALT = 5;
constexpr int FLOOR_GRANITE = 6;
constexpr int FLOOR_STYLE_COUNT = 7;
constexpr const char* FLOOR_STYLE_NAMES[FLOOR_STYLE_COUNT] = {
	"floor_slabs",
	"floor_sand",
	"floor_cracked",
	"floor_sandstone",
	"floor_wetstone",
	"floor_basalt",
	"floor_granite"
};

constexpr int CEILING_STARS = 0; // over painted walls
constexpr int CEILING_SLABS = 1; // over stone walls
constexpr int CEILING_ROUGH = 2; // over rough walls
// Over sandstone walls: worn rock under a crust of sand; cracked where a light shaft comes through
// (Dungeon::lightShaft), or sandier.
constexpr int CEILING_SANDSTONE = 3;
constexpr int CEILING_SANDSTONE_CRACKED = 4;
constexpr int CEILING_SANDSTONE_SANDY = 5;
constexpr int CEILING_WETSTONE = 6; // the other zones' stones (plan zone-looks)
constexpr int CEILING_BASALT = 7;
constexpr int CEILING_GRANITE = 8;
constexpr int CEILING_STYLE_COUNT = 9;
constexpr const char* CEILING_STYLE_NAMES[CEILING_STYLE_COUNT] = {
	"ceiling_stars",
	"ceiling_slabs",
	"ceiling_rough",
	"ceiling_sandstone",
	"ceiling_sandstone_cracked",
	"ceiling_sandstone_sandy",
	"ceiling_wetstone",
	"ceiling_basalt",
	"ceiling_granite"
};
// Of the desert's cells with rock above and below, about one in this many has a cracked ceiling and a light shaft.
constexpr uint32_t LIGHT_SHAFT_EVERY = 6;

// Sand spilling from a FLOOR_SAND cell onto the stone floor beside it, drawn over that floor (SurfaceCell::sandLeft,
// sandRight): floor_sand, opaque at its left edge and thinning out by about half the cell; mirrored for the right.
constexpr const char* FLOOR_SAND_EDGE_TEXTURE = MEDIA_DIR "textures/dungeon/floor_sand_edge.png";

// The face of a solid cell: media/textures/dungeon/rock.png, one image over 2 x 2 cells.
constexpr const char* ROCK_TEXTURE = MEDIA_DIR "textures/dungeon/rock.png";

struct SurfaceCell {
	int wall = WALL_STONE; // back and side walls
	int floor = FLOOR_SLABS;
	int ceiling = CEILING_SLABS;
	bool wallMirror = false; // u flipped on the walls: stone on the left of WALL_PLASTER_BROKEN
	// Sand spills onto this stone floor from a sand floor beside it (FLOOR_SAND_EDGE_TEXTURE): no hard seam.
	bool sandLeft = false;
	bool sandRight = false;
};

// Ladder pieces: one model per Ladder cell, media/models/ladders/ladder_<style>_<piece>.md3 +
// media/textures/ladders/ladder_<style>_<piece>.png, built by tools/blender/models/ladder.py in the same tile units as
// the props. Pieces stack seamlessly in any order; scatterLadders (decor_scatter.cpp) picks one style per shaft and a
// piece per cell.
constexpr int LADDER_STYLE_COUNT = 2;
constexpr int LADDER_MID_COUNT = 3; // interchangeable middle pieces, indices 0..2
constexpr int LADDER_TOP = 3; // highest cell of the shaft
constexpr int LADDER_BOTTOM = 4; // standing on the floor
constexpr int LADDER_PIECE_COUNT = 5;
constexpr const char* LADDER_STYLE_NAMES[LADDER_STYLE_COUNT] = {"wood", "vine"};
constexpr const char* LADDER_PIECE_NAMES[LADDER_PIECE_COUNT] = {"a", "b", "c", "top", "bottom"};
// Mirrored lianas would kink at the seams (their twist turns the other way), wooden pieces are symmetric enough.
constexpr bool LADDER_MIRRORS[LADDER_STYLE_COUNT] = {true, false};

struct LadderCell {
	int8_t style = -1; // -1 = no ladder, else index into LADDER_STYLE_NAMES
	int8_t piece = 0; // index into LADDER_PIECE_NAMES
	bool mirror = false;
};

struct DecorCell {
	int8_t type = -1; // -1 = none, else index into DECOR_NAMES
	bool mirror = false;
	float offsetX = 0.f; // tile units
};

// Wall decals: quads on the back wall textured from the media/textures/decorations/decals.png atlas (4 x 4 cells),
// generated by tools/textures/decals.py in this order.
enum class DecalAnchor : int8_t {
	Free, // anywhere on the wall
	Ceiling, // hangs from the ceiling, needs a wall above
	Floor, // grows from the floor, needs a wall below
};

struct DecalDef {
	DecalAnchor anchor;
	float size; // quad side, tile units
	const char* name; // tools: the editor's palette, levelcheck --legend
};

// A cell of a square texture atlas (the decals, the inscriptions), grid x grid, numbered row by row from the top
// left. Row 0 is the top of the image, which the loader puts at t = 1.
struct AtlasRect {
	float u0, v0, u1, v1;
};
constexpr AtlasRect atlasCell(int index, int grid) {
	const float step = 1.f / static_cast<float>(grid);
	const int row = index / grid;
	const float u0 = static_cast<float>(index % grid) * step;
	const float v1 = 1.f - static_cast<float>(row) * step;
	return {u0, v1 - step, u0 + step, v1};
}

constexpr int DECAL_ATLAS_GRID = 4;
constexpr int DECAL_CARTOUCHE = 9; // an inscription without its own glyphs shows it
constexpr int DECAL_COUNT = 16;
constexpr DecalDef DECAL_DEFS[DECAL_COUNT] = {
	{DecalAnchor::Free, 0.6f, "crack"},
	{DecalAnchor::Free, 0.55f, "impact crack"},
	{DecalAnchor::Floor, 0.7f, "floor crack"}, // crack rising from the floor
	{DecalAnchor::Ceiling, 0.8f, "vines"}, // hanging vines
	{DecalAnchor::Ceiling, 0.7f, "flowering vines"},
	{DecalAnchor::Ceiling, 0.75f, "roots"},
	{DecalAnchor::Ceiling, 0.85f, "seepage"}, // damp seepage with moss
	{DecalAnchor::Free, 0.75f, "glyph column"}, // hieroglyph column
	{DecalAnchor::Free, 0.8f, "glyph row"}, // hieroglyph row
	{DecalAnchor::Free, 0.6f, "cartouche"},
	{DecalAnchor::Free, 0.5f, "eye of Horus"},
	{DecalAnchor::Free, 0.75f, "winged sun"},
	{DecalAnchor::Floor, 0.7f, "papyrus"},
	{DecalAnchor::Floor, 0.45f, "dry grass"},
	{DecalAnchor::Floor, 0.75f, "creeper"},
	{DecalAnchor::Free, 0.5f, "moss"},
};
// The tier each decal comes in with (DECOR_TIERS): cracks and growth anywhere, the single eye of Horus in the worked
// tunnels, glyph columns, rows and cartouches in the tombs, the painted winged sun in the temples.
constexpr int8_t DECAL_TIERS[DECAL_COUNT] = {0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 1, 3, 0, 0, 0, 0};

struct DecalCell {
	int8_t type = -1; // -1 = none, else index into DECAL_DEFS
	bool mirror = false;
	float x = 0.f, y = 0.f; // quad centre on the back wall, tile units
};

#endif
