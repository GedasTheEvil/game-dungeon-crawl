#include "character_model.h"
#include <filesystem>
#include <string>

bool CharacterModel::Load(const char* name, Texture&& tex, const ClipFiles& files, bool keepFrames) {
	texture = std::move(tex);
	ClipMeshes meshes;
	const bool loaded = LoadClips(name, files, info, &meshes);
	auto clips = std::make_shared<CharacterClips>();
	for (int s = 0; s < MODEL_STATE_COUNT; s++) {
		const ClipInfo& clip = info.clips[s];
		if (!clip.present)
			continue;
		auto& c = clips->clips[s];
		c = std::make_unique<AnimatedModel>();
		c->Adopt(std::move(meshes[s]));
		c->setSpeed(ModelInfo::CLIP_SPEED);
		c->loop = clip.loop;
	}
	shape = clips;
	if (!loaded)
		return false;

	loadSounds(name);
	for (auto& c : clips->clips)
		if (c)
			c->Compile(keepFrames);
	return true;
}

void CharacterModel::Share(const CharacterModel& other, const char* name, Texture&& tex) {
	shape = other.shape;
	info = other.info;
	texture = std::move(tex);
	loadSounds(name);
}

void CharacterModel::loadSounds(const char* name) {
	const std::string sound = std::string("sounds/") + name;
	for (auto [suffix, target] : {std::pair{"_die.wav", &dieSound},
								  {"_att.wav", &attackSound},
								  {"_jump.wav", &jumpSound},
								  {"_wake.wav", &wakeSound},
								  {"_spit.wav", &spitSound}})
		if (std::string path = sound + suffix; std::filesystem::exists(path))
			target->Load(path.c_str());
}

void CharacterModel::Show(ModelState state, const ClipPlayback& playback) const {
	const ModelState shown = info.Shown(state);
	Clip(shown)->Show(playback[static_cast<int>(shown)], texture.ID());
}
