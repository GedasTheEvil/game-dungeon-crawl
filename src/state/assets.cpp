#include "assets.h"
#include "../world/monster_kinds.h"
#include <cstdio>
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
	{MonsterAnubis, "Anubis", "monsters/anubis", "monsters/anubis", 3, 350, 30, 1200, 10000, 19, 180, Locomotion::Walk,
	 RED_BLOOD},
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
	{MonsterGiantRat, "Giant rat", "monsters/rat", "monsters/rat_giant", 4, 60, 8, 700, 2000, 42, 180,
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
	// The finale's guardian, a head taller than an Anubis; see BOSS_DEFS. A level 30 player (398 HP, about 6 armour)
	// dies to 4 blows.
	// Reckless: the player cannot shake him off behind a row of traps.
	{MonsterAnubisBoss, "Anubis boss", "monsters/anubis", "monsters/anubis_boss", 4.5f, 1500, 110, 1400, 20000, 26, 180,
	 Locomotion::Walk, RED_BLOOD, Courage::Reckless, 10},
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
};

struct ItemDef {
	std::unique_ptr<Item> ItemPrototypes::*slot;
	const char* label;
	const char* name; // models/items/<name>.md3, textures/items/<name>.png
	float scale;
	int damage, range; // weapons only; range in tenths of a tile (the bow's: how far it aims)
	WeaponMotion motion;
	const char* swingSound = nullptr;  // sounds/items/<name>.wav: the attack begins
	const char* strikeSound = nullptr; // a melee hit lands, the arrow leaves
};

// The chest faces the camera at rotA 0 (tools/blender/models/items.py). Motion: grip, rest / windup / strike tilt,
// thrust, hit / swing / attack ms. The club is slow and heavy, the sword quick, the spear thrusts.
const ItemDef ITEM_DEFS[] = {
	{&ItemPrototypes::chest, "Treasure chest", "treasure_chest", 8, 1, 1, {}},
	{&ItemPrototypes::club, "Club", "club", 6, 9, 2, {0.12f, 35, -40, 115, 0, 300, 560, 900}, "club_swing", "club_hit"},
	{&ItemPrototypes::sword,
	 "Sword",
	 "sword",
	 9,
	 35,
	 3,
	 {0.1f, 40, -10, 120, 0, 180, 360, 550},
	 "sword_swing",
	 "sword_hit"},
	{&ItemPrototypes::bow,
	 "Bow",
	 "bow",
	 12,
	 12,
	 30,
	 {0.5f, 0, 0, 0, 0, BOW_DRAW_MS, BOW_DRAW_MS + 100, 1000},
	 "bow_draw",
	 "bow_release"},
	{&ItemPrototypes::spear,
	 "Spear",
	 "spear",
	 15,
	 15,
	 5,
	 {0.35f, 70, 70, 70, 0.3f, 200, 420, 750},
	 "spear_swing",
	 "spear_hit"},
	{&ItemPrototypes::potion, "Potion", "potion", 5, 1, 1, {}},
};

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
	switch (kind) {
	case ItemKind::Club:
		return club.get();
	case ItemKind::Sword:
		return sword.get();
	case ItemKind::Spear:
		return spear.get();
	case ItemKind::Bow:
		return bow.get();
	default:
		return potion.get();
	}
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
	}
	for (const auto& def : BOSS_DEFS)
		monsterTypes[def.id].boss = def.rules;
	for (int id = 1; id <= MONSTER_TYPE_MAX; id++) // the minion rules and the kinds table must agree
		if (monsterTypes[static_cast<size_t>(id)].isBoss() != isBossMonster(id))
			LOG_ERRORF("assets", "Monster type %d: boss in %s only", id,
					   isBossMonster(id) ? "monster_kinds" : "BOSS_DEFS");
}

void loadItems(ItemPrototypes& items, const Progress& progress, BarSpan span) {
	for (size_t i = 0; i < std::size(ITEM_DEFS); i++) {
		const ItemDef& def = ITEM_DEFS[i];
		char label[64];
		snprintf(label, sizeof(label), "Loading Item Models [%s]", def.label);
		progress(span.at(i, std::size(ITEM_DEFS)), label);
		auto& item = items.*def.slot;
		item = std::make_unique<Item>();
		item->loadModel(def.name);
		item->scale = def.scale;
		item->damage = def.damage;
		item->range = def.range;
		item->motion = def.motion;
		char sound[64];
		if (def.swingSound) {
			snprintf(sound, sizeof(sound), "sounds/items/%s.wav", def.swingSound);
			item->swingSound.Load(sound);
		}
		if (def.strikeSound) {
			snprintf(sound, sizeof(sound), "sounds/items/%s.wav", def.strikeSound);
			item->strikeSound.Load(sound);
		}
	}
	items.arrowTex.LoadPNG("textures/items/arrow.png");
	items.arrow = loadStaticModel("models/items/arrow.md3", items.arrowTex);
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
