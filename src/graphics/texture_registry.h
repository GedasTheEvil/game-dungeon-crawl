#ifndef TEXTURE_REGISTRY_H
#define TEXTURE_REGISTRY_H

#include "textures.h"

struct TextureRegistry {
	Textura monsterTextures[4];
	Textura nullTex;
	Textura blackTex;
	Textura column_t;
	Textura anubis_t, scarab_t, plant_t, worm_t, chest_t, player_t;
	Textura club_t, bow_t, sword_t, potion_t, spear_t, trap_t, sphinx_t;
	Textura Dt[9];
	Textura bg, black_t, ankh_t;
	Textura load_bg, riddle_bg, plasma_t, menu_bg, menu_save_bg;
	Textura progBar;
};

#endif
