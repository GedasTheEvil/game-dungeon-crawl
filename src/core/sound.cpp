#include "sound.h"
#include <SDL/SDL_mixer.h>
#include <algorithm>
#include "logger.h"

namespace {
int gAudioUsers = 0;
bool gAudioOpened = false;
int gEffectsVolume = MIX_MAX_VOLUME; // Audio::SetVolumes
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

// Every play sets its channel's volume: one played quieter leaves none behind for the next sound on the channel.
int Sound::Play(float gain) const {
	if (!chunk)
		return -1;
	const int channel = Mix_PlayChannel(-1, chunk, 0);
	if (channel >= 0)
		Mix_Volume(channel, static_cast<int>(static_cast<float>(gEffectsVolume) * std::clamp(gain, 0.f, 1.f)));
	return channel;
}

void Sound::Stop(int channel) {
	if (channel >= 0)
		Mix_HaltChannel(channel);
}

Music::~Music() {
	if (music)
		Mix_FreeMusic(music);
}

namespace {
constexpr int MUSIC_FADE_MS = 1500;
constexpr int AMBIENCE_CHANNEL = 0; // reserved: the effects never take it
constexpr int AMBIENCE_FADE_MS = 1500;
} // namespace

// Mix_FadeInMusic and Mix_FreeMusic wait for a fade-out to end: the new file starts on the first update after it.
void Music::Play(const std::string& filename) {
	if (filename != wanted) {
		wanted = filename;
		if (Mix_PlayingMusic() != 0)
			Mix_FadeOutMusic(MUSIC_FADE_MS);
	}
	if (Mix_PlayingMusic() != 0)
		return;
	if (wanted != loaded) {
		if (music)
			Mix_FreeMusic(music);
		loaded = wanted;
		music = Mix_LoadMUS(loaded.c_str());
		if (!music)
			LOG_ERRORF("audio", "Music load error: %s: %s", loaded.c_str(), Mix_GetError());
	}
	if (music)
		Mix_FadeInMusic(music, -1, MUSIC_FADE_MS);
}

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
	gEffectsVolume = effects * MIX_MAX_VOLUME / 100;
	Mix_Volume(-1, gEffectsVolume);
}
