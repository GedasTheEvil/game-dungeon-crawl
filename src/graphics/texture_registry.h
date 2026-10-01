#ifndef TEXTURE_REGISTRY_H
#define TEXTURE_REGISTRY_H

#include "textures.h"

// Textures that are not part of a model set (Assets): UI backgrounds, props, effects.
struct TextureRegistry {
	Texture nullTex; // plain white, for untextured quads and particles
	Texture spikes, sphinx, ankh, questionMark, columns;
	Texture portal; // the scrolling plasma of the entrance, exit and teleporter gates
	Texture papyrus, loadingBackground, riddleBackground;
	Texture hudIcons;	// the player HUD's quick slot icons
	Texture ribbon;		// the journal's bookmark ribbons, tinted per section
	Texture loadingBar; // also the monster health bars
};

#endif
