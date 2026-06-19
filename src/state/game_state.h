#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "../entities/monster.h"
#include "../entities/player.h"
#include "../graphics/texture_registry.h"
#include "../graphics/shader.h"
#include "../world/dungeon.h"
#include "../graphics/font.h"
#include "../entities/item.h"
#include "../ui/inventory.h"
#include "../core/sound.h"
#include "../ui/stats.h"
#include "../core/timer.h"
#include "../entities/trap.h"
#include "../ui/riddle.h"
#include "../ui/menu.h"
#include "../ui/winlose.h"
#include <memory>

struct word {
	char name[25];
};

struct Camera {
	float rotW = -110.f;
	float rotM = 0.f;
	float rotN = 0.f;
};

struct RenderSettings {
	bool Cartoon = true;
	bool Orig_model = true;
	int resX = 800;
	int resY = 500;
};

struct SoundBank {
	Sound ss[2];
	Sound drink_s;
	Sound jump_s;
	Sound soundtrack;
};

struct FontPair {
	Font font;
	Font load_font;
};

struct MonsterPrototypes {
	std::unique_ptr<monster> anubis, scarab, plant, worm;
};

struct ItemPrototypes {
	std::unique_ptr<item> chest, club, sword, bow, potion, spear;
};

struct TrapPair {
	std::unique_ptr<trap> TrapD;
	std::unique_ptr<trap> DeathTrap;
};

struct SceneModels {
	std::unique_ptr<AnimatedCartoonModel> sphinx, ankh, column, question;
};

struct GameTimers {
	std::unique_ptr<timer> mdlChange;
	std::unique_ptr<timer> AttTimer;
};

struct UIContext {
	std::unique_ptr<inventory> invent;
	std::unique_ptr<stats> Stats;
	std::unique_ptr<Riddle> rid;
	MainMenu menu;
	std::unique_ptr<winL> wlc;
};

class GameState {
  public:
	TextureRegistry textures;
	Camera camera;
	RenderSettings render;
	bool Cache_loaded = false;
	bool IHaveWon = false;
	int curMap = 1;
	char status[255] = {};
	std::unique_ptr<timer> status_timer;
	SoundBank sounds;
	FontPair fonts;
	MonsterPrototypes monsters;
	ItemPrototypes items;
	std::unique_ptr<PlayerEntity> Player;
	TrapPair traps;
	SceneModels models;
	GameTimers timers;
	UIContext ui;
	Dungeon dungeon;
	word saveNames[6] = {};

	GameState();
	~GameState();
	void Load();
	void DrawLoad(float xxx, const char text[]);
	void Save(const char filename[]);
	void LoadSave(const char filename[]);
};

#endif
