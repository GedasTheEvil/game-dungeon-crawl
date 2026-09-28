#ifndef TEXTURE_REGISTRY_H
#define TEXTURE_REGISTRY_H

#include "textures.h"

struct TextureRegistry {
	Texture nullTex;
	Texture anubis_t, scarab_t, plant_t, worm_t, rat_t, giantRat_t, bat_t, giantBat_t, chest_t, player_t;
	Texture club_t, bow_t, sword_t, potion_t, spear_t, trap_t, sphinx_t;
	Texture bg, ankh_t, question_t;
	Texture load_bg, riddle_bg, plasma_t;
	Texture progBar;
};

#endif
