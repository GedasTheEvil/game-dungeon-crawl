#ifndef Cache_H
#define Cache_H

/// @file cache.h
// shared class for all game fun resourses
// making something private would beat the purpose :)

#include "monster.h"
#include "textures.h"
#include "shader.h"
#include "Dungeon.h"
#include "font.h"
#include "item.h"
#include "inventory.h"
#include "sound.h"
#include "stats.h"
#include "timer.h"
#include "Dungeon.h"
#include "trap.h"
#include "riddle.h"
#include "menu.h"
#include "winloose.h"
#include <memory>

struct word {
	char name[25];
};

class Cashe {
  public:
	char status[255];
	std::unique_ptr<timer> status_timer;
	bool IHaveWon;
	Textura Mt[4], nullTex, blackTex, column_t;
	Textura anubis_t, scarab_t, plant_t, worm_t, chest_t, player_t;
	Textura club_t, bow_t, sword_t, potion_t, spear_t, trap_t, sphinx_t;
	Textura Dt[9];
	Textura bg, black_t, ankh_t;
	Textura load_bg, riddle_bg, plasma_t, menu_bg, menu_save_bg;
	Textura progBar;
	Sound ss[2];
	Sound drink_s, jump_s;
	Sound soundtrack;
	Font font;
	Font load_font;
	bool Cache_loaded;
	monster *anubis, *scarab, *plant, *worm;
	item *chest, *club, *sword, *bow, *potion, *spear;
	std::unique_ptr<inventory> invent;
	std::unique_ptr<stats> Stats;
	monster* Player;
	std::unique_ptr<timer> jump_timer;
	std::unique_ptr<timer> jump_up_timer;
	std::unique_ptr<timer> jump_inc;
	std::unique_ptr<timer> fall_inc;
	bool jumping;
	float jump_dir_x;
	float jump_speed;
	float jump_vel;
	float jump_start_y;
	bool Orig_model;
	std::unique_ptr<trap> TrapD;
	std::unique_ptr<trap> DeathTrap;
	CartoonANI *sphinx, *ankh;
	CartoonANI* column;
	CartoonANI* question;
	std::unique_ptr<timer> mdlChange;
	std::unique_ptr<timer> AttTimer;
	std::unique_ptr<Riddle> rid;
	int jump_counter;
	int curMap;
	bool falling;
	bool Cartoon;
	Dungeon dungeon;
	MainMenu menu;

	word saveNames[6];

	std::unique_ptr<winL> wlc;

	Cashe();
	~Cashe();
	void Load();
	void DrawLoad(float xxx, const char text[]);
	void Save(const char filename[]);
	void LoadSave(const char filename[]);
};

#endif