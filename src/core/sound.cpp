#include "sound.h"
#include <SDL/SDL_mixer.h>
#include <cstdio>
#include "logger.h"

namespace {
int gSoundInstanceCount = 0;
bool gAudioOpened = false;
} // namespace

Sound::Sound() {
	gSoundInstanceCount++;

	int audioRate = 22050;
	Uint16 audioFormat = AUDIO_S16;
	int audioChannels = 2;
	int audioBuffers = 4096;

	int curRate;
	Uint16 curFormat;
	int curChannels;
	if (!gAudioOpened && Mix_QuerySpec(&curRate, &curFormat, &curChannels) == 0) {
		if (Mix_OpenAudio(audioRate, audioFormat, audioChannels, audioBuffers)) {
			LOG_ERROR("audio", "Unable to open audio!");
		} else {
			gAudioOpened = true;
		}
	} else if (Mix_QuerySpec(&curRate, &curFormat, &curChannels) != 0) {
		gAudioOpened = true;
	}

	OGG = false;
	WAV = false;
	data = nullptr;
	Mdata = nullptr;
}

Sound::~Sound() {
	if (WAV)
		Mix_FreeChunk(data);
	if (OGG)
		Mix_FreeMusic(Mdata);
	WAV = false;
	OGG = false;

	gSoundInstanceCount--;
	if (gAudioOpened && gSoundInstanceCount == 0) {
		Mix_CloseAudio();
		gAudioOpened = false;
	}

	void* selfPtr = this;
	LOG_DEBUGF("audio", "Deleting sound %p", selfPtr);
}

bool Sound::LoadWAV(const char filename[]) {
	if (WAV || OGG) {
		LOG_ERROR("audio", "Sound load error:A sound file has already been loaded");
		return false;
	}

	data = Mix_LoadWAV(filename);

	if (data == nullptr) {
		LOG_ERRORF("audio", "Sound load error: failed loading [NULL, %s]", Mix_GetError());

		return false;
	}

	WAV = true;

	return true;
}

bool Sound::LoadOGG(const char filename[]) {
	if (WAV || OGG) {
		LOG_ERROR("audio", "Sound load error:A sound file has already been loaded");
		return false;
	}

	Mdata = Mix_LoadMUS(filename);

	if (Mdata == nullptr) {
		LOG_ERRORF("audio", "Sound load error: failed loading [NULL, %s]", Mix_GetError());

		return false;
	}

	OGG = true;

	return true;
}

void Sound::Play() {
	if (WAV)
		Mix_PlayChannel(-1, data, 0);
	else if (OGG)
		Mix_PlayMusic(Mdata, -1);
	else
		LOG_ERROR("audio", "Error playing sound : No sound was loaded");
}
