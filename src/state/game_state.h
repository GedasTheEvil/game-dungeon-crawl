#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "../entities/monster.h"
#include "../entities/player.h"
#include "../graphics/texture_registry.h"
#include "../graphics/ani.h"
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
#include "../ui/map_view.h"
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
	bool Cartoon = false; // toon shading off by default (F1 toggles)
	int resX = 800;
	int resY = 500;
};

struct SoundBank {
	Sound drink_s;
	Sound jump_s;
	Sound soundtrack;
	Sound keyPickup, gateOpen, gateLocked, lever, rockRumble, rockCrash;
};

struct FontPair {
	Font font;
	Font load_font;
	Font status; // proportional, for the gameplay status message
	Font hud;	 // bold digits for the HUD level gem
};

struct MonsterPrototypes {
	std::unique_ptr<monster> anubis, scarab, plant, worm, rat, giantRat, bat, giantBat;
};

struct ItemPrototypes {
	std::unique_ptr<item> chest, club, sword, bow, potion, spear;
};

struct TrapPair {
	std::unique_ptr<trap> TrapD;
	std::unique_ptr<trap> DeathTrap;
};

struct SceneModels {
	std::unique_ptr<AnimatedModel> sphinx, ankh, question;
};

struct DecorSet {
	Textura tex[DECOR_COUNT];
	std::unique_ptr<AnimatedModel> model[DECOR_COUNT]; // null if the file failed to load
	Textura decalTex;								   // atlas, DECAL_DEFS order
	Textura torchTex;
	std::unique_ptr<AnimatedModel> torch; // null if the file failed to load
	Textura ladderTex[LADDER_STYLE_COUNT][LADDER_PIECE_COUNT];
	Textura wallTex[WALL_STYLE_COUNT], floorTex[FLOOR_STYLE_COUNT], ceilingTex[CEILING_STYLE_COUNT], rockTex;
	std::unique_ptr<AnimatedModel> ladder[LADDER_STYLE_COUNT][LADDER_PIECE_COUNT]; // null if the file failed to load
};

// Keys, gates, levers and rock falls: static models in tile units like the props (tools/blender/models/mechanism.py).
// Keys, gates and lever plates have one texture per lock colour (index colour - 1). A compiled model keeps its
// texture, so each colour is its own copy of the model. Null if the file failed to load.
struct MechanismSet {
	Textura keyTex[LOCK_COLOUR_COUNT], gateTex[LOCK_COLOUR_COUNT], leverBaseTex[LOCK_COLOUR_COUNT];
	Textura leverHandleTex, rockTex, crackTex;
	std::unique_ptr<AnimatedModel> key[LOCK_COLOUR_COUNT], gate[LOCK_COLOUR_COUNT], leverBase[LOCK_COLOUR_COUNT];
	std::unique_ptr<AnimatedModel> leverHandle, rock, crack;
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
	DraftMap map;
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
	DecorSet decor;
	MechanismSet mechanisms;
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
	void NewGame();
};

#endif
