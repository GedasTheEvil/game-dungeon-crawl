#ifndef ASSETS_H
#define ASSETS_H

#include "../entities/monster.h"
#include "../entities/item.h"
#include "../entities/trap.h"
#include "../graphics/texture_registry.h"
#include "../graphics/animated_model.h"
#include "../graphics/font.h"
#include "../core/sound.h"
#include "../world/decor.h"
#include "../world/level.h"
#include <array>
#include <functional>
#include <memory>

struct SoundBank {
	Sound drink_s;
	Music soundtrack;
	Sound keyPickup, gateOpen, gateLocked, lever, rockRumble, rockCrash;
	Sound arrowHit, arrowWall; // an arrow in a monster, in a wall or the floor
};

struct FontSet {
	Font font;
	Font loading;
	Font status; // proportional, for the gameplay status message
	Font hud;	 // bold digits for the HUD level gem
};

struct ItemPrototypes {
	std::unique_ptr<Item> chest, club, sword, bow, potion, spear;
	// The bow's arrow in flight: not an inventory item, a static model in metres (items.py). Null if missing.
	Texture arrowTex;
	std::unique_ptr<AnimatedModel> arrow;
};

struct TrapSet {
	std::unique_ptr<Trap> spikes;
	std::unique_ptr<Trap> deathTrap; // big spikes on a death tile
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

// Everything loaded once at start-up and only read afterwards: textures, models, sounds, fonts, monster types.
struct Assets {
	TextureRegistry textures;
	SoundBank sounds;
	FontSet fonts;
	// By MonsterTypeId (level.h); index 0 is unused.
	std::array<MonsterType, MONSTER_TYPE_MAX + 1> monsterTypes;
	ItemPrototypes items;
	TrapSet traps;
	SceneModels models;
	DecorSet decor;
	MechanismSet mechanisms;

	// The loading screen's own background, bar and font, before everything else.
	void LoadLoadingScreen();
	// progress(percent, text) draws the loading screen.
	void Load(const std::function<void(float, const char*)>& progress);
};

#endif
