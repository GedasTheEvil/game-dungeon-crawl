#ifndef MODEL_INFO_H
#define MODEL_INFO_H

#include "md3_mesh.h"
#include "../world/monster_kinds.h"
#include "../world/rng.h"
#include <array>
#include <vector>

// Animation clips. Each is one file, see ClipFile; a missing optional clip shows the reference clip.
enum class ModelState : unsigned char {
	Die = 0,
	Idle = 1,
	Move = 2,
	Attack = 3,
	Jump = 4,
	Climb = 5,
	Rise = 6,
	Spit = 7
};
constexpr int MODEL_STATE_COUNT = 8;

// Playback position of one animation. Monsters of one type share one model, so each keeps its own.
struct AnimPlayback {
	float frame = 0.0f;
	int stepStart = 0; // GameClock ms when the frame last advanced
};

// Playback of every clip, indexed by ModelState. Kept by each monster / the player, the clips are shared.
using ClipPlayback = std::array<AnimPlayback, MODEL_STATE_COUNT>;

// Steps p through a clip of frameCount frames on the clock: speed / 25 frames every FRAME_STEP_MS. A looping clip
// starts over, a one-shot one holds its last frame.
constexpr int FRAME_STEP_MS = 100;
void AdvancePlayback(AnimPlayback& p, int frameCount, bool loop, int speed);

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
// Walkers with a ranged attack (the Anubis): the spit clip. POC: the attack clip stands in for it, once, until a
// _spit clip with a release frame (docs/plan/anubis-ranged-attack.md).
inline const ClipFiles RANGED_CLIPS = {{ModelState::Move, "", true, true},
									   {ModelState::Attack, "_att", true, true},
									   {ModelState::Die, "_die", true, false},
									   {ModelState::Idle, "_idle", false, true},
									   {ModelState::Spit, "_att", true, false}};
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
// Coiled (the cobra): lies coiled (_idle) until it rears up (_rise, once), then walks; spits venom (_spit, once).
inline const ClipFiles COILED_CLIPS = {
	{ModelState::Move, "", true, true},		  {ModelState::Attack, "_att", true, true},
	{ModelState::Die, "_die", true, false},	  {ModelState::Idle, "_idle", true, true},
	{ModelState::Rise, "_rise", true, false}, {ModelState::Spit, "_spit", true, false}};
// The player: <name>.md3 idle (standing), _walk, _die, optional _jump and _climb. No attack clip (the weapon swings).
inline const ClipFiles PLAYER_CLIPS = {{ModelState::Idle, "", true, true},
									   {ModelState::Move, "_walk", true, true},
									   {ModelState::Die, "_die", true, false},
									   {ModelState::Jump, "_jump", false, false},
									   {ModelState::Climb, "_climb", false, true}};

// The clip files of a monster kind: by how it moves, and whether it spits.
const ClipFiles& ClipFilesOf(const MonsterKind& kind);

// One clip of a model, as the sim sees it.
struct ClipInfo {
	bool present = false; // its file was loaded
	int frames = 0;
	bool loop = true;
};

// What the sim reads from a character's model files (no GL): the clips' frame counts and loop flags, and the frame 0
// extents the hitboxes and lifts are built from. Characters keep their own ModelState and ClipPlayback.
struct ModelInfo {
	static constexpr int CLIP_SPEED = 35; // AdvancePlayback's speed of every character clip
	std::array<ClipInfo, MODEL_STATE_COUNT> clips{};
	ModelState reference = ModelState::Move; // first clip of the ClipFiles
	// Frame 0 extents in model units: a flyer hangs from the ceiling by the idle clip's top.
	float referenceTop = 1.f;
	float halfX = 0.5f, halfZ = 0.5f; // of the reference clip, round the centre (Centrify)
	float idleBottom = 0.f, idleTop = 1.f;

	// Half width of the hitbox: the larger of the two, the body length whichever way the model is turned.
	[[nodiscard]] float HalfWidth() const { return halfX > halfZ ? halfX : halfZ; }
	[[nodiscard]] const ClipInfo& Clip(ModelState state) const { return clips[static_cast<int>(state)]; }
	// The clip a state shows: its own, or the reference clip standing in.
	[[nodiscard]] ModelState Shown(ModelState state) const { return Clip(state).present ? state : reference; }
	// Random move / idle phase, so monsters spawned in the same tick don't march in step.
	[[nodiscard]] ClipPlayback SpawnPlayback(Rng& rng) const;
	// Enters state: a one-shot clip (die, jump) plays from the start.
	void Enter(ModelState& current, ModelState state, ClipPlayback& playback) const;
	void Advance(ModelState state, ClipPlayback& playback) const;
	// A one-shot clip (die, jump) has reached its last frame.
	[[nodiscard]] bool Finished(ModelState state, const ClipPlayback& playback) const;
	// Of a one-shot clip: 0 at its first frame, 1 at its last.
	[[nodiscard]] float Progress(ModelState state, const ClipPlayback& playback) const;
};

// The clips of models/<name><suffix>.md3 (name: "<category>/<name>"), every one normalized like the reference clip
// (Centrify), so the model doesn't jump between animations. Fills info; meshes (by ModelState, if given) gets the
// loaded clips for drawing. False: no reference clip.
using ClipMeshes = std::array<Md3Mesh, MODEL_STATE_COUNT>;
bool LoadClips(const char* name, const ClipFiles& files, ModelInfo& info, ClipMeshes* meshes = nullptr);

#endif
