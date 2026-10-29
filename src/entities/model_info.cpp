#include "model_info.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <tuple>
#include "../core/logger.h"
#include "../core/timer.h"
#include "../core/paths.h"

const ClipFiles& ClipFilesOf(const MonsterKind& kind) {
	if (kind.roll)
		return ROLLER_CLIPS;
	if (kind.sandBurrow)
		return SAND_BURROWER_CLIPS;
	switch (kind.locomotion) {
	case Locomotion::Ambush:
		return AMBUSH_CLIPS;
	case Locomotion::AmbushHop:
		return AMBUSH_HOP_CLIPS;
	case Locomotion::Entombed:
		return kind.ranged ? ENTOMBED_RANGED_CLIPS : ENTOMBED_CLIPS;
	case Locomotion::Coiled:
	case Locomotion::Burrow:
		return COILED_CLIPS;
	case Locomotion::Floating:
	case Locomotion::Figurine:
		return FLOATING_CLIPS;
	case Locomotion::Walk:
	case Locomotion::WalkJump:
		return !kind.ranged ? MONSTER_CLIPS : kind.ranged->ownClip ? THROWER_CLIPS : RANGED_CLIPS;
	default:
		return MONSTER_CLIPS;
	}
}

void AdvancePlayback(AnimPlayback& p, int frameCount, bool loop, float speed) {
	if (frameCount == 1)
		return;

	p.step = 0.04f * speed;
	const int now = GameClock::now();
	if (now - p.stepStart < FRAME_STEP_MS)
		return;
	p.stepStart = now;
	p.frame = NextFrame(p.frame, frameCount, loop, p.step);
}

float NextFrame(float frame, int frameCount, bool loop, float step) {
	if (!loop)
		return std::min(frame + step, static_cast<float>(frameCount - 1));
	// Past the end: on into the next round, so a fast clip (a big step) keeps its pace.
	const float next = frame + step;
	return static_cast<double>(next) >= frameCount - 0.2 ? std::max(next - static_cast<float>(frameCount), 0.f) : next;
}

FrameBlend BlendOf(const AnimPlayback& p, int frameCount, bool loop, int now, int rate) {
	const int last = frameCount - 1;
	if (last <= 0)
		return {};
	if (rate <= 0) {
		const int a = std::clamp(static_cast<int>(p.frame), 0, last);
		return {a, a, 0.f};
	}
	float pos = p.frame;
	if (p.step > 0.f) {
		const double shown = std::floor(static_cast<double>(now) * rate / 1000.0) * 1000.0 / rate;
		const auto k = std::clamp(static_cast<float>((shown - p.stepStart) / FRAME_STEP_PERIOD_MS), 0.f, 1.f);
		float next = NextFrame(p.frame, frameCount, loop, p.step);
		if (next < p.frame) // a loop starting over: on past the last frame into the next round
			next += static_cast<float>(frameCount);
		pos += (next - p.frame) * k;
	}
	const auto whole = std::floor(pos);
	const float t = pos - whole;
	const auto a = static_cast<int>(whole);
	if (loop) {
		const int wrapped = ((a % frameCount) + frameCount) % frameCount;
		return {wrapped, (wrapped + 1) % frameCount, t};
	}
	if (a >= last)
		return {last, last, 0.f};
	return {a, a + 1, t};
}

ClipPlayback ModelInfo::SpawnPlayback(Rng& rng) const {
	ClipPlayback playback{};
	for (ModelState state : {ModelState::Move, ModelState::Idle})
		if (const ClipInfo& c = Clip(state); c.present && c.frames > 1)
			playback[static_cast<int>(state)].frame = static_cast<float>(rng.below(c.frames - 1));
	return playback;
}

void ModelInfo::Enter(ModelState& current, ModelState state, ClipPlayback& playback) const {
	const bool entering = state != current;
	current = state;
	const ModelState shown = Shown(state);
	if (entering && !Clip(shown).loop)
		playback[static_cast<int>(shown)].frame = 0.f;
}

void ModelInfo::Advance(ModelState state, ClipPlayback& playback) const {
	const ModelState shown = Shown(state);
	AdvancePlayback(playback[static_cast<int>(shown)], Clip(shown).frames, Clip(shown).loop, CLIP_SPEED);
}

void ModelInfo::Advance(ModelState state, ClipPlayback& playback, int riseMs, float walkSpeed) const {
	if (state == ModelState::Move) {
		const ModelState shown = Shown(state);
		return AdvancePlayback(playback[static_cast<int>(shown)], Clip(shown).frames, Clip(shown).loop, walkSpeed);
	}
	if (state != ModelState::Rise || riseMs <= 0)
		return Advance(state, playback);
	const int steps = (riseMs + FRAME_STEP_MS - 1) / FRAME_STEP_MS;
	const int last = Clip(Shown(state)).frames - 1;
	const int speed = (last * 25 + steps - 1) / steps; // AdvancePlayback steps speed / 25 frames
	AdvancePlayback(playback[static_cast<int>(Shown(state))], last + 1, Clip(Shown(state)).loop, speed);
}

float ModelInfo::WalkSpeed(float cyclesPerSecond) const {
	// AdvancePlayback steps 0.04 * speed frames every FRAME_STEP_PERIOD_MS.
	const auto frames = static_cast<float>(Clip(Shown(ModelState::Move)).frames);
	return cyclesPerSecond * frames * static_cast<float>(FRAME_STEP_PERIOD_MS) / 40.f;
}

bool ModelInfo::Finished(ModelState state, const ClipPlayback& playback) const {
	const ModelState shown = Shown(state);
	return playback[static_cast<int>(shown)].frame >= static_cast<float>(Clip(shown).frames - 1);
}

float ModelInfo::Progress(ModelState state, const ClipPlayback& playback) const {
	const ModelState shown = Shown(state);
	const int last = Clip(shown).frames - 1;
	return last <= 0 ? 1.f : std::min(playback[static_cast<int>(shown)].frame / static_cast<float>(last), 1.f);
}

bool LoadClips(const char* name, const ClipFiles& files, ModelInfo& info, ClipMeshes* meshes) {
	info = ModelInfo{};
	const ModelState reference = info.reference = files.front().state;
	ClipMeshes loaded;
	ModelNormalization norm;
	for (const ClipFile& file : files) {
		const std::string path = std::string(MEDIA_DIR "models/") + name + file.suffix + ".md3";
		if (!file.required && !std::filesystem::exists(path))
			continue;
		LOG_INFOF("entities", "Loading model: %s", path.c_str());
		Md3Mesh& mesh = loaded[static_cast<int>(file.state)];
		if (!mesh.Load(path.c_str()))
			continue;
		// Every clip uses the reference clip's normalization.
		if (file.state == reference)
			norm = mesh.Centrify();
		else
			mesh.Normalize(norm);
		info.clips[static_cast<int>(file.state)] = {true, mesh.frameCount, file.loop};
	}
	if (!info.Clip(reference).present) {
		LOG_ERRORF("entities", "No reference clip for %s", name);
		return false;
	}
	const Md3Mesh& ref = loaded[static_cast<int>(reference)];
	info.referenceTop = ref.YRange(0).second;
	std::tie(info.halfX, info.halfZ) = ref.HalfXZ(0);
	if (info.Clip(ModelState::Idle).present) {
		const Md3Mesh& idle = loaded[static_cast<int>(ModelState::Idle)];
		info.idleBottom = idle.YRange(0).first;
		info.idleTop = idle.YRange(0).second;
	}
	if (meshes != nullptr)
		*meshes = std::move(loaded);
	return true;
}
