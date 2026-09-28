#include "item.h"
#include <GL/gl.h>
#include "../graphics/ink.h"

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
	mdl->Advance_Animation();
}

bool Item::loadModel(const char filename[], Texture& texture, bool compile) {
	tex = texture;
	mdl = std::make_unique<AnimatedModel>();
	mdl->Load(filename);
	mdl->Centrify();
	mdl->BindTexture(tex.ID());
	if (compile)
		mdl->Compile();
	return true;
}
