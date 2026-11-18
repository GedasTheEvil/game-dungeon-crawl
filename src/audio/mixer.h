#ifndef MIXER_H
#define MIXER_H

#include <SDL3/SDL_audio.h>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

// stb_vorbis's decoder (external/stb/stb_vorbis.c), opaque here.
struct stb_vorbis;

// A fade: the level goes in a straight line from one gain to another over a time, then stays there.
struct GainRamp {
	float from = 1.f;
	float to = 1.f;
	uint64_t startMs = 0;
	int durationMs = 0;

	[[nodiscard]] float At(uint64_t nowMs) const;
	[[nodiscard]] bool Done(uint64_t nowMs) const;
};

// PCM in the mixer's device format, converted once at load (a sound effect, an ambience loop).
using Clip = std::vector<Uint8>;

// The game's audio mixer on SDL3 (plan sdl3-audio; SDL3_mixer is not packaged): one opened playback device, one
// SDL_AudioStream per channel and one for the music, all bound to it, so SDL mixes and converts them. A channel's
// volume and fade are its stream's gain; the fades move when Update steps them, once per game update. Silent (plays
// nothing, loads nothing) when the device does not open.
class Mixer {
  public:
	static constexpr int CHANNELS = 8; // as SDL_mixer had
	static constexpr int RESERVED = 1; // channel 0 (the ambience): Play with channel -1 never takes it

	// Opens the default playback device (the audio subsystem too; SDL_AUDIO_DRIVER=dummy for silence).
	Mixer();
	~Mixer();
	Mixer(const Mixer&) = delete;
	Mixer& operator=(const Mixer&) = delete;

	[[nodiscard]] bool Open() const { return device != 0; }
	[[nodiscard]] const SDL_AudioSpec& Spec() const { return spec; }
	// A WAV file in the device format; empty if it failed (logged) or no device.
	[[nodiscard]] Clip LoadWav(const char path[]) const;

	// Plays the clip on the channel (-1: the first free one past the reserved; none free: -1) at volume 0..1, from
	// silence over fadeMs. A looped clip plays until Halt and must live until then. Returns the channel or -1.
	int Play(const Clip& clip, int channel, float volume, bool loop, int fadeMs, uint64_t nowMs);
	void Halt(int channel); // -1: nothing
	[[nodiscard]] bool Playing(int channel) const;
	void SetVolume(int channel, float volume); // -1: every channel
	[[nodiscard]] float Gain(int channel) const; // its stream's gain now: volume times fade

	// Opens the .ogg (stb_vorbis) and stops the music playing; false if it failed (logged) or no device.
	bool LoadMusic(const std::string& path);
	// The loaded music from its start, looped, from silence over fadeMs.
	void FadeInMusic(int fadeMs, uint64_t nowMs);
	// To silence from its level now; it stops at the end (MusicPlaying is true until then).
	void FadeOutMusic(int fadeMs, uint64_t nowMs);
	void HaltMusic();
	[[nodiscard]] bool MusicPlaying() const;
	void SetMusicVolume(float volume);
	[[nodiscard]] float MusicGain() const;

	// Steps the fades to nowMs and stops the music whose fade-out ended. Once per game update.
	void Update(uint64_t nowMs);
	// Closes the device (its thread ends); the streams stay, unheard. For a scenario batch before it forks.
	void CloseDevice();

  private:
	struct Channel {
		SDL_AudioStream* stream = nullptr;
		const Clip* loop = nullptr; // the looped clip, put again whenever the stream runs dry (audio thread)
		float volume = 1.f;
		GainRamp fade;
	};
	struct MusicTrack {
		SDL_AudioStream* stream = nullptr;
		stb_vorbis* vorbis = nullptr; // the decoder (audio thread, under the stream's lock)
		int channels = 0;
		bool playing = false; // under the stream's lock
		bool stopping = false; // a fade-out: halts when the fade ends
		float volume = 1.f;
		GainRamp fade;
	};

	static void SDLCALL refillLoop(void* userdata, SDL_AudioStream* stream, int additional, int total);
	static void SDLCALL decodeMusic(void* userdata, SDL_AudioStream* stream, int additional, int total);
	void applyGain(const Channel& channel) const;
	void applyMusicGain() const;
	void closeMusic();

	SDL_AudioDeviceID device = 0;
	SDL_AudioSpec spec{};
	std::array<Channel, CHANNELS> channels{};
	MusicTrack music;
	uint64_t lastUpdateMs = 0; // the last Update's time
};

#endif
