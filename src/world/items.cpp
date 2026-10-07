#include "items.h"
#include <array>
#include <cstdio>
#include <string>

namespace {
// The weapons and the potions; an amulet is {ItemType::AMULET, its place among the amulets}.
constexpr std::array<ItemFileId, FIRST_AMULET> FILE_IDS = {{
	{ItemType::MELEE_WEAPON, 0},  {ItemType::MELEE_WEAPON, 3},	{ItemType::MELEE_WEAPON, 1},
	{ItemType::MELEE_WEAPON, 4},  {ItemType::MELEE_WEAPON, 5},	{ItemType::MELEE_WEAPON, 6},
	{ItemType::MELEE_WEAPON, 7},  {ItemType::MELEE_WEAPON, 2},	{ItemType::RANGED_WEAPON, 0},
	{ItemType::RANGED_WEAPON, 1}, {ItemType::RANGED_WEAPON, 2}, {ItemType::RANGED_WEAPON, 3},
	{ItemType::RANGED_WEAPON, 4}, {ItemType::POTION, 0},		{ItemType::POTION, 1},
	{ItemType::POTION, 2},		  {ItemType::POTION, 3},		{ItemType::POTION, 4},
	{ItemType::POTION, 5},		  {ItemType::POTION, 6},		{ItemType::POTION, 7},
}};

constexpr std::array<ItemText, FIRST_AMULET> TEXTS = {{
	{"Club", "Club", "club", "", "Good old club.", "Now with spikes."},
	{"Dagger", "Dagger", "dagger", "", "Quick and close.", "Mind the fingers."},
	{"Short Sword", "Sword", "short sword", "", "Bronze blade of a", "forgotten guard."},
	{"Khopesh", "Khopesh", "khopesh", "", "The sickle sword", "of the pharaoh's guard."},
	{"Epsilon Axe", "Ep. axe", "epsilon axe", "", "A broad bronze crescent.", "Swing it and wait."},
	{"Duckbill Axe", "Db. axe", "duckbill axe", "", "Narrow and heavy.", "It goes through shells."},
	{"Mace", "Mace", "mace", "", "A stone on a stick.", "The oldest argument."},
	{"Spear", "Spear", "spear", "", "Long reach.", "None shall pass!"},
	{"Self-Bow", "Bow", "self-bow", "", "One stave of acacia.", "For slow monsters."},
	{"Composite Bow", "Comp. bow", "composite bow", "", "Wood, horn and sinew,", "glued by a master."},
	{"Sling", "Sling", "sling", "", "A shepherd's sling.", "Stones are everywhere."},
	{"Throwing Stick", "Stick", "throwing stick", "", "Brings down a duck.", "Comes back, mostly."},
	{"Javelin", "Javelin", "javelin", "", "Light enough to throw,", "heavy enough to stay."},
	{"Small Health", "Heal", "small health", "Heals 25% of max health", "Bitter herbs from", "the Nile marshes."},
	{"Large Health", "Heal+", "large health", "Heals 50% of max health", "Brewed by the priests", "of Sekhmet."},
	{"Aphethamine", "Might", "might", "Might +2, for good", "It tingles. Best not", "ask what is in it."},
	{"Stone Skin", "Armor", "armor", "Armor +2, for good", "Skin as hard as", "temple granite."},
	{"Elixir of Life", "Life", "life", "Max health +5%, full heal", "The breath of Osiris,", "sealed in a flask."},
	{"Small Stamina", "Vigor", "small stamina", "Restores 50% of stamina", "Date wine and honey.", "Mostly honey."},
	{"Large Stamina", "Vigor+", "large stamina", "Restores all stamina", "Sun-steeped water", "from the temple of Ra."},
	{"Antidote", "Cure", "antidote", "Cures all poison", "Milk of the snake", "goddess Renenutet."},
}};

// Weapons only, in ItemKind order: blunt, slash, pierce percent.
constexpr std::array<DamageMix, WEAPON_KIND_COUNT> WEAPON_MIXES = {{
	{85, 15, 0}, // club
	{0, 30, 70}, // dagger
	{0, 85, 15}, // short sword
	{0, 100, 0}, // khopesh
	{30, 70, 0}, // epsilon axe
	{20, 0, 80}, // duckbill axe
	{100, 0, 0}, // mace
	{0, 15, 85}, // spear
	{0, 0, 100}, // self-bow
	{0, 0, 100}, // composite bow
	{100, 0, 0}, // sling
	{90, 10, 0}, // throwing stick
	{0, 10, 90}, // javelin
}};
// The amulets in ItemKind order.
constexpr std::array<Amulet, AMULET_KIND_COUNT> AMULETS = [] {
	std::array<Amulet, AMULET_KIND_COUNT> amulets{};
	int i = 0;
	for (int type = 0; type < AMULET_TYPE_COUNT; type++)
		for (int tier = 0; tier < AMULET_TIER_COUNT; tier++)
			if (static_cast<AmuletType>(type) != AmuletType::Regeneration ||
				tier >= static_cast<int>(AmuletTier::Normal))
				amulets[static_cast<size_t>(i++)] = {static_cast<AmuletType>(type), static_cast<AmuletTier>(tier)};
	return amulets;
}();
static_assert(AMULETS.back().type == AmuletType::Regeneration && AMULETS.back().tier == AmuletTier::Grand,
			  "AMULETS ends with grand regeneration, the last ItemKind");

// Per type: the bonus of each tier (lesser, minor, normal, grand; 0: no such tier) and the texts its tiers share.
struct AmuletTypeDef {
	std::array<int, AMULET_TIER_COUNT> amount;
	const char* noun;	   // "Amulet of <noun>"
	const char* shortNoun; // the slot's name band, with the tier as I .. IV
	const char* effect;	   // printf format of the amount
	const char* lore1;
	const char* lore2;
};
constexpr std::array<AmuletTypeDef, AMULET_TYPE_COUNT> AMULET_TYPES = {{
	{{1, 2, 4, 6}, "Strength", "Str.", "Might +%d while worn", "A jackal's tooth", "on a cord. Bite back."},
	{{1, 2, 4, 6}, "Armor", "Arm.", "Armor +%d while worn", "A bronze scarab.", "Hard shell, soft heart."},
	{{5, 10, 20, 30}, "Health", "Life", "Max health +%d%% while worn", "A carnelian heart.", "It beats with yours."},
	{{10, 25, 50, 80},
	 "Poison Warding",
	 "Pois.",
	 "Resists poison %d%% of the time",
	 "Serket's scorpion.",
	 "Her children spare you."},
	{{25, 50, 75, 100}, "Trap Warding", "Trap", "Trap damage -%d%%", "The eye of Horus.", "It sees the spikes first."},
	{{8, 16, 28, 40}, "Blunt Warding", "Blunt", "Blunt damage -%d%%", "The djed pillar.", "It does not bend."},
	{{8, 16, 28, 40}, "Slash Warding", "Slash", "Slash damage -%d%%", "The knot of Isis.", "Blades slip on it."},
	{{8, 16, 28, 40}, "Pierce Warding", "Pierce", "Pierce damage -%d%%", "The shen ring.", "Points glance off it."},
	{{0, 0, 1, 2},
	 "Regeneration",
	 "Regen",
	 "Heals %d health a second when safe",
	 "A faience lotus.",
	 "It opens when all is calm."},
}};
constexpr const char* TIER_NUMERALS[AMULET_TIER_COUNT] = {"I", "II", "III", "IV"};

const AmuletTypeDef& typeDef(AmuletType type) { return AMULET_TYPES[static_cast<size_t>(type)]; }
int amount(Amulet a) { return typeDef(a.type).amount[static_cast<size_t>(a.tier)]; }

// Built once: the names are put together from the type and the tier.
struct AmuletTexts {
	std::array<std::string, AMULET_KIND_COUNT> names, shortNames, labels, effects;
	std::array<ItemText, AMULET_KIND_COUNT> texts{};
	AmuletTexts() {
		constexpr const char* TIER_WORDS[AMULET_TIER_COUNT] = {"lesser", "minor", "", "grand"};
		for (size_t i = 0; i < AMULETS.size(); i++) {
			const Amulet a = AMULETS[i];
			const AmuletTypeDef& def = typeDef(a.type);
			const std::string noun = def.noun;
			switch (a.tier) {
			case AmuletTier::Lesser:
				names[i] = "Lesser Amulet of " + noun;
				break;
			case AmuletTier::Minor:
				names[i] = "Amulet of Minor " + noun;
				break;
			case AmuletTier::Normal:
				names[i] = "Amulet of " + noun;
				break;
			case AmuletTier::Grand:
				names[i] = "Grand Amulet of " + noun;
				break;
			}
			shortNames[i] = std::string(def.shortNoun) + " " + TIER_NUMERALS[static_cast<size_t>(a.tier)];
			const std::string tierWord = TIER_WORDS[static_cast<size_t>(a.tier)];
			labels[i] = tierWord.empty() ? "" : tierWord + " ";
			labels[i] += "amulet of ";
			labels[i] += noun;
			for (char& c : labels[i])
				c = static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
			char effect[64];
			if (a.type == AmuletType::TrapWard && amount(a) >= 100)
				std::snprintf(effect, sizeof(effect), "Immune to traps");
			else
				std::snprintf(effect, sizeof(effect), def.effect, amount(a));
			effects[i] = effect;
			texts[i] = {names[i].c_str(), shortNames[i].c_str(), labels[i].c_str(), effects[i].c_str(), def.lore1,
						def.lore2};
		}
	}
};
} // namespace

Amulet amuletOf(ItemKind amulet) { return AMULETS[static_cast<size_t>(itemIndex(amulet) - FIRST_AMULET)]; }

std::optional<ItemKind> amuletKind(AmuletType type, AmuletTier tier) {
	for (size_t i = 0; i < AMULETS.size(); i++)
		if (AMULETS[i].type == type && AMULETS[i].tier == tier)
			return itemAt(FIRST_AMULET + static_cast<int>(i));
	return std::nullopt;
}

AmuletBonus amuletBonus(std::optional<ItemKind> worn) {
	AmuletBonus bonus;
	if (!worn || !isAmulet(*worn))
		return bonus;
	const Amulet a = amuletOf(*worn);
	const int n = amount(a);
	switch (a.type) {
	case AmuletType::Strength:
		bonus.might = n;
		break;
	case AmuletType::Armor:
		bonus.armor = n;
		break;
	case AmuletType::Health:
		bonus.maxHpPercent = n;
		break;
	case AmuletType::PoisonWard:
		bonus.poisonResistPercent = n;
		break;
	case AmuletType::TrapWard:
		bonus.trapCutPercent = n;
		break;
	case AmuletType::BluntWard:
		bonus.resist[static_cast<size_t>(DamageType::Blunt)] = NORMAL - n;
		break;
	case AmuletType::SlashWard:
		bonus.resist[static_cast<size_t>(DamageType::Slash)] = NORMAL - n;
		break;
	case AmuletType::PierceWard:
		bonus.resist[static_cast<size_t>(DamageType::Pierce)] = NORMAL - n;
		break;
	case AmuletType::Regeneration:
		bonus.regenHpPerSecond = n;
		break;
	}
	return bonus;
}

const DamageMix& weaponMix(ItemKind weapon) { return WEAPON_MIXES[static_cast<size_t>(itemIndex(weapon))]; }

ItemFileId fileIdOf(ItemKind kind) {
	if (isAmulet(kind))
		return {ItemType::AMULET, itemIndex(kind) - FIRST_AMULET};
	return FILE_IDS[static_cast<size_t>(itemIndex(kind))];
}

std::optional<ItemKind> itemFromFile(int type, int id) {
	if (type == ItemType::AMULET)
		return id >= 0 && id < AMULET_KIND_COUNT ? std::optional(itemAt(FIRST_AMULET + id)) : std::nullopt;
	for (int i = 0; i < FIRST_AMULET; i++)
		if (FILE_IDS[static_cast<size_t>(i)].type == type && FILE_IDS[static_cast<size_t>(i)].id == id)
			return itemAt(i);
	return std::nullopt;
}

ItemRange groupItems(ItemGroup group) {
	ItemRange range = {ITEM_KIND_COUNT, 0};
	for (int i = ITEM_KIND_COUNT - 1; i >= 0; i--) {
		if (itemGroup(itemAt(i)) == group) {
			range.first = i;
			range.count++;
		}
	}
	return range;
}

const ItemText& itemText(ItemKind kind) {
	if (isAmulet(kind)) {
		static const AmuletTexts AMULET_TEXTS;
		return AMULET_TEXTS.texts[static_cast<size_t>(itemIndex(kind) - FIRST_AMULET)];
	}
	return TEXTS[static_cast<size_t>(itemIndex(kind))];
}
