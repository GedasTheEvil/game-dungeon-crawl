#ifndef TEXTURE_REGISTRY_H
#define TEXTURE_REGISTRY_H

#include "textures.h"

// Textures that are not part of a model set (Assets): UI backgrounds, props, effects.
struct TextureRegistry {
	Texture nullTex; // plain white, for untextured quads and particles
	Texture spikes, sphinx, ankh, questionMark, columns;
	Texture portal,
		portalRed; // the scrolling plasma of the entrance and teleporter gates (blue), of the exit gate (red)
	Texture papyrus, loadingBackground, riddleBackground;
	Texture hudIcons; // the player HUD's quick slot icons
	Texture skillIcons; // the heka mark and the skills (Heka screen, HUD)
	Texture ribbon; // the journal's bookmark ribbons, tinted per section
	Texture expeditionPage; // a torn notebook page of the earlier expedition: on a skeleton, in the journal
	Texture journalPaper, journalCloth; // the journal's grid paper and its cover cloth
	Texture loadingBar; // also the monster health bars
};

#endif
