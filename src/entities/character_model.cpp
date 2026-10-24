#include "character_model.h"
#include <filesystem>
#include <string>
#include "../core/paths.h"

CharacterModel::Parsed CharacterModel::Parse(const char* name, const ClipFiles& files) {
	Parsed parsed;
	parsed.loaded = LoadClips(name, files, parsed.info, &parsed.meshes);
	return parsed;
}

bool CharacterModel::Load(const char* name, Texture&& tex, const ClipFiles& files, bool keepFrames) {
	return Load(name, std::move(tex), Parse(name, files), keepFrames);
}

bool CharacterModel::Load(const char* name, Texture&& tex, Parsed&& parsed, bool keepFrames) {
	texture = std::move(tex);
	info = parsed.info;
	ClipMeshes& meshes = parsed.meshes;
	const bool loaded = parsed.loaded;
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
	const std::string sound = std::string(MEDIA_DIR "sounds/") + name;
	for (auto [suffix, target] :
		 {std::pair{"_die.wav", &dieSound},
		  {"_att.wav", &attackSound},
		  {"_jump.wav", &jumpSound},
		  {"_wake.wav", &wakeSound},
		  {"_att_ranged.wav", &attackRangedSound},
		  {"_roar.wav", &roarSound}})
		if (std::string path = sound + suffix; std::filesystem::exists(path))
			target->Load(path.c_str());
}

void CharacterModel::Show(ModelState state, const ClipPlayback& playback) const {
	const ModelState shown = info.Shown(state);
	Clip(shown)->Show(playback[static_cast<int>(shown)], texture.ID());
}
