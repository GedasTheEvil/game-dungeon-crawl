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
	// pose 0..1 through the model's frames (the bow's draw, items.py); the other items have one frame.
	void Draw(float pose = 0.f);
	// models/items/<name>.md3 with textures/items/<name>.png.
	bool loadModel(const char* name);
};

#endif
