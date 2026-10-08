#ifndef ITEM_H
#define ITEM_H

#include <memory>
#include "../core/sound.h"
#include "../graphics/animated_model.h"
#include "../graphics/textures.h"
#include "../world/damage.h"
#include "../world/items.h"

// A weapon, potion or the treasure chest: a static model, drawn where the caller has placed it. The potions that come
// in one vessel share its model, each with its own texture (shareModel).
class Item {
  private:
	std::shared_ptr<AnimatedModel> mdl;
	Texture tex;

	void drawScaled(float drawScale, float pose);

  public:
	static constexpr float DRAW_DEPTH = 30.f; // Draw() pushes the model this far back
	int damage = 1, range = 1;				  // weapons only; range in tenths of a tile (WeaponDef)
	DamageMix mix = {100, 0, 0};			  // weapons only
	[[nodiscard]] float Reach() const { return 0.1f * static_cast<float>(range); } // tiles: melee reach, bow aim
	WeaponMotion motion;														   // weapons only
	Sound swingSound, strikeSound; // weapons: the attack begins; it hits (melee) or the arrow leaves
	float rotA = 0;
	float scale = 0;
	// pose 0..1 through the model's frames (the bow's draw, items.py); the other items have one frame.
	void Draw(float pose = 0.f);
	void DrawHeld(float pose); // in the player's fist: Ink::heldWeaponScale instead of the figures' scale
	// models/items/<name>.md3 with textures/items/<texture>.png (default: the same name).
	bool loadModel(const char* name, const char* texture = nullptr);
	// owner's model with textures/items/<texture>.png, on the same UVs.
	void shareModel(const Item& owner, const char* texture);
};

#endif
