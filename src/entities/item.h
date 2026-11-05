#ifndef ITEM_H
#define ITEM_H

#include <cstdint>
#include <memory>
#include "../core/sound.h"
#include "../graphics/animated_model.h"
#include "../graphics/textures.h"
#include "../world/items.h"
#include "../world/rgb.h"

// A weapon, potion, amulet, ring or the treasure chest: a static model, drawn where the caller has placed it. The
// potions that come in one vessel share its model, each with its own texture (shareModel). An item may have a second
// part, its own model on the same frame, tinted and see-through, drawn by DrawPart after the first (in the world:
// after the opaque scene): a ring's gem (plan ring-accessories) or a potion's glass shell (plan potion-glass-vessels).
enum class PartStyle : std::uint8_t {
	Gem, // unlit, shining in the dark, with a slow glint
	Glass, // lit by the scene like the model, its shine the texture's metal mask; no glint
};
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
		PartStyle style = PartStyle::Gem;
	};
	Part part;

	void drawScaled(float drawScale, float pose, int texture = -1);
	void drawPartScaled(float drawScale);
	void drawGlint(const AnimPlayback& playback, Rgb c) const; // the gem again, bigger, added on (in its frame)

  public:
	static constexpr float DRAW_DEPTH = 30.f; // Draw() pushes the model this far back
	Sound swingSound, strikeSound; // weapons: the attack begins; it hits (melee) or the arrow leaves
	float rotA = 0;
	float scale = 0;
	// pose 0..1 through the model's frames (the bow's draw, items.py); the other items have one frame.
	void Draw(float pose = 0.f);
	// With another texture on the same UVs (the plain white one for a contour, Dungeon::drawContour).
	void Draw(float pose, int texture);
	void DrawHeld(float pose); // in the player's fist: Ink::heldWeaponScale instead of the figures' scale
	// media/models/items/<name>.md3 with media/textures/items/<texture>.png (default: the same name).
	bool loadModel(const char* name, const char* texture = nullptr);
	// owner's model with media/textures/items/<texture>.png, on the same UVs.
	void shareModel(const Item& owner, const char* texture);

	// The second part: media/models/items/<name>.md3 with media/textures/items/<name>.png, placed as the first model
	// (both in the same units, from the same origin); tint x the current colour, alpha its see-through.
	void loadPart(const char* name, Rgb tint, float alpha, PartStyle style = PartStyle::Gem);
	// owner's second part, as it is or in another tint.
	void sharePart(const Item& owner);
	void sharePart(const Item& owner, Rgb tint);
	[[nodiscard]] bool HasPart() const { return part.mdl != nullptr; }
	// The second part where Draw draws the first: alpha blended, no depth write; a gem unlit with a slow additive
	// glint, glass lit. Leaves blending off and depth writes on. Nothing without one.
	void DrawPart();
};

#endif
