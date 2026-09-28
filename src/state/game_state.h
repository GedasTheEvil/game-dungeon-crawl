#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "../entities/monster.h"
#include "../entities/player.h"
#include "../graphics/texture_registry.h"
#include "../graphics/animated_model.h"
#include "../world/dungeon.h"
#include "../graphics/font.h"
#include "../entities/item.h"
#include "../ui/inventory.h"
#include "../core/sound.h"
#include "../entities/player_stats.h"
#include "../core/timer.h"
#include "../entities/trap.h"
#include "../ui/riddle.h"
#include "../ui/menu.h"
#include "../ui/end_screens.h"
#include "../ui/map_view.h"
#include <array>
#include <memory>
#include <string>

struct SaveName {
	char name[25];
};

struct Camera {
	float rotW = -110.f;
	float rotM = 0.f;
	float rotN = 0.f;
};

struct RenderSettings {
	bool Cartoon = false; // toon shading off by default (F1 toggles)
	int resX = 800;
	int resY = 500;
};

struct SoundBank {
	Sound drink_s;
	Music soundtrack;
	Sound keyPickup, gateOpen, gateLocked, lever, rockRumble, rockCrash;
};

struct FontPair {
	Font font;
	Font load_font;
	Font status; // proportional, for the gameplay status message
	Font hud;	 // bold digits for the HUD level gem
};

struct ItemPrototypes {
	std::unique_ptr<Item> chest, club, sword, bow, potion, spear;
};

struct TrapPair {
	std::unique_ptr<Trap> TrapD;
	std::unique_ptr<Trap> DeathTrap;
};

struct SceneModels {
	std::unique_ptr<AnimatedModel> sphinx, ankh, question;
};

struct DecorSet {
	Texture tex[DECOR_COUNT];
	std::unique_ptr<AnimatedModel> model[DECOR_COUNT]; // null if the file failed to load
	Texture decalTex;								   // atlas, DECAL_DEFS order
	Texture torchTex;
	std::unique_ptr<AnimatedModel> torch; // null if the file failed to load
	Texture ladderTex[LADDER_STYLE_COUNT][LADDER_PIECE_COUNT];
	Texture wallTex[WALL_STYLE_COUNT], floorTex[FLOOR_STYLE_COUNT], ceilingTex[CEILING_STYLE_COUNT], rockTex;
	std::unique_ptr<AnimatedModel> ladder[LADDER_STYLE_COUNT][LADDER_PIECE_COUNT]; // null if the file failed to load
};

// Keys, gates, levers and rock falls: static models in tile units like the props (tools/blender/models/mechanism.py).
// Keys, gates and lever plates have one texture per lock colour (index colour - 1). A compiled model keeps its
// texture, so each colour is its own copy of the model. Null if the file failed to load.
struct MechanismSet {
	Texture keyTex[LOCK_COLOUR_COUNT], gateTex[LOCK_COLOUR_COUNT], leverBaseTex[LOCK_COLOUR_COUNT];
	Texture leverHandleTex, rockTex, crackTex;
	std::unique_ptr<AnimatedModel> key[LOCK_COLOUR_COUNT], gate[LOCK_COLOUR_COUNT], leverBase[LOCK_COLOUR_COUNT];
	std::unique_ptr<AnimatedModel> leverHandle, rock, crack;
};

struct GameTimers {
	Timer idleModel{300};  // back to the idle clip after walking
	Timer weaponRest{250}; // the weapon swings back after an attack
};

struct UIContext {
	std::unique_ptr<Inventory> inventory;
	std::unique_ptr<Riddle> riddle;
	MainMenu menu;
	std::unique_ptr<EndScreens> endScreens;
	DraftMap map;
};

class GameState {
  public:
	TextureRegistry textures;
	Camera camera;
	RenderSettings render;
	bool cacheLoaded = false;
	bool hasWon = false;
	int curMap = 1;
	std::string status; // the gameplay status message, shown for STATUS_MS after ShowStatus
	Timer statusTimer{STATUS_MS};
	SoundBank sounds;
	FontPair fonts;
	// By MonsterTypeId (level.h); index 0 is unused.
	std::array<MonsterType, MONSTER_TYPE_MAX + 1> monsterTypes;
	ItemPrototypes items;
	std::unique_ptr<Player> player;
	TrapPair traps;
	SceneModels models;
	DecorSet decor;
	MechanismSet mechanisms;
	GameTimers timers;
	UIContext ui;
	Dungeon dungeon;
	SaveName saveNames[6] = {};

	GameState();
	~GameState();
	void Load();
	void DrawLoad(float xxx, const char text[]);
	void Save(const char filename[]);
	void LoadSave(const char filename[]);
	void NewGame();
	[[gnu::format(printf, 2, 3)]] void ShowStatus(const char* format, ...);
	static constexpr int STATUS_MS = 3000;
};

// The one game state, created in main() before the window and destroyed after the main loop.
void CreateGame();
void DestroyGame();
GameState& Game();

#endif
