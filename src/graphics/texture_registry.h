#ifndef TEXTURE_REGISTRY_H
#define TEXTURE_REGISTRY_H

#include "textures.h"

struct TextureRegistry {
	Textura monsterTextures[4];
	Textura nullTex;
	Textura blackTex;
	Textura anubis_t, scarab_t, plant_t, worm_t, rat_t, giantRat_t, bat_t, giantBat_t, chest_t, player_t;
	Textura club_t, bow_t, sword_t, potion_t, spear_t, trap_t, sphinx_t;
	Textura Dt[9];
	Textura bg, black_t, ankh_t, gold_t;
	Textura load_bg, riddle_bg, plasma_t;
	Textura progBar;
};

#endif
