#include "character_model.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <tuple>
#include "../core/logger.h"

namespace {
constexpr int CLIP_SPEED = 35;
} // namespace

bool CharacterModel::Load(const char* name, Texture&& tex, const ClipFiles& files, bool keepFrames) {
	texture = std::move(tex);
	reference = files.front().state;
	ModelNormalization norm;
	for (const ClipFile& file : files) {
		const std::string path = std::string("models/") + name + file.suffix + ".md3";
		if (!file.required && !std::filesystem::exists(path))
			continue;
		LOG_INFOF("entities", "Loading model: %s", path.c_str());
		auto& c = clips[static_cast<int>(file.state)];
		c = std::make_unique<AnimatedModel>();
		c->Load(path.c_str());
		c->BindTexture(static_cast<int>(texture.ID()));
		c->setSpeed(CLIP_SPEED);
		// Every clip uses the reference clip's normalization, so the model doesn't jump between animations.
		if (file.state == reference)
			norm = c->Centrify();
		else
			c->Normalize(norm);
		c->loop = file.loop;
	}
	if (!Clip(reference)) {
		LOG_ERRORF("entities", "No reference clip for %s", name);
		return false;
	}
	referenceTop = Clip(reference)->YRange(0).second;
	std::tie(halfX, halfZ) = Clip(reference)->HalfXZ(0);
	if (const AnimatedModel* idle = Clip(ModelState::Idle)) {
		idleBottom = idle->YRange(0).first;
		idleTop = idle->YRange(0).second;
	}

	const std::string sound = std::string("sounds/") + name;
	for (auto [suffix, target] : {std::pair{"_die.wav", &dieSound},
								  {"_att.wav", &attackSound},
								  {"_jump.wav", &jumpSound},
								  {"_wake.wav", &wakeSound},
								  {"_spit.wav", &spitSound}})
		if (std::string path = sound + suffix; std::filesystem::exists(path))
			target->Load(path.c_str());

	for (auto& c : clips)
		if (c)
			c->Compile(keepFrames);
	return true;
}

ClipPlayback CharacterModel::SpawnPlayback(Rng& rng) const {
	ClipPlayback playback{};
	for (ModelState state : {ModelState::Move, ModelState::Idle})
		if (const AnimatedModel* c = Clip(state); c && c->FrameCount() > 1)
			playback[static_cast<int>(state)].frame = static_cast<float>(rng.below(c->FrameCount() - 1));
	return playback;
}

void CharacterModel::Enter(ModelState& current, ModelState state, ClipPlayback& playback) const {
	const bool entering = state != current;
	current = state;
	const ModelState shown = Shown(state);
	if (entering && !Clip(shown)->loop)
		playback[static_cast<int>(shown)].frame = 0.f;
}

void CharacterModel::Show(ModelState state, const ClipPlayback& playback) const {
	const ModelState shown = Shown(state);
	Clip(shown)->Show(playback[static_cast<int>(shown)]);
}

void CharacterModel::Advance(ModelState state, ClipPlayback& playback) const {
	const ModelState shown = Shown(state);
	Clip(shown)->Advance(playback[static_cast<int>(shown)]);
}

bool CharacterModel::Finished(ModelState state, const ClipPlayback& playback) const {
	const ModelState shown = Shown(state);
	return playback[static_cast<int>(shown)].frame >= static_cast<float>(Clip(shown)->FrameCount() - 1);
}

float CharacterModel::Progress(ModelState state, const ClipPlayback& playback) const {
	const ModelState shown = Shown(state);
	const int last = Clip(shown)->FrameCount() - 1;
	return last <= 0 ? 1.f : std::min(playback[static_cast<int>(shown)].frame / static_cast<float>(last), 1.f);
}
