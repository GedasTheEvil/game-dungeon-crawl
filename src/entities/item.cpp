#include "item.h"
#include <GL/gl.h>
#include "../graphics/ink.h"
#include <string>

void Item::Draw() {
	if (!mdl)
		return;

	glPushMatrix();
	glTranslatef(0, 0, -30);
	const float drawScale = scale * Ink::figureScale();
	glScalef(drawScale, drawScale, drawScale);
	tex.Bind();
	glRotatef(rotA, 0, 1, 0);
	mdl->Show();
	glPopMatrix();
	mdl->Advance();
}

bool Item::loadModel(const char* name) {
	const std::string stem = std::string("items/") + name;
	tex.LoadPNG(("textures/" + stem + ".png").c_str());
	mdl = std::make_unique<AnimatedModel>();
	mdl->Load(("models/" + stem + ".md3").c_str());
	mdl->Centrify();
	mdl->BindTexture(tex.ID());
	mdl->Compile();
	return true;
}
