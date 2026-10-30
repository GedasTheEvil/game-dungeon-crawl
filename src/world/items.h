#ifndef ITEMS_H
#define ITEMS_H

// The items the player can carry, without rendering: shared by the game, the editor, levelgen and the unit tests.

#include "damage.h"
#include "poison.h"
#include "rgb.h"
#include "skills.h"
#include <array>
#include <cstdint>
#include <optional>
#include <string>

// Every item, in inventory screen and save order: the melee weapons, the ranged ones, the potions, the amulets, then
// the rings.
// New kinds go at the end of their group, and groups only at the end, so a save's slots stay in this order.
enum class ItemKind : std::uint8_t {
	Club,
	Dagger,
	ShortSword,
	Khopesh,
	EpsilonAxe,
	DuckbillAxe,
	Mace,
	Spear,
	SelfBow, // the first ranged weapon
	CompositeBow,
	Sling,
	ThrowingStick,
	Javelin,
	SmallHealth,
	LargeHealth,
	Might,
	Armor,
	Life,
	SmallStamina,
	LargeStamina,
	Antidote,
	LesserResistance, // plan resistance-potion
	GreaterResistance,
	WaterBlessing, // plan water-blessing-potion: the Blessing of Hapi
	// The amulets (plan amulets), one inventory row per type: lesser, minor, normal, grand.
	StrengthLesser, // the first amulet
	StrengthMinor,
	StrengthNormal,
	StrengthGrand,
	ArmorLesser,
	ArmorMinor,
	ArmorNormal,
	ArmorGrand,
	HealthLesser,
	HealthMinor,
	HealthNormal,
	HealthGrand,
	PoisonWardLesser,
	PoisonWardMinor,
	PoisonWardNormal,
	PoisonWardGrand,
	TrapWardLesser,
	TrapWardMinor,
	TrapWardNormal,
	TrapWardGrand,
	BluntWardLesser,
	BluntWardMinor,
	BluntWardNormal,
	BluntWardGrand,
	SlashWardLesser,
	SlashWardMinor,
	SlashWardNormal,
	SlashWardGrand,
	PierceWardLesser,
	PierceWardMinor,
	PierceWardNormal,
	PierceWardGrand,
	RegenerationNormal, // regeneration has no lesser or minor tier
	RegenerationGrand,
	VenomLesser, // plan venom-amulet
	VenomMinor,
	VenomNormal,
	VenomGrand,
	FireWardLesser, // plan fire-damage
	FireWardMinor,
	FireWardNormal,
	FireWardGrand,
	CrushingLesser, // plan amulet-extras: more damage of one type
	CrushingMinor,
	CrushingNormal,
	CrushingGrand,
	CleavingLesser,
	CleavingMinor,
	CleavingNormal,
	CleavingGrand,
	PiercingLesser,
	PiercingMinor,
	PiercingNormal,
	PiercingGrand,
	ShotsLesser, // plan amulet-more-shots: a ranged weapon holds more shots
	ShotsMinor,
	ShotsNormal,
	ShotsGrand,
	QuickShotsLesser, // plan amulet-quick-shots: the shots come back faster
	QuickShotsMinor,
	QuickShotsNormal,
	QuickShotsGrand,
	// The rings (plan ring-accessories), one inventory row per gem: copper, silver, gold.
	RubyCopper, // the first ring
	RubySilver,
	RubyGold,
	SapphireCopper,
	SapphireSilver,
	SapphireGold,
	EmeraldCopper,
	EmeraldSilver,
	EmeraldGold,
	TopazCopper,
	TopazSilver,
	TopazGold,
	CrystalCopper,
	CrystalSilver,
	CrystalGold,
	AmethystCopper,
	AmethystSilver,
	AmethystGold,
};
constexpr int ITEM_KIND_COUNT = static_cast<int>(ItemKind::AmethystGold) + 1;
constexpr int WEAPON_KIND_COUNT = static_cast<int>(ItemKind::SmallHealth); // the first potion
constexpr int FIRST_AMULET = static_cast<int>(ItemKind::StrengthLesser);
constexpr int FIRST_RING = static_cast<int>(ItemKind::RubyCopper);
constexpr int POTION_KIND_COUNT = FIRST_AMULET - WEAPON_KIND_COUNT;
constexpr int AMULET_KIND_COUNT = FIRST_RING - FIRST_AMULET;
constexpr int RING_KIND_COUNT = ITEM_KIND_COUNT - FIRST_RING;
// The weapons the player holds, by itemIndex.
using OwnedWeapons = std::array<bool, WEAPON_KIND_COUNT>;

constexpr int itemIndex(ItemKind kind) { return static_cast<int>(kind); }
constexpr ItemKind itemAt(int index) { return static_cast<ItemKind>(index); } // 0 .. ITEM_KIND_COUNT - 1
// Saves from the Egyptian weapons on have their slots in ItemKind order: this many (the weapons and the potions up to
// the antidote), or more since the amulets.
constexpr int ORDERED_SAVE_SLOTS = itemIndex(ItemKind::Antidote) + 1;
constexpr bool isWeapon(ItemKind kind) { return itemIndex(kind) < WEAPON_KIND_COUNT; }
constexpr bool isPotion(ItemKind kind) {
	return itemIndex(kind) >= WEAPON_KIND_COUNT && itemIndex(kind) < FIRST_AMULET;
}
constexpr bool isAmulet(ItemKind kind) { return itemIndex(kind) >= FIRST_AMULET && itemIndex(kind) < FIRST_RING; }
constexpr bool isRing(ItemKind kind) { return itemIndex(kind) >= FIRST_RING; }
constexpr bool isRanged(ItemKind kind) { return itemIndex(kind) >= itemIndex(ItemKind::SelfBow) && isWeapon(kind); }

// What a ranged weapon shoots (Dungeon::Shoot): the bows an arrow, the sling a stone; the throwing stick and the
// javelin fly themselves. No ammunition: a few shots that come back (WeaponDef::shots).
enum class MissileKind : std::uint8_t { Arrow, Stone, Stick, Javelin };
constexpr int MISSILE_KIND_COUNT = 4;

// The inventory's tabs, one per group of items.
enum class ItemGroup : std::uint8_t { Weapons, Potions, Amulets, Rings };
constexpr int ITEM_GROUP_COUNT = 4;

constexpr ItemGroup itemGroup(ItemKind kind) {
	return isRing(kind)		? ItemGroup::Rings
		   : isAmulet(kind) ? ItemGroup::Amulets
		   : isPotion(kind) ? ItemGroup::Potions
							: ItemGroup::Weapons;
}

// A group's items are next to each other in ItemKind order: indexes first .. first + count - 1.
struct ItemRange {
	int first;
	int count;
};
[[nodiscard]] ItemRange groupItems(ItemGroup group);

// Level files (a treasure tile's attr and value), save games and scenario scripts name an item by a type and an id.
// The weapon ids restart per type: the club and the self-bow are both 0. A new weapon takes the next free id of its
// type, so old levels and saves keep reading.
namespace ItemType {
constexpr int EMPTY = 0; // an empty treasure chest
constexpr int MELEE_WEAPON = 1;
constexpr int RANGED_WEAPON = 2;
constexpr int POTION = 3;
constexpr int AMULET = 4; // id: the amulet's place among the amulets (0 = lesser strength)
constexpr int RING = 5; // id: the ring's place among the rings (0 = copper ruby)
} // namespace ItemType

struct ItemFileId {
	int type; // ItemType
	int id;
};

[[nodiscard]] ItemFileId fileIdOf(ItemKind kind);
// nullopt for an unknown pair (and for an empty chest).
[[nodiscard]] std::optional<ItemKind> itemFromFile(int type, int id);

// How a weapon is held and swung (drawWeapon). Tilts in degrees from upright, towards the facing side. An attack
// raises the weapon back to windupTilt, brings it down through strikeTilt, where the hit lands (hitMs), and returns
// it to restTilt by swingMs. The bow is drawn until hitMs instead and the arrow leaves. hitMs is the frame delay (the
// wind-up, the draw), recoveryMs the time from the hit until the next attack can begin; swingMs runs inside the
// recovery.
struct WeaponMotion {
	float grip = 0.1f; // the fist holds it this far up its length, from the lowest point
	float restTilt = 45.f, windupTilt = 45.f, strikeTilt = 45.f;
	float thrust = 0.f; // pushed forward this many lengths at the strike instead (the spear)
	int hitMs = 200, swingMs = 400;
	int recoveryMs = 800;
	[[nodiscard]] int AttackMs() const { return hitMs + recoveryMs; } // from one attack to the next
};

// Where the swing has the weapon (WeaponMotion): its tilt, its thrust in lengths and the bow's draw 0..1.
struct SwingPose {
	float tilt, thrust, draw;
};
// Of a swing's time to its hit: raising the weapon back, the rest bringing it down.
constexpr float SWING_WINDUP_SHARE = 0.6f;
constexpr float SWING_WINDUP_PULL = 0.3f; // a thrust draws back this share of its reach first
// elapsedMs since the attack began; < 0: no attack, at rest. Smooth into the windup, gathering speed down to the
// strike at hitMs, smooth back to rest by swingMs. back 0..1: a power hit's charge (plan skill-power-hit) draws the
// windup further back, up to SWING_CHARGE_BACK_DEG (a thrust: SWING_CHARGE_PULL more of its reach).
[[nodiscard]] SwingPose swingPose(const WeaponMotion& m, int elapsedMs, float back = 0.f);
constexpr float SWING_CHARGE_BACK_DEG = 35.f;
constexpr float SWING_CHARGE_PULL = 0.3f;
// While a power hit charges: the weapon rises into the windup over its windup time, then further back with the
// charge (back 0..1). heldMs since the key went down.
[[nodiscard]] SwingPose chargePose(const WeaponMotion& m, int heldMs, float back);
// Of a swing's hitMs, the windup: a charged swing starts there at the release.
[[nodiscard]] int windupMs(const WeaponMotion& m);

// What drinking a potion does to the player. Percentages of max health / max stamina; maxHpPercent grows max health,
// then heals fully.
struct PotionGain {
	int healPercent = 0;
	int staminaPercent = 0;
	int might = 0;
	int armor = 0;
	int maxHpPercent = 0;
	bool cure = false; // ends all poison
	int resistPercent = 0; // for PotionEffect::RESIST_MS, the chance a poisoned hit does not poison
	bool waterBlessing = false; // for PotionEffect::WATER_BLESSING_MS (twice with the Favour of Hapi skill)
};

enum class FieldNote : unsigned char; // journal.h

struct ItemText {
	const char* name;
	const char* shortName; // fits an inventory slot's name band
	const char* label; // plain word for the editor ("small health", "might")
	const char* effect; // potions and amulets
	const char* lore1 = nullptr; // the card's two lore lines: content/texts/items.txt, filled by itemText
	const char* lore2 = nullptr;
};
[[nodiscard]] const ItemText& itemText(ItemKind kind);

struct WeaponDef {
	const char* model = ""; // media/models/items/<model>.md3, media/textures/items/<model>.png
	float scale = 1.f;
	int damage = 0; // at weapon level 1; the loot grades go by it (loot.cpp), unless gradeDamage is set
	int range = 0; // tenths of a tile (a ranged weapon's: how far it aims)
	// Ranged weapons (plan ranged-weaker-than-melee): the shots it holds (ItemBag::ShotsLeft), and no shot at a
	// monster closer than minRange, tenths of a tile measured as range is (Dungeon::ShotRefused).
	int shots = 0;
	int minRange = 0;
	// The loot grades sort by this instead of damage (0: by damage): a ranged weapon keeps the grade of its damage
	// before it was made weaker than its melee twin, so chests keep their order.
	int gradeDamage = 0;
	DamageMix mix{}; // blunt, slash, pierce, fire percent. Its main type (mainType) is the journal's.
	// Each weapon level adds this share of the base damage: the club grows most, the sword least (it starts
	// strongest).
	int growthPercent = 20;
	WeaponMotion motion{};
	const char* swingSound = ""; // media/sounds/items/<name>.wav: the attack begins
	const char* strikeSound = ""; // a melee hit lands, the shot leaves
	MissileKind missile = MissileKind::Arrow; // ranged weapons
	bool thrown = false; // the weapon itself leaves the hand (Dungeon::Shoot): the hand is empty until the swing ends
	int generatedFrom = 0; // levelgen's weapon chests hold it from this difficulty on (0: from the start)
};

// The vessels the potions come in (tools/blender/models/items.py). Several potions share one, each with its own
// texture: look-alikes differ by the colour of the liquid.
enum class PotionModel : std::uint8_t { Flask, Lotus, Pilgrim, Canopic, Cobra, Ankh };
constexpr int POTION_MODEL_COUNT = 6;

struct PotionModelDef {
	const char* model; // media/models/items/<model>.md3
	float scale; // on a chest (Item::scale), as is: no CHEST_MIN_SCALE for a potion
	float detailScale; // the inventory's detail view
};
[[nodiscard]] const PotionModelDef& potionModelDef(PotionModel model);

struct PotionDef {
	PotionGain gain{};
	PotionModel model{};
	const char* texture = ""; // media/textures/items/<texture>.png, on its model's UVs
	Rgb colour{}; // the icons' tint: the inventory's potions tab, the HUD quick slot
	FieldNote note{}; // the journal's, on the first one found
	int generatedWeight = 0; // levelgen's potion chests, in percent
	bool mimicLoot = true; // a mimic can leave it (not the antidote: it lies only on the levels with poisoners)
};

// The weapons and the potions, one row each; an amulet's row is its type's (AmuletTypeDef in items.cpp).
struct ItemDef {
	ItemFileId file{};
	ItemText text{};
	WeaponDef weapon{}; // weapons only
	PotionDef potion{}; // potions only
};
[[nodiscard]] const WeaponDef& weaponDef(ItemKind weapon);
[[nodiscard]] const PotionDef& potionDef(ItemKind potion);
[[nodiscard]] inline const DamageMix& weaponMix(ItemKind weapon) { return weaponDef(weapon).mix; }
// Tiles: the melee reach from the player's box edge, or how far the ranged weapon aims (WeaponDef::range).
[[nodiscard]] inline float weaponReach(ItemKind weapon) { return 0.1f * static_cast<float>(weaponDef(weapon).range); }
// With the Sharp eye skill a ranged weapon aims SHARP_EYE_RANGE_PERCENT % as far (plan skill-sharp-eye); melee reach is
// not changed.
constexpr int SHARP_EYE_RANGE_PERCENT = 130;
[[nodiscard]] inline float weaponReach(ItemKind weapon, const Heka& heka) {
	const float reach = weaponReach(weapon);
	return isRanged(weapon) && heka.Has(SkillId::SharpEye) ? reach * static_cast<float>(SHARP_EYE_RANGE_PERCENT) / 100.f
														   : reach;
}
// A ranged weapon deals at most this share of its melee twin's damage per second (plan ranged-weaker-than-melee; the
// rule at ITEMS, items.cpp).
constexpr int RANGED_TWIN_DPS_PERCENT = 70;
// Tiles: a ranged weapon's minimum range (WeaponDef::minRange), 0 for melee.
[[nodiscard]] inline float weaponMinRange(ItemKind weapon) {
	return 0.1f * static_cast<float>(weaponDef(weapon).minRange);
}
[[nodiscard]] inline MissileKind missileOf(ItemKind weapon) { return weaponDef(weapon).missile; }
[[nodiscard]] inline bool isThrown(ItemKind weapon) { return isWeapon(weapon) && weaponDef(weapon).thrown; }
[[nodiscard]] inline PotionGain potionGain(ItemKind potion) {
	return isPotion(potion) ? potionDef(potion).gain : PotionGain{};
}

// Potion strengths, in percent of the player's max health or max stamina.
namespace PotionEffect {
constexpr int SMALL_HEAL_PERCENT = 25;
constexpr int LARGE_HEAL_PERCENT = 50;
constexpr int SMALL_STAMINA_PERCENT = 50;
constexpr int LARGE_STAMINA_PERCENT = 100;
constexpr int MIGHT = 1; // might potion, for good
constexpr int ARMOR = 1; // armour potion, for good
constexpr int LIFE_MAX_HP_PERCENT = 5; // elixir of life: more max health, then a full heal
constexpr int LESSER_RESIST_PERCENT = 50; // resistance potions: added to the amulet's ward, up to 100%
constexpr int GREATER_RESIST_PERCENT = 95;
constexpr int RESIST_MS = 120000;
constexpr int WATER_BLESSING_MS = 120000; // the Blessing of Hapi; twice as long with the Favour of Hapi skill
} // namespace PotionEffect

// One amulet is worn at a time; it gives its bonus while it is on. All tiers of a type share the model.
enum class AmuletType : std::uint8_t {
	Strength,
	Armor,
	Health,
	PoisonWard,
	TrapWard,
	BluntWard,
	SlashWard,
	PierceWard,
	Regeneration,
	Venom, // the player's hits poison
	FireWard, // plan fire-damage
	Crushing, // plan amulet-extras: the matching share of the weapon's damage grows (blunt)
	Cleaving, // slash
	Piercing, // pierce
	Shots, // plan amulet-more-shots: more shots in every ranged weapon
	QuickShots, // plan amulet-quick-shots: the shots come back faster
};
constexpr int AMULET_TYPE_COUNT = 16;
enum class AmuletTier : std::uint8_t { Lesser, Minor, Normal, Grand };
constexpr int AMULET_TIER_COUNT = 4;

struct Amulet {
	AmuletType type;
	AmuletTier tier;
};
[[nodiscard]] Amulet amuletOf(ItemKind amulet);
// nullopt for a tier the type does not have (lesser and minor regeneration).
[[nodiscard]] std::optional<ItemKind> amuletKind(AmuletType type, AmuletTier tier);
// The model all tiers of a type share: media/models/items/<name>.md3 (tools/blender/models/amulets.py).
[[nodiscard]] const char* amuletModel(AmuletType type);

// What the worn amulet gives, all zero without one.
struct AmuletBonus {
	int might = 0; // added to every hit
	int armor = 0; // taken off every hit, like the armour
	int maxHpPercent = 0; // more max HP, a share of the base max HP
	int poisonResistPercent = 0; // chance that a poisoned hit does not poison
	int trapCutPercent = 0; // less spike, death trap and chest dart damage, chest traps go off less; 100: immune
	int regenHpPerSecond = 0; // while no monster chases the player and no poison runs
	int venomPercent = 0; // chance that a weapon hit poisons the monster (Monster::TakePoison)
	PoisonTier venomTier = PoisonTier::Weak;
	// Percent more damage of each type in a weapon hit (plan amulet-extras): only the type's share of the weapon's
	// damage mix grows (a sword, 85% slash, with +50% Cleaving: +42%).
	std::array<int, DAMAGE_TYPE_COUNT> typeMightPercent{};
	int shotsPercent = 0; // more shots in every ranged weapon (PlayerStats::Shots)
	int shotRecoveryPercent = 0; // the shots come back this much faster (PlayerStats::ShotRecoveryPace)
	Resistances resist = NO_RESISTANCES;
};
[[nodiscard]] AmuletBonus amuletBonus(std::optional<ItemKind> worn);

// The rings (plan ring-accessories): two worn at once, one per hand. The gem says what a ring does, the metal how much.
// All metals of a gem share the band and gem models (ringBandModel, ringGemModel), the metal and the gem as texture and
// tint.
enum class RingGem : std::uint8_t {
	Ruby, // heals slowly, also in a fight
	Sapphire, // stamina comes back faster
	Emerald, // poison wears off faster
	Topaz, // walk and sprint faster (not the jump)
	Crystal, // rock crystal: longer weapon reach
	Amethyst, // cuts through the monsters' resistances
};
constexpr int RING_GEM_COUNT = 6;
enum class RingMetal : std::uint8_t { Copper, Silver, Gold };
constexpr int RING_METAL_COUNT = 3;

struct Ring {
	RingGem gem;
	RingMetal metal;
};
[[nodiscard]] constexpr Ring ringOf(ItemKind ring) {
	const int i = itemIndex(ring) - FIRST_RING;
	return {static_cast<RingGem>(i / RING_METAL_COUNT), static_cast<RingMetal>(i % RING_METAL_COUNT)};
}
[[nodiscard]] constexpr ItemKind ringKind(RingGem gem, RingMetal metal) {
	return itemAt(FIRST_RING + static_cast<int>(gem) * RING_METAL_COUNT + static_cast<int>(metal));
}
// The gem's colour (the gem model's tint, the HUD's dot) and its see-through share (the gem's alpha).
[[nodiscard]] Rgb ringGemColour(RingGem gem);
[[nodiscard]] const char* ringGemSlug(RingGem gem); // "ruby": its lore and field note keys
constexpr float RING_GEM_ALPHA = 0.62f;
// The models all rings share: media/models/items/<name>.md3; the band's texture per metal, the gem's one texture.
constexpr const char* RING_BAND_MODEL = "ring_band";
constexpr const char* RING_GEM_MODEL = "ring_gem";
[[nodiscard]] const char* ringBandTexture(RingMetal metal); // media/textures/items/<name>.png

// The worn rings, left and right hand.
enum class Hand : std::uint8_t { Left, Right };
constexpr int HAND_COUNT = 2;
using WornRings = std::array<std::optional<ItemKind>, HAND_COUNT>;

// One ring's numbers, by metal (copper, silver, gold). Both worn rings always count, two of the same too; the sums
// stop at the caps below.
namespace RingEffect {
constexpr std::array<int, RING_METAL_COUNT> RUBY_MS_PER_HP = {8000, 5000, 3000};
constexpr std::array<int, RING_METAL_COUNT> SAPPHIRE_PERCENT = {15, 25, 40}; // faster stamina regeneration
constexpr std::array<int, RING_METAL_COUNT> EMERALD_PERCENT = {20, 35, 50}; // poison's time runs this much faster
constexpr std::array<int, RING_METAL_COUNT> TOPAZ_PERCENT = {5, 8, 12}; // walk, sprint and climb speed
constexpr std::array<int, RING_METAL_COUNT> CRYSTAL_PERCENT = {10, 15, 25}; // weapon reach
constexpr std::array<int, RING_METAL_COUNT> AMETHYST_PERCENT = {25, 50, 100}; // of a resistance's way to NORMAL
constexpr int TOPAZ_CAP = 20;
constexpr int CRYSTAL_CAP = 40;
constexpr int AMETHYST_CAP = 100;
constexpr int MS_PER_HOUR = 3600000;
} // namespace RingEffect

// What the worn rings give together, all zero without one.
struct RingBonus {
	int regenHpPerHour = 0; // the rubies: HP an hour, also with monsters about; not while poisoned
	int staminaRegenPercent = 0; // more stamina regeneration (PlayerStats::RegenerateStamina)
	int poisonFasterPercent = 0; // the poison's time left runs down this much faster (Poison::Advance)
	int speedPercent = 0; // faster walk, sprint and climb on the ground; not the jump
	int reachPercent = 0; // more weapon reach, melee and ranged
	int resistCutPercent = 0; // a monster's resistance moves this share of the way to NORMAL (cutResistances)
};
[[nodiscard]] RingBonus ringBonus(const WornRings& worn);
// What the worn rings give together in the gem's effect, for the ring card: "heals 1 health every 1.5 s", "walk and
// sprint 13% faster"; empty when no ring of the gem is worn.
[[nodiscard]] std::string ringSumText(const RingBonus& bonus, RingGem gem);
#endif
