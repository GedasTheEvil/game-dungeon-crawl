#include "item.h"
#include <GL/gl.h>
#include "../graphics/ink.h"
#include <algorithm>
#include <string>

void Item::Draw(float pose) { drawScaled(scale * Ink::figureScale(), pose); }

void Item::DrawHeld(float pose) { drawScaled(scale * Ink::heldWeaponScale(), pose); }

void Item::drawScaled(float drawScale, float pose) {
	if (!mdl)
		return;

	glPushMatrix();
	glTranslatef(0, 0, -DRAW_DEPTH);
	glScalef(drawScale, drawScale, drawScale);
	tex.Bind();
	glRotatef(rotA, 0, 1, 0);
	AnimPlayback playback;
	playback.frame = std::clamp(pose, 0.f, 1.f) * static_cast<float>(mdl->FrameCount() - 1);
	mdl->Show(playback, tex.ID());
	glPopMatrix();
}

bool Item::loadModel(const char* name, const char* texture) {
	tex.LoadPNG(("textures/items/" + std::string(texture != nullptr ? texture : name) + ".png").c_str());
	mdl = std::make_shared<AnimatedModel>();
	mdl->Load(("models/items/" + std::string(name) + ".md3").c_str());
	mdl->Centrify();
	mdl->BindTexture(tex.ID());
	mdl->Compile();
	return true;
}

void Item::shareModel(const Item& owner, const char* texture) {
	tex.LoadPNG(("textures/items/" + std::string(texture) + ".png").c_str());
	mdl = owner.mdl;
}
