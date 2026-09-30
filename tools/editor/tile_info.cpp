#include "tile_info.h"
#include "../../src/world/items.h"
#include <string>
#include <array>
#include <vector>

namespace {

constexpr std::array<TileInfo, TILE_COUNT> TILES = {{
	{"Wall", nullptr, "Solid block. The player stands on it and cannot walk through it."},
	{"Empty", nullptr, "Open space. With no Wall below, the player falls."},
	{"Door", "gate.png", "Sphinx statue. What it does depends on the gate type."},
	{"Death", "death.png", "Large spike trap. Damages the player on contact."},
	{"Monster", "monster.png", "Spawns a monster when the cell comes into view. Max. 32 monsters are live at a time."},
	{"Spike", "spikes.png", "Small spike trap. Damages the player on contact."},
	{"Ladder", "ladder.png", "The player climbs between vertically adjacent ladder cells and does not fall on them."},
	{"3D", "3D.png", "Not used by the game. Renders as open space."},
	{"Treasure", "treasure.png", "Chest with an item on top. Interact to pick it up, the cell then becomes Empty."},
	{"Ankh", "ankh.png", "Level goal. Interact with it to win the game."},
	{"Key", "key.png", "Key on the floor, picked up on touch. Opens the gates of its colour."},
	{"Gate", "gate_lock.png",
	 "Portcullis. Opens with the key of its colour or when a lever of its colour is pulled; a boss gate when the boss "
	 "dies."},
	{"Lever", "lever.png", "Interact to pull it. Opens every gate of the same colour."},
	{"RockFall", "rockfall.png",
	 "Loose ceiling, walkable. A rock falls ~1 s after the player steps in: crushes (1000) or grazes (50). Put a Wall "
	 "above it."},
}};

struct Choice {
	int number;
	const char* name;	 // in the choices list
	std::string meaning; // after "n = "
};

using Choices = std::vector<Choice>;

const Choices GATE_TYPES = {
	{1, "entrance", "Entrance, the player start. One per level"},
	{2, "exit", "Exit, loads the next level"},
	{3, "riddle", "Riddle, asks a riddle, then becomes an empty gate"},
	{4, "empty", "Empty gate, decoration only"},
	{5, "teleporter", "Teleporter, interact to jump to the teleporter with the same value (pair id)"},
	{0, "decoration", "Decoration only"},
};

const Choices MONSTER_TYPES = {
	{MonsterScarab, "scarab", "Scarab"},
	{MonsterWorm, "worm", "Worm"},
	{MonsterPlant, "plant", "Plant"},
	{MonsterAnubis, "anubis", "Anubis"},
	{MonsterRat, "rat", "Rat"},
	{MonsterGiantRat, "giant rat", "Giant rat"},
	{MonsterBat, "bat", "Bat"},
	{MonsterGiantBat, "giant bat", "Giant bat"},
	{MonsterMimic, "mimic", "Mimic, a treasure chest until the player comes near"},
	{MonsterGiantScarab, "giant scarab", "Giant scarab, leaps over pits and traps"},
	{MonsterBossScarab, "boss scarab",
	 "Boss scarab, summons scarabs; its death opens the boss gates. One boss per level"},
};

const Choices LOCK_COLOURS = {
	{1, "red", "Red (Carnelian)"},
	{2, "blue", "Blue (Lapis)"},
	{3, "green", "Green (Turquoise)"},
	{4, "gold", "Gold (Amber)"},
};

const Choices GATE_COLOURS = {
	{1, "red", "Red (Carnelian)"},
	{2, "blue", "Blue (Lapis)"},
	{3, "green", "Green (Turquoise)"},
	{4, "gold", "Gold (Amber)"},
	{BOSS_LOCK, "boss", "Boss gate, opens when the level's boss dies"},
};

const Choices ITEM_TYPES = {
	{ItemType::MELEE_WEAPON, "melee weapon", "Melee weapon"},
	{ItemType::RANGED_WEAPON, "ranged weapon", "Ranged weapon"},
	{ItemType::POTION, "potion", "Potion"},
	{ItemType::EMPTY, "empty chest", "Empty chest"},
};

// The ids of one item type, from the game's item table (src/world/items.h).
Choices itemIds(int type) {
	Choices ids;
	for (int i = 0; i < ITEM_KIND_COUNT; i++) {
		ItemKind kind = itemAt(i);
		ItemFileId file = fileIdOf(kind);
		if (file.type != type)
			continue;
		const ItemText& text = itemText(kind);
		std::string meaning = text.name;
		if (*text.effect != '\0')
			meaning += std::string(", ") + text.effect;
		ids.push_back({file.id, text.label, meaning});
	}
	return ids;
}

const Choices MELEE_WEAPONS = itemIds(ItemType::MELEE_WEAPON);
const Choices RANGED_WEAPONS = itemIds(ItemType::RANGED_WEAPON);
const Choices POTIONS = itemIds(ItemType::POTION);

const Choices GATE_STATES = {{0, "closed", "Closed"}, {1, "open", "Open"}};
const Choices ZERO_ONLY = {{0, "only", "Set by the game while playing"}};

FieldHint unusedField() {
	FieldHint hint;
	hint.label = "not used";
	return hint;
}

// Any number is fine: an id that links tiles.
FieldHint anyNumber(const char* label, int number, const char* meaning) {
	FieldHint hint;
	hint.used = true;
	hint.label = label;
	hint.current = std::to_string(number) + " = " + meaning;
	hint.choices = "any number";
	return hint;
}

// `invalidMeaning` is shown for numbers that are not in the list.
FieldHint field(const char* label, const Choices& choices, int number, const char* invalidMeaning) {
	FieldHint hint;
	hint.used = true;
	hint.label = label;
	hint.valid = false;
	hint.current = std::to_string(number) + " = " + invalidMeaning;
	for (const Choice& c : choices) {
		if (!hint.choices.empty())
			hint.choices += ", ";
		hint.choices += std::to_string(c.number) + " " + c.name;
		if (c.number == number) {
			hint.valid = true;
			hint.current = std::to_string(number) + " = " + c.meaning;
		}
	}
	return hint;
}

// A field the tile ignores is still worth a warning when it is not 0.
void checkUnused(FieldHint& hint, int number) {
	if (hint.used || number == 0)
		return;
	hint.valid = false;
	hint.current = std::to_string(number) + ", ignored, keep it 0";
}

} // namespace

bool isTileType(int type) { return type >= 0 && type < TILE_COUNT; }

const TileInfo& tileInfo(int type) {
	static const TileInfo UNKNOWN = {"Unknown", nullptr, "Not a tile type. The game treats it as open space."};
	return isTileType(type) ? TILES[static_cast<size_t>(type)] : UNKNOWN;
}

CellHint describeCell(const Tile& cell) {
	CellHint hint;
	hint.title = std::string(tileInfo(cell.type).name) + " (type " + std::to_string(cell.type) + ")";
	hint.description = tileInfo(cell.type).description;
	hint.attribute = unusedField();
	hint.value = unusedField();

	switch (cell.type) {
	case Door:
		hint.attribute = field("gate type", GATE_TYPES, cell.attr, "unknown, decoration only");
		if (cell.attr == GateTeleport)
			hint.value = anyNumber("pair id", cell.value, "the other teleporter with this value is the target");
		break;
	case MonsterSpawn:
		hint.attribute = field("monster type", MONSTER_TYPES, cell.attr, "unknown, spawns a copy of the player");
		break;
	case Treasure:
		hint.attribute = field("item type", ITEM_TYPES, cell.attr, "unknown, gives nothing");
		if (cell.attr == ItemType::MELEE_WEAPON)
			hint.value = field("weapon", MELEE_WEAPONS, cell.value, "unknown, gives nothing");
		else if (cell.attr == ItemType::RANGED_WEAPON)
			hint.value = field("weapon", RANGED_WEAPONS, cell.value, "unknown, gives nothing");
		else if (cell.attr == ItemType::POTION)
			hint.value = field("potion", POTIONS, cell.value, "unknown, gives nothing");
		break;
	case Key:
		hint.attribute = field("lock colour", LOCK_COLOURS, cell.attr, "no colour, opens nothing");
		break;
	case Gate:
		hint.attribute = field("lock colour", GATE_COLOURS, cell.attr, "no colour, no key or lever opens it");
		hint.value = field("state", GATE_STATES, cell.value, "not a start state, use 0 or 1");
		break;
	case Lever:
		hint.attribute = field("lock colour", LOCK_COLOURS, cell.attr, "no colour, opens nothing");
		hint.value = field("state", ZERO_ONLY, cell.value, "keep it 0");
		break;
	case RockFall:
		hint.value = field("state", ZERO_ONLY, cell.value, "keep it 0");
		break;
	default:
		break;
	}
	checkUnused(hint.attribute, cell.attr);
	checkUnused(hint.value, cell.value);
	return hint;
}
