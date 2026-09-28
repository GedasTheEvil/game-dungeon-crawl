#ifndef TEXTURE_REGISTRY_H
#define TEXTURE_REGISTRY_H

#include "textures.h"

struct TextureRegistry {
	Texture nullTex;
	Texture trap_t, sphinx_t;
	Texture bg, ankh_t, question_t;
	Texture load_bg, riddle_bg, plasma_t;
	Texture progBar;
};

#endif
