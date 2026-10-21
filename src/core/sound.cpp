#include "sound.h"
#include <SDL/SDL_mixer.h>
#include "logger.h"

namespace {
int gAudioUsers = 0;
bool gAudioOpened = false;
} // namespace

AudioUser::AudioUser() {
	if (gAudioUsers++ > 0 || gAudioOpened)
		return;
	int rate;
	Uint16 format;
	int channels;
	const bool alreadyOpen = Mix_QuerySpec(&rate, &format, &channels) != 0;
	if (!alreadyOpen && Mix_OpenAudio(22050, AUDIO_S16, 2, 4096) != 0)
		LOG_ERROR("audio", "Unable to open audio!");
	else
		gAudioOpened = true;
}

AudioUser::~AudioUser() {
	if (--gAudioUsers == 0 && gAudioOpened) {
		Mix_CloseAudio();
		gAudioOpened = false;
	}
}

Sound::~Sound() {
	if (chunk)
		Mix_FreeChunk(chunk);
}

bool Sound::Load(const char filename[]) {
	if (chunk) {
		LOG_ERRORF("audio", "Sound already loaded, not loading %s", filename);
		return false;
	}
	chunk = Mix_LoadWAV(filename);
	if (!chunk) {
		LOG_ERRORF("audio", "Sound load error: %s: %s", filename, Mix_GetError());
		return false;
	}
	return true;
}

void Sound::Play() const {
	if (chunk)
		Mix_PlayChannel(-1, chunk, 0);
}

Music::~Music() {
	if (music)
		Mix_FreeMusic(music);
}

bool Music::Load(const char filename[]) {
	if (music) {
		LOG_ERRORF("audio", "Music already loaded, not loading %s", filename);
		return false;
	}
	music = Mix_LoadMUS(filename);
	if (!music) {
		LOG_ERRORF("audio", "Music load error: %s: %s", filename, Mix_GetError());
		return false;
	}
	return true;
}

void Music::Play() const {
	if (music)
		Mix_PlayMusic(music, -1);
}

namespace {
constexpr int AMBIENCE_CHANNEL = 0; // reserved: the effects never take it
constexpr int AMBIENCE_FADE_MS = 1500;
} // namespace

Ambience::~Ambience() {
	if (chunk) {
		Mix_HaltChannel(AMBIENCE_CHANNEL);
		Mix_FreeChunk(chunk);
	}
}

void Ambience::Play(const std::string& filename) {
	if (filename == playing)
		return;
	playing = filename;
	Mix_ReserveChannels(AMBIENCE_CHANNEL + 1);
	if (chunk) {
		Mix_HaltChannel(AMBIENCE_CHANNEL);
		Mix_FreeChunk(chunk);
		chunk = nullptr;
	}
	if (filename.empty())
		return;
	chunk = Mix_LoadWAV(filename.c_str());
	if (!chunk) {
		LOG_ERRORF("audio", "Ambience load error: %s: %s", filename.c_str(), Mix_GetError());
		return;
	}
	Mix_FadeInChannel(AMBIENCE_CHANNEL, chunk, -1, AMBIENCE_FADE_MS);
}

void Audio::SetVolumes(int music, int effects) {
	Mix_VolumeMusic(music * MIX_MAX_VOLUME / 100);
	Mix_Volume(-1, effects * MIX_MAX_VOLUME / 100);
}
