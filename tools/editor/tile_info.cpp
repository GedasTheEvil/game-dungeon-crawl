#include "tile_info.h"
#include "../../src/world/items.h"
#include "../../src/world/monster_kinds.h"
#include <string>
#include <array>
#include <vector>

namespace {

// Icons under tools/editor/icons/ (make_icons.py), nullptr: drawn as a flat colour. Names and descriptions come from
// the game's tile table (src/world/tile_defs.h).
// By type number; 0 and 7 are no type.
constexpr std::array<const char*, TILE_TYPE_COUNT> ICONS = {{
	nullptr,
	nullptr,
	"gate.png",
	"death.png",
	"monster.png",
	"spikes.png",
	"ladder.png",
	nullptr,
	"treasure.png",
	"ankh.png",
	"key.png",
	"gate_lock.png",
	"lever.png",
	"rockfall.png",
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

Choices monsterTypes() {
	Choices types;
	for (int type = 1; type <= MONSTER_TYPE_MAX; type++)
		if (const MonsterKind* kind = monsterKind(type))
			types.push_back({type, kind->label, kind->description});
	return types;
}

// Key and lever colours; gates also take the boss lock.
Choices lockColours(bool withBoss) {
	Choices colours;
	for (int colour = 1; colour <= (withBoss ? BOSS_LOCK : LOCK_COLOUR_COUNT); colour++)
		colours.push_back({colour, lockColour(colour).name, lockColour(colour).choice});
	return colours;
}

const Choices MONSTER_TYPES = monsterTypes();
const Choices LOCK_CHOICES = lockColours(false);
const Choices GATE_CHOICES = lockColours(true);

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

TileInfo tileInfo(int type) {
	const TileDef& def = tileDef(type);
	return {def.name, isTileType(type) ? ICONS[static_cast<size_t>(type)] : nullptr, def.description};
}

CellHint describeStructure(Structure s) {
	CellHint hint;
	hint.title = std::string(structureDef(s).name) + " (structure " + std::to_string(static_cast<int>(s)) + ")";
	hint.description = structureDef(s).description;
	hint.attribute = unusedField();
	hint.value = unusedField();
	return hint;
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
		hint.attribute = field("lock colour", LOCK_CHOICES, cell.attr, "no colour, opens nothing");
		break;
	case Gate:
		hint.attribute = field("lock colour", GATE_CHOICES, cell.attr, "no colour, no key or lever opens it");
		hint.value = field("state", GATE_STATES, cell.value, "not a start state, use 0 or 1");
		break;
	case Lever:
		hint.attribute = field("lock colour", LOCK_CHOICES, cell.attr, "no colour, opens nothing");
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
