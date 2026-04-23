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
#include <memory>
#include "../input/gameplay_config.h"

char mapName2[200];

Cashe::Cashe() = default;

Cashe::~Cashe() { printf("Deleting cashe %p \n", (void*)this); }

void Cashe::Load() {
	// init main load resourses
	load_font.Load("Fonts/papyrus.bmp", 7, -1.0);
	load_bg.LoadBMP("Textures/scarab_slate.bmp");
	bg.LoadBMP("Textures/papyrus_sheet.bmp");
	progBar.LoadBMP("Textures/loading.bmp");
	nullTex.LoadBMP("Textures/null.bmp");
	blackTex.LoadBMP("Textures/wallback.bmp");
	black_t.LoadBMP("Textures/black.bmp");
	DrawLoad(4, "Loading Textures");
	menu_bg.LoadBMP("Textures/menu_main.bmp");
	menu_save_bg.LoadBMP("Textures/menu_save.bmp");
	player_t.LoadBMP("Textures/player.bmp");
	DrawLoad(5, "Loading Textures");
	anubis_t.LoadBMP("Textures/anubis.bmp");
	DrawLoad(6, "Loading Textures");
	worm_t.LoadBMP("Textures/worm.bmp");
	DrawLoad(7, "Loading Textures");
	scarab_t.LoadBMP("Textures/scarab.bmp");
	DrawLoad(8, "Loading Textures");
	bow_t.LoadBMP("Textures/scarab.bmp");
	DrawLoad(9, "Loading Textures");
	chest_t.LoadBMP("Textures/tchest.bmp");
	DrawLoad(10, "Loading Textures");
	Dt[0].LoadBMP("Textures/sand.bmp");
	DrawLoad(11, "Loading Textures");
	Dt[1].LoadBMP("Textures/rock.bmp");
	DrawLoad(12, "Loading Textures");
	Dt[2].LoadBMP("Textures/vein.bmp");
	DrawLoad(13, "Loading Textures");
	club_t.LoadBMP("Textures/club.bmp");
	DrawLoad(14, "Loading Textures");
	sword_t.LoadBMP("Textures/sword.bmp");
	DrawLoad(15, "Loading Textures");
	potion_t.LoadBMP("Textures/potion.bmp");
	DrawLoad(16, "Loading Textures");
	spear_t.LoadBMP("Textures/spear.bmp");
	DrawLoad(17, "Loading Textures");
	plant_t.LoadBMP("Textures/plant.bmp");
	riddle_bg.LoadBMP("Textures/riddlebg.bmp");
	rid = std::make_unique<Riddle>();

	DrawLoad(20, "Loading Monster Models [Player]");
	Player = std::make_unique<monster>(0, 0, 1, 1, 1, 0);
	Player->LoadMDL("human", player_t, progBar, true);
	Player->scale = 15;
	Player->setCords(0, 0);

	DrawLoad(30, "Loading Monster Models [Worm]");
	worm = std::make_unique<monster>(0, 0, 1, 40, 15, 1500);
	worm->LoadMDL("worm", worm_t, progBar, true);
	worm->scale = 18;
	worm->MaxHP = 20;

	DrawLoad(40, "Loading Monster Models [Scarab]");
	scarab = std::make_unique<monster>(0, 0, 2, 25, 3, 500);
	scarab->LoadMDL("scarab", scarab_t, progBar, true);
	scarab->scale = 10;
	scarab->rotA = 180;
	scarab->MaxHP = 15;

	DrawLoad(50, "Loading Monster Models [Anubis]");
	anubis = std::make_unique<monster>(0, 0, 3, 200, 50, 10000);
	anubis->LoadMDL("anubis", anubis_t, progBar, true);
	anubis->scale = 19;
	anubis->rotA = 180;
	anubis->MaxHP = 200;

	DrawLoad(60, "Loading Item Models [Treasure chest]");
	chest = std::make_unique<item>();
	chest->LoadMDL("Models/tchest.mdl", chest_t);
	chest->scale = 8;
	chest->rotA = -90;

	DrawLoad(65, "Loading Monster Models [Man-eater plant]");
	plant = std::make_unique<monster>(0, 0, 0, 50, 5, 1000);
	plant->LoadMDL("plant", plant_t, progBar, true);
	plant->scale = 12;
	plant->MaxHP = 30;

	DrawLoad(70, "Loading Item Models [Club]");
	club = std::make_unique<item>();
	club->LoadMDL("Models/club.mdl", club_t);
	club->damage = 9;
	club->scale = 6;
	club->range = 2;

	DrawLoad(74, "Loading Item Models [Sword]");
	sword = std::make_unique<item>();
	sword->LoadMDL("Models/sword.mdl", sword_t);
	sword->scale = 9;
	sword->damage = 35;
	sword->range = 4;

	DrawLoad(76, "Loading Item Models [Bow]");
	bow = std::make_unique<item>();
	bow->LoadMDL("Models/bow.mdl", bow_t);
	bow->scale = 12;
	bow->damage = 12;
	bow->range = 16;

	DrawLoad(77, "Loading Item Models [Bow]");
	spear = std::make_unique<item>();
	spear->LoadMDL("Models/spear.mdl", spear_t);
	spear->scale = 15;
	spear->damage = 15;
	spear->range = 8;

	DrawLoad(78, "Loading Item Models [Potion]");
	potion = std::make_unique<item>();
	potion->LoadMDL("Models/potion.mdl", potion_t);
	potion->scale = 5;

	sphinx_t.LoadBMP("Textures/sphinx.bmp");
	sphinx = std::make_unique<CartoonANI>();
	sphinx->Load("Models/sphinx.mdl");
	sphinx->BindTexture(sphinx_t.ID());
	sphinx->Centrify();
	sphinx->Compile();

	ankh_t.LoadBMP("Textures/ankh.bmp");
	ankh = std::make_unique<CartoonANI>();
	ankh->Load("Models/ankh.mdl");
	ankh->BindTexture(ankh_t.ID());
	ankh->Centrify();
	ankh->Compile();

	column_t.LoadBMP("Textures/columns.bmp");
	column = std::make_unique<CartoonANI>();
	column->Load("Models/columns.mdl");
	column->BindTexture(column_t.ID());
	column->Centrify();
	column->Compile();

	question = std::make_unique<CartoonANI>();
	question->Load("Models/questionmark.mdl");
	question->BindTexture(scarab_t.ID());
	question->Centrify();
	question->Compile();

	plasma_t.LoadBMP("Textures/plasma.bmp");

	DrawLoad(85, "Loading inventory");
	invent = std::make_unique<inventory>();
	drink_s.LoadWAV("Sounds/Drink.wav");
	jump_s.LoadWAV("Sounds/Jump.wav");

	DrawLoad(88, "Loading stats");
	Stats = std::make_unique<stats>();

	trap_t.LoadBMP("Textures/spikes.bmp");
	TrapD = std::make_unique<trap>();
	TrapD->LoadMDL("Models/spikes.mdl", trap_t);
	TrapD->scale = 16;

	DeathTrap = std::make_unique<trap>();
	DeathTrap->LoadMDL("Models/spikes.mdl", trap_t);
	DeathTrap->scale = 40;

	DrawLoad(95, "Loading game font");
	font.Load("Fonts/papyrus.bmp", 3, -0.3);

	jump_timer = std::make_unique<timer>(JUMP_TIMER_MS);
	jump_up_timer = std::make_unique<timer>(JUMP_UP_TIMER_MS);
	mdlChange = std::make_unique<timer>(300);
	AttTimer = std::make_unique<timer>(250);
	jump_inc = std::make_unique<timer>(JUMP_TICK_MS);
	fall_inc = std::make_unique<timer>(FALL_TICK_MS);
	status_timer = std::make_unique<timer>(3000);

	DrawLoad(95, "Loading game Map");

	snprintf(mapName2, sizeof(mapName2), "Levels/lvl%d", curMap);
	if (!dungeon.Load(mapName2))
		printf("Failed loading map");

	DrawLoad(100, "Loading game soundtrack");
	soundtrack.LoadOGG("Sounds/soundtrack.ogg");
	soundtrack.Play();

	std::ifstream f("Saves/gamelist.dat");
	if (f) {
		for (int a = 0; a < 6; a++)
			f >> saveNames[a].name;
		f.close();
	} else {
		printf("Failed loading save list\n");
	}

	wlc = std::make_unique<winL>();

	snprintf(status, sizeof(status), "%s", "");

	Cache_loaded = 1;
}
//==============================================================
void Cashe::DrawLoad(float xxx, const char text[]) {
	printf("DrawLoad: %s\n", text);

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
	load_bg.Bind();

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
	progBar.Bind();

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
	load_font.print(10, 15, text);
	glDisable(GL_BLEND);

	glFlush();

	glutSwapBuffers();
}
//==============================================================
void Cashe::Save(const char filename[]) {
	if (!Cache_loaded) {
		printf("can't save without loading cashe\n");
		return;
	}

	std::ofstream dump(filename);
	if (!dump) {
		printf("can't open save file %s\n", filename);
		return;
	}

	dump << curMap << " ";

	Stats->Dump(dump);
	invent->Dump(dump);
	dungeon.Dump(dump);

	dump.close();
}
//==============================================================
void Cashe::LoadSave(const char filename[]) {
	if (!Cache_loaded) {
		printf("can't load without loading cashe\n");
		return;
	}

	Player->Reanimate();

	printf("Loading save %s \n", filename);
	std::ifstream dump(filename);
	if (!dump) {
		printf("can't open save file %s\n", filename);
		return;
	}

	dump >> curMap;
	printf("Got MapNo : %d \n", curMap);

	Stats->LoadDump(dump);
	printf("Done loading Stats\n");
	invent->LoadDump(dump);
	printf("Done loading Inventory\n");
	dungeon.LoadDump(dump);
	printf("Done loading map\n");
	dump.close();
}
//==============================================================
