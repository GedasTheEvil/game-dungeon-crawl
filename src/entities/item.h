#ifndef ITEM_H
#define ITEM_H

#include <memory>
#include "../core/sound.h"
#include "../graphics/animated_model.h"
#include "../graphics/textures.h"
#include "../world/damage.h"

// How a weapon is held and swung (ITEM_DEFS in assets.cpp, drawWeapon). Tilts in degrees from upright, towards the
// facing side. An attack raises the weapon back to windupTilt, brings it down through strikeTilt, where the hit
// lands (hitMs), and returns it to restTilt by swingMs. The bow is drawn until hitMs instead and the arrow leaves.
struct WeaponMotion {
	float grip = 0.1f; // the fist holds it this far up its length, from the lowest point
	float restTilt = 45.f, windupTilt = 45.f, strikeTilt = 45.f;
	float thrust = 0.f; // pushed forward this many lengths at the strike instead (the spear)
	int hitMs = 200, swingMs = 400;
	int attackMs = 1000; // from one attack to the next
};

// A weapon, potion or the treasure chest: a static model, drawn where the caller has placed it.
class Item {
  private:
	std::unique_ptr<AnimatedModel> mdl;
	Texture tex;

  public:
	static constexpr float DRAW_DEPTH = 30.f; // Draw() pushes the model this far back
	int damage = 1, range = 1;				  // weapons only; range in tenths of a tile (ITEM_DEFS)
	DamageMix mix = {100, 0, 0};			  // weapons only
	[[nodiscard]] float Reach() const { return 0.1f * static_cast<float>(range); } // tiles: melee reach, bow aim
	WeaponMotion motion;														   // weapons only
	Sound swingSound, strikeSound; // weapons: the attack begins; it hits (melee) or the arrow leaves
	float rotA = 0;
	float scale = 0;
	// pose 0..1 through the model's frames (the bow's draw, items.py); the other items have one frame.
	void Draw(float pose = 0.f);
	// models/items/<name>.md3 with textures/items/<name>.png.
	bool loadModel(const char* name);
};

#endif
