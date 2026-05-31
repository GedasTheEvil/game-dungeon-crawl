#include "sound.h"
#include <SDL/SDL_mixer.h>
#include <cstdio>

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
			printf("Unable to open audio!\n");
		} else {
			gAudioOpened = true;
		}
	} else if (Mix_QuerySpec(&curRate, &curFormat, &curChannels) != 0) {
		gAudioOpened = true;
	}

	OGG = 0;
	WAV = 0;
	data = nullptr;
	Mdata = nullptr;
}

Sound::~Sound() {
	if (WAV)
		Mix_FreeChunk(data);
	if (OGG)
		Mix_FreeMusic(Mdata);
	WAV = 0;
	OGG = 0;

	gSoundInstanceCount--;
	if (gAudioOpened && gSoundInstanceCount == 0) {
		Mix_CloseAudio();
		gAudioOpened = false;
	}

	printf("Deleting sound %p \n", (void*)this);
}

bool Sound::LoadWAV(const char filename[]) {
	if (WAV || OGG) {
		printf("Sound load error:A sound file has already been loaded\n");
		return 0;
	}

	data = Mix_LoadWAV(filename);

	if (data == nullptr) {
		printf("Sound load error: failed loading [NULL, %s]\n", Mix_GetError());

		return 0;
	}

	WAV = 1;

	return 1;
}

bool Sound::LoadOGG(const char filename[]) {
	if (WAV || OGG) {
		printf("Sound load error:A sound file has already been loaded\n");
		return 0;
	}

	Mdata = Mix_LoadMUS(filename);

	if (Mdata == nullptr) {
		printf("Sound load error: failed loading [NULL, %s]\n", Mix_GetError());

		return 0;
	}

	OGG = 1;

	return 1;
}

void Sound::Play() {
	if (WAV)
		Mix_PlayChannel(-1, data, 0);
	else if (OGG)
		Mix_PlayMusic(Mdata, -1);
	else
		printf("Error playing sound : No sound was loaded\n");
}
