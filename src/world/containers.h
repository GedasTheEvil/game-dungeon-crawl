#ifndef CONTAINERS_H
#define CONTAINERS_H

#include <cstdint>
#include "decor.h"

// Treasure containers (plan treasure-containers): what a treasure cell looks like. The item lies hidden in a closed
// container; the interact key opens it (its clip: a lid lifts, the jar breaks), then the item shows, turning, and the
// next press takes it. The look of a cell (container, turn, place in the cell) rolls from the level's file name and
// the cell (scatterDecor, decor_scatter.h), so a level looks the same on every load and the gameplay stream is left
// alone. Models: media/models/items/<model>.md3, the chest built by tools/blender/models/items.py (the mimic copies
// it), the others by containers.py: frame 0 closed, the last frame open, the clip runs over openMs.
enum class Container : uint8_t {
	Chest, // the wooden chest: the mimic (Chest of Seth) passes for one, so every depth keeps plenty of them
	Coffer, // painted, a gabled lid
	Basket, // reed, a lid
	Canopic, // four jars, the sons of Horus: their heads lift off
	Jar, // a large sealed pottery jar: it smashes, a puff of dust
	Table, // an offering table, the item under a linen cover
	// An anthropoid coffin lying on the floor, a tomb-curse object (plan tomb-curse-containers): its lid is pushed off.
	// Some carry an execration text on the lid (TreasureLook::inscribed): opening one curses (Dungeon::coffinCurse).
	Coffin,
};
constexpr int CONTAINER_COUNT = 7;
constexpr const char* CONTAINER_NAMES[CONTAINER_COUNT] =
	{"chest", "coffer", "basket", "canopic", "jar", "table", "coffin"};

struct ContainerDef {
	const char* model; // media/models/items/<model>.md3 and media/textures/items/<model>.png
	float scale; // Item::scale: the model's largest side in world units (containers.py prints them)
	int openMs; // the opening clip, the item shows at its end
	float itemLift; // world units the item stands above the floor once open (the table's top)
	bool smashes; // the jar: no lid, it breaks (dust, shards stay)
	bool lengthwise = false; // the coffin: too long to stand side-on, it takes the stances facing the camera only
};
constexpr ContainerDef CONTAINER_DEFS[CONTAINER_COUNT] = {
	{"treasure_chest", 8.f, 700, 0.f, false},
	{"container_coffer", 6.6f, 700, 0.f, false},
	{"container_basket", 6.1f, 600, 0.f, false},
	{"container_canopic", 12.f, 900, 0.f, false},
	{"container_jar", 7.5f, 500, 0.f, true},
	{"container_table", 7.1f, 800, 4.f, false},
	{"container_coffin", 17.3f, 900, 0.5f, false, true}, // the item lies on the coffin's floor
};
constexpr const ContainerDef& containerDef(Container c) { return CONTAINER_DEFS[static_cast<int>(c)]; }
// The longest opening clip.
constexpr int containerOpenMsMax() {
	int ms = 0;
	for (const ContainerDef& def : CONTAINER_DEFS)
		ms = def.openMs > ms ? def.openMs : ms;
	return ms;
}

// The share of each container in percent, per decoration tier (DECOR_TIER_FROM): the cave has chests, baskets and
// jars; the painted coffers and the canopic jars come with the worked tunnels, the offering tables and the coffins
// with the tombs. The wooden chests stay at 40 % or more, so a mimic among them is not given away; the jars at 10 %
// from the tombs on, where the mimic jars lie among them.
constexpr int CONTAINER_PERCENT[DECOR_TIER_COUNT][CONTAINER_COUNT] = {
	{60, 0, 20, 0, 20, 0, 0}, // cave
	{45, 15, 15, 10, 15, 0, 0}, // worked tunnel
	{40, 12, 8, 15, 10, 8, 7}, // tomb
	{40, 12, 5, 13, 10, 12, 8}, // temple / necropolis
};
// The share of the coffins with an execration text on the lid (plan tomb-curse-containers): rolled with the look, so
// it is the same on every load and readable up close (Dungeon::coffinTell).
constexpr int COFFIN_INSCRIBED_PERCENT = 40;

// How a container stands in its cell: turned (yaw, degrees about the vertical, 0 = facing the camera, + turns the
// back to the left) and moved from the cell's spot for an item (x to the right, z towards the camera, tile units).
struct TreasureLook {
	Container kind = Container::Chest;
	int16_t yaw = 0;
	float x = 0.f, z = 0.f;
	bool inscribed = false; // a coffin with an execration text on its lid (COFFIN_INSCRIBED_PERCENT)
};

// The stances, by weight: facing the camera, angled, side-on, pushed against the back wall, its back against
// a side wall (only where that cell has rock on the side).
struct StanceDef {
	int16_t yaw;
	float x, z;
	int weight;
	int8_t wallSide; // the side wall it needs: -1 left, +1 right, 0 none
};
constexpr int STANCE_COUNT = 8;
constexpr StanceDef STANCES[STANCE_COUNT] = {
	{0, 0.f, 0.f, 6, 0}, // facing the camera
	{25, 0.f, 0.f, 2, 0}, // angled
	{-25, 0.f, 0.f, 2, 0},
	{90, 0.f, 0.f, 1, 0}, // side-on
	{-90, 0.f, 0.f, 1, 0},
	{0, 0.f, -0.3f, 2, 0}, // against the back wall
	{90, -0.3f, 0.f, 3, -1}, // its back against the left wall
	{-90, 0.3f, 0.f, 3, 1}, // the right wall
};

// A kill's chest and the one a dead mimic leaves (plan treasure-containers): a plain wooden chest facing the camera
// where the mimic stood, open from the start. A dead mimic jar leaves its smashed jar (plan tomb-curse-containers).
constexpr TreasureLook droppedLook(Container kind = Container::Chest) { return TreasureLook{kind}; }

#endif
