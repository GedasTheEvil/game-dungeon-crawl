#include "assets.h"
#include "../world/monster_kinds.h"
#include <cstdio>
#include <cstring>
#include "../core/logger.h"
#include "../core/gameplay_config.h"
#include "../core/paths.h"

namespace {
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
		snprintf(path, sizeof(path), MEDIA_DIR "textures/mechanisms/key_%s.png", LOCK_COLOURS[c].name);
		set.keyTex[c].LoadPNG(path);
		snprintf(path, sizeof(path), MEDIA_DIR "textures/mechanisms/gate_%s.png", LOCK_COLOURS[c].name);
		set.gateTex[c].LoadPNG(path);
		snprintf(path, sizeof(path), MEDIA_DIR "textures/mechanisms/lever_base_%s.png", LOCK_COLOURS[c].name);
		set.leverBaseTex[c].LoadPNG(path);
		snprintf(path, sizeof(path), MEDIA_DIR "textures/mechanisms/sun_disc_%s.png", LOCK_COLOURS[c].name);
		set.sunDiscTex[c].LoadPNG(path);
	}
	set.key = loadStaticModel(MEDIA_DIR "models/mechanisms/key.md3", set.keyTex[0]);
	set.gate = loadStaticModel(MEDIA_DIR "models/mechanisms/gate.md3", set.gateTex[0]);
	set.leverBase = loadStaticModel(MEDIA_DIR "models/mechanisms/lever_base.md3", set.leverBaseTex[0]);
	set.bossGateTex.LoadPNG(MEDIA_DIR "textures/mechanisms/gate_boss.png");
	set.leverHandleTex.LoadPNG(MEDIA_DIR "textures/mechanisms/lever_handle.png");
	set.rockTex.LoadPNG(MEDIA_DIR "textures/mechanisms/rock.png");
	set.crackTex.LoadPNG(MEDIA_DIR "textures/mechanisms/ceiling_crack.png");
	set.leverHandle = loadStaticModel(MEDIA_DIR "models/mechanisms/lever_handle.md3", set.leverHandleTex);
	set.rock = loadStaticModel(MEDIA_DIR "models/mechanisms/rock.md3", set.rockTex);
	set.crack = loadStaticModel(MEDIA_DIR "models/mechanisms/ceiling_crack.md3", set.crackTex);
	set.plateTex.LoadPNG(MEDIA_DIR "textures/mechanisms/pressure_plate.png");
	set.plate = loadStaticModel(MEDIA_DIR "models/mechanisms/pressure_plate.md3", set.plateTex);
	set.dartHolesTex.LoadPNG(MEDIA_DIR "textures/mechanisms/dart_holes.png");
	set.dartHoles = loadStaticModel(MEDIA_DIR "models/mechanisms/dart_holes.md3", set.dartHolesTex);
	set.spikeSlotsTex.LoadPNG(MEDIA_DIR "textures/mechanisms/spike_slots.png");
	set.spikeSlots = loadStaticModel(MEDIA_DIR "models/mechanisms/spike_slots.md3", set.spikeSlotsTex);
	set.wallSpikesTex.LoadPNG(MEDIA_DIR "textures/mechanisms/wall_spikes.png");
	set.wallSpikes = loadStaticModel(MEDIA_DIR "models/mechanisms/wall_spikes.md3", set.wallSpikesTex);
	set.sunDisc = loadStaticModel(MEDIA_DIR "models/mechanisms/sun_disc.md3", set.sunDiscTex[0]);
	set.mirrorStandTex.LoadPNG(MEDIA_DIR "textures/mechanisms/mirror_stand.png");
	set.mirrorStand = loadStaticModel(MEDIA_DIR "models/mechanisms/mirror_stand.md3", set.mirrorStandTex);
	set.mirrorTex.LoadPNG(MEDIA_DIR "textures/mechanisms/mirror.png");
	set.mirror = loadStaticModel(MEDIA_DIR "models/mechanisms/mirror.md3", set.mirrorTex);
	set.mirrorPointerTex.LoadPNG(MEDIA_DIR "textures/mechanisms/mirror_pointer.png");
	set.mirrorPointer = loadStaticModel(MEDIA_DIR "models/mechanisms/mirror_pointer.md3", set.mirrorPointerTex);
}
} // namespace

Item* ItemPrototypes::Of(ItemKind kind) const {
	if (isAmulet(kind))
		return amulets[static_cast<size_t>(amuletOf(kind).type)].get();
	if (isPotion(kind))
		return potions[static_cast<size_t>(itemIndex(kind) - WEAPON_KIND_COUNT)].get();
	return weapons[static_cast<size_t>(itemIndex(kind))].get();
}

void Assets::LoadLoadingScreen() {
	fonts.loading.Load(MEDIA_DIR "textures/fonts/papyrus.png", 7, -1.0);
	textures.loadingBackground.LoadPNG(MEDIA_DIR "textures/ui/scarab_slate.png", TexFilter::Flat);
	textures.loadingBar.LoadPNG(MEDIA_DIR "textures/ui/loading.png", TexFilter::Flat);
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

void loadMonsterTypes(MonsterTypes& monsterTypes, std::array<CharacterModel, MONSTER_TYPE_MAX + 1>& monsterModels,
					  const Progress& progress, BarSpan span) {
	for (int id = 1; id <= MONSTER_TYPE_MAX; id++) {
		const MonsterKind& kind = *monsterKind(id);
		char label[64];
		snprintf(label, sizeof(label), "Loading Monster Models [%s]", kind.name);
		progress(span.at(static_cast<size_t>(id - 1), MONSTER_TYPE_MAX), label);
		char texture[64];
		snprintf(texture, sizeof(texture), MEDIA_DIR "textures/%s.png", kind.texture);
		Texture tex;
		tex.LoadPNG(texture);
		MonsterType& type = monsterTypes[static_cast<size_t>(id)];
		static_cast<MonsterKind&>(type) = kind;
		const ClipFiles& clips = ClipFilesOf(kind);
		// Kin on one model (normal, giant, boss) share its clips: parsed once, each with its own texture.
		CharacterModel& model = monsterModels[static_cast<size_t>(id)];
		const CharacterModel* kin = nullptr;
		for (int k = 1; k < id && kin == nullptr; k++)
			if (const MonsterKind& other = *monsterKind(k);
				std::strcmp(other.model, kind.model) == 0 && &ClipFilesOf(other) == &clips)
				kin = &monsterModels[static_cast<size_t>(k)];
		if (kin != nullptr)
			model.Share(*kin, *kind.sounds != '\0' ? kind.sounds : kind.model, std::move(tex));
		else
			model.Load(kind.model, std::move(tex), clips);
		type.model = model.Info();
	}
}

std::unique_ptr<Item> loadItem(const char* name, float scale) {
	auto item = std::make_unique<Item>();
	item->loadModel(name);
	item->scale = scale;
	return item;
}

constexpr float AMULET_SCALE = 4.f; // on a chest and in the inventory, like the potions (about 5)

// Each vessel loaded once, by its first potion; the others with it share the model.
void loadPotions(ItemPrototypes& items) {
	std::array<const Item*, POTION_MODEL_COUNT> vessels{};
	for (size_t i = 0; i < POTION_KIND_COUNT; i++) {
		const PotionDef& def = potionDef(itemAt(WEAPON_KIND_COUNT + static_cast<int>(i)));
		const PotionModelDef& model = potionModelDef(def.model);
		auto item = std::make_unique<Item>();
		const Item*& vessel = vessels[static_cast<size_t>(def.model)];
		if (vessel != nullptr)
			item->shareModel(*vessel, def.texture);
		else {
			item->loadModel(model.model, def.texture);
			vessel = item.get();
		}
		item->scale = model.scale;
		items.potions[i] = std::move(item);
	}
}

void loadItems(ItemPrototypes& items, const Progress& progress, BarSpan span) {
	constexpr size_t STEPS = WEAPON_KIND_COUNT + 1;
	for (size_t i = 0; i < WEAPON_KIND_COUNT; i++) {
		const ItemKind kind = itemAt(static_cast<int>(i));
		const WeaponDef& def = weaponDef(kind);
		char label[64];
		snprintf(label, sizeof(label), "Loading Item Models [%s]", itemText(kind).name);
		progress(span.at(i, STEPS), label);
		auto& item = items.weapons[i];
		item = loadItem(def.model, def.scale);
		char sound[64];
		snprintf(sound, sizeof(sound), MEDIA_DIR "sounds/items/%s.wav", def.swingSound);
		item->swingSound.Load(sound);
		snprintf(sound, sizeof(sound), MEDIA_DIR "sounds/items/%s.wav", def.strikeSound);
		item->strikeSound.Load(sound);
	}
	progress(span.at(WEAPON_KIND_COUNT, STEPS), "Loading Item Models [Containers, potions and amulets]");
	for (size_t i = 0; i < CONTAINER_COUNT; i++) // face the camera at rotA 0 (tools/blender/models/containers.py)
		items.containers[i] = loadItem(CONTAINER_DEFS[i].model, CONTAINER_DEFS[i].scale);
	loadPotions(items);
	for (size_t i = 0; i < AMULET_TYPE_COUNT; i++)
		items.amulets[i] = loadItem(amuletModel(static_cast<AmuletType>(i)), AMULET_SCALE);
	constexpr const char* MISSILES[MISSILE_KIND_COUNT] = {"arrow", "sling_stone", "throwing_stick", "javelin"};
	for (size_t i = 0; i < MISSILE_KIND_COUNT; i++) {
		char path[64];
		snprintf(path, sizeof(path), MEDIA_DIR "textures/items/%s.png", MISSILES[i]);
		items.missileTex[i].LoadPNG(path);
		snprintf(path, sizeof(path), MEDIA_DIR "models/items/%s.md3", MISSILES[i]);
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
	decor.decalTex.LoadPNG(MEDIA_DIR "textures/decorations/decals.png");
	decor.inscriptionTex.LoadPNG(MEDIA_DIR "textures/decorations/inscriptions.png");
	decor.nameCartoucheTex.LoadPNG(MEDIA_DIR "textures/decorations/name_cartouches.png");
	auto loadSurfaces = [](Texture* tex, const char* const* names, int count) {
		for (int s = 0; s < count; s++) {
			char path[64];
			snprintf(path, sizeof(path), MEDIA_DIR "textures/dungeon/%s.png", names[s]);
			tex[s].LoadPNG(path);
		}
	};
	loadSurfaces(decor.wallTex, WALL_STYLE_NAMES, WALL_STYLE_COUNT);
	loadSurfaces(decor.floorTex, FLOOR_STYLE_NAMES, FLOOR_STYLE_COUNT);
	loadSurfaces(decor.ceilingTex, CEILING_STYLE_NAMES, CEILING_STYLE_COUNT);
	decor.rockTex.LoadPNG(ROCK_TEXTURE);
	for (int d = 0; d < DECOR_COUNT; d++) {
		if (decorModel(d) != d)
			continue; // drawn with another prop's model (the decoy, the coffin's)
		char path[64];
		snprintf(path, sizeof(path), MEDIA_DIR "textures/decorations/decor_%s.png", DECOR_NAMES[d]);
		decor.tex[d].LoadPNG(path);
		snprintf(path, sizeof(path), MEDIA_DIR "models/decorations/decor_%s.md3", DECOR_NAMES[d]);
		auto model = std::make_unique<AnimatedModel>();
		if (!model->Load(path))
			continue;
		model->BindTexture(decor.tex[d].ID());
		model->Compile(); // no Centrify: the files are in tile units
		decor.model[d] = std::move(model);
	}
	decor.torchTex.LoadPNG(MEDIA_DIR "textures/decorations/decor_torch.png");
	auto torchModel = std::make_unique<AnimatedModel>();
	if (torchModel->Load(MEDIA_DIR "models/decorations/decor_torch.md3")) {
		torchModel->BindTexture(decor.torchTex.ID());
		torchModel->Compile();
		decor.torch = std::move(torchModel);
	}
	for (int s = 0; s < LADDER_STYLE_COUNT; s++)
		for (int p = 0; p < LADDER_PIECE_COUNT; p++) {
			char path[64];
			snprintf(path, sizeof(path), MEDIA_DIR "textures/ladders/ladder_%s_%s.png", LADDER_STYLE_NAMES[s],
					 LADDER_PIECE_NAMES[p]);
			decor.ladderTex[s][p].LoadPNG(path);
			snprintf(path, sizeof(path), MEDIA_DIR "models/ladders/ladder_%s_%s.md3", LADDER_STYLE_NAMES[s],
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
	sounds.drink_s.Load(MEDIA_DIR "sounds/items/potion_drink.wav");
	sounds.amulet_s.Load(MEDIA_DIR "sounds/items/amulet.wav");
	sounds.keyPickup.Load(MEDIA_DIR "sounds/mechanisms/key_pickup.wav");
	sounds.gateOpen.Load(MEDIA_DIR "sounds/mechanisms/gate_open.wav");
	sounds.teleport.Load(MEDIA_DIR "sounds/mechanisms/teleport.wav");
	sounds.summonDig.Load(MEDIA_DIR "sounds/monsters/summon_dig.wav");
	sounds.summonDrop.Load(MEDIA_DIR "sounds/monsters/summon_drop.wav");
	sounds.gateLocked.Load(MEDIA_DIR "sounds/mechanisms/gate_locked.wav");
	sounds.lever.Load(MEDIA_DIR "sounds/mechanisms/lever.wav");
	sounds.rockRumble.Load(MEDIA_DIR "sounds/mechanisms/rock_rumble.wav");
	sounds.plateClick.Load(MEDIA_DIR "sounds/mechanisms/plate_click.wav");
	sounds.dart.Load(MEDIA_DIR "sounds/mechanisms/dart.wav");
	sounds.spikeScrape.Load(MEDIA_DIR "sounds/mechanisms/spike_scrape.wav");
	sounds.spikeThunk.Load(MEDIA_DIR "sounds/mechanisms/spike_thunk.wav");
	sounds.mirrorTurn.Load(MEDIA_DIR "sounds/mechanisms/mirror_turn.wav");
	sounds.sunDisc.Load(MEDIA_DIR "sounds/mechanisms/sun_disc.wav");
	sounds.dodge.Load(MEDIA_DIR "sounds/items/sling_swing.wav");
	sounds.rockCrash.Load(MEDIA_DIR "sounds/mechanisms/rock_crash.wav");
	sounds.sandHiss.Load(MEDIA_DIR "sounds/mechanisms/sand_hiss.wav");
	sounds.sandPour.Load(MEDIA_DIR "sounds/mechanisms/sand_pour.wav");
	sounds.sandStep.Load(MEDIA_DIR "sounds/mechanisms/sand_step.wav");
	sounds.lidWood.Load(MEDIA_DIR "sounds/items/lid_wood.wav");
	sounds.lidReed.Load(MEDIA_DIR "sounds/items/lid_reed.wav");
	sounds.lidStone.Load(MEDIA_DIR "sounds/items/lid_stone.wav");
	sounds.jarSmash.Load(MEDIA_DIR "sounds/items/jar_smash.wav");
	sounds.arrowHit.Load(MEDIA_DIR "sounds/items/arrow_hit.wav");
	sounds.arrowWall.Load(MEDIA_DIR "sounds/items/arrow_wall.wav");
	sounds.stoneHit.Load(MEDIA_DIR "sounds/items/stone_hit.wav");
	sounds.stoneWall.Load(MEDIA_DIR "sounds/items/stone_wall.wav");
	sounds.pageTurn.Load(MEDIA_DIR "sounds/ui/page_turn.wav");
	sounds.wade.Load(MEDIA_DIR "sounds/water/wade.wav");
	sounds.splash.Load(MEDIA_DIR "sounds/water/splash.wav");
}

void loadTraps(TrapSet& traps, TextureRegistry& textures) {
	textures.spikes.LoadPNG(MEDIA_DIR "textures/traps/spikes.png");
	traps.spikes = std::make_unique<Trap>();
	traps.spikes->loadModel(MEDIA_DIR "models/traps/spikes.md3", textures.spikes);
	traps.spikes->scale = SPIKES_SCALE;

	traps.deathTrap = std::make_unique<Trap>(*traps.spikes);
	traps.deathTrap->scale = DEATH_TRAP_SCALE;
}

void loadFonts(FontSet& fonts) {
	fonts.font.Load(MEDIA_DIR "textures/fonts/papyrus.png", 3, -0.3);
	fonts.status.Load(MEDIA_DIR "textures/fonts/papyrus.png", 5, 0.3f, true);
	fonts.hud.Load(MEDIA_DIR "textures/fonts/impact.png", 11, 0.2f, true);
	fonts.hudBody.Load(MEDIA_DIR "textures/fonts/papyrus.png", 4.2f, 0.12f, true);
	fonts.hudSmall.Load(MEDIA_DIR "textures/fonts/papyrus.png", 3.f, 0.08f, true);
}
} // namespace

void Assets::Load(const std::function<void(float, const char*)>& progress) {
	textures.papyrus.LoadPNG(MEDIA_DIR "textures/ui/papyrus_sheet.png", TexFilter::Flat);
	textures.hudIcons.LoadPNG(MEDIA_DIR "textures/ui/hud_icons.png");
	textures.skillIcons.LoadPNG(MEDIA_DIR "textures/ui/skill_icons.png");
	textures.ribbon.LoadPNG(MEDIA_DIR "textures/ui/ribbon.png");
	textures.ribbon.ClampToEdge(); // drawn as one quad: its edges must not wrap round to the other side
	textures.expeditionPage.LoadPNG(MEDIA_DIR "textures/ui/expedition_page.png"); // tools/textures/expedition_page.py
	textures.expeditionPage.ClampToEdge();
	textures.journalPaper.LoadPNG(MEDIA_DIR "textures/ui/journal_paper.png");
	textures.journalPaper.ClampToEdge();
	textures.journalCloth.LoadPNG(MEDIA_DIR "textures/ui/journal_cloth.png"); // tiles
	textures.nullTex.LoadPNG(MEDIA_DIR "textures/null.png");
	textures.riddleBackground.LoadPNG(MEDIA_DIR "textures/ui/riddlebg.png", TexFilter::Flat);

	loadMonsterTypes(monsterTypes, monsterModels, progress, {30, 65});
	loadItems(items, progress, {65, 80});

	loadProp(textures.sphinx, models.sphinx, MEDIA_DIR "textures/props/sphinx.png",
			 MEDIA_DIR "models/props/sphinx.md3");
	loadProp(textures.ankh, models.ankh, MEDIA_DIR "textures/props/ankh.png", MEDIA_DIR "models/props/ankh.md3");
	loadProp(textures.questionMark, models.question, MEDIA_DIR "textures/props/questionmark.png",
			 MEDIA_DIR "models/props/questionmark.md3");
	loadProp(textures.columns, models.columns, MEDIA_DIR "textures/props/columns.png",
			 MEDIA_DIR "models/props/columns.md3");
	textures.portal.LoadPNG(MEDIA_DIR "textures/effects/plasma.png");
	textures.portalRed.LoadPNG(MEDIA_DIR "textures/effects/plasma_red.png");

	progress(80, "Loading decorations");
	loadDecor(decor);

	progress(83, "Loading mechanisms");
	loadMechanisms(mechanisms);
	loadSounds(sounds);
	loadTraps(traps, textures);

	progress(95, "Loading game font");
	loadFonts(fonts);

	progress(100, "Loading game soundtrack");
	sounds.soundtrack.Load(MEDIA_DIR "sounds/music/soundtrack.ogg");
}
