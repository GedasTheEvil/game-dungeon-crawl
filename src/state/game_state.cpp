#include "game_state.h"
#include <GL/gl.h>
#include "../graphics/gl_includes.h"
#include <fstream>
#include <cstdio>
#include "../core/logger.h"
#include <memory>
#include "../input/gameplay_config.h"
#include "../world/campaign.h"

namespace {
// Static tile-unit model like the props: no Centrify, textured only. Null if the file is missing.
std::unique_ptr<AnimatedCartoonModel> loadStaticModel(const char* path, Textura& tex) {
	auto model = std::make_unique<AnimatedCartoonModel>();
	if (!model->Load(path))
		return nullptr;
	model->BindTexture(tex.ID());
	model->outline = false;
	model->Compile();
	return model;
}

void loadMechanisms(MechanismSet& set) {
	char path[96];
	for (int c = 0; c < LOCK_COLOUR_COUNT; c++) {
		snprintf(path, sizeof(path), "Textures/mechanisms/key_%s.png", LOCK_COLOUR_NAMES[c]);
		set.keyTex[c].LoadPNG(path);
		snprintf(path, sizeof(path), "Textures/mechanisms/gate_%s.png", LOCK_COLOUR_NAMES[c]);
		set.gateTex[c].LoadPNG(path);
		snprintf(path, sizeof(path), "Textures/mechanisms/lever_base_%s.png", LOCK_COLOUR_NAMES[c]);
		set.leverBaseTex[c].LoadPNG(path);
		set.key[c] = loadStaticModel("Models/mechanisms/key.md3", set.keyTex[c]);
		set.gate[c] = loadStaticModel("Models/mechanisms/gate.md3", set.gateTex[c]);
		set.leverBase[c] = loadStaticModel("Models/mechanisms/lever_base.md3", set.leverBaseTex[c]);
	}
	set.leverHandleTex.LoadPNG("Textures/mechanisms/lever_handle.png");
	set.rockTex.LoadPNG("Textures/mechanisms/rock.png");
	set.crackTex.LoadPNG("Textures/mechanisms/ceiling_crack.png");
	set.leverHandle = loadStaticModel("Models/mechanisms/lever_handle.md3", set.leverHandleTex);
	set.rock = loadStaticModel("Models/mechanisms/rock.md3", set.rockTex);
	set.crack = loadStaticModel("Models/mechanisms/ceiling_crack.md3", set.crackTex);
}
} // namespace

GameState::GameState() = default;

GameState::~GameState() {
	void* selfPtr = this;
	LOG_DEBUGF("game", "Deleting cashe %p", selfPtr);
}

void GameState::Load() {
	// init main load resourses
	fonts.load_font.Load("Fonts/papyrus.png", 7, -1.0);
	textures.load_bg.LoadPNG("Textures/ui/scarab_slate.png");
	textures.bg.LoadPNG("Textures/ui/papyrus_sheet.png");
	textures.progBar.LoadPNG("Textures/ui/loading.png");
	textures.nullTex.LoadPNG("Textures/null.png");
	textures.blackTex.LoadPNG("Textures/dungeon/wallback.png");
	textures.black_t.LoadPNG("Textures/dungeon/black.png");
	DrawLoad(4, "Loading Textures");
	textures.player_t.LoadPNG("Textures/characters/archeologist.png");
	DrawLoad(5, "Loading Textures");
	textures.anubis_t.LoadPNG("Textures/monsters/anubis.png");
	DrawLoad(6, "Loading Textures");
	textures.worm_t.LoadPNG("Textures/monsters/worm.png");
	DrawLoad(7, "Loading Textures");
	textures.scarab_t.LoadPNG("Textures/monsters/scarab.png");
	DrawLoad(8, "Loading Textures");
	textures.bow_t.LoadPNG("Textures/items/gold.png");
	textures.gold_t.LoadPNG("Textures/items/gold.png");
	DrawLoad(9, "Loading Textures");
	textures.chest_t.LoadPNG("Textures/items/tchest.png");
	DrawLoad(10, "Loading Textures");
	textures.Dt[0].LoadPNG("Textures/sand.png");
	DrawLoad(11, "Loading Textures");
	textures.Dt[1].LoadPNG("Textures/rock.png");
	DrawLoad(12, "Loading Textures");
	textures.Dt[2].LoadPNG("Textures/vein.png");
	DrawLoad(13, "Loading Textures");
	textures.club_t.LoadPNG("Textures/items/club.png");
	DrawLoad(14, "Loading Textures");
	textures.sword_t.LoadPNG("Textures/items/sword.png");
	DrawLoad(15, "Loading Textures");
	textures.potion_t.LoadPNG("Textures/items/potion.png");
	DrawLoad(16, "Loading Textures");
	textures.spear_t.LoadPNG("Textures/items/spear.png");
	DrawLoad(17, "Loading Textures");
	textures.plant_t.LoadPNG("Textures/monsters/plant.png");
	textures.rat_t.LoadPNG("Textures/monsters/rat.png");
	textures.giantRat_t.LoadPNG("Textures/monsters/rat_giant.png");
	textures.bat_t.LoadPNG("Textures/monsters/bat.png");
	textures.giantBat_t.LoadPNG("Textures/monsters/bat_giant.png");
	textures.riddle_bg.LoadPNG("Textures/ui/riddlebg.png");
	ui.rid = std::make_unique<Riddle>();

	DrawLoad(20, "Loading Monster Models [Player]");
	Player = std::make_unique<PlayerEntity>(0, 0, 1, 1, 1, 0);
	Player->loadModel("characters/archeologist", textures.player_t, textures.progBar, true, PLAYER_CLIPS);
	Player->scale = 15;
	Player->setCords(0, 0);

	DrawLoad(30, "Loading Monster Models [Worm]");
	monsters.worm = std::make_unique<monster>(0, 0, 1, 40, 15, 1500);
	monsters.worm->loadModel("monsters/worm", textures.worm_t, textures.progBar, true);
	monsters.worm->scale = 18;
	monsters.worm->maxHealth = 20;

	DrawLoad(40, "Loading Monster Models [Scarab]");
	monsters.scarab = std::make_unique<monster>(0, 0, 2, 25, 3, 500);
	monsters.scarab->loadModel("monsters/scarab", textures.scarab_t, textures.progBar, true);
	monsters.scarab->scale = 10;
	monsters.scarab->rotA = 180;
	monsters.scarab->maxHealth = 15;
	monsters.scarab->setBloodColor(0.6f, 0.1f, 0.8f); // Purple blood

	DrawLoad(50, "Loading Monster Models [Anubis]");
	monsters.anubis = std::make_unique<monster>(0, 0, 3, 200, 50, 10000);
	monsters.anubis->loadModel("monsters/anubis", textures.anubis_t, textures.progBar, true);
	monsters.anubis->scale = 19;
	monsters.anubis->rotA = 180;
	monsters.anubis->maxHealth = 200;

	DrawLoad(60, "Loading Item Models [Treasure chest]");
	items.chest = std::make_unique<item>();
	items.chest->loadModel("Models/items/tchest.md3", textures.chest_t);
	items.chest->scale = 8;
	items.chest->rotA = -90;

	DrawLoad(65, "Loading Monster Models [Man-eater plant]");
	monsters.plant = std::make_unique<monster>(0, 0, 0, 50, 5, 1000);
	monsters.plant->loadModel("monsters/plant", textures.plant_t, textures.progBar, true);
	monsters.plant->scale = 12;
	monsters.plant->maxHealth = 30;
	monsters.plant->setBloodColor(0.1f, 0.4f, 0.1f); // Dark green blood

	DrawLoad(67, "Loading Monster Models [Rat]");
	monsters.rat = std::make_unique<monster>(0, 0, 5, 12, 2, 300);
	monsters.rat->loadModel("monsters/rat", textures.rat_t, textures.progBar, true);
	monsters.rat->scale = 26;
	monsters.rat->rotA = 180;
	monsters.rat->maxHealth = 12;

	// Same files as the rat, bigger and darker.
	DrawLoad(68, "Loading Monster Models [Giant rat]");
	monsters.giantRat = std::make_unique<monster>(0, 0, 2, 60, 8, 2000);
	monsters.giantRat->loadModel("monsters/rat", textures.giantRat_t, textures.progBar, true);
	monsters.giantRat->scale = 42;
	monsters.giantRat->rotA = 180;
	monsters.giantRat->maxHealth = 60;

	// Flyers: roost on the ceiling, swoop through the player (monster::Fly). The giant bat uses the same files.
	DrawLoad(69, "Loading Monster Models [Bat]");
	monsters.bat = std::make_unique<monster>(0, 0, 5, 8, 3, 400);
	monsters.bat->loadModel("monsters/bat", textures.bat_t, textures.progBar, true);
	monsters.bat->scale = 18;
	monsters.bat->rotA = 180;
	monsters.bat->maxHealth = 8;
	monsters.bat->flies = true;

	monsters.giantBat = std::make_unique<monster>(0, 0, 4, 40, 10, 1800);
	monsters.giantBat->loadModel("monsters/bat", textures.giantBat_t, textures.progBar, true);
	monsters.giantBat->scale = 30;
	monsters.giantBat->rotA = 180;
	monsters.giantBat->maxHealth = 40;
	monsters.giantBat->flies = true;
	monsters.giantBat->setBloodColor(0.45f, 0.05f, 0.05f);

	DrawLoad(70, "Loading Item Models [Club]");
	items.club = std::make_unique<item>();
	items.club->loadModel("Models/items/club.md3", textures.club_t);
	items.club->damage = 9;
	items.club->scale = 6;
	items.club->range = 2;

	DrawLoad(74, "Loading Item Models [Sword]");
	items.sword = std::make_unique<item>();
	items.sword->loadModel("Models/items/sword.md3", textures.sword_t);
	items.sword->scale = 9;
	items.sword->damage = 35;
	items.sword->range = 4;

	DrawLoad(76, "Loading Item Models [Bow]");
	items.bow = std::make_unique<item>();
	items.bow->loadModel("Models/items/bow.md3", textures.bow_t);
	items.bow->scale = 12;
	items.bow->damage = 12;
	items.bow->range = 16;

	DrawLoad(77, "Loading Item Models [Bow]");
	items.spear = std::make_unique<item>();
	items.spear->loadModel("Models/items/spear.md3", textures.spear_t);
	items.spear->scale = 15;
	items.spear->damage = 15;
	items.spear->range = 8;

	DrawLoad(78, "Loading Item Models [Potion]");
	items.potion = std::make_unique<item>();
	items.potion->loadModel("Models/items/potion.md3", textures.potion_t);
	items.potion->scale = 5;

	textures.sphinx_t.LoadPNG("Textures/props/sphinx.png");
	models.sphinx = std::make_unique<AnimatedCartoonModel>();
	models.sphinx->Load("Models/props/sphinx.md3");
	models.sphinx->BindTexture(textures.sphinx_t.ID());
	models.sphinx->Centrify();
	models.sphinx->Compile();

	textures.ankh_t.LoadPNG("Textures/props/ankh.png");
	models.ankh = std::make_unique<AnimatedCartoonModel>();
	models.ankh->Load("Models/props/ankh.md3");
	models.ankh->BindTexture(textures.ankh_t.ID());
	models.ankh->Centrify();
	models.ankh->Compile();

	models.question = std::make_unique<AnimatedCartoonModel>();
	models.question->Load("Models/props/questionmark.md3");
	models.question->BindTexture(textures.gold_t.ID());
	models.question->Centrify();
	models.question->Compile();

	textures.plasma_t.LoadPNG("Textures/effects/plasma.png");

	DrawLoad(80, "Loading decorations");
	decor.decalTex.LoadPNG("Textures/decorations/decals.png", true);
	for (int d = 0; d < DECOR_COUNT; d++) {
		char path[64];
		snprintf(path, sizeof(path), "Textures/decorations/decor_%s.png", DECOR_NAMES[d]);
		decor.tex[d].LoadPNG(path);
		snprintf(path, sizeof(path), "Models/decorations/decor_%s.md3", DECOR_NAMES[d]);
		auto model = std::make_unique<AnimatedCartoonModel>();
		if (!model->Load(path))
			continue;
		model->BindTexture(decor.tex[d].ID());
		model->outline = false;
		model->Compile(); // no Centrify: the files are in tile units
		decor.model[d] = std::move(model);
	}
	decor.torchTex.LoadPNG("Textures/decorations/decor_torch.png");
	auto torchModel = std::make_unique<AnimatedCartoonModel>();
	if (torchModel->Load("Models/decorations/decor_torch.md3")) {
		torchModel->BindTexture(decor.torchTex.ID());
		torchModel->outline = false;
		torchModel->Compile();
		decor.torch = std::move(torchModel);
	}
	for (int s = 0; s < LADDER_STYLE_COUNT; s++)
		for (int p = 0; p < LADDER_PIECE_COUNT; p++) {
			char path[64];
			snprintf(path, sizeof(path), "Textures/ladders/ladder_%s_%s.png", LADDER_STYLE_NAMES[s],
					 LADDER_PIECE_NAMES[p]);
			decor.ladderTex[s][p].LoadPNG(path);
			snprintf(path, sizeof(path), "Models/ladders/ladder_%s_%s.md3", LADDER_STYLE_NAMES[s],
					 LADDER_PIECE_NAMES[p]);
			auto model = std::make_unique<AnimatedCartoonModel>();
			if (!model->Load(path))
				continue;
			model->BindTexture(decor.ladderTex[s][p].ID());
			model->outline = false;
			model->Compile();
			decor.ladder[s][p] = std::move(model);
		}

	DrawLoad(83, "Loading mechanisms");
	loadMechanisms(mechanisms);

	DrawLoad(85, "Loading inventory");
	ui.invent = std::make_unique<inventory>();
	sounds.drink_s.LoadWAV("Sounds/Drink.wav");
	sounds.jump_s.LoadWAV("Sounds/Jump.wav");
	sounds.keyPickup.LoadWAV("Sounds/key_pickup.wav");
	sounds.gateOpen.LoadWAV("Sounds/gate_open.wav");
	sounds.gateLocked.LoadWAV("Sounds/gate_locked.wav");
	sounds.lever.LoadWAV("Sounds/lever.wav");
	sounds.rockRumble.LoadWAV("Sounds/rock_rumble.wav");
	sounds.rockCrash.LoadWAV("Sounds/rock_crash.wav");

	DrawLoad(88, "Loading stats");
	ui.Stats = std::make_unique<stats>();

	textures.trap_t.LoadPNG("Textures/traps/spikes.png");
	traps.TrapD = std::make_unique<trap>();
	traps.TrapD->loadModel("Models/traps/spikes.md3", textures.trap_t);
	traps.TrapD->scale = 16;

	traps.DeathTrap = std::make_unique<trap>();
	traps.DeathTrap->loadModel("Models/traps/spikes.md3", textures.trap_t);
	traps.DeathTrap->scale = 40;

	DrawLoad(95, "Loading game font");
	fonts.font.Load("Fonts/papyrus.png", 3, -0.3);
	fonts.status.Load("Fonts/papyrus.png", 5, 0.3f, true);

	Player->jump.jump_timer = std::make_unique<timer>(JUMP_TIMER_MS);
	Player->jump.jump_up_timer = std::make_unique<timer>(JUMP_UP_TIMER_MS);
	timers.mdlChange = std::make_unique<timer>(300);
	timers.AttTimer = std::make_unique<timer>(250);
	Player->jump.jump_inc = std::make_unique<timer>(JUMP_TICK_MS);
	Player->jump.fall_inc = std::make_unique<timer>(FALL_TICK_MS);
	Player->jump.fall_velocity = FALL_STEP;
	status_timer = std::make_unique<timer>(3000);

	DrawLoad(95, "Loading game Map");

	if (!dungeon.LoadCampaignLevel(curMap))
		LOG_WARNING("game", "Failed loading map");

	DrawLoad(100, "Loading game soundtrack");
	sounds.soundtrack.LoadOGG("Sounds/soundtrack.ogg");
	sounds.soundtrack.Play();

	std::ifstream f("Saves/gamelist.dat");
	if (f) {
		for (int a = 0; a < 6; a++)
			f >> saveNames[a].name;
		f.close();
	} else {
		LOG_WARNING("game", "Failed loading save list");
	}

	ui.wlc = std::make_unique<winL>();

	snprintf(status, sizeof(status), "%s", "");

	Cache_loaded = true;
}
//==============================================================
void GameState::NewGame() {
	curMap = 1;
	dungeon.LoadCampaignLevel(curMap);
	IHaveWon = false;
	Player->Reanimate();
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
	if (!Cache_loaded) {
		LOG_ERROR("game", "can't save without loading cashe");
		return;
	}

	std::ofstream dump(filename);
	if (!dump) {
		LOG_ERRORF("game", "can't open save file %s", filename);
		return;
	}

	dump << curMap << " ";

	ui.Stats->Dump(dump);
	ui.invent->Dump(dump);
	dungeon.Dump(dump);

	dump.close();
}
//==============================================================
void GameState::LoadSave(const char filename[]) {
	if (!Cache_loaded) {
		LOG_ERROR("game", "can't load without loading cashe");
		return;
	}

	Player->Reanimate();

	LOG_INFOF("game", "Loading save %s", filename);
	std::ifstream dump(filename);
	if (!dump) {
		LOG_ERRORF("game", "can't open save file %s", filename);
		return;
	}

	dump >> curMap;
	LOG_INFOF("game", "Got MapNo : %d", curMap);

	ui.Stats->LoadDump(dump);
	LOG_INFO("game", "Done loading Stats");
	ui.invent->LoadDump(dump);
	LOG_INFO("game", "Done loading Inventory");
	dungeon.LoadDump(dump);
	dungeon.scatterDecorations(campaignLevelFile(curMap).c_str());
	LOG_INFO("game", "Done loading map");
	dump.close();
}
//==============================================================
