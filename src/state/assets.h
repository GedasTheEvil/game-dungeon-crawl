#ifndef ASSETS_H
#define ASSETS_H

#include "../entities/character_model.h"
#include "../entities/monster.h"
#include "../entities/item.h"
#include "../entities/trap.h"
#include "../graphics/texture_registry.h"
#include "../graphics/animated_model.h"
#include "../graphics/font.h"
#include "../core/sound.h"
#include "../world/containers.h"
#include "../world/decor.h"
#include "../world/items.h"
#include "../world/level.h"
#include <array>
#include <functional>
#include <memory>

struct SoundBank {
	Sound drink_s;
	Sound amulet_s; // an amulet put on or taken off
	Music soundtrack;
	Sound keyPickup, gateOpen, gateLocked, lever, rockRumble, rockCrash, teleport, plateClick, dart;
	Sound sandHiss, sandPour, sandStep; // a sand fall starts, pours down; a step in sand (plan desert-chapter)
	Sound spikeScrape, spikeThunk; // wall spikes about to come out, coming out (plan rhythm-traps)
	Sound mirrorTurn, sunDisc; // a mirror turned, a sun disc lit (plan sunlight-mirrors)
	Sound dodge; // a whoosh (the sling swing): the player dodges a blow
	Sound lidWood, lidReed, lidStone, jarSmash; // a treasure container opens (WorldSound)
	Sound arrowHit, arrowWall; // an arrow in a monster, in a wall or the floor
	Sound stoneHit, stoneWall; // a sling stone or throwing stick in a monster, off a wall
	Sound summonDig, summonDrop; // a boss's minion digs out of the floor, drops from the ceiling (Summon)
	Sound wade, splash; // a step in half water, landing in it
	Sound pageTurn; // the journal's paper rustle
};

struct FontSet {
	Font font;
	Font loading;
	Font status; // proportional, for the gameplay status message
	Font hud; // bold digits for the HUD level gem
	Font hudBody; // the player HUD's health numbers
	Font hudSmall; // the player HUD's key caps and counts
};

struct ItemPrototypes {
	std::array<std::unique_ptr<Item>, CONTAINER_COUNT> containers; // by Container (containers.h)
	std::array<std::unique_ptr<Item>, WEAPON_KIND_COUNT> weapons; // in ItemKind order
	std::array<std::unique_ptr<Item>, POTION_KIND_COUNT> potions; // in ItemKind order; one vessel's share its model
	std::array<std::unique_ptr<Item>, AMULET_TYPE_COUNT> amulets; // by AmuletType: its tiers share the model
	[[nodiscard]] Item* Of(ItemKind kind) const;
	// The missiles in flight, by MissileKind: static models in metres (items.py), not centred. Null if missing.
	std::array<Texture, MISSILE_KIND_COUNT> missileTex;
	std::array<std::unique_ptr<AnimatedModel>, MISSILE_KIND_COUNT> missiles;
};

struct TrapSet {
	std::unique_ptr<Trap> spikes;
	std::unique_ptr<Trap> deathTrap; // big spikes on a death tile: the spikes' model, scaled up
};

struct SceneModels {
	std::unique_ptr<AnimatedModel> sphinx, ankh, question, columns; // columns: the teleporter
};

struct DecorSet {
	Texture tex[DECOR_COUNT];
	std::unique_ptr<AnimatedModel> model[DECOR_COUNT]; // null if the file failed to load
	Texture decalTex; // atlas, DECAL_DEFS order
	Texture inscriptionTex; // atlas, a cell per inscription id (inscriptions.h)
	Texture nameCartoucheTex; // atlas of the name cartouches' pictures (names.h)
	Texture torchTex;
	std::unique_ptr<AnimatedModel> torch; // null if the file failed to load
	Texture ladderTex[LADDER_STYLE_COUNT][LADDER_PIECE_COUNT];
	Texture wallTex[WALL_STYLE_COUNT], floorTex[FLOOR_STYLE_COUNT], ceilingTex[CEILING_STYLE_COUNT], rockTex;
	std::unique_ptr<AnimatedModel> ladder[LADDER_STYLE_COUNT][LADDER_PIECE_COUNT]; // null if the file failed to load
};

// Keys, gates, levers, rock falls and the sunlight mirrors: static models in tile units like the props
// (tools/blender/models/mechanism.py). Keys, gates, lever plates and sun discs have one model and one texture per lock
// colour (index colour - 1), bound at draw time; the boss gate (BOSS_LOCK) is the gate model with its own texture. Null
// if the file failed to load.
struct MechanismSet {
	Texture keyTex[LOCK_COLOUR_COUNT], gateTex[LOCK_COLOUR_COUNT], leverBaseTex[LOCK_COLOUR_COUNT];
	Texture leverHandleTex, rockTex, crackTex, bossGateTex, plateTex, dartHolesTex, spikeSlotsTex, wallSpikesTex;
	Texture sunDiscTex[LOCK_COLOUR_COUNT], mirrorStandTex, mirrorTex, mirrorPointerTex; // plan sunlight-mirrors
	std::unique_ptr<AnimatedModel> key, gate, leverBase;
	std::unique_ptr<AnimatedModel> leverHandle, rock, crack, plate, dartHoles, spikeSlots, wallSpikes;
	std::unique_ptr<AnimatedModel> sunDisc, mirrorStand, mirror, mirrorPointer;
};

// Everything loaded once at start-up and only read afterwards: textures, models, sounds, fonts, monster types.
struct Assets {
	TextureRegistry textures;
	SoundBank sounds;
	FontSet fonts;
	// By MonsterTypeId (level.h); index 0 is unused. The sim's table (the kind and its ModelInfo) and the drawing's.
	MonsterTypes monsterTypes;
	std::array<CharacterModel, MONSTER_TYPE_MAX + 1> monsterModels;
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
