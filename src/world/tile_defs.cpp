#include "tile_defs.h"
#include "inscriptions.h"
#include "monster_kinds.h"
#include <array>

namespace {
constexpr TileDef UNKNOWN = {"Unknown", "Not a tile type. The game treats it as open space.", true, true};

// By type number; 0 and 7 are no type (level.h).
constexpr std::array<TileDef, TILE_TYPE_COUNT> TILES = {{
	UNKNOWN,
	{"None", "No object: the cell is just its structure.", true, true},
	{"Door", "Sphinx statue. What it does depends on the gate type.", false, false, '\0', "gate.png"},
	{"Death", "Large spike trap. Damages the player on contact.", true, true, 'X', "death.png"},
	{"Monster", "Spawns a monster when the cell comes into view. Max. 32 monsters are live at a time.", true, true,
	 '\0', "monster.png"},
	{"Spike", "Small spike trap. Damages the player on contact.", true, true, '^', "spikes.png"},
	{"Ladder", "The player climbs between vertically adjacent ladder cells and does not fall on them.", false, false,
	 'H', "ladder.png"},
	UNKNOWN,
	{"Treasure",
	 "A container (chest, jar, ...) holding an item. Interact to open, again to take it; the cell then becomes Empty.",
	 true, true, '$', "treasure.png"},
	{"Ankh", "Level goal. Interact with it to win the game.", true, false, 'A', "ankh.png"}, // the statue hides a torch
	{"Key", "Key on the floor, picked up on touch. Opens the gates of its colour.", true, true, '\0', "key.png"},
	{"Gate",
	 "Portcullis. Opens with the key of its colour or when a lever of its colour is pulled; a boss gate when the boss "
	 "dies.",
	 true, true, '\0', "gate_lock.png"},
	{"Lever", "Interact to pull it. Opens every gate of the same colour.", true, true, '/', "lever.png"},
	{"RockFall",
	 "Loose ceiling, walkable. A rock falls ~1 s after the player steps in: crushes (1000) or grazes (50). Attribute "
	 "1: a sand fall, sand pours from a crack and buries the cell under a drift (40, a short slow; the cell turns to "
	 "sand). Put a wall above it.",
	 true, true, 'v', "rockfall.png"},
	{"DartPlate",
	 "Pressure plate in the floor. The player or a heavy monster on it (not a jump) sets off 3 poison darts (medium) "
	 "from the holes in the back wall above it. Re-arms 3 s after it is free.",
	 true, true, '_', "dartplate.png"},
	{"Inscription",
	 "Hieroglyphs on the back wall. Interact to read them: the translation goes into the journal. Attribute: the text "
	 "id in content/inscriptions/*.txt, or 100: a name cartouche, the level's name for its name gate. From level 6 on "
	 "in "
	 "the campaign. Put a floor under it.",
	 false, false, 'i', "inscription.png"},
	{"WallSpikes",
	 "Rhythm trap: poison spikes shoot out of the back wall and go back in, on a fixed cycle. Out, they block like a "
	 "closed gate; whoever stands in the cell as they come out takes a stab (10, weak poison) and is shoved out. "
	 "Attribute: speed (0 normal 4 s, 1 fast 3.2 s, 2 slow 4.8 s). Value: phase, 0 to 7 eighths of the cycle. Put a "
	 "floor under it and a wall above it.",
	 false, false, '|'},
	{"LightShaft",
	 "Sunlight through a crack in the rock: a beam runs from here through open cells until rock or a closed gate stops "
	 "it. Attribute: its way (0 down, 1 left, 2 up, 3 right). Put rock behind it (above it for a beam down).",
	 true, true, 'l'},
	{"Mirror",
	 "A bronze mirror on a stand: a beam that falls on it goes on the way it points. Interact turns it a quarter turn "
	 "clockwise. Value: the way it points (0 down, 1 left, 2 up, 3 right).",
	 true, true, '('},
	{"SunDisc",
	 "A sun disc on the back wall. A beam on it lights it for good and opens every gate of its colour, as a lever. "
	 "Attribute: the lock colour.",
	 false, false, 'a'},
	{"Page",
	 "A torn page of the earlier expedition's notebook, on a skeleton; picked up on touch, it goes into the journal. "
	 "Attribute: the page number, 1 to 18, in order of depth over the campaign. Optional: keep it off the exit path "
	 "(only the last one lies on it). Put a floor under it.",
	 true, true, '='},
	{"TrappedTreasure",
	 "A treasure (attribute and value: the item) whose opening sets off a trap: poison darts, a puff of poison gas, a "
	 "curse or a scarab swarm, by depth; canopic jars and pottery jars curse. Half of them hold the item and one bonus "
	 "more, the rest nothing. Never put what the level needs in one: the checker does not count its antidote.",
	 true, true, '%', "trapped_treasure.png"},
}};

constexpr std::array<StructureDef, STRUCTURE_COUNT> STRUCTURES = {{
	{"Wall", "Solid rock. The player stands on it and cannot walk through it. Holds no object."},
	{"Empty", "Open space. With no wall below, the player falls."},
	{"Half water", "Open space, half filled with water. The player wades at half speed and cannot jump in it. Put deep "
				   "water or a wall under it."},
	{"Deep water", "Full of water, solid like a wall, under half water. Holds no object."},
	{"Sand", "Open space, a drift of sand on the floor. The player walks at 3/4 speed and cannot sprint in it (jumping "
			 "is fine). A sand worm lurks under it. Put a wall under it."},
	{"Quicksand", "Looks like sand (a slow swirl up close). Standing in it sinks the player: slower each second, stuck "
				  "after 3 s, hurt after 5 s; a jump frees them, at a stamina cost. A passable trap. Put a wall under "
				  "it."},
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
		return isNameGate(t) ? '!' : 'O';
	default:
		return 'D';
	}
}

std::vector<GlyphDef> buildLegend() {
	std::vector<GlyphDef> legend = {
		{'#', wallTile(), "wall"},
		{'.', Tile{}, "open"},
		{'S', {Door, GateEntrance, 0}, "entrance"},
		{'E', {Door, GateExit, 0}, "exit"},
		{'?', {Door, GateRiddle, 0}, "riddle gate"},
		{'O', {Door, GateTeleport, 1}, "teleporter (pair id 1; more pairs: 'def')"},
		{'!', {Door, GateTeleport, 1 | NAME_SEAL}, "name gate (teleporter pair id 1, sealed by the level's name)"},
		{'D', {Door, 0, 0}, "gate without a purpose (checker warns)"},
		{'H', {Ladder, 0, 0}, "ladder"},
		{'$', {Treasure, 3, 0}, "treasure (small health potion; others: 'def')"},
		{'%', {TrappedTreasure, 3, 0}, "trapped treasure (small health potion; others: 'def')"},
		{'A', {Ankh, 0, 0}, "ankh"},
		{'^', {Spike, 0, 0}, "spikes"},
		{'X', {Death, 0, 0}, "death trap"},
		{'v', {RockFall, 0, 0}, "rock fall"},
		{'&', {RockFall, SAND_FALL, 0}, "sand fall"},
		{'_', {DartPlate, 0, 0}, "dart trap plate"},
		{'/', {Lever, 1, 0}, "lever (red; other colours: 'set')"},
		{'i', {Inscription, 0, 0}, "inscription (no text: give the text id with 'def')"},
		{':', {Inscription, NAME_CARTOUCHE_ID, 0}, "name cartouche (the level's name, for its name gate)"},
		{'|', {WallSpikes, 0, 0}, "wall spikes (normal speed, phase 0; others: 'def')"},
		{'l', {LightShaft, 0, 0}, "light shaft (beam down; other ways: 'def')"},
		{'(', {Mirror, 0, 0}, "mirror (pointing down; other ways: 'def')"},
		{'a', {SunDisc, 1, 0}, "sun disc (red; other colours: 'def')"},
		{'=', {Page, 1, 0}, "expedition page (number 1; others: 'def')"},
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

const TileDef& tileDef(int type) { return isTileType(type) ? TILES[static_cast<size_t>(type)] : UNKNOWN; }

const StructureDef& structureDef(Structure s) { return STRUCTURES[static_cast<size_t>(s)]; }

char tileGlyph(const Tile& t) {
	switch (t.type) {
	case NoObject:
		if (hasFixture(t))
			return tileGlyph(fixtureAsTile(t.fixture));
		return isSolidStructure(t.structure) ? '#' : '.';
	case Door:
		return doorGlyph(t);
	case MonsterSpawn: {
		const MonsterKind* kind = monsterKind(t.attr);
		return kind != nullptr ? kind->glyph : UNKNOWN_MONSTER_GLYPH;
	}
	case Key:
		return isLockColour(t.attr) ? lockColour(t.attr).keyGlyph : 'q';
	case Gate:
		return isGateColour(t.attr) ? lockColour(t.attr).gateGlyph : 'Q';
	case Inscription:
		return t.attr == NAME_CARTOUCHE_ID ? ':' : 'i';
	case RockFall:
		return isSandFall(t) ? '&' : 'v';
	default:
		return tileDef(t.type).glyph != '\0' ? tileDef(t.type).glyph : '?';
	}
}

const std::vector<GlyphDef>& glyphLegend() {
	static const std::vector<GlyphDef> LEGEND = buildLegend();
	return LEGEND;
}
