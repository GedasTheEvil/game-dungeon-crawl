#ifndef CHARACTER_MODEL_H
#define CHARACTER_MODEL_H

#include "model_info.h"
#include "../graphics/animated_model.h"
#include "../graphics/textures.h"
#include "../core/sound.h"
#include "../world/rng.h"
#include <array>
#include <memory>
#include <vector>

// The clips of one set of model files. Kin on one model (the scarab, the giant scarab, the boss scarab) share them and
// only differ in their texture, bound at draw time.
struct CharacterClips {
	std::array<std::unique_ptr<AnimatedModel>, MODEL_STATE_COUNT> clips; // by ModelState, nullptr if no file
};

// The clips, texture and sounds of one character (a monster type or the player), loaded once and shared by every
// instance. Instances keep their own ModelState and ClipPlayback.
class CharacterModel {
  private:
	std::shared_ptr<const CharacterClips> shape = std::make_shared<CharacterClips>(); // empty until Load
	ModelInfo info;
	Texture texture;
	void loadSounds(const char* name);

  public:
	// sounds/<category>/<name>_{die,att,jump,wake,spit}.wav, all optional
	Sound dieSound, attackSound, jumpSound, wakeSound, spitSound;

	// name: "<category>/<name>", the same under models/, textures/ and sounds/.
	// Takes the texture over. keepFrames: see AnimatedModel::Compile.
	bool Load(const char* name, Texture&& tex, const ClipFiles& files, bool keepFrames = false);
	// The clips (and measures) of `other`, loaded from the same model files, with its own texture and sounds.
	void Share(const CharacterModel& other, const char* name, Texture&& tex);
	// The clips' frame counts and loop flags, the frame 0 extents: what the sim reads.
	[[nodiscard]] const ModelInfo& Info() const { return info; }
	[[nodiscard]] AnimatedModel* Clip(ModelState state) const { return shape->clips[static_cast<int>(state)].get(); }
	void BindTexture() const { texture.Bind(); }
	void Show(ModelState state, const ClipPlayback& playback) const;
};

#endif
