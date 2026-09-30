#include "tile_defs.h"
#include "monster_kinds.h"
#include <array>

namespace {
constexpr std::array<TileDef, TILE_TYPE_COUNT> TILES = {{
	{"Wall", "Solid block. The player stands on it and cannot walk through it.", false, false},
	{"Empty", "Open space. With no Wall below, the player falls.", true, true},
	{"Door", "Sphinx statue. What it does depends on the gate type.", false, false},
	{"Death", "Large spike trap. Damages the player on contact.", true, true},
	{"Monster", "Spawns a monster when the cell comes into view. Max. 32 monsters are live at a time.", true, true},
	{"Spike", "Small spike trap. Damages the player on contact.", true, true},
	{"Ladder", "The player climbs between vertically adjacent ladder cells and does not fall on them.", false, false},
	{"3D", "Not used by the game. Renders as open space.", true, true},
	{"Treasure", "Chest with an item on top. Interact to pick it up, the cell then becomes Empty.", true, true},
	{"Ankh", "Level goal. Interact with it to win the game.", true, false}, // the statue hides a torch
	{"Key", "Key on the floor, picked up on touch. Opens the gates of its colour.", true, true},
	{"Gate",
	 "Portcullis. Opens with the key of its colour or when a lever of its colour is pulled; a boss gate when the boss "
	 "dies.",
	 true, true},
	{"Lever", "Interact to pull it. Opens every gate of the same colour.", true, true},
	{"RockFall",
	 "Loose ceiling, walkable. A rock falls ~1 s after the player steps in: crushes (1000) or grazes (50). Put a Wall "
	 "above it.",
	 true, true},
}};

char doorGlyph(const Tile& t) {
	switch (t.attr) {
	case GateEntrance:
		return 'S';
	case GateExit:
		return 'E';
	case GateRiddle:
		return '?';
	case GateTeleport:
		return 'O';
	default:
		return 'D';
	}
}

std::vector<GlyphDef> buildLegend() {
	std::vector<GlyphDef> legend = {
		{'#', {Wall, 0, 0}, "wall"},
		{'.', {Empty, 0, 0}, "open"},
		{'S', {Door, GateEntrance, 0}, "entrance"},
		{'E', {Door, GateExit, 0}, "exit"},
		{'?', {Door, GateRiddle, 0}, "riddle gate"},
		{'O', {Door, GateTeleport, 1}, "teleporter (pair id 1; more pairs: 'def')"},
		{'D', {Door, 0, 0}, "gate without a purpose (checker warns)"},
		{'H', {Ladder, 0, 0}, "ladder"},
		{'$', {Treasure, 3, 0}, "treasure (small health potion; others: 'def')"},
		{'A', {Ankh, 0, 0}, "ankh"},
		{'^', {Spike, 0, 0}, "spikes"},
		{'X', {Death, 0, 0}, "death trap"},
		{'v', {RockFall, 0, 0}, "rock fall"},
		{'/', {Lever, 1, 0}, "lever (red; other colours: 'set')"},
	};
	for (int type = 1; type <= MONSTER_TYPE_MAX; type++)
		if (const MonsterKind* kind = monsterKind(type))
			legend.push_back({kind->glyph, {MonsterSpawn, type, 0}, kind->label});
	for (int colour = 1; colour <= BOSS_LOCK; colour++) {
		const LockColour& lock = lockColour(colour);
		if (lock.keyGlyph != '\0')
			legend.push_back({lock.keyGlyph, {Key, colour, 0}, lock.name});
		legend.push_back({lock.gateGlyph, {Gate, colour, 0}, lock.name});
	}
	return legend;
}
} // namespace

const TileDef& tileDef(int type) {
	static const TileDef UNKNOWN = {"Unknown", "Not a tile type. The game treats it as open space.", true, true};
	return isTileType(type) ? TILES[static_cast<size_t>(type)] : UNKNOWN;
}

char tileGlyph(const Tile& t) {
	switch (t.type) {
	case Wall:
		return '#';
	case Empty:
	case Area3D:
		return '.';
	case Door:
		return doorGlyph(t);
	case Death:
		return 'X';
	case MonsterSpawn: {
		const MonsterKind* kind = monsterKind(t.attr);
		return kind != nullptr ? kind->glyph : UNKNOWN_MONSTER_GLYPH;
	}
	case Spike:
		return '^';
	case Ladder:
		return 'H';
	case Treasure:
		return '$';
	case Ankh:
		return 'A';
	case Key:
		return isLockColour(t.attr) ? lockColour(t.attr).keyGlyph : 'q';
	case Gate:
		return isGateColour(t.attr) ? lockColour(t.attr).gateGlyph : 'Q';
	case Lever:
		return '/';
	case RockFall:
		return 'v';
	default:
		return '?';
	}
}

const std::vector<GlyphDef>& glyphLegend() {
	static const std::vector<GlyphDef> LEGEND = buildLegend();
	return LEGEND;
}
