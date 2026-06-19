#include "cashe.h"
#include <GL/gl.h>
#ifndef WIN32
#include <GL/glut.h>
#endif
#ifdef WIN32
#include <GL/freeglut.h>
#endif
#include <fstream>
#include <cstdio>
#include "../core/logger.h"
#include <memory>
#include "../input/gameplay_config.h"

char mapName2[200];

Cashe::Cashe() = default;

Cashe::~Cashe() { LOG_DEBUGF("game", "Deleting cashe %p", (void*)this); }

void Cashe::Load() {
	// init main load resourses
	fonts.load_font.Load("Fonts/papyrus.bmp", 7, -1.0);
	textures.load_bg.LoadBMP("Textures/scarab_slate.bmp");
	textures.bg.LoadBMP("Textures/papyrus_sheet.bmp");
	textures.progBar.LoadBMP("Textures/loading.bmp");
	textures.nullTex.LoadBMP("Textures/null.bmp");
	textures.blackTex.LoadBMP("Textures/wallback.bmp");
	textures.black_t.LoadBMP("Textures/black.bmp");
	DrawLoad(4, "Loading Textures");
	textures.menu_bg.LoadBMP("Textures/menu_main.bmp");
	textures.menu_save_bg.LoadBMP("Textures/menu_save.bmp");
	textures.player_t.LoadBMP("Textures/player.bmp");
	DrawLoad(5, "Loading Textures");
	textures.anubis_t.LoadBMP("Textures/anubis.bmp");
	DrawLoad(6, "Loading Textures");
	textures.worm_t.LoadBMP("Textures/worm.bmp");
	DrawLoad(7, "Loading Textures");
	textures.scarab_t.LoadBMP("Textures/scarab.bmp");
	DrawLoad(8, "Loading Textures");
	textures.bow_t.LoadBMP("Textures/scarab.bmp");
	DrawLoad(9, "Loading Textures");
	textures.chest_t.LoadBMP("Textures/tchest.bmp");
	DrawLoad(10, "Loading Textures");
	textures.Dt[0].LoadBMP("Textures/sand.bmp");
	DrawLoad(11, "Loading Textures");
	textures.Dt[1].LoadBMP("Textures/rock.bmp");
	DrawLoad(12, "Loading Textures");
	textures.Dt[2].LoadBMP("Textures/vein.bmp");
	DrawLoad(13, "Loading Textures");
	textures.club_t.LoadBMP("Textures/club.bmp");
	DrawLoad(14, "Loading Textures");
	textures.sword_t.LoadBMP("Textures/sword.bmp");
	DrawLoad(15, "Loading Textures");
	textures.potion_t.LoadBMP("Textures/potion.bmp");
	DrawLoad(16, "Loading Textures");
	textures.spear_t.LoadBMP("Textures/spear.bmp");
	DrawLoad(17, "Loading Textures");
	textures.plant_t.LoadBMP("Textures/plant.bmp");
	textures.riddle_bg.LoadBMP("Textures/riddlebg.bmp");
	ui.rid = std::make_unique<Riddle>();

	DrawLoad(20, "Loading Monster Models [Player]");
	Player = std::make_unique<PlayerEntity>(0, 0, 1, 1, 1, 0);
	Player->LoadMDL("human", textures.player_t, textures.progBar, true);
	Player->scale = 15;
	Player->setCords(0, 0);

	DrawLoad(30, "Loading Monster Models [Worm]");
	monsters.worm = std::make_unique<monster>(0, 0, 1, 40, 15, 1500);
	monsters.worm->LoadMDL("worm", textures.worm_t, textures.progBar, true);
	monsters.worm->scale = 18;
	monsters.worm->MaxHP = 20;

	DrawLoad(40, "Loading Monster Models [Scarab]");
	monsters.scarab = std::make_unique<monster>(0, 0, 2, 25, 3, 500);
	monsters.scarab->LoadMDL("scarab", textures.scarab_t, textures.progBar, true);
	monsters.scarab->scale = 10;
	monsters.scarab->rotA = 180;
	monsters.scarab->MaxHP = 15;
	monsters.scarab->setBloodColor(0.6f, 0.1f, 0.8f); // Purple blood

	DrawLoad(50, "Loading Monster Models [Anubis]");
	monsters.anubis = std::make_unique<monster>(0, 0, 3, 200, 50, 10000);
	monsters.anubis->LoadMDL("anubis", textures.anubis_t, textures.progBar, true);
	monsters.anubis->scale = 19;
	monsters.anubis->rotA = 180;
	monsters.anubis->MaxHP = 200;

	DrawLoad(60, "Loading Item Models [Treasure chest]");
	items.chest = std::make_unique<item>();
	items.chest->LoadMDL("Models/tchest.mdl", textures.chest_t);
	items.chest->scale = 8;
	items.chest->rotA = -90;

	DrawLoad(65, "Loading Monster Models [Man-eater plant]");
	monsters.plant = std::make_unique<monster>(0, 0, 0, 50, 5, 1000);
	monsters.plant->LoadMDL("plant", textures.plant_t, textures.progBar, true);
	monsters.plant->scale = 12;
	monsters.plant->MaxHP = 30;
	monsters.plant->setBloodColor(0.1f, 0.4f, 0.1f); // Dark green blood

	DrawLoad(70, "Loading Item Models [Club]");
	items.club = std::make_unique<item>();
	items.club->LoadMDL("Models/club.mdl", textures.club_t);
	items.club->damage = 9;
	items.club->scale = 6;
	items.club->range = 2;

	DrawLoad(74, "Loading Item Models [Sword]");
	items.sword = std::make_unique<item>();
	items.sword->LoadMDL("Models/sword.mdl", textures.sword_t);
	items.sword->scale = 9;
	items.sword->damage = 35;
	items.sword->range = 4;

	DrawLoad(76, "Loading Item Models [Bow]");
	items.bow = std::make_unique<item>();
	items.bow->LoadMDL("Models/bow.mdl", textures.bow_t);
	items.bow->scale = 12;
	items.bow->damage = 12;
	items.bow->range = 16;

	DrawLoad(77, "Loading Item Models [Bow]");
	items.spear = std::make_unique<item>();
	items.spear->LoadMDL("Models/spear.mdl", textures.spear_t);
	items.spear->scale = 15;
	items.spear->damage = 15;
	items.spear->range = 8;

	DrawLoad(78, "Loading Item Models [Potion]");
	items.potion = std::make_unique<item>();
	items.potion->LoadMDL("Models/potion.mdl", textures.potion_t);
	items.potion->scale = 5;

	textures.sphinx_t.LoadBMP("Textures/sphinx.bmp");
	models.sphinx = std::make_unique<AnimatedCartoonModel>();
	models.sphinx->Load("Models/sphinx.mdl");
	models.sphinx->BindTexture(textures.sphinx_t.ID());
	models.sphinx->Centrify();
	models.sphinx->Compile();

	textures.ankh_t.LoadBMP("Textures/ankh.bmp");
	models.ankh = std::make_unique<AnimatedCartoonModel>();
	models.ankh->Load("Models/ankh.mdl");
	models.ankh->BindTexture(textures.ankh_t.ID());
	models.ankh->Centrify();
	models.ankh->Compile();

	textures.column_t.LoadBMP("Textures/columns.bmp");
	models.column = std::make_unique<AnimatedCartoonModel>();
	models.column->Load("Models/columns.mdl");
	models.column->BindTexture(textures.column_t.ID());
	models.column->Centrify();
	models.column->Compile();

	models.question = std::make_unique<AnimatedCartoonModel>();
	models.question->Load("Models/questionmark.mdl");
	models.question->BindTexture(textures.scarab_t.ID());
	models.question->Centrify();
	models.question->Compile();

	textures.plasma_t.LoadBMP("Textures/plasma.bmp");

	DrawLoad(85, "Loading inventory");
	ui.invent = std::make_unique<inventory>();
	sounds.drink_s.LoadWAV("Sounds/Drink.wav");
	sounds.jump_s.LoadWAV("Sounds/Jump.wav");

	DrawLoad(88, "Loading stats");
	ui.Stats = std::make_unique<stats>();

	textures.trap_t.LoadBMP("Textures/spikes.bmp");
	traps.TrapD = std::make_unique<trap>();
	traps.TrapD->LoadMDL("Models/spikes.mdl", textures.trap_t);
	traps.TrapD->scale = 16;

	traps.DeathTrap = std::make_unique<trap>();
	traps.DeathTrap->LoadMDL("Models/spikes.mdl", textures.trap_t);
	traps.DeathTrap->scale = 40;

	DrawLoad(95, "Loading game font");
	fonts.font.Load("Fonts/papyrus.bmp", 3, -0.3);

	Player->jump.jump_timer = std::make_unique<timer>(JUMP_TIMER_MS);
	Player->jump.jump_up_timer = std::make_unique<timer>(JUMP_UP_TIMER_MS);
	timers.mdlChange = std::make_unique<timer>(300);
	timers.AttTimer = std::make_unique<timer>(250);
	Player->jump.jump_inc = std::make_unique<timer>(JUMP_TICK_MS);
	Player->jump.fall_inc = std::make_unique<timer>(FALL_TICK_MS);
	status_timer = std::make_unique<timer>(3000);

	DrawLoad(95, "Loading game Map");

	snprintf(mapName2, sizeof(mapName2), "Levels/lvl%d", curMap);
	if (!dungeon.Load(mapName2))
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
void Cashe::DrawLoad(float xxx, const char text[]) {
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
void Cashe::Save(const char filename[]) {
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
void Cashe::LoadSave(const char filename[]) {
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
	LOG_INFO("game", "Done loading map");
	dump.close();
}
//==============================================================
