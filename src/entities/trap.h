#ifndef TRAP_H
#define TRAP_H
#include <memory>
#include "../graphics/animated_model.h"
#include "../graphics/textures.h"

// A trap model (spikes, the death trap's big spikes), shared by every tile of its kind. It only draws; the damage is
// Dungeon::updateTraps.
class Trap {
  private:
	std::unique_ptr<AnimatedModel> mdl;
	Texture tex;

  public:
	float scale = 3;

	Trap();
	void Show();
	bool loadModel(const char filename[], Texture& texture, bool compile = true);
};

#endif
