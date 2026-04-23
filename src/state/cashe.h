#ifndef Cache_H
#define Cache_H

/// @file cache.h
// shared class for all game fun resourses
// making something private would beat the purpose :)

#include "../entities/monster.h"
#include "../graphics/textures.h"
#include "../graphics/shader.h"
#include "../world/Dungeon.h"
#include "../graphics/font.h"
#include "../entities/item.h"
#include "../ui/inventory.h"
#include "../core/sound.h"
#include "../ui/stats.h"
#include "../core/timer.h"
#include "../entities/trap.h"
#include "../ui/riddle.h"
#include "../ui/menu.h"
#include "../ui/winloose.h"
#include <memory>

struct word {
	char name[25];
};

class Cashe {
  public:
	char status[255] = {};
	std::unique_ptr<timer> status_timer;
	bool IHaveWon = false;
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
	bool Cache_loaded = false;
	std::unique_ptr<monster> anubis, scarab, plant, worm;
	std::unique_ptr<item> chest, club, sword, bow, potion, spear;
	std::unique_ptr<inventory> invent;
	std::unique_ptr<stats> Stats;
	std::unique_ptr<monster> Player;
	std::unique_ptr<timer> jump_timer;
	std::unique_ptr<timer> jump_up_timer;
	std::unique_ptr<timer> jump_inc;
	std::unique_ptr<timer> fall_inc;
	bool jumping = false;
	float jump_dir_x = 0.0f;
	float jump_speed = 0.0f;
	float jump_vel = 0.0f;
	float jump_start_y = 0.0f;
	bool Orig_model = true;
	std::unique_ptr<trap> TrapD;
	std::unique_ptr<trap> DeathTrap;
	std::unique_ptr<CartoonANI> sphinx, ankh;
	std::unique_ptr<CartoonANI> column;
	std::unique_ptr<CartoonANI> question;
	std::unique_ptr<timer> mdlChange;
	std::unique_ptr<timer> AttTimer;
	std::unique_ptr<Riddle> rid;
	int jump_counter = 0;
	int curMap = 1;
	bool falling = false;
	bool Cartoon = true;
	Dungeon dungeon;
	MainMenu menu;

	word saveNames[6] = {};

	std::unique_ptr<winL> wlc;

	Cashe();
	~Cashe();
	void Load();
	void DrawLoad(float xxx, const char text[]);
	void Save(const char filename[]);
	void LoadSave(const char filename[]);
};

#endif