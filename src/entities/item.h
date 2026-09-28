#ifndef ITEM_H
#define ITEM_H

#include <memory>
#include "../graphics/animated_model.h"
#include "../graphics/textures.h"

// A weapon, potion or the treasure chest: a static model, drawn where the caller has placed it.
class Item {
  private:
	std::unique_ptr<AnimatedModel> mdl;
	Texture tex;

  public:
	int damage = 1, range = 1; // weapons only
	float rotA = 0;
	float scale = 0;
	void Draw();
	bool loadModel(const char filename[], Texture& texture, bool compile = true);
};

#endif
