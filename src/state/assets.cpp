#include "assets.h"
#include "../world/monster_kinds.h"
#include <cstdio>
#include <string_view>
#include "../core/logger.h"
#include "../core/gameplay_config.h"

namespace {
struct MonsterDef { // NOLINT(clang-analyzer-optin.performance.Padding): a small table, ordered to read
	MonsterTypeId id;
	const char* label;
	const char* model;	 // under models/ and sounds/
	const char* texture; // under textures/: the giant rat, bat and scarab are the same model, bigger and darker
	float speed;
	int maxHealth, damage, attackMs, xp;
	float scale, rotA;
	Locomotion locomotion;
	Rgb blood;
	Courage courage = Courage::Coward;
	int trapDamagePct = 100;
};

constexpr Rgb RED_BLOOD = {0.7f, 0.1f, 0.1f};
constexpr Rgb SCARAB_BLOOD = {0.6f, 0.1f, 0.8f};

// Small ones are quick and bite often but barely hurt; big ones are slow, hit hard and take long to kill.
// Flyers roost on the ceiling and swoop through the player (Monster::Fly).
const MonsterDef MONSTER_DEFS[] = {
	{MonsterWorm, "Worm", "monsters/worm", "monsters/worm", 1, 30, 9, 1000, 1200, 18, 0, Locomotion::Walk, RED_BLOOD},
	{MonsterScarab, "Scarab", "monsters/scarab", "monsters/scarab", 4, 10, 2, 600, 300, 7, 180, Locomotion::Walk,
	 SCARAB_BLOOD},
	{MonsterGiantScarab,
	 "Giant scarab",
	 "monsters/scarab",
	 "monsters/scarab_giant",
	 2,
	 90,
	 12,
	 1000,
	 2500,
	 24,
	 180,
	 Locomotion::WalkJump,
	 {0.4f, 0.05f, 0.55f}},
	// Bigger, faster and harder than the giant scarab; see BOSS_DEFS. A full clear of levels 1-4 makes the player
	// level 8 (134 HP): 3-4 bites kill them.
	{MonsterBossScarab,
	 "Boss scarab",
	 "monsters/scarab",
	 "monsters/scarab_boss",
	 3,
	 320,
	 40,
	 900,
	 6000,
	 34,
	 180,
	 Locomotion::WalkJump,
	 {0.1f, 0.2f, 0.75f}},
	// Reckless like the Anubis boss; its trap share is between the boss (10%) and the mummy (50%). Strong enough for
	// the late levels (13-30): a level 55 player (about 700 HP, 11 armour) takes about 15 blows.
	{MonsterAnubis, "Anubis", "monsters/anubis", "monsters/anubis", 3, 600, 55, 1100, 10000, 19, 180, Locomotion::Walk,
	 RED_BLOOD, Courage::Reckless, 25},
	{MonsterPlant,
	 "Man-eater plant",
	 "monsters/plant",
	 "monsters/plant",
	 0,
	 30,
	 5,
	 800,
	 1000,
	 12,
	 0,
	 Locomotion::Stationary,
	 {0.1f, 0.4f, 0.1f}},
	{MonsterRat, "Rat", "monsters/rat", "monsters/rat", 9, 12, 2, 400, 300, 13, 180, Locomotion::Walk, RED_BLOOD},
	// Faster than the player's walk (~1.25 tiles/s): the bow alone does not keep it off
	// (docs/plan/solved/giant-rat-speed.md).
	{MonsterGiantRat, "Giant rat", "monsters/rat", "monsters/rat_giant", 24, 60, 8, 700, 2000, 42, 180,
	 Locomotion::WalkJump, RED_BLOOD},
	{MonsterBat, "Bat", "monsters/bat", "monsters/bat", 5, 8, 3, 800, 400, 18, 180, Locomotion::Fly, RED_BLOOD},
	{MonsterGiantBat,
	 "Giant bat",
	 "monsters/bat",
	 "monsters/bat_giant",
	 4,
	 40,
	 10,
	 800,
	 1800,
	 30,
	 180,
	 Locomotion::Fly,
	 {0.45f, 0.05f, 0.05f}},
	// A giant bat grown fat on blood; see BOSS_DEFS. The player comes to lvl10 at about level 21 (290 HP): 3-4 bites
	// kill them.
	{MonsterVampireBat,
	 "Vampire bat",
	 "monsters/bat",
	 "monsters/bat_vampire",
	 4,
	 400,
	 80,
	 800,
	 12000,
	 42,
	 180,
	 Locomotion::Fly,
	 {0.5f, 0.02f, 0.08f}},
	// Fast for its bulk, hits hard and slowly. Levels 11 on, the Anubis boss's minion (docs/plan/solved/boss-rooms.md).
	{MonsterMummy,
	 "Mummy",
	 "monsters/mummy",
	 "monsters/mummy",
	 2.5f,
	 150,
	 20,
	 1600,
	 2500,
	 18,
	 180,
	 Locomotion::Entombed,
	 {0.35f, 0.25f, 0.12f},
	 Courage::Reckless, // a crushing rock (500) can kill it
	 50},
	// The finale's guardian, a head taller than an Anubis, the strongest boss; see BOSS_DEFS. The player comes to lvl30
	// at about level 55 (about 700 HP, 11 armour): 6 blows kill them.
	// Reckless: the player cannot shake him off behind a row of traps.
	{MonsterAnubisBoss, "Anubis boss", "monsters/anubis", "monsters/anubis_boss", 4.5f, 2400, 140, 1300, 30000, 26, 180,
	 Locomotion::Walk, RED_BLOOD, Courage::Reckless, 10},
	// HP between the giant rat and the mummy, bites harder than both. Slow on land, fast in the water (WADING_DEFS).
	// Levels 7-9 (docs/plan/solved/crocodiles-and-flooded-cells.md). A long, low body: 1.5 tiles nose to tail.
	{MonsterCrocodile, "Crocodile", "monsters/crocodile", "monsters/crocodile", 7, 110, 26, 1100, 2400, 60, 180,
	 Locomotion::Submerged, RED_BLOOD},
	// Small and quick like the rat, a slower sting that poisons (POISON_DEFS); levels from the scorpion queen's
	// (docs/plan/solved/poison-and-antidote.md).
	{MonsterScorpion,
	 "Scorpion",
	 "monsters/scorpion",
	 "monsters/scorpion",
	 10,
	 14,
	 3,
	 900,
	 450,
	 15,
	 180,
	 Locomotion::Walk,
	 {0.45f, 0.62f, 0.55f}},
	// Between the giant rat and the crocodile, a poisoned bite and a venom spit from afar (SPIT_DEFS); lies coiled
	// until the player comes near (docs/plan/cobra.md).
	{MonsterCobra, "Cobra", "monsters/cobra", "monsters/cobra", 6, 35, 6, 1100, 1200, 24, 180, Locomotion::Coiled,
	 RED_BLOOD},
	// The cobra's giant kin, levels after Apep (docs/plan/giant-cobra.md).
	{MonsterGiantCobra, "Giant cobra", "monsters/cobra", "monsters/cobra_giant", 8, 160, 26, 1300, 2600, 36, 180,
	 Locomotion::Coiled, RED_BLOOD},
	// The scorpion's giant kin and the scorpion queen's minion: medium poison (docs/plan/scorpion-queen-boss.md).
	{MonsterGiantScorpion,
	 "Giant scorpion",
	 "monsters/scorpion",
	 "monsters/scorpion_giant",
	 9,
	 70,
	 12,
	 1000,
	 2000,
	 26,
	 180,
	 Locomotion::Walk,
	 {0.45f, 0.62f, 0.55f}},
	// Rooted and harmless: the scorpion queen's brood hatches from it (Summon::Hatch). Dies to a few blows.
	{MonsterEggCluster,
	 "Egg cluster",
	 "monsters/egg_cluster",
	 "monsters/egg_cluster",
	 0,
	 60,
	 0,
	 1000,
	 300,
	 16,
	 0,
	 Locomotion::Stationary,
	 {0.85f, 0.75f, 0.35f}},
	// The lvl15 boss on the scorpion model, the size of a cart; see BOSS_DEFS (docs/plan/scorpion-queen-boss.md).
	{MonsterScorpionQueen,
	 "Scorpion queen",
	 "monsters/scorpion",
	 "monsters/scorpion_queen",
	 5,
	 700,
	 45,
	 1100,
	 15000,
	 48,
	 180,
	 Locomotion::Walk,
	 {0.5f, 0.7f, 0.6f},
	 Courage::Reckless,
	 10},
	// The lvl20 boss on the cobra model, long as the hall; see BOSS_DEFS (docs/plan/apep-serpent-boss.md).
	{MonsterApep,
	 "Apep",
	 "monsters/cobra",
	 "monsters/cobra_apep",
	 6,
	 1100,
	 60,
	 1200,
	 20000,
	 70,
	 180,
	 Locomotion::Burrow,
	 {0.35f, 0.05f, 0.1f},
	 Courage::Reckless,
	 10},
	// The lvl25 boss on the crocodile model; see BOSS_DEFS (docs/plan/sobek-boss.md).
	{MonsterSobek, "Sobek", "monsters/crocodile", "monsters/crocodile_sobek", 6, 1600, 70, 1300, 25000, 100, 180,
	 Locomotion::Submerged, RED_BLOOD, Courage::Reckless, 10},
	// Scale and yaw of the treasure chest item: idle, it looks just like one.
	{MonsterMimic,
	 "Mimic",
	 "monsters/mimic",
	 "monsters/mimic",
	 0,
	 40,
	 10,
	 800,
	 1500,
	 8,
	 0,
	 Locomotion::Ambush,
	 {0.5f, 0.05f, 0.1f}},
};

// Minions per boss: type, alive on arrival, alive at most, ms between summons, summons per fight, life steal %, how
// they come. Starting values, to tune from playthroughs (docs/plan/solved/boss-rooms.md).
const struct {
	MonsterTypeId id;
	BossRules rules;
} BOSS_DEFS[] = {
	{MonsterBossScarab, {MonsterScarab, 3, 5, 1500, 12, 0, Summon::DigOut}},
	{MonsterVampireBat, {MonsterBat, 2, 4, 2000, 8, 30, Summon::Drop}},
	{MonsterAnubisBoss, {MonsterMummy, 2, 4, 2000, 10, 0, Summon::Coffin}},
	{MonsterScorpionQueen, {MonsterGiantScorpion, 2, 4, 3000, 10, 0, Summon::Hatch}},
	{MonsterApep, {MonsterCobra, 2, 4, 3000, 10, 0, Summon::DigOut}},
	{MonsterSobek, {MonsterCrocodile, 1, 3, 5000, 6, 0, Summon::DigOut}},
};

// How each monster type takes blunt, slash and pierce damage (docs/plan/solved/damage-types-and-resistances.md); a
// type not listed takes all of it normally. Each weapon is the best against some: the club against bats, mimics and
// the Anubis guard, the sword against worms, plants and mummies, the spear (and the bow) against scarabs and the
// Anubis boss.
const struct {
	MonsterTypeId id;
	Resistances resist;
} RESISTANCE_DEFS[] = {
	{MonsterWorm, {RESISTS, WEAK, NORMAL}},			 // soft: a blow squashes, a blade cuts
	{MonsterScarab, {NORMAL, RESISTS, WEAK}},		 // the shell turns a blade, a point goes between the plates
	{MonsterGiantScarab, {NORMAL, RESISTS, WEAK}},	 //
	{MonsterBossScarab, {NORMAL, RESISTS, WEAK}},	 //
	{MonsterPlant, {TOUGH, WEAK, TOUGH}},			 // stems: only a blade cuts them; points and blows go astray
	{MonsterBat, {WEAK, RESISTS, TOUGH}},			 // swat it; an arrow goes through the wing
	{MonsterGiantBat, {WEAK, RESISTS, TOUGH}},		 //
	{MonsterVampireBat, {WEAK, RESISTS, TOUGH}},	 //
	{MonsterMimic, {WEAK, RESISTS, TOUGH}},			 // wood: crack it; a point only sticks in it
	{MonsterAnubis, {WEAK, RESISTS, NORMAL}},		 // bronze armour dents, a blade glances off it
	{MonsterAnubisBoss, {NORMAL, RESISTS, WEAK}},	 // armoured too well to dent, but open at the joints
	{MonsterMummy, {RESISTS, WEAK, TOUGH}},			 // dry linen tears; nothing inside to stab
	{MonsterCrocodile, {NORMAL, RESISTS, NORMAL}},	 // the scutes turn a blade
	{MonsterScorpion, {WEAK, NORMAL, RESISTS}},		 // a blow cracks the thin shell; a point glances off the plates
	{MonsterCobra, {RESISTS, WEAK, NORMAL}},		 // the coils give under a blow; a blade cuts the thin body
	{MonsterGiantCobra, {RESISTS, WEAK, NORMAL}},	 //
	{MonsterGiantScorpion, {WEAK, NORMAL, RESISTS}}, //
	{MonsterScorpionQueen, {WEAK, NORMAL, RESISTS}}, // the club cracks her shell
	{MonsterEggCluster, {NORMAL, WEAK, RESISTS}},	 // a blade slits the leathery eggs
	{MonsterApep, {RESISTS, WEAK, NORMAL}},			 // as the cobras
	{MonsterSobek, {NORMAL, RESISTS, NORMAL}},		 // as the crocodiles
};

// What each monster group's bite or blow deals (docs/plan/solved/monster-attack-damage-types.md), read from what the
// model attacks with. Keyed by the model: the members of a group (bigger or darker) deal the same kinds, only more. A
// new group needs its row.
const struct {
	const char* model;
	DamageMix mix;
} ATTACK_MIX_DEFS[] = {
	{"monsters/worm", {60, 40, 0}},		   // grinding maw
	{"monsters/scarab", {40, 60, 0}},	   // rams, mandibles
	{"monsters/plant", {0, 40, 60}},	   // bite, thorny vines
	{"monsters/rat", {0, 30, 70}},		   // teeth
	{"monsters/bat", {0, 20, 80}},		   // fangs, claws
	{"monsters/mummy", {100, 0, 0}},	   // fists
	{"monsters/anubis", {80, 0, 20}},	   // was-sceptre, its forked foot
	{"monsters/crocodile", {50, 0, 50}},   // crushing jaws
	{"monsters/scorpion", {0, 30, 70}},	   // claws, sting
	{"monsters/mimic", {40, 0, 60}},	   // lid slam, teeth
	{"monsters/cobra", {0, 0, 100}},	   // fangs
	{"monsters/egg_cluster", {100, 0, 0}}, // never bites
};

// How the walkers move through half water, and their speed in it times their speed on land; the others wade slowed
// (Wading::Slowed, WADE_SPEED_FACTOR). Flyers and rooted monsters do not wade.
const struct {
	MonsterTypeId id;
	Wading wading;
	float waterSpeed;
} WADING_DEFS[] = {
	{MonsterRat, Wading::Unaffected, 1.f},
	{MonsterGiantRat, Wading::Unaffected, 1.f},
	{MonsterAnubis, Wading::Unaffected, 1.f},
	{MonsterAnubisBoss, Wading::Unaffected, 1.f},
	// Slow on land (slower than a rat: the player outwalks it), in the water faster than the player walks on land.
	{MonsterCrocodile, Wading::Swimmer, 2.5f},
	{MonsterCobra, Wading::Swimmer, 1.25f},
	{MonsterGiantCobra, Wading::Swimmer, 1.25f},
	{MonsterApep, Wading::Swimmer, 1.25f},
	{MonsterSobek, Wading::Swimmer, 2.5f},
};

// The monsters whose bite or sting poisons the player, and the tier (docs/plan/solved/poison-and-antidote.md).
const struct {
	MonsterTypeId id;
	PoisonTier tier;
} POISON_DEFS[] = {
	{MonsterScorpion, PoisonTier::Weak},		{MonsterCobra, PoisonTier::Medium},
	{MonsterGiantCobra, PoisonTier::Medium},	{MonsterGiantScorpion, PoisonTier::Medium},
	{MonsterScorpionQueen, PoisonTier::Strong},
};

// The monsters that spit venom at the player from afar (Monster::Spit, Dungeon::Venom); the release and the mouth
// height come from the model's spit clip (tools/blender/models/cobra.py).
const struct {
	MonsterTypeId id;
	SpitRules rules;
} SPIT_DEFS[] = {
	{MonsterCobra, {2, PoisonTier::Medium, 2.5f, 3500, 0.41f, 0.87f}}, // release: frame 7 of 18
	{MonsterGiantCobra, {5, PoisonTier::Medium, 3.f, 3000, 0.41f, 0.87f}},
};

struct WeaponDef {
	const char* name; // models/items/<name>.md3, textures/items/<name>.png
	float scale;
	int damage, range; // range in tenths of a tile (a ranged weapon's: how far it aims)
	DamageMix mix;
	WeaponMotion motion;
	const char* swingSound;	 // sounds/items/<name>.wav: the attack begins
	const char* strikeSound; // a melee hit lands, the shot leaves
};

// In ItemKind order. Motion: grip, rest / windup / strike tilt, thrust, hit (frame delay) / swing / recovery ms. The
// club is slow and heavy, the short sword quick, the spear thrusts. Mix: blunt, slash, pierce percent.
const WeaponDef WEAPON_DEFS[] = {
	{"club", 6, 10, 2, {85, 15, 0}, {0.12f, 35, -40, 115, 0, 300, 560, 600}, "club_swing", "club_hit"},
	{"dagger", 4, 8, 1, {0, 30, 70}, {0.15f, 45, 15, 100, 0.25f, 150, 320, 250}, "sword_swing", "spear_hit"},
	{"sword", 9, 35, 3, {0, 85, 15}, {0.1f, 40, -10, 120, 0, 180, 360, 370}, "sword_swing", "sword_hit"},
	// The khopesh flows from swing to swing; the axes and the mace wind up long.
	{"khopesh", 8, 45, 3, {0, 100, 0}, {0.12f, 40, -30, 125, 0, 300, 560, 400}, "sword_swing", "sword_hit"},
	{"epsilon_axe", 8, 55, 3, {30, 70, 0}, {0.1f, 35, -45, 120, 0, 550, 820, 400}, "club_swing", "axe_hit"},
	{"duckbill_axe", 7, 50, 3, {20, 0, 80}, {0.1f, 35, -45, 115, 0, 500, 770, 400}, "club_swing", "axe_hit"},
	{"mace", 7, 50, 2, {100, 0, 0}, {0.1f, 35, -50, 115, 0, 600, 870, 400}, "club_swing", "mace_hit"},
	{"spear", 15, 20, 5, {0, 15, 85}, {0.35f, 70, 70, 70, 0.3f, 200, 420, 550}, "spear_swing", "spear_hit"},
	{"bow",
	 12,
	 12,
	 30,
	 {0, 0, 100},
	 {0.5f, 0, 0, 0, 0, BOW_DRAW_MS, BOW_DRAW_MS + 100, 550},
	 "bow_draw",
	 "bow_release"},
	{"composite_bow", 11, 22, 40, {0, 0, 100}, {0.5f, 0, 0, 0, 0, 650, 750, 650}, "bow_draw", "bow_release"},
	// Whirled overhead from hanging down, let go in front.
	{"sling", 5, 10, 20, {100, 0, 0}, {0.05f, 160, -150, 45, 0, 350, 600, 450}, "sling_swing", "sling_release"},
	{"throwing_stick", 5, 14, 12, {90, 10, 0}, {0.08f, 40, -60, 100, 0, 300, 500, 400}, "club_swing", "throw"},
	{"javelin", 11, 30, 15, {0, 10, 90}, {0.45f, 60, 20, 80, 0, 500, 700, 700}, "spear_swing", "throw"},
};
static_assert(std::size(WEAPON_DEFS) == WEAPON_KIND_COUNT, "one WEAPON_DEFS row per weapon, in ItemKind order");

// Static tile-unit model like the props: no Centrify, textured only. Null if the file is missing.
std::unique_ptr<AnimatedModel> loadStaticModel(const char* path, Texture& tex) {
	auto model = std::make_unique<AnimatedModel>();
	if (!model->Load(path))
		return nullptr;
	model->BindTexture(tex.ID());
	model->Compile();
	return model;
}

void loadMechanisms(MechanismSet& set) {
	char path[96];
	for (int c = 0; c < LOCK_COLOUR_COUNT; c++) {
		snprintf(path, sizeof(path), "textures/mechanisms/key_%s.png", LOCK_COLOURS[c].name);
		set.keyTex[c].LoadPNG(path);
		snprintf(path, sizeof(path), "textures/mechanisms/gate_%s.png", LOCK_COLOURS[c].name);
		set.gateTex[c].LoadPNG(path);
		snprintf(path, sizeof(path), "textures/mechanisms/lever_base_%s.png", LOCK_COLOURS[c].name);
		set.leverBaseTex[c].LoadPNG(path);
		set.key[c] = loadStaticModel("models/mechanisms/key.md3", set.keyTex[c]);
		set.gate[c] = loadStaticModel("models/mechanisms/gate.md3", set.gateTex[c]);
		set.leverBase[c] = loadStaticModel("models/mechanisms/lever_base.md3", set.leverBaseTex[c]);
	}
	set.bossGateTex.LoadPNG("textures/mechanisms/gate_boss.png");
	set.bossGate = loadStaticModel("models/mechanisms/gate.md3", set.bossGateTex);
	set.leverHandleTex.LoadPNG("textures/mechanisms/lever_handle.png");
	set.rockTex.LoadPNG("textures/mechanisms/rock.png");
	set.crackTex.LoadPNG("textures/mechanisms/ceiling_crack.png");
	set.leverHandle = loadStaticModel("models/mechanisms/lever_handle.md3", set.leverHandleTex);
	set.rock = loadStaticModel("models/mechanisms/rock.md3", set.rockTex);
	set.crack = loadStaticModel("models/mechanisms/ceiling_crack.md3", set.crackTex);
}
} // namespace

Item* ItemPrototypes::Of(ItemKind kind) const {
	return isPotion(kind) ? potion.get() : weapons[static_cast<size_t>(itemIndex(kind))].get();
}

void Assets::LoadLoadingScreen() {
	fonts.loading.Load("fonts/papyrus.png", 7, -1.0);
	textures.loadingBackground.LoadPNG("textures/ui/scarab_slate.png", TexFilter::Flat);
	textures.loadingBar.LoadPNG("textures/ui/loading.png", TexFilter::Flat);
}

namespace {
using Progress = std::function<void(float, const char*)>;

// The share [from, to) of the loading bar a table fills, one equal step per entry, so it never overruns
struct BarSpan {
	float from;
	float to;
	float at(size_t i, size_t count) const {
		return from + (to - from) * static_cast<float>(i) / static_cast<float>(count);
	}
};

void loadMonsterTypes(std::array<MonsterType, MONSTER_TYPE_MAX + 1>& monsterTypes, const Progress& progress,
					  BarSpan span) {
	for (size_t i = 0; i < std::size(MONSTER_DEFS); i++) {
		const MonsterDef& def = MONSTER_DEFS[i];
		char label[64];
		snprintf(label, sizeof(label), "Loading Monster Models [%s]", def.label);
		progress(span.at(i, std::size(MONSTER_DEFS)), label);
		char texture[64];
		snprintf(texture, sizeof(texture), "textures/%s.png", def.texture);
		Texture tex;
		tex.LoadPNG(texture);
		MonsterType& type = monsterTypes[def.id];
		const ClipFiles& clips = def.locomotion == Locomotion::Ambush	  ? AMBUSH_CLIPS
								 : def.locomotion == Locomotion::Entombed ? ENTOMBED_CLIPS
								 : def.locomotion == Locomotion::Coiled || def.locomotion == Locomotion::Burrow
									 ? COILED_CLIPS
									 : MONSTER_CLIPS;
		type.model.Load(def.model, std::move(tex), clips);
		type.speed = def.speed;
		type.maxHealth = def.maxHealth;
		type.damage = def.damage;
		type.attackMs = def.attackMs;
		type.xp = def.xp;
		type.scale = def.scale;
		type.rotA = def.rotA;
		type.locomotion = def.locomotion;
		type.blood = def.blood;
		type.courage = def.courage;
		type.trapDamagePct = def.trapDamagePct;
		type.name = def.label;
		type.id = def.id;
		type.texture = def.texture;
		for (const auto& mix : ATTACK_MIX_DEFS)
			if (std::string_view(mix.model) == def.model)
				type.attackMix = mix.mix;
		if (type.attackMix[0] + type.attackMix[1] + type.attackMix[2] != 100)
			LOG_ERRORF("assets", "Monster type %d: no attack mix for %s in ATTACK_MIX_DEFS", static_cast<int>(def.id),
					   def.model);
	}
	for (const auto& def : BOSS_DEFS)
		monsterTypes[def.id].boss = def.rules;
	for (const auto& def : RESISTANCE_DEFS)
		monsterTypes[def.id].resist = def.resist;
	for (const auto& def : WADING_DEFS) {
		monsterTypes[def.id].wading = def.wading;
		monsterTypes[def.id].waterSpeed = def.waterSpeed;
	}
	for (const auto& def : POISON_DEFS)
		monsterTypes[def.id].poison = def.tier;
	for (const auto& def : SPIT_DEFS)
		monsterTypes[def.id].spit = def.rules;
	monsterTypes[MonsterSobek].charges = true;	   // Monster::StartCharge
	for (int id = 1; id <= MONSTER_TYPE_MAX; id++) // the checker's poisoners and POISON_DEFS must agree
		if (monsterTypes[static_cast<size_t>(id)].poison.has_value() != isPoisoner(id))
			LOG_ERRORF("assets", "Monster type %d: poisoner in %s only", id,
					   isPoisoner(id) ? "monster_kinds" : "POISON_DEFS");
	for (int id = 1; id <= MONSTER_TYPE_MAX; id++) // the minion rules and the kinds table must agree
		if (monsterTypes[static_cast<size_t>(id)].isBoss() != isBossMonster(id))
			LOG_ERRORF("assets", "Monster type %d: boss in %s only", id,
					   isBossMonster(id) ? "monster_kinds" : "BOSS_DEFS");
}

std::unique_ptr<Item> loadItem(const char* name, float scale) {
	auto item = std::make_unique<Item>();
	item->loadModel(name);
	item->scale = scale;
	return item;
}

void loadItems(ItemPrototypes& items, const Progress& progress, BarSpan span) {
	constexpr size_t STEPS = WEAPON_KIND_COUNT + 1;
	for (size_t i = 0; i < WEAPON_KIND_COUNT; i++) {
		const WeaponDef& def = WEAPON_DEFS[i];
		char label[64];
		snprintf(label, sizeof(label), "Loading Item Models [%s]", itemText(itemAt(static_cast<int>(i))).name);
		progress(span.at(i, STEPS), label);
		auto& item = items.weapons[i];
		item = loadItem(def.name, def.scale);
		item->damage = def.damage;
		item->range = def.range;
		item->mix = def.mix;
		item->motion = def.motion;
		char sound[64];
		snprintf(sound, sizeof(sound), "sounds/items/%s.wav", def.swingSound);
		item->swingSound.Load(sound);
		snprintf(sound, sizeof(sound), "sounds/items/%s.wav", def.strikeSound);
		item->strikeSound.Load(sound);
	}
	progress(span.at(WEAPON_KIND_COUNT, STEPS), "Loading Item Models [Chest and potion]");
	items.chest = loadItem("treasure_chest", 8); // faces the camera at rotA 0 (tools/blender/models/items.py)
	items.potion = loadItem("potion", 5);
	constexpr const char* MISSILES[MISSILE_KIND_COUNT] = {"arrow", "sling_stone", "throwing_stick", "javelin"};
	for (size_t i = 0; i < MISSILE_KIND_COUNT; i++) {
		char path[64];
		snprintf(path, sizeof(path), "textures/items/%s.png", MISSILES[i]);
		items.missileTex[i].LoadPNG(path);
		snprintf(path, sizeof(path), "models/items/%s.md3", MISSILES[i]);
		items.missiles[i] = loadStaticModel(path, items.missileTex[i]);
	}
}

// A prop model in tile units, centred, with its texture.
void loadProp(Texture& tex, std::unique_ptr<AnimatedModel>& model, const char* texturePath, const char* modelPath) {
	tex.LoadPNG(texturePath);
	model = std::make_unique<AnimatedModel>();
	model->Load(modelPath);
	model->BindTexture(tex.ID());
	model->Centrify();
	model->Compile();
}

void loadDecor(DecorSet& decor) {
	decor.decalTex.LoadPNG("textures/decorations/decals.png");
	auto loadSurfaces = [](Texture* tex, const char* const* names, int count) {
		for (int s = 0; s < count; s++) {
			char path[64];
			snprintf(path, sizeof(path), "textures/dungeon/%s.png", names[s]);
			tex[s].LoadPNG(path);
		}
	};
	loadSurfaces(decor.wallTex, WALL_STYLE_NAMES, WALL_STYLE_COUNT);
	loadSurfaces(decor.floorTex, FLOOR_STYLE_NAMES, FLOOR_STYLE_COUNT);
	loadSurfaces(decor.ceilingTex, CEILING_STYLE_NAMES, CEILING_STYLE_COUNT);
	decor.rockTex.LoadPNG(ROCK_TEXTURE);
	for (int d = 0; d < DECOR_COUNT; d++) {
		char path[64];
		snprintf(path, sizeof(path), "textures/decorations/decor_%s.png", DECOR_NAMES[d]);
		decor.tex[d].LoadPNG(path);
		snprintf(path, sizeof(path), "models/decorations/decor_%s.md3", DECOR_NAMES[d]);
		auto model = std::make_unique<AnimatedModel>();
		if (!model->Load(path))
			continue;
		model->BindTexture(decor.tex[d].ID());
		model->Compile(); // no Centrify: the files are in tile units
		decor.model[d] = std::move(model);
	}
	decor.torchTex.LoadPNG("textures/decorations/decor_torch.png");
	auto torchModel = std::make_unique<AnimatedModel>();
	if (torchModel->Load("models/decorations/decor_torch.md3")) {
		torchModel->BindTexture(decor.torchTex.ID());
		torchModel->Compile();
		decor.torch = std::move(torchModel);
	}
	for (int s = 0; s < LADDER_STYLE_COUNT; s++)
		for (int p = 0; p < LADDER_PIECE_COUNT; p++) {
			char path[64];
			snprintf(path, sizeof(path), "textures/ladders/ladder_%s_%s.png", LADDER_STYLE_NAMES[s],
					 LADDER_PIECE_NAMES[p]);
			decor.ladderTex[s][p].LoadPNG(path);
			snprintf(path, sizeof(path), "models/ladders/ladder_%s_%s.md3", LADDER_STYLE_NAMES[s],
					 LADDER_PIECE_NAMES[p]);
			auto model = std::make_unique<AnimatedModel>();
			if (!model->Load(path))
				continue;
			model->BindTexture(decor.ladderTex[s][p].ID());
			model->Compile();
			decor.ladder[s][p] = std::move(model);
		}
}

void loadSounds(SoundBank& sounds) {
	sounds.drink_s.Load("sounds/items/potion_drink.wav");
	sounds.keyPickup.Load("sounds/mechanisms/key_pickup.wav");
	sounds.gateOpen.Load("sounds/mechanisms/gate_open.wav");
	sounds.teleport.Load("sounds/mechanisms/teleport.wav");
	sounds.summonDig.Load("sounds/monsters/summon_dig.wav");
	sounds.summonDrop.Load("sounds/monsters/summon_drop.wav");
	sounds.gateLocked.Load("sounds/mechanisms/gate_locked.wav");
	sounds.lever.Load("sounds/mechanisms/lever.wav");
	sounds.rockRumble.Load("sounds/mechanisms/rock_rumble.wav");
	sounds.rockCrash.Load("sounds/mechanisms/rock_crash.wav");
	sounds.arrowHit.Load("sounds/items/arrow_hit.wav");
	sounds.arrowWall.Load("sounds/items/arrow_wall.wav");
	sounds.stoneHit.Load("sounds/items/stone_hit.wav");
	sounds.stoneWall.Load("sounds/items/stone_wall.wav");
	sounds.pageTurn.Load("sounds/ui/page_turn.wav");
	sounds.wade.Load("sounds/water/wade.wav");
	sounds.splash.Load("sounds/water/splash.wav");
}

void loadTraps(TrapSet& traps, TextureRegistry& textures) {
	textures.spikes.LoadPNG("textures/traps/spikes.png");
	traps.spikes = std::make_unique<Trap>();
	traps.spikes->loadModel("models/traps/spikes.md3", textures.spikes);
	traps.spikes->scale = SPIKES_SCALE;

	traps.deathTrap = std::make_unique<Trap>();
	traps.deathTrap->loadModel("models/traps/spikes.md3", textures.spikes);
	traps.deathTrap->scale = DEATH_TRAP_SCALE;
}

void loadFonts(FontSet& fonts) {
	fonts.font.Load("fonts/papyrus.png", 3, -0.3);
	fonts.status.Load("fonts/papyrus.png", 5, 0.3f, true);
	fonts.hud.Load("fonts/impact.png", 11, 0.2f, true);
	fonts.hudBody.Load("fonts/papyrus.png", 4.2f, 0.12f, true);
	fonts.hudSmall.Load("fonts/papyrus.png", 3.f, 0.08f, true);
}
} // namespace

void Assets::Load(const std::function<void(float, const char*)>& progress) {
	textures.papyrus.LoadPNG("textures/ui/papyrus_sheet.png", TexFilter::Flat);
	textures.hudIcons.LoadPNG("textures/ui/hud_icons.png");
	textures.ribbon.LoadPNG("textures/ui/ribbon.png");
	textures.ribbon.ClampToEdge(); // drawn as one quad: its edges must not wrap round to the other side
	textures.journalPaper.LoadPNG("textures/ui/journal_paper.png");
	textures.journalPaper.ClampToEdge();
	textures.journalCloth.LoadPNG("textures/ui/journal_cloth.png"); // tiles
	textures.nullTex.LoadPNG("textures/null.png");
	textures.riddleBackground.LoadPNG("textures/ui/riddlebg.png", TexFilter::Flat);

	loadMonsterTypes(monsterTypes, progress, {30, 65});
	loadItems(items, progress, {65, 80});

	loadProp(textures.sphinx, models.sphinx, "textures/props/sphinx.png", "models/props/sphinx.md3");
	loadProp(textures.ankh, models.ankh, "textures/props/ankh.png", "models/props/ankh.md3");
	loadProp(textures.questionMark, models.question, "textures/props/questionmark.png",
			 "models/props/questionmark.md3");
	loadProp(textures.columns, models.columns, "textures/props/columns.png", "models/props/columns.md3");
	textures.portal.LoadPNG("textures/effects/plasma.png");

	progress(80, "Loading decorations");
	loadDecor(decor);

	progress(83, "Loading mechanisms");
	loadMechanisms(mechanisms);
	loadSounds(sounds);
	loadTraps(traps, textures);

	progress(95, "Loading game font");
	loadFonts(fonts);

	progress(100, "Loading game soundtrack");
	sounds.soundtrack.Load("sounds/music/soundtrack.ogg");
}
