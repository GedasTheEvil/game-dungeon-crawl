#include "model_info.h"
#include <algorithm>
#include <filesystem>
#include <string>
#include <tuple>
#include "../core/logger.h"
#include "../core/timer.h"

const ClipFiles& ClipFilesOf(Locomotion locomotion) {
	switch (locomotion) {
	case Locomotion::Ambush:
		return AMBUSH_CLIPS;
	case Locomotion::Entombed:
		return ENTOMBED_CLIPS;
	case Locomotion::Coiled:
	case Locomotion::Burrow:
		return COILED_CLIPS;
	default:
		return MONSTER_CLIPS;
	}
}

void AdvancePlayback(AnimPlayback& p, int frameCount, bool loop, int speed) {
	if (frameCount == 1)
		return;

	const int now = GameClock::now();
	if (now - p.stepStart < FRAME_STEP_MS)
		return;
	p.stepStart = now;

	const auto frames = static_cast<float>(frameCount);
	const float step = 0.04f * static_cast<float>(speed);
	if (!loop && p.frame < frames)
		p.frame += step;

	if (!loop && p.frame >= frames - 1)
		p.frame = frames - 1;

	if (loop)
		p.frame += step;

	if (loop && p.frame >= (frameCount - 0.2))
		p.frame = 0.0;
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
		const std::string path = std::string("models/") + name + file.suffix + ".md3";
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
