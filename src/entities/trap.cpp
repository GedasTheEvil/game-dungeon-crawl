#include "trap.h"
#include <GL/gl.h>

Trap::Trap() : mdl(std::make_unique<AnimatedModel>()) {}

void Trap::Show() {
	glPushMatrix();
	glTranslatef(0, 0, -30);
	glPushMatrix();
	glScalef(scale, scale, scale);
	tex.Bind();
	mdl->Show();
	glPopMatrix();
	glPopMatrix();
}

bool Trap::loadModel(const char filename[], Texture& texture, bool compile) {
	tex = texture;

	mdl->Load(filename);
	mdl->BindTexture(texture.ID());
	mdl->Centrify();

	if (compile)
		mdl->Compile();
	return true;
}
