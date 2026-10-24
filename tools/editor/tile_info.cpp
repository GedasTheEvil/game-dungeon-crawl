#include "tile_info.h"
#include "dressing_brush.h"
#include "../../src/world/items.h"
#include "../../src/world/monster_kinds.h"
#include <string>
#include <vector>

namespace {

struct Choice {
	int number;
	const char* name; // in the choices list
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
const Choices SPIKE_SPEEDS = {
	{0, "normal", "Normal: out 1.2 s of every 4 s"},
	{1, "fast", "Fast: out 0.8 s of every 3.2 s"},
	{2, "slow", "Slow: out 1.6 s of every 4.8 s"},
};
const Choices BEAM_DIRS_LIST = {
	{0, "down", "Down"},
	{1, "left", "Left"},
	{2, "up", "Up"},
	{3, "right", "Right"},
};
const Choices SPIKE_PHASES_LIST = {
	{0, "0/8", "Out at the start of the cycle"},
	{1, "1/8", "An eighth of the cycle later"},
	{2, "2/8", "A quarter later"},
	{3, "3/8", "Three eighths later"},
	{4, "4/8", "Half a cycle later"},
	{5, "5/8", "Five eighths later"},
	{6, "6/8", "Three quarters later"},
	{7, "7/8", "Seven eighths later"},
};

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
	return {def.name, def.icon, def.description};
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
			hint.value = anyNumber(
				"pair id",
				cell.value,
				"the other teleporter with this value is the target; + 256: a name gate, sealed by "
				"the level's name"
			);
		break;
	case MonsterSpawn:
		hint.attribute = field("monster type", MONSTER_TYPES, cell.attr, "unknown, spawns a copy of the player");
		break;
	case Treasure:
	case TrappedTreasure:
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
	case Inscription:
		hint.attribute = anyNumber(
			"text id",
			cell.attr,
			"id: line in content/inscriptions/*.txt, 1 to 64; 100: a name cartouche"
		);
		break;
	case Page:
		hint.attribute = anyNumber("page", cell.attr, "its number, 1 to 18 in order of depth over the campaign");
		hint.value = field("state", ZERO_ONLY, cell.value, "keep it 0");
		break;
	case RockFall:
	case DartPlate:
		hint.value = field("state", ZERO_ONLY, cell.value, "keep it 0");
		break;
	case LightShaft:
		hint.attribute = field("way", BEAM_DIRS_LIST, cell.attr, "the game keeps only the low 2 bits");
		break;
	case Mirror:
		hint.value = field("way", BEAM_DIRS_LIST, cell.value, "the game keeps only the low 2 bits");
		break;
	case SunDisc:
		hint.attribute = field("lock colour", LOCK_CHOICES, cell.attr, "no colour, opens nothing");
		hint.value = field("state", ZERO_ONLY, cell.value, "keep it 0");
		break;
	case WallSpikes:
	case FireVent:
		hint.attribute = field("speed", SPIKE_SPEEDS, cell.attr, "unknown, normal speed");
		hint.value = field("phase", SPIKE_PHASES_LIST, cell.value, "the game keeps only the low 3 bits");
		break;
	default:
		break;
	}
	checkUnused(hint.attribute, cell.attr);
	checkUnused(hint.value, cell.value);
	return hint;
}

namespace {
struct PropInfo {
	const char* label;
	const char* description;
};
constexpr PropInfo PROPS[DECOR_COUNT] = {
	{"Web", "A cobweb in the upper corner of the back wall, tucked into a side wall if there is one. Needs a ceiling."},
	{"Pottery", "Clay jars on the floor."},
	{"Canopic", "Canopic jars, the lids heads of the four sons of Horus."},
	{"Rubble", "Fallen stones."},
	{"Sand", "A heap of sand blown in."},
	{"Skeleton", "Remains of someone who came before. Every expedition page lies on one."},
	{"Brazier", "A bronze brazier with its fire: it lights the cell. No wall torch over it."},
	{"Lamp", "An oil lamp on a stand, lit. No wall torch over it."},
	{"Scrolls", "Papyrus scrolls on the floor."},
	{"Ushabti", "A servant figure. A living ushabti stands among them until the player comes near."},
	{"Cat", "A statue of a seated cat, Bastet."},
	{"Jackal", "A statue of a lying jackal, Anubis."},
	{"Osiris", "A statue of Osiris."},
	{"Ibis", "Thoth as a standing ibis. Its crown reaches up to a wall torch: none over it."},
	{"Ibis sits", "Thoth as a seated ibis. No wall torch over it."},
	{"Baboon", "Thoth as a seated baboon. No wall torch over it."},
	{"Baboon up", "Thoth as a standing baboon. No wall torch over it."},
	{"Sarcophagus", "A stone sarcophagus."},
	{"Coffin", "An empty coffin, as on every mummy's spawn tile (the scatter puts that one by itself)."},
	{"Reeds", "Papyrus reeds, in half water only. They sway."},
	{"Weed", "Water weed under the surface, in half water only."},
	{"Pads", "Lotus leaves afloat, in half water only."},
	{"Lotus", "Lotus leaves with a blue lotus in bloom, in half water only. The water lotus hides among them."},
	{"Large coffin", "The mummy priest's coffin, a head longer and gilded."},
	{"Sleeper",
	 "Decoy: a coffin with a mummy asleep in it that never wakes. Beside the real mummy's coffin, it keeps "
	 "the player guessing."},
};

const char* anchorText(DecalAnchor anchor) {
	switch (anchor) {
	case DecalAnchor::Ceiling:
		return "It hangs from the ceiling: needs rock above.";
	case DecalAnchor::Floor:
		return "It grows from the floor: needs a dry floor, no sand, no prop in the cell.";
	case DecalAnchor::Free:
		break;
	}
	return "At eye height: no torch in the cell.";
}

CellHint dressingHint(const std::string& title, const std::string& description) {
	CellHint hint;
	hint.title = title;
	hint.description = description;
	return hint;
}
} // namespace

const char* propLabel(int prop) {
	if (prop == PLACED_UNSET)
		return "Unset";
	if (prop == PLACED_BARE)
		return "Bare";
	return prop >= 0 && prop < DECOR_COUNT ? PROPS[prop].label : "?";
}

const char* decalLabel(int brush) {
	switch (brush) {
	case PLACED_UNSET:
		return "Unset";
	case PLACED_BARE:
		return "Bare";
	case TORCH_BRUSH:
		return "Torch";
	case NO_TORCH_BRUSH:
		return "No torch";
	default:
		break;
	}
	return brush >= 0 && brush < DECAL_COUNT ? DECAL_DEFS[brush].name : "?";
}

CellHint describeProp(int prop) {
	if (prop == PLACED_UNSET)
		return dressingHint(
			"Unset prop",
			"The scatter rolls the cell's prop as on any level (by depth, 30% of free "
			"floor, half of the half water). Coffins on mummy spawns come by themselves."
		);
	if (prop == PLACED_BARE)
		return dressingHint("Bare", "No prop: the scatter leaves the cell empty.");
	if (prop < 0 || prop >= DECOR_COUNT)
		return dressingHint("Unknown prop", "");
	return dressingHint(
		std::string(PROPS[prop].label) + " (prop " + std::to_string(prop) + ")",
		std::string(PROPS[prop].description) + " Placed, whatever the depth. M: mirror."
	);
}

CellHint describeDecal(int brush) {
	switch (brush) {
	case PLACED_UNSET:
		return dressingHint(
			"Unset decal",
			"The scatter rolls the cell's decal (35%) and torch (25%, 4 cells apart) "
			"as on any level. Clears both fields."
		);
	case PLACED_BARE:
		return dressingHint("Bare", "No decal and no torch in the cell.");
	case TORCH_BRUSH:
		return dressingHint(
			"Torch",
			"A wall torch, it lights the cell. Random torches keep 4 cells away from it. "
			"Clears a free decal in the cell."
		);
	case NO_TORCH_BRUSH:
		return dressingHint("No torch", "No torch in the cell; the decal stays as it is.");
	default:
		break;
	}
	if (brush < 0 || brush >= DECAL_COUNT)
		return dressingHint("Unknown decal", "");
	return dressingHint(
		std::string(DECAL_DEFS[brush].name) + " (decal " + std::to_string(brush) + ")",
		std::string("On the back wall, whatever the depth. ") + anchorText(DECAL_DEFS[brush].anchor) + " M: mirror."
	);
}
