#include "game_state.h"
#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include <fstream>
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include "../core/logger.h"
#include <memory>
#include "../core/gameplay_config.h"
#include "../world/campaign.h"

namespace {
struct MonsterDef { // NOLINT(clang-analyzer-optin.performance.Padding): a small table, ordered to read
	MonsterTypeId id;
	const char* label;
	const char* model;	 // under models/ and sounds/
	const char* texture; // under textures/: the giant rat and bat are the same model, bigger and darker
	int speed, maxHealth, damage, xp;
	float scale, rotA;
	Locomotion locomotion;
	Rgb blood;
};

constexpr Rgb RED_BLOOD = {0.7f, 0.1f, 0.1f};

// Flyers roost on the ceiling and swoop through the player (Monster::Fly).
const MonsterDef MONSTER_DEFS[] = {
	{MonsterWorm, "Worm", "monsters/worm", "monsters/worm", 1, 20, 15, 1500, 18, 0, Locomotion::Walk, RED_BLOOD},
	{MonsterScarab,
	 "Scarab",
	 "monsters/scarab",
	 "monsters/scarab",
	 2,
	 15,
	 3,
	 500,
	 10,
	 180,
	 Locomotion::Walk,
	 {0.6f, 0.1f, 0.8f}},
	{MonsterAnubis, "Anubis", "monsters/anubis", "monsters/anubis", 3, 200, 50, 10000, 19, 180, Locomotion::Walk,
	 RED_BLOOD},
	{MonsterPlant,
	 "Man-eater plant",
	 "monsters/plant",
	 "monsters/plant",
	 0,
	 30,
	 5,
	 1000,
	 12,
	 0,
	 Locomotion::Stationary,
	 {0.1f, 0.4f, 0.1f}},
	{MonsterRat, "Rat", "monsters/rat", "monsters/rat", 5, 12, 2, 300, 13, 180, Locomotion::Walk, RED_BLOOD},
	{MonsterGiantRat, "Giant rat", "monsters/rat", "monsters/rat_giant", 2, 60, 8, 2000, 42, 180, Locomotion::WalkJump,
	 RED_BLOOD},
	{MonsterBat, "Bat", "monsters/bat", "monsters/bat", 5, 8, 3, 400, 18, 180, Locomotion::Fly, RED_BLOOD},
	{MonsterGiantBat,
	 "Giant bat",
	 "monsters/bat",
	 "monsters/bat_giant",
	 4,
	 40,
	 10,
	 1800,
	 30,
	 180,
	 Locomotion::Fly,
	 {0.45f, 0.05f, 0.05f}},
};

struct ItemDef {
	std::unique_ptr<Item> ItemPrototypes::*slot;
	const char* label;
	const char* name; // models/items/<name>.md3, textures/items/<name>.png
	float scale;
	int damage, range; // weapons only
};

// The chest faces the camera at rotA 0 (tools/blender/models/items.py).
const ItemDef ITEM_DEFS[] = {
	{&ItemPrototypes::chest, "Treasure chest", "treasure_chest", 8, 1, 1},
	{&ItemPrototypes::club, "Club", "club", 6, 9, 2},
	{&ItemPrototypes::sword, "Sword", "sword", 9, 35, 4},
	{&ItemPrototypes::bow, "Bow", "bow", 12, 12, 16},
	{&ItemPrototypes::spear, "Spear", "spear", 15, 15, 8},
	{&ItemPrototypes::potion, "Potion", "potion", 5, 1, 1},
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
		snprintf(path, sizeof(path), "textures/mechanisms/key_%s.png", LOCK_COLOUR_NAMES[c]);
		set.keyTex[c].LoadPNG(path);
		snprintf(path, sizeof(path), "textures/mechanisms/gate_%s.png", LOCK_COLOUR_NAMES[c]);
		set.gateTex[c].LoadPNG(path);
		snprintf(path, sizeof(path), "textures/mechanisms/lever_base_%s.png", LOCK_COLOUR_NAMES[c]);
		set.leverBaseTex[c].LoadPNG(path);
		set.key[c] = loadStaticModel("models/mechanisms/key.md3", set.keyTex[c]);
		set.gate[c] = loadStaticModel("models/mechanisms/gate.md3", set.gateTex[c]);
		set.leverBase[c] = loadStaticModel("models/mechanisms/lever_base.md3", set.leverBaseTex[c]);
	}
	set.leverHandleTex.LoadPNG("textures/mechanisms/lever_handle.png");
	set.rockTex.LoadPNG("textures/mechanisms/rock.png");
	set.crackTex.LoadPNG("textures/mechanisms/ceiling_crack.png");
	set.leverHandle = loadStaticModel("models/mechanisms/lever_handle.md3", set.leverHandleTex);
	set.rock = loadStaticModel("models/mechanisms/rock.md3", set.rockTex);
	set.crack = loadStaticModel("models/mechanisms/ceiling_crack.md3", set.crackTex);
}
std::unique_ptr<GameState> gGame;
} // namespace

void CreateGame() { gGame = std::make_unique<GameState>(); }

void DestroyGame() { gGame.reset(); }

GameState& Game() {
	assert(gGame && "CreateGame() not called");
	return *gGame;
}

GameState::GameState() = default;

GameState::~GameState() {
	void* selfPtr = this;
	LOG_DEBUGF("game", "Deleting cashe %p", selfPtr);
}

void GameState::Load() {
	// init main load resourses
	fonts.load_font.Load("fonts/papyrus.png", 7, -1.0);
	textures.load_bg.LoadPNG("textures/ui/scarab_slate.png", TexFilter::Flat);
	textures.bg.LoadPNG("textures/ui/papyrus_sheet.png", TexFilter::Flat);
	textures.progBar.LoadPNG("textures/ui/loading.png", TexFilter::Flat);
	textures.nullTex.LoadPNG("textures/null.png");
	textures.riddle_bg.LoadPNG("textures/ui/riddlebg.png", TexFilter::Flat);
	ui.riddle = std::make_unique<Riddle>();

	DrawLoad(20, "Loading Monster Models [Player]");
	{
		Texture tex;
		tex.LoadPNG("textures/characters/archeologist.png");
		player = std::make_unique<Player>();
		player->Load("characters/archeologist", tex);
		player->scale = 15;
	}

	int progress = 30;
	for (const MonsterDef& def : MONSTER_DEFS) {
		char label[64];
		snprintf(label, sizeof(label), "Loading Monster Models [%s]", def.label);
		DrawLoad(static_cast<float>(progress), label);
		progress += 5;
		char texture[64];
		snprintf(texture, sizeof(texture), "textures/%s.png", def.texture);
		Texture tex;
		tex.LoadPNG(texture);
		MonsterType& type = monsterTypes[def.id];
		type.model.Load(def.model, tex, MONSTER_CLIPS);
		type.speed = def.speed;
		type.maxHealth = def.maxHealth;
		type.damage = def.damage;
		type.xp = def.xp;
		type.scale = def.scale;
		type.rotA = def.rotA;
		type.locomotion = def.locomotion;
		type.blood = def.blood;
	}

	for (const ItemDef& def : ITEM_DEFS) {
		char label[64];
		snprintf(label, sizeof(label), "Loading Item Models [%s]", def.label);
		DrawLoad(static_cast<float>(progress), label);
		progress += 2;
		auto& item = items.*def.slot;
		item = std::make_unique<Item>();
		item->loadModel(def.name);
		item->scale = def.scale;
		item->damage = def.damage;
		item->range = def.range;
	}

	textures.sphinx_t.LoadPNG("textures/props/sphinx.png");
	models.sphinx = std::make_unique<AnimatedModel>();
	models.sphinx->Load("models/props/sphinx.md3");
	models.sphinx->BindTexture(textures.sphinx_t.ID());
	models.sphinx->Centrify();
	models.sphinx->Compile();

	textures.ankh_t.LoadPNG("textures/props/ankh.png");
	models.ankh = std::make_unique<AnimatedModel>();
	models.ankh->Load("models/props/ankh.md3");
	models.ankh->BindTexture(textures.ankh_t.ID());
	models.ankh->Centrify();
	models.ankh->Compile();

	textures.question_t.LoadPNG("textures/props/questionmark.png");
	models.question = std::make_unique<AnimatedModel>();
	models.question->Load("models/props/questionmark.md3");
	models.question->BindTexture(textures.question_t.ID());
	models.question->Centrify();
	models.question->Compile();

	textures.plasma_t.LoadPNG("textures/effects/plasma.png");

	DrawLoad(80, "Loading decorations");
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

	DrawLoad(83, "Loading mechanisms");
	loadMechanisms(mechanisms);

	DrawLoad(85, "Loading inventory");
	ui.inventory = std::make_unique<Inventory>();
	sounds.drink_s.Load("sounds/items/potion_drink.wav");
	sounds.keyPickup.Load("sounds/mechanisms/key_pickup.wav");
	sounds.gateOpen.Load("sounds/mechanisms/gate_open.wav");
	sounds.gateLocked.Load("sounds/mechanisms/gate_locked.wav");
	sounds.lever.Load("sounds/mechanisms/lever.wav");
	sounds.rockRumble.Load("sounds/mechanisms/rock_rumble.wav");
	sounds.rockCrash.Load("sounds/mechanisms/rock_crash.wav");

	textures.trap_t.LoadPNG("textures/traps/spikes.png");
	traps.TrapD = std::make_unique<Trap>();
	traps.TrapD->loadModel("models/traps/spikes.md3", textures.trap_t);
	traps.TrapD->scale = 16;

	traps.DeathTrap = std::make_unique<Trap>();
	traps.DeathTrap->loadModel("models/traps/spikes.md3", textures.trap_t);
	traps.DeathTrap->scale = 40;

	DrawLoad(95, "Loading game font");
	fonts.font.Load("fonts/papyrus.png", 3, -0.3);
	fonts.status.Load("fonts/papyrus.png", 5, 0.3f, true);
	fonts.hud.Load("fonts/impact.png", 11, 0.2f, true);

	timers.idleModel.Reset();
	timers.weaponRest.Reset();
	player->jump.fall_velocity = FALL_STEP;
	statusTimer = Timer(STATUS_MS);

	DrawLoad(95, "Loading game Map");

	if (!dungeon.LoadCampaignLevel(curMap))
		LOG_WARNING("game", "Failed loading map");

	DrawLoad(100, "Loading game soundtrack");
	sounds.soundtrack.Load("sounds/music/soundtrack.ogg");
	sounds.soundtrack.Play();

	std::ifstream f("saves/gamelist.dat");
	if (f) {
		for (int a = 0; a < 6; a++)
			f >> saveNames[a].name;
		f.close();
	} else {
		LOG_WARNING("game", "Failed loading save list");
	}

	ui.endScreens = std::make_unique<EndScreens>();

	status.clear();

	cacheLoaded = true;
}
//==============================================================
void GameState::ShowStatus(const char* format, ...) {
	char buf[256];
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	status = buf;
	statusTimer.Reset();
}
//==============================================================
void GameState::NewGame() {
	player->stats = PlayerStats{};
	ui.inventory->Reset();
	curMap = 1;
	dungeon.LoadCampaignLevel(curMap);
	hasWon = false;
	player->Reanimate();
}
//==============================================================
void GameState::DrawLoad(float xxx, const char text[]) {
	LOG_INFOF("loading", "DrawLoad: %s", text);

	if (xxx > 100)
		xxx = 100;

	xxx *= 1.18;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear The Screen And The Depth Buffer
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);		// Select The Projection Matrix
	glLoadIdentity();					// Reset The Projection Matrix
	glOrtho(0, 140, 0, 140, -200, 200); // Set Up An Ortho Screen
	glMatrixMode(GL_MODELVIEW);			// Select The Modelview Matrix

	// Background image
	textures.load_bg.Bind();

	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1);
	glTexCoord2f(0, 0);
	glVertex3i(0, 0, -40);
	glTexCoord2f(0, 1);
	glVertex3i(0, 140, -40);
	glTexCoord2f(1, 1);
	glVertex3i(140, 140, -40);
	glTexCoord2f(1, 0);
	glVertex3i(140, 0, -40);
	glEnd();

	// progressbar
	textures.progBar.Bind();

	glColor3f(1.2, 0.6, 0);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex3f(10, 28, 0);
	glTexCoord2f(1, 0);
	glVertex3f(xxx + 10, 28, 0);
	glTexCoord2f(1, 1);
	glVertex3f(xxx + 10, 38, 0);
	glTexCoord2f(0, 1);
	glVertex3f(10, 38, 0);
	glEnd();

	glColor3f(1, 1, 1);

	glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
	glEnable(GL_BLEND);
	fonts.load_font.print(10, 15, text);
	glDisable(GL_BLEND);

	glFlush();

	glutSwapBuffers();
}
//==============================================================
void GameState::Save(const char filename[]) {
	if (!cacheLoaded) {
		LOG_ERROR("game", "can't save without loading cashe");
		return;
	}

	std::ofstream dump(filename);
	if (!dump) {
		LOG_ERRORF("game", "can't open save file %s", filename);
		return;
	}

	dump << curMap << " ";

	player->stats.Dump(dump);
	ui.inventory->Dump(dump);
	dungeon.Dump(dump);

	dump.close();
}
//==============================================================
void GameState::LoadSave(const char filename[]) {
	if (!cacheLoaded) {
		LOG_ERROR("game", "can't load without loading cashe");
		return;
	}

	player->Reanimate();

	LOG_INFOF("game", "Loading save %s", filename);
	std::ifstream dump(filename);
	if (!dump) {
		LOG_ERRORF("game", "can't open save file %s", filename);
		return;
	}

	dump >> curMap;
	LOG_INFOF("game", "Got MapNo : %d", curMap);

	player->stats.LoadDump(dump);
	LOG_INFO("game", "Done loading Stats");
	ui.inventory->LoadDump(dump);
	LOG_INFO("game", "Done loading Inventory");
	dungeon.LoadDump(dump);
	dungeon.scatterDecorations(campaignLevelFile(curMap).c_str());
	LOG_INFO("game", "Done loading map");
	dump.close();
}
//==============================================================
