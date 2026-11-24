#include "trap.h"
#include "../render/gl/matrix_stack.h"

Trap::Trap() : mdl(std::make_shared<AnimatedModel>()) {}

void Trap::Show(float size) {
	gMv.push();
	gMv.translate(0, 0, -30);
	gMv.scale(size, size, size);
	if (tex != nullptr)
		tex->Bind();
	mdl->Show();
	gMv.pop();
}

bool Trap::loadModel(const char filename[], const Texture& texture, bool compile) {
	tex = &texture;

	mdl->Load(filename);
	mdl->BindTexture(texture.ID());
	mdl->Centrify();

	if (compile)
		mdl->Compile();
	return true;
}
