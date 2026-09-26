#include "tile_info.h"
#include <array>
#include <vector>

namespace {

constexpr std::array<TileInfo, TILE_COUNT> TILES = {{
	{"Wall", nullptr, "Solid block. The player stands on it and cannot walk through it."},
	{"Empty", nullptr, "Open space. With no Wall below, the player falls."},
	{"Door", "gate.png", "Sphinx statue. What it does depends on the gate type."},
	{"Death", "death.png", "Large spike trap. Damages the player on contact."},
	{"Monster", "monster.png", "Spawns a monster when the cell comes into view. Max. 9 monsters are active at a time."},
	{"Spike", "spikes.png", "Small spike trap. Damages the player on contact."},
	{"Ladder", "ladder.png", "The player climbs between vertically adjacent ladder cells and does not fall on them."},
	{"3D", "3D.png", "Not used by the game. Renders as open space."},
	{"Treasure", "treasure.png", "Chest with an item on top. Interact to pick it up, the cell then becomes Empty."},
	{"Ankh", "ankh.png", "Level goal. Interact with it to win the game."},
	{"Key", "key.png", "Key on the floor, picked up on touch. Opens the gates of its colour."},
	{"Gate", "gate_lock.png", "Portcullis. Opens with the key of its colour or when a lever of its colour is pulled."},
	{"Lever", "lever.png", "Interact to pull it. Opens every gate of the same colour."},
	{"RockFall", "rockfall.png",
	 "Loose ceiling, walkable. A rock falls ~1 s after the player steps in: 20 damage. Put a Wall above it."},
}};

struct Choice {
	int number;
	const char* name;	 // in the choices list
	const char* meaning; // after "n = "
};

using Choices = std::vector<Choice>;

const Choices GATE_TYPES = {
	{1, "entrance", "Entrance, the player start. One per level"},
	{2, "exit", "Exit, loads the next level"},
	{3, "riddle", "Riddle, asks a riddle, then becomes an empty gate"},
	{4, "empty", "Empty gate, decoration only"},
	{0, "decoration", "Decoration only"},
};

const Choices MONSTER_TYPES = {
	{MonsterScarab, "scarab", "Scarab"}, {MonsterWorm, "worm", "Worm"},
	{MonsterPlant, "plant", "Plant"},	 {MonsterAnubis, "anubis", "Anubis"},
	{MonsterRat, "rat", "Rat"},			 {MonsterGiantRat, "giant rat", "Giant rat"},
	{MonsterBat, "bat", "Bat"},			 {MonsterGiantBat, "giant bat", "Giant bat"},
};

const Choices LOCK_COLOURS = {
	{1, "red", "Red (Carnelian)"},
	{2, "blue", "Blue (Lapis)"},
	{3, "green", "Green (Turquoise)"},
	{4, "gold", "Gold (Amber)"},
};

constexpr int ITEM_MELEE = 1;
constexpr int ITEM_RANGED = 2;
constexpr int ITEM_POTION = 3;

const Choices ITEM_TYPES = {
	{ITEM_MELEE, "melee weapon", "Melee weapon"},
	{ITEM_RANGED, "ranged weapon", "Ranged weapon"},
	{ITEM_POTION, "potion", "Potion"},
	{0, "empty chest", "Empty chest"},
};

const Choices MELEE_WEAPONS = {{0, "club", "Club"}, {1, "sword", "Sword"}, {2, "spear", "Spear"}};
const Choices RANGED_WEAPONS = {{0, "bow", "Bow"}};
const Choices POTIONS = {
	{0, "small health", "Small health, +25 HP"},
	{1, "large health", "Large health, +50 HP"},
	{2, "strength", "Strength, +2 might"},
	{3, "armor", "Armor, +2 armor"},
	{4, "life", "Life, +5% max. HP and full heal"},
	{5, "small stamina", "Small stamina, 50% stamina"},
	{6, "large stamina", "Large stamina, full stamina"},
};

const Choices GATE_STATES = {{0, "closed", "Closed"}, {1, "open", "Open"}};
const Choices ZERO_ONLY = {{0, "only", "Set by the game while playing"}};

FieldHint unusedField() {
	FieldHint hint;
	hint.label = "not used";
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

CellHint describeCell(const Tint& cell) {
	CellHint hint;
	hint.title = std::string(tileInfo(cell.a).name) + " (type " + std::to_string(cell.a) + ")";
	hint.description = tileInfo(cell.a).description;
	hint.attribute = unusedField();
	hint.value = unusedField();

	switch (cell.a) {
	case Door:
		hint.attribute = field("gate type", GATE_TYPES, cell.b, "unknown, decoration only");
		break;
	case Monster:
		hint.attribute = field("monster type", MONSTER_TYPES, cell.b, "unknown, spawns a copy of the player");
		break;
	case Treasure:
		hint.attribute = field("item type", ITEM_TYPES, cell.b, "unknown, gives nothing");
		if (cell.b == ITEM_MELEE)
			hint.value = field("weapon", MELEE_WEAPONS, cell.c, "unknown, gives nothing");
		else if (cell.b == ITEM_RANGED)
			hint.value = field("weapon", RANGED_WEAPONS, cell.c, "unknown, gives nothing");
		else if (cell.b == ITEM_POTION)
			hint.value = field("potion", POTIONS, cell.c, "unknown, gives nothing");
		break;
	case Key:
		hint.attribute = field("lock colour", LOCK_COLOURS, cell.b, "no colour, opens nothing");
		break;
	case Gate:
		hint.attribute = field("lock colour", LOCK_COLOURS, cell.b, "no colour, no key or lever opens it");
		hint.value = field("state", GATE_STATES, cell.c, "not a start state, use 0 or 1");
		break;
	case Lever:
		hint.attribute = field("lock colour", LOCK_COLOURS, cell.b, "no colour, opens nothing");
		hint.value = field("state", ZERO_ONLY, cell.c, "keep it 0");
		break;
	case RockFall:
		hint.value = field("state", ZERO_ONLY, cell.c, "keep it 0");
		break;
	default:
		break;
	}
	checkUnused(hint.attribute, cell.b);
	checkUnused(hint.value, cell.c);
	return hint;
}
