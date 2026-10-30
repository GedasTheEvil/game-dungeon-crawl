#ifndef ITEM_H
#define ITEM_H

#include <memory>
#include "../core/sound.h"
#include "../graphics/animated_model.h"
#include "../graphics/textures.h"
#include "../world/items.h"
#include "../world/rgb.h"

// A weapon, potion, amulet, ring or the treasure chest: a static model, drawn where the caller has placed it. The
// potions that come in one vessel share its model, each with its own texture (shareModel). An item may have a second
// part (a ring's gem, plan ring-accessories): its own model on the same frame, tinted, see-through and shining, drawn
// by DrawPart after the first (in the world: after the opaque scene).
class Item {
  private:
	std::shared_ptr<AnimatedModel> mdl;
	Texture tex;
	ModelNormalization norm; // loadModel's Centrify: a second part's model is placed as the first

	struct Part {
		std::shared_ptr<AnimatedModel> mdl;
		std::shared_ptr<Texture> tex;
		Rgb tint{1.f, 1.f, 1.f};
		float alpha = 1.f;
		float centreY = 0.f; // the glint grows round it
	};
	Part part;

	void drawScaled(float drawScale, float pose);
	void drawPartScaled(float drawScale);

  public:
	static constexpr float DRAW_DEPTH = 30.f; // Draw() pushes the model this far back
	Sound swingSound, strikeSound; // weapons: the attack begins; it hits (melee) or the arrow leaves
	float rotA = 0;
	float scale = 0;
	// pose 0..1 through the model's frames (the bow's draw, items.py); the other items have one frame.
	void Draw(float pose = 0.f);
	void DrawHeld(float pose); // in the player's fist: Ink::heldWeaponScale instead of the figures' scale
	// media/models/items/<name>.md3 with media/textures/items/<texture>.png (default: the same name).
	bool loadModel(const char* name, const char* texture = nullptr);
	// owner's model with media/textures/items/<texture>.png, on the same UVs.
	void shareModel(const Item& owner, const char* texture);

	// The second part: media/models/items/<name>.md3 with media/textures/items/<name>.png, placed as the first model
	// (both in the same units, from the same origin); tint x the current colour, alpha its see-through.
	void loadPart(const char* name, Rgb tint, float alpha);
	// owner's second part, in another tint.
	void sharePart(const Item& owner, Rgb tint);
	[[nodiscard]] bool HasPart() const { return part.mdl != nullptr; }
	// The second part where Draw draws the first: alpha blended, no depth write, unlit, with a slow additive glint.
	// Leaves blending off and depth writes on. Nothing without one.
	void DrawPart();
};

#endif
