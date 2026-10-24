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
	AttackRanged = 7
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

// media/models/<category>/<name><suffix>.md3 for one clip. The first entry of a list is the reference clip:
// it is required, sets the normalization of all clips (frame 0) and stands in for missing optional ones.
struct ClipFile {
	ModelState state;
	const char* suffix;
	bool required;
	bool loop; // false: plays once and holds the last frame
};
using ClipFiles = std::vector<ClipFile>;

// Monsters: <name>.md3 move, _att attack, _die die, optional _idle (e.g. a bat on the ceiling), _jump (a leap) and
// _climb (a climber on a ladder, its back to the camera).
inline const ClipFiles MONSTER_CLIPS = {
	{ModelState::Move, "", true, true},
	{ModelState::Attack, "_att", true, true},
	{ModelState::Die, "_die", true, false},
	{ModelState::Idle, "_idle", false, true},
	{ModelState::Jump, "_jump", false, false},
	{ModelState::Climb, "_climb", false, true}
};
// Sand burrowers (the worm, Rerek; MonsterKind::sandBurrow): the MONSTER_CLIPS and an optional _rise, played once as
// it bursts up out of the sand and backwards as it dives (Monster::showBurrow, plan worm-boss).
inline const ClipFiles SAND_BURROWER_CLIPS = {
	{ModelState::Move, "", true, true},
	{ModelState::Attack, "_att", true, true},
	{ModelState::Die, "_die", true, false},
	{ModelState::Idle, "_idle", false, true},
	{ModelState::Rise, "_rise", false, false}
};
// Walkers with a ranged attack (the Anubis): the melee swing is the ranged clip too, played once; the shot leaves at
// its release (plan anubis-ranged-attack).
inline const ClipFiles RANGED_CLIPS = {
	{ModelState::Move, "", true, true},
	{ModelState::Attack, "_att", true, true},
	{ModelState::Die, "_die", true, false},
	{ModelState::Idle, "_idle", false, true},
	{ModelState::AttackRanged, "_att", true, false},
	{ModelState::Climb, "_climb", false, true}
};
// Walkers that throw (the baboon): the ranged clip is its own file, _att_ranged, played once (RangedRules::ownClip).
inline const ClipFiles THROWER_CLIPS = {
	{ModelState::Move, "", true, true},
	{ModelState::Attack, "_att", true, true},
	{ModelState::Die, "_die", true, false},
	{ModelState::Idle, "_idle", false, true},
	{ModelState::AttackRanged, "_att_ranged", true, false},
	{ModelState::Climb, "_climb", false, true}
};
// Rollers (the giant scarab, Khepri; RollRules): the MONSTER_CLIPS and the roll, _att_ranged, played once.
inline const ClipFiles ROLLER_CLIPS = {
	{ModelState::Move, "", true, true},
	{ModelState::Attack, "_att", true, true},
	{ModelState::Die, "_die", true, false},
	{ModelState::Idle, "_idle", false, true},
	{ModelState::Jump, "_jump", false, false},
	{ModelState::AttackRanged, "_att_ranged", true, false}
};
// Ambushers (the mimic): the idle clip is the disguise and the reference, so the awake loop never has to show it.
inline const ClipFiles AMBUSH_CLIPS = {
	{ModelState::Idle, "_idle", true, true},
	{ModelState::Move, "", true, true},
	{ModelState::Attack, "_att", true, true},
	{ModelState::Die, "_die", true, false}
};
// Ambushers that hop after the player once woken (the cursed mimic): the hop is the move clip, a leap over a pit the
// optional _jump.
inline const ClipFiles AMBUSH_HOP_CLIPS = {
	{ModelState::Idle, "_idle", true, true},
	{ModelState::Move, "", true, true},
	{ModelState::Attack, "_att", true, true},
	{ModelState::Die, "_die", true, false},
	{ModelState::Jump, "_jump", false, false}
};
// Entombed (the mummy): walks like the MONSTER_CLIPS, lies in its coffin (_idle) until it climbs out (_rise, once).
inline const ClipFiles ENTOMBED_CLIPS = {
	{ModelState::Move, "", true, true},
	{ModelState::Attack, "_att", true, true},
	{ModelState::Die, "_die", true, false},
	{ModelState::Idle, "_idle", true, true},
	{ModelState::Rise, "_rise", true, false},
	{ModelState::Climb, "_climb", false, true}
};
// Entombed with a ranged attack (the mummy priest): the staff swing is the ranged clip too, as RANGED_CLIPS.
inline const ClipFiles ENTOMBED_RANGED_CLIPS = {
	{ModelState::Move, "", true, true},
	{ModelState::Attack, "_att", true, true},
	{ModelState::Die, "_die", true, false},
	{ModelState::Idle, "_idle", true, true},
	{ModelState::Rise, "_rise", true, false},
	{ModelState::AttackRanged, "_att", true, false},
	{ModelState::Climb, "_climb", false, true}
};
// Coiled (the cobra): lies coiled (_idle) until it rears up (_rise, once), then walks; spits venom (_att_ranged, once).
inline const ClipFiles COILED_CLIPS = {
	{ModelState::Move, "", true, true},
	{ModelState::Attack, "_att", true, true},
	{ModelState::Die, "_die", true, false},
	{ModelState::Idle, "_idle", true, true},
	{ModelState::Rise, "_rise", true, false},
	{ModelState::AttackRanged, "_att_ranged", true, false}
};
// Floating (the blue lotus): rooted in half water, floats as a flower (_idle) until it rears up (_rise, once), then
// sways (the move clip, the reference) and bites.
inline const ClipFiles FLOATING_CLIPS = {
	{ModelState::Move, "", true, true},
	{ModelState::Attack, "_att", true, true},
	{ModelState::Die, "_die", true, false},
	{ModelState::Idle, "_idle", true, true},
	{ModelState::Rise, "_rise", true, false}
};
// The player: <name>.md3 idle (standing), _walk, _die, optional _jump and _climb. No attack clip (the weapon swings).
inline const ClipFiles PLAYER_CLIPS = {
	{ModelState::Idle, "", true, true},
	{ModelState::Move, "_walk", true, true},
	{ModelState::Die, "_die", true, false},
	{ModelState::Jump, "_jump", false, false},
	{ModelState::Climb, "_climb", false, true}
};

// The clip files of a monster kind: by how it moves, and whether it has a ranged attack.
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
	static constexpr int CLIP_SPEED = 35; // AdvancePlayback's default speed of a character clip
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
	// Advance with the rise clip run in riseMs (rounded up to FRAME_STEP_MS steps); riseMs 0: CLIP_SPEED. The walk
	// clip (Move) plays at walkSpeed (plan anubis-walk-clip-speed).
	void Advance(ModelState state, ClipPlayback& playback, int riseMs, int walkSpeed = CLIP_SPEED) const;
	// A one-shot clip (die, jump) has reached its last frame.
	[[nodiscard]] bool Finished(ModelState state, const ClipPlayback& playback) const;
	// Of a one-shot clip: 0 at its first frame, 1 at its last.
	[[nodiscard]] float Progress(ModelState state, const ClipPlayback& playback) const;
};

// The clips of media/models/<name><suffix>.md3 (name: "<category>/<name>"), every one normalized like the reference
// clip (Centrify), so the model doesn't jump between animations. Fills info; meshes (by ModelState, if given) gets the
// loaded clips for drawing. False: no reference clip.
using ClipMeshes = std::array<Md3Mesh, MODEL_STATE_COUNT>;
bool LoadClips(const char* name, const ClipFiles& files, ModelInfo& info, ClipMeshes* meshes = nullptr);

#endif
