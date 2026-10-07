#include "items.h"
#include "journal.h"
#include "../core/gameplay_config.h"
#include <array>
#include <cstdio>
#include <string>

namespace {
// The weapons, then the potions, in ItemKind order. A weapon's motion: grip, rest / windup / strike tilt, thrust, hit
// (frame delay) / swing / recovery ms. Levelgen: weapons by depth like in the campaign (the deeper, the more kinds),
// potions by weight; no antidote, as generated levels have no poisoners.
constexpr std::array<ItemDef, FIRST_AMULET> ITEMS = {{
	// The club is slow and heavy, and grows most.
	{.file = {ItemType::MELEE_WEAPON, 0},
	 .text = {"Club", "Club", "club", "", "Good old club.", "Now with spikes."},
	 .weapon = {.model = "club",
				.scale = 6,
				.damage = 10,
				.range = 2,
				.mix = {85, 15, 0},
				.growthPercent = 40,
				.motion = {0.12f, 35, -40, 115, 0, 300, 560, 600},
				.swingSound = "club_swing",
				.strikeSound = "club_hit"}},
	{.file = {ItemType::MELEE_WEAPON, 3},
	 .text = {"Dagger", "Dagger", "dagger", "", "Quick and close.", "Mind the fingers."},
	 .weapon = {.model = "dagger",
				.scale = 4,
				.damage = 8,
				.range = 1,
				.mix = {0, 30, 70},
				.growthPercent = 30,
				.motion = {0.15f, 45, 15, 100, 0.25f, 150, 320, 250},
				.swingSound = "sword_swing",
				.strikeSound = "spear_hit"}},
	// Quick; it starts strongest, so it grows least.
	{.file = {ItemType::MELEE_WEAPON, 1},
	 .text = {"Short Sword", "Sword", "short sword", "", "Bronze blade of a", "forgotten guard."},
	 .weapon = {.model = "sword",
				.scale = 9,
				.damage = 35,
				.range = 3,
				.mix = {0, 85, 15},
				.growthPercent = 10,
				.motion = {0.1f, 40, -10, 120, 0, 180, 360, 370},
				.swingSound = "sword_swing",
				.strikeSound = "sword_hit",
				.generatedFrom = 3}},
	// The khopesh flows from swing to swing; the axes and the mace wind up long.
	{.file = {ItemType::MELEE_WEAPON, 4},
	 .text = {"Khopesh", "Khopesh", "khopesh", "", "The sickle sword", "of the pharaoh's guard."},
	 .weapon = {.model = "khopesh",
				.scale = 6.5f,
				.damage = 45,
				.range = 3,
				.mix = {0, 100, 0},
				.motion = {0.12f, 40, -30, 125, 0, 300, 560, 400},
				.swingSound = "sword_swing",
				.strikeSound = "sword_hit",
				.generatedFrom = 5}},
	{.file = {ItemType::MELEE_WEAPON, 5},
	 .text = {"Epsilon Axe", "Ep. axe", "epsilon axe", "", "A broad bronze crescent.", "Swing it and wait."},
	 .weapon = {.model = "epsilon_axe",
				.scale = 8,
				.damage = 55,
				.range = 3,
				.mix = {30, 70, 0},
				.motion = {0.1f, 35, -45, 120, 0, 550, 820, 400},
				.swingSound = "club_swing",
				.strikeSound = "axe_hit",
				.generatedFrom = 7}},
	{.file = {ItemType::MELEE_WEAPON, 6},
	 .text = {"Duckbill Axe", "Db. axe", "duckbill axe", "", "Narrow and heavy.", "It goes through shells."},
	 .weapon = {.model = "duckbill_axe",
				.scale = 7,
				.damage = 50,
				.range = 3,
				.mix = {20, 0, 80},
				.motion = {0.1f, 35, -45, 115, 0, 500, 770, 400},
				.swingSound = "club_swing",
				.strikeSound = "axe_hit",
				.generatedFrom = 7}},
	{.file = {ItemType::MELEE_WEAPON, 7},
	 .text = {"Mace", "Mace", "mace", "", "A stone on a stick.", "The oldest argument."},
	 .weapon = {.model = "mace",
				.scale = 7,
				.damage = 50,
				.range = 2,
				.mix = {100, 0, 0},
				.motion = {0.1f, 35, -50, 115, 0, 600, 870, 400},
				.swingSound = "club_swing",
				.strikeSound = "mace_hit",
				.generatedFrom = 7}},
	// It thrusts.
	{.file = {ItemType::MELEE_WEAPON, 2},
	 .text = {"Spear", "Spear", "spear", "", "Long reach.", "None shall pass!"},
	 .weapon = {.model = "spear",
				.scale = 15,
				.damage = 20,
				.range = 5,
				.mix = {0, 15, 85},
				.motion = {0.35f, 70, 70, 70, 0.3f, 200, 420, 550},
				.swingSound = "spear_swing",
				.strikeSound = "spear_hit",
				.generatedFrom = 3}},
	{.file = {ItemType::RANGED_WEAPON, 0},
	 .text = {"Self-Bow", "Bow", "self-bow", "", "One stave of acacia.", "For slow monsters."},
	 .weapon = {.model = "bow",
				.scale = 12,
				.damage = 12,
				.range = 30,
				.mix = {0, 0, 100},
				.motion = {0.5f, 0, 0, 0, 0, BOW_DRAW_MS, BOW_DRAW_MS + 100, 550},
				.swingSound = "bow_draw",
				.strikeSound = "bow_release"}},
	{.file = {ItemType::RANGED_WEAPON, 1},
	 .text = {"Composite Bow", "Comp. bow", "composite bow", "", "Wood, horn and sinew,", "glued by a master."},
	 .weapon = {.model = "composite_bow",
				.scale = 11,
				.damage = 22,
				.range = 40,
				.mix = {0, 0, 100},
				.motion = {0.5f, 0, 0, 0, 0, 650, 750, 650},
				.swingSound = "bow_draw",
				.strikeSound = "bow_release",
				.generatedFrom = 7}},
	// Whirled overhead from hanging down, let go in front.
	{.file = {ItemType::RANGED_WEAPON, 2},
	 .text = {"Sling", "Sling", "sling", "", "A shepherd's sling.", "Stones are everywhere."},
	 .weapon = {.model = "sling",
				.scale = 5,
				.damage = 10,
				.range = 20,
				.mix = {100, 0, 0},
				.motion = {0.05f, 160, -150, 45, 0, 350, 600, 450},
				.swingSound = "sling_swing",
				.strikeSound = "sling_release",
				.missile = MissileKind::Stone}},
	{.file = {ItemType::RANGED_WEAPON, 3},
	 .text = {"Throwing Stick", "Stick", "throwing stick", "", "Brings down a duck.", "Comes back, mostly."},
	 .weapon = {.model = "throwing_stick",
				.scale = 5,
				.damage = 14,
				.range = 12,
				.mix = {90, 10, 0},
				.motion = {0.08f, 40, -60, 100, 0, 300, 500, 400},
				.swingSound = "club_swing",
				.strikeSound = "throw",
				.missile = MissileKind::Stick,
				.thrown = true,
				.generatedFrom = 4}},
	{.file = {ItemType::RANGED_WEAPON, 4},
	 .text = {"Javelin", "Javelin", "javelin", "", "Light enough to throw,", "heavy enough to stay."},
	 .weapon = {.model = "javelin",
				.scale = 11,
				.damage = 30,
				.range = 15,
				.mix = {0, 10, 90},
				.motion = {0.45f, 60, 20, 80, 0, 500, 700, 700},
				.swingSound = "spear_swing",
				.strikeSound = "throw",
				.missile = MissileKind::Javelin,
				.thrown = true,
				.generatedFrom = 4}},
	{.file = {ItemType::POTION, 0},
	 .text = {"Small Health", "Heal", "small health", "Heals 25% of max health", "Bitter herbs from",
			  "the Nile marshes."},
	 .potion = {.gain = {.healPercent = PotionEffect::SMALL_HEAL_PERCENT},
				.colour = {1.f, 0.f, 0.f},
				.note = FieldNote::HealthPotions,
				.generatedWeight = 30}},
	{.file = {ItemType::POTION, 1},
	 .text = {"Large Health", "Heal+", "large health", "Heals 50% of max health", "Brewed by the priests",
			  "of Sekhmet."},
	 .potion = {.gain = {.healPercent = PotionEffect::LARGE_HEAL_PERCENT},
				.colour = {0.7f, 0.f, 0.3f},
				.note = FieldNote::HealthPotions,
				.generatedWeight = 15}},
	{.file = {ItemType::POTION, 2},
	 .text = {"Aphethamine", "Might", "might", "Might +2, for good", "It tingles. Best not", "ask what is in it."},
	 .potion = {.gain = {.might = PotionEffect::MIGHT},
				.colour = {0.4f, 0.f, 0.6f},
				.note = FieldNote::Might,
				.generatedWeight = 10}},
	{.file = {ItemType::POTION, 3},
	 .text = {"Stone Skin", "Armor", "armor", "Armor +2, for good", "Skin as hard as", "temple granite."},
	 .potion = {.gain = {.armor = PotionEffect::ARMOR},
				.colour = {1.f, 0.6f, 0.f},
				.note = FieldNote::Armor,
				.generatedWeight = 10}},
	{.file = {ItemType::POTION, 4},
	 .text = {"Elixir of Life", "Life", "life", "Max health +5%, full heal", "The breath of Osiris,",
			  "sealed in a flask."},
	 .potion = {.gain = {.maxHpPercent = PotionEffect::LIFE_MAX_HP_PERCENT},
				.colour = {0.7f, 0.6f, 0.3f},
				.note = FieldNote::Life,
				.generatedWeight = 5}},
	// Small Stamina: green faience
	{.file = {ItemType::POTION, 5},
	 .text = {"Small Stamina", "Vigor", "small stamina", "Restores 50% of stamina", "Date wine and honey.",
			  "Mostly honey."},
	 .potion = {.gain = {.staminaPercent = PotionEffect::SMALL_STAMINA_PERCENT},
				.colour = {0.45f, 0.85f, 0.25f},
				.note = FieldNote::StaminaPotions,
				.generatedWeight = 20}},
	// Large Stamina: turquoise
	{.file = {ItemType::POTION, 6},
	 .text = {"Large Stamina", "Vigor+", "large stamina", "Restores all stamina", "Sun-steeped water",
			  "from the temple of Ra."},
	 .potion = {.gain = {.staminaPercent = PotionEffect::LARGE_STAMINA_PERCENT},
				.colour = {0.15f, 0.78f, 0.72f},
				.note = FieldNote::StaminaPotions,
				.generatedWeight = 10}},
	// Antidote: dark malachite
	{.file = {ItemType::POTION, 7},
	 .text = {"Antidote", "Cure", "antidote", "Cures all poison", "Milk of the snake", "goddess Renenutet."},
	 .potion = {.gain = {.cure = true},
				.colour = {0.05f, 0.45f, 0.2f},
				.note = FieldNote::Antidote,
				.generatedWeight = 0,
				.mimicLoot = false}},
}};

constexpr bool validItems() {
	int weights = 0;
	for (size_t i = 0; i < ITEMS.size(); i++) {
		const ItemDef& def = ITEMS[i];
		for (size_t j = 0; j < i; j++)
			if (ITEMS[j].file.type == def.file.type && ITEMS[j].file.id == def.file.id)
				return false;
		const bool weapon = static_cast<int>(i) < WEAPON_KIND_COUNT;
		if (weapon &&
			(def.file.type == ItemType::POTION || def.weapon.mix[0] + def.weapon.mix[1] + def.weapon.mix[2] != 100))
			return false;
		if (!weapon && def.file.type != ItemType::POTION)
			return false;
		weights += weapon ? 0 : def.potion.generatedWeight;
	}
	return weights == 100;
}
static_assert(validItems(), "ITEMS: weapons then potions, unique file ids, damage mixes and potion weights of 100%");

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
static_assert(AMULETS.back().type == AmuletType::Venom && AMULETS.back().tier == AmuletTier::Grand,
			  "AMULETS ends with grand venom, the last ItemKind");

// Per type: the bonus of each tier (lesser, minor, normal, grand; 0: no such tier) and the texts its tiers share.
struct AmuletTypeDef {
	const char* model; // models/items/<model>.md3
	std::array<int, AMULET_TIER_COUNT> amount;
	const char* noun;	   // "Amulet of <noun>"
	const char* shortNoun; // the slot's name band, with the tier as I .. IV
	const char* effect;	   // printf format of the amount
	const char* lore1;
	const char* lore2;
};
constexpr std::array<AmuletTypeDef, AMULET_TYPE_COUNT> AMULET_TYPES = {{
	{"amulet_strength",
	 {1, 2, 4, 6},
	 "Strength",
	 "Str.",
	 "Might +%d while worn",
	 "A jackal's tooth",
	 "on a cord. Bite back."},
	{"amulet_armor",
	 {1, 2, 4, 6},
	 "Armor",
	 "Arm.",
	 "Armor +%d while worn",
	 "A bronze scarab.",
	 "Hard shell, soft heart."},
	{"amulet_health",
	 {5, 10, 20, 30},
	 "Health",
	 "Life",
	 "Max health +%d%% while worn",
	 "A carnelian heart.",
	 "It beats with yours."},
	{"amulet_poison",
	 {10, 25, 50, 80},
	 "Poison Warding",
	 "Pois.",
	 "Resists poison %d%% of the time",
	 "Serket's scorpion.",
	 "Her children spare you."},
	{"amulet_traps",
	 {25, 50, 75, 100},
	 "Trap Warding",
	 "Trap",
	 "Trap damage -%d%%",
	 "The eye of Horus.",
	 "It sees the spikes first."},
	{"amulet_blunt",
	 {8, 16, 28, 40},
	 "Blunt Warding",
	 "Blunt",
	 "Blunt damage -%d%%",
	 "The djed pillar.",
	 "It does not bend."},
	{"amulet_slash",
	 {8, 16, 28, 40},
	 "Slash Warding",
	 "Slash",
	 "Slash damage -%d%%",
	 "The knot of Isis.",
	 "Blades slip on it."},
	{"amulet_pierce",
	 {8, 16, 28, 40},
	 "Pierce Warding",
	 "Pierce",
	 "Pierce damage -%d%%",
	 "The shen ring.",
	 "Points glance off it."},
	{"amulet_regeneration",
	 {0, 0, 1, 2},
	 "Regeneration",
	 "Regen",
	 "Heals %d health a second when safe",
	 "A faience lotus.",
	 "It opens when all is calm."},
	{"amulet_venom",
	 {10, 15, 20, 30},
	 "Venom",
	 "Venom",
	 "%d%% of hits poison (%s)",
	 "Wadjet's rearing cobra.",
	 "Your blade grows fangs."},
}};
// The venom amulet's poison, by tier: the stronger amulets poison more often and worse.
constexpr std::array<PoisonTier, AMULET_TIER_COUNT> VENOM_TIERS = {PoisonTier::Weak, PoisonTier::Weak,
																   PoisonTier::Medium, PoisonTier::Strong};
constexpr const char* POISON_TIER_WORDS[POISON_TIER_COUNT] = {"weak", "medium", "strong"};
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
			else if (a.type == AmuletType::Venom)
				std::snprintf(effect, sizeof(effect), def.effect, amount(a),
							  POISON_TIER_WORDS[static_cast<size_t>(VENOM_TIERS[static_cast<size_t>(a.tier)])]);
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
	case AmuletType::Venom:
		bonus.venomPercent = n;
		bonus.venomTier = VENOM_TIERS[static_cast<size_t>(a.tier)];
		break;
	}
	return bonus;
}

const char* amuletModel(AmuletType type) { return typeDef(type).model; }

const WeaponDef& weaponDef(ItemKind weapon) { return ITEMS[static_cast<size_t>(itemIndex(weapon))].weapon; }

const PotionDef& potionDef(ItemKind potion) { return ITEMS[static_cast<size_t>(itemIndex(potion))].potion; }

ItemFileId fileIdOf(ItemKind kind) {
	if (isAmulet(kind))
		return {ItemType::AMULET, itemIndex(kind) - FIRST_AMULET};
	return ITEMS[static_cast<size_t>(itemIndex(kind))].file;
}

std::optional<ItemKind> itemFromFile(int type, int id) {
	if (type == ItemType::AMULET)
		return id >= 0 && id < AMULET_KIND_COUNT ? std::optional(itemAt(FIRST_AMULET + id)) : std::nullopt;
	for (int i = 0; i < FIRST_AMULET; i++)
		if (ITEMS[static_cast<size_t>(i)].file.type == type && ITEMS[static_cast<size_t>(i)].file.id == id)
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
	return ITEMS[static_cast<size_t>(itemIndex(kind))].text;
}
