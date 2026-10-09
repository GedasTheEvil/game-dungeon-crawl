#include "assets.h"
#include "../world/monster_kinds.h"
#include <cstdio>
#include <cstring>
#include "../core/logger.h"
#include "../core/gameplay_config.h"

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
		snprintf(path, sizeof(path), "textures/mechanisms/key_%s.png", LOCK_COLOURS[c].name);
		set.keyTex[c].LoadPNG(path);
		snprintf(path, sizeof(path), "textures/mechanisms/gate_%s.png", LOCK_COLOURS[c].name);
		set.gateTex[c].LoadPNG(path);
		snprintf(path, sizeof(path), "textures/mechanisms/lever_base_%s.png", LOCK_COLOURS[c].name);
		set.leverBaseTex[c].LoadPNG(path);
	}
	set.key = loadStaticModel("models/mechanisms/key.md3", set.keyTex[0]);
	set.gate = loadStaticModel("models/mechanisms/gate.md3", set.gateTex[0]);
	set.leverBase = loadStaticModel("models/mechanisms/lever_base.md3", set.leverBaseTex[0]);
	set.bossGateTex.LoadPNG("textures/mechanisms/gate_boss.png");
	set.leverHandleTex.LoadPNG("textures/mechanisms/lever_handle.png");
	set.rockTex.LoadPNG("textures/mechanisms/rock.png");
	set.crackTex.LoadPNG("textures/mechanisms/ceiling_crack.png");
	set.leverHandle = loadStaticModel("models/mechanisms/lever_handle.md3", set.leverHandleTex);
	set.rock = loadStaticModel("models/mechanisms/rock.md3", set.rockTex);
	set.crack = loadStaticModel("models/mechanisms/ceiling_crack.md3", set.crackTex);
	set.plateTex.LoadPNG("textures/mechanisms/pressure_plate.png");
	set.plate = loadStaticModel("models/mechanisms/pressure_plate.md3", set.plateTex);
	set.dartHolesTex.LoadPNG("textures/mechanisms/dart_holes.png");
	set.dartHoles = loadStaticModel("models/mechanisms/dart_holes.md3", set.dartHolesTex);
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
	for (int id = 1; id <= MONSTER_TYPE_MAX; id++) {
		const MonsterKind& kind = *monsterKind(id);
		char label[64];
		snprintf(label, sizeof(label), "Loading Monster Models [%s]", kind.name);
		progress(span.at(static_cast<size_t>(id - 1), MONSTER_TYPE_MAX), label);
		char texture[64];
		snprintf(texture, sizeof(texture), "textures/%s.png", kind.texture);
		Texture tex;
		tex.LoadPNG(texture);
		MonsterType& type = monsterTypes[static_cast<size_t>(id)];
		static_cast<MonsterKind&>(type) = kind;
		const ClipFiles& clips = ClipFilesOf(kind.locomotion);
		// Kin on one model (normal, giant, boss) share its clips: parsed once, each with its own texture.
		const MonsterType* kin = nullptr;
		for (int k = 1; k < id && kin == nullptr; k++)
			if (const MonsterKind& other = *monsterKind(k);
				std::strcmp(other.model, kind.model) == 0 && &ClipFilesOf(other.locomotion) == &clips)
				kin = &monsterTypes[static_cast<size_t>(k)];
		if (kin != nullptr)
			type.model.Share(kin->model, kind.model, std::move(tex));
		else
			type.model.Load(kind.model, std::move(tex), clips);
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
	progress(span.at(WEAPON_KIND_COUNT, STEPS), "Loading Item Models [Chest, potions and amulets]");
	items.chest = loadItem("treasure_chest", 8); // faces the camera at rotA 0 (tools/blender/models/items.py)
	loadPotions(items);
	for (size_t i = 0; i < AMULET_TYPE_COUNT; i++)
		items.amulets[i] = loadItem(amuletModel(static_cast<AmuletType>(i)), AMULET_SCALE);
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
	sounds.amulet_s.Load("sounds/items/amulet.wav");
	sounds.keyPickup.Load("sounds/mechanisms/key_pickup.wav");
	sounds.gateOpen.Load("sounds/mechanisms/gate_open.wav");
	sounds.teleport.Load("sounds/mechanisms/teleport.wav");
	sounds.summonDig.Load("sounds/monsters/summon_dig.wav");
	sounds.summonDrop.Load("sounds/monsters/summon_drop.wav");
	sounds.gateLocked.Load("sounds/mechanisms/gate_locked.wav");
	sounds.lever.Load("sounds/mechanisms/lever.wav");
	sounds.rockRumble.Load("sounds/mechanisms/rock_rumble.wav");
	sounds.plateClick.Load("sounds/mechanisms/plate_click.wav");
	sounds.dart.Load("sounds/mechanisms/dart.wav");
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

	traps.deathTrap = std::make_unique<Trap>(*traps.spikes);
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
