#ifndef PLAYER_VIEW_H
#define PLAYER_VIEW_H

#include "character_model.h"
#include "player.h"
#include "../graphics/texture_registry.h"
#include <array>

// The player's figure: the model, its texture and sounds, drawn at the frame origin as the Player's last Animate left
// it.
class PlayerView {
  private:
	CharacterModel model;
	// Corner indices of the two fists (model -x, +x), the same in every clip (one mesh); -1: not found.
	std::array<int, 2> fists{-1, -1};

	void findFists();

  public:
	// models/<name>... with the texture; hands the player the clips and measures (Player::SetModel).
	bool Load(const char* name, Texture&& texture, Player& player);
	[[nodiscard]] const CharacterModel& Model() const { return model; } // its sounds
	void Draw(const Player& player, const TextureRegistry& textures) const;
	// The fist nearer the camera facing dir (+1 right, -1 left), in the shown clip frame: where the weapon is held.
	// In the frame Draw() is called in, world units.
	[[nodiscard]] std::array<float, 3> Fist(const Player& player, int dir) const;
};

#endif
