#ifndef CHARACTER_MODEL_H
#define CHARACTER_MODEL_H

#include "../graphics/animated_model.h"
#include "../graphics/textures.h"
#include "../core/sound.h"
#include "../world/rng.h"
#include <array>
#include <memory>
#include <vector>

// Animation clips. Each is one file, see ClipFile; a missing optional clip shows the reference clip.
enum class ModelState : unsigned char { Die = 0, Idle = 1, Move = 2, Attack = 3, Jump = 4, Climb = 5, Rise = 6 };
constexpr int MODEL_STATE_COUNT = 7;

// Playback of every clip, indexed by ModelState. Kept by each monster / the player, the clips are shared.
using ClipPlayback = std::array<AnimPlayback, MODEL_STATE_COUNT>;

// models/<category>/<name><suffix>.md3 for one clip. The first entry of a list is the reference clip:
// it is required, sets the normalization of all clips (frame 0) and stands in for missing optional ones.
struct ClipFile {
	ModelState state;
	const char* suffix;
	bool required;
	bool loop; // false: plays once and holds the last frame
};
using ClipFiles = std::vector<ClipFile>;

// Monsters: <name>.md3 move, _att attack, _die die, optional _idle (e.g. a bat on the ceiling) and _jump (a leap).
inline const ClipFiles MONSTER_CLIPS = {{ModelState::Move, "", true, true},
										{ModelState::Attack, "_att", true, true},
										{ModelState::Die, "_die", true, false},
										{ModelState::Idle, "_idle", false, true},
										{ModelState::Jump, "_jump", false, false}};
// Ambushers (the mimic): the idle clip is the disguise and the reference, so the awake loop never has to show it.
inline const ClipFiles AMBUSH_CLIPS = {{ModelState::Idle, "_idle", true, true},
									   {ModelState::Move, "", true, true},
									   {ModelState::Attack, "_att", true, true},
									   {ModelState::Die, "_die", true, false}};
// Entombed (the mummy): walks like the MONSTER_CLIPS, lies in its coffin (_idle) until it climbs out (_rise, once).
inline const ClipFiles ENTOMBED_CLIPS = {{ModelState::Move, "", true, true},
										 {ModelState::Attack, "_att", true, true},
										 {ModelState::Die, "_die", true, false},
										 {ModelState::Idle, "_idle", true, true},
										 {ModelState::Rise, "_rise", true, false}};
// The player: <name>.md3 idle (standing), _walk, _die, optional _jump and _climb. No attack clip (the weapon swings).
inline const ClipFiles PLAYER_CLIPS = {{ModelState::Idle, "", true, true},
									   {ModelState::Move, "_walk", true, true},
									   {ModelState::Die, "_die", true, false},
									   {ModelState::Jump, "_jump", false, false},
									   {ModelState::Climb, "_climb", false, true}};

// The clips, texture and sounds of one character (a monster type or the player), loaded once and shared by every
// instance. Instances keep their own ModelState and ClipPlayback.
class CharacterModel {
  private:
	std::array<std::unique_ptr<AnimatedModel>, MODEL_STATE_COUNT> clips; // by ModelState, nullptr if no file
	ModelState reference = ModelState::Move;							 // first clip of the ClipFiles
	Texture texture;

  public:
	// Frame 0 extents in model units: a flyer hangs from the ceiling by the idle clip's top.
	float referenceTop = 1.f;
	float halfX = 0.5f, halfZ = 0.5f; // of the reference clip, round the centre (Centrify)
	// Half width of the hitbox: the larger of the two, the body length whichever way the model is turned.
	[[nodiscard]] float HalfWidth() const { return halfX > halfZ ? halfX : halfZ; }
	float idleBottom = 0.f, idleTop = 1.f;
	Sound dieSound, attackSound, jumpSound, wakeSound; // sounds/<category>/<name>_{die,att,jump,wake}.wav, all optional

	// name: "<category>/<name>", the same under models/, textures/ and sounds/.
	bool Load(const char* name, Texture&& tex, const ClipFiles& files); // takes the texture over
	[[nodiscard]] ModelState Reference() const { return reference; }
	[[nodiscard]] AnimatedModel* Clip(ModelState state) const { return clips[static_cast<int>(state)].get(); }
	// The clip a state shows: its own, or the reference clip standing in.
	[[nodiscard]] ModelState Shown(ModelState state) const { return Clip(state) ? state : reference; }
	// Random move / idle phase, so monsters spawned in the same tick don't march in step.
	[[nodiscard]] ClipPlayback SpawnPlayback(Rng& rng) const;
	// Enters state: a one-shot clip (die, jump) plays from the start.
	void Enter(ModelState& current, ModelState state, ClipPlayback& playback) const;
	void BindTexture() const { texture.Bind(); }
	void Show(ModelState state, const ClipPlayback& playback) const;
	void Advance(ModelState state, ClipPlayback& playback) const;
	// A one-shot clip (die, jump) has reached its last frame.
	[[nodiscard]] bool Finished(ModelState state, const ClipPlayback& playback) const;
	// Of a one-shot clip: 0 at its first frame, 1 at its last.
	[[nodiscard]] float Progress(ModelState state, const ClipPlayback& playback) const;
};

#endif
