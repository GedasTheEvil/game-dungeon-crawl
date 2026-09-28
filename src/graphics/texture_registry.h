#ifndef TEXTURE_REGISTRY_H
#define TEXTURE_REGISTRY_H

#include "textures.h"

// Textures that are not part of a model set (Assets): UI backgrounds, props, effects.
struct TextureRegistry {
	Texture nullTex; // plain white, for untextured quads and particles
	Texture spikes, sphinx, ankh, questionMark;
	Texture portal; // the scrolling entrance / exit portal
	Texture papyrus, loadingBackground, riddleBackground;
	Texture loadingBar; // also the monster health bars
};

#endif
