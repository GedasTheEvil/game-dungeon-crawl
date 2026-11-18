#include "sound.h"
#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <memory>
#include "../core/logger.h"
#include "mixer.h"

namespace {
int gAudioUsers = 0;
std::unique_ptr<Mixer> gMixer; // while an AudioUser lives
// Audio::SetVolumes, 0..1
float gMusicVolume = 1.f;
float gEffectsVolume = 1.f;

constexpr int MUSIC_FADE_MS = 1500;
constexpr int AMBIENCE_CHANNEL = 0; // reserved (Mixer::RESERVED): the effects never take it
constexpr int AMBIENCE_FADE_MS = 1500;

uint64_t nowMs() { return SDL_GetTicks(); }
} // namespace

AudioUser::AudioUser() {
	if (gAudioUsers++ > 0)
		return;
	gMixer = std::make_unique<Mixer>();
	gMixer->SetMusicVolume(gMusicVolume);
	gMixer->SetVolume(-1, gEffectsVolume);
}

AudioUser::~AudioUser() {
	if (--gAudioUsers == 0)
		gMixer.reset();
}

bool Sound::Load(const char filename[]) {
	if (!clip.empty()) {
		LOG_ERRORF("audio", "Sound already loaded, not loading %s", filename);
		return false;
	}
	clip = gMixer->LoadWav(filename);
	return !clip.empty();
}

// Every play sets its channel's volume: one played quieter leaves none behind for the next sound on the channel.
int Sound::Play(float gain) const {
	return gMixer->Play(clip, -1, gEffectsVolume * std::clamp(gain, 0.f, 1.f), false, 0, nowMs());
}

void Sound::Stop(int channel) {
	if (gMixer)
		gMixer->Halt(channel);
}

Music::~Music() { gMixer->HaltMusic(); }

// The new file starts on the first update after the fade-out of the one playing ends.
void Music::Play(const std::string& filename) {
	const uint64_t now = nowMs();
	gMixer->Update(now);
	if (filename != wanted) {
		wanted = filename;
		gMixer->FadeOutMusic(MUSIC_FADE_MS, now);
	}
	if (gMixer->MusicPlaying())
		return;
	if (wanted != loaded) {
		loaded = wanted;
		loadedOk = gMixer->LoadMusic(loaded);
	}
	if (loadedOk)
		gMixer->FadeInMusic(MUSIC_FADE_MS, now);
}

Ambience::~Ambience() { gMixer->Halt(AMBIENCE_CHANNEL); }

void Ambience::Play(const std::string& filename) {
	if (filename == playing)
		return;
	playing = filename;
	gMixer->Halt(AMBIENCE_CHANNEL);
	clip.clear();
	if (filename.empty())
		return;
	clip = gMixer->LoadWav(filename.c_str());
	gMixer->Play(clip, AMBIENCE_CHANNEL, gEffectsVolume, true, AMBIENCE_FADE_MS, nowMs());
}

void Audio::SetVolumes(int music, int effects) {
	gMusicVolume = static_cast<float>(music) / 100.f;
	gEffectsVolume = static_cast<float>(effects) / 100.f;
	if (!gMixer)
		return;
	gMixer->SetMusicVolume(gMusicVolume);
	gMixer->SetVolume(-1, gEffectsVolume);
}

void Audio::CloseDevice() {
	if (gMixer)
		gMixer->CloseDevice();
}
