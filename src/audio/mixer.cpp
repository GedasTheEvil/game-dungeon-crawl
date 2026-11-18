#include "mixer.h"
#include <SDL3/SDL_init.h>
#include <algorithm>
#include "../core/logger.h"
#include "../../external/stb/stb_vorbis.h"

float GainRamp::At(uint64_t nowMs) const {
	if (Done(nowMs))
		return to;
	if (nowMs <= startMs)
		return from;
	const float t = static_cast<float>(nowMs - startMs) / static_cast<float>(durationMs);
	return from + (to - from) * t;
}

bool GainRamp::Done(uint64_t nowMs) const { return nowMs >= startMs + static_cast<uint64_t>(std::max(durationMs, 0)); }

namespace {
GainRamp steady(float level) { return {.from = level, .to = level}; }

bool validChannel(int channel) { return channel >= 0 && channel < Mixer::CHANNELS; }
} // namespace

Mixer::Mixer() {
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		LOG_ERRORF("audio", "Unable to start SDL audio: %s", SDL_GetError());
		return;
	}
	device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
	if (device == 0 || !SDL_GetAudioDeviceFormat(device, &spec, nullptr)) {
		LOG_ERRORF("audio", "Unable to open audio: %s", SDL_GetError());
		if (device != 0)
			SDL_CloseAudioDevice(device);
		device = 0;
		return;
	}
	for (Channel& channel : channels) {
		channel.stream = SDL_CreateAudioStream(&spec, &spec);
		SDL_SetAudioStreamGetCallback(channel.stream, &Mixer::refillLoop, &channel);
		SDL_BindAudioStream(device, channel.stream);
	}
	// The source format follows each track (SDL_SetAudioStreamFormat in LoadMusic).
	music.stream = SDL_CreateAudioStream(&spec, &spec);
	SDL_SetAudioStreamGetCallback(music.stream, &Mixer::decodeMusic, &music);
	SDL_BindAudioStream(device, music.stream);
}

Mixer::~Mixer() {
	CloseDevice();
	for (Channel& channel : channels)
		SDL_DestroyAudioStream(channel.stream);
	SDL_DestroyAudioStream(music.stream);
	if (music.vorbis != nullptr)
		stb_vorbis_close(music.vorbis);
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

Clip Mixer::LoadWav(const char path[]) const {
	if (!Open())
		return {};
	SDL_AudioSpec fileSpec{};
	Uint8* data = nullptr;
	Uint32 length = 0;
	if (!SDL_LoadWAV(path, &fileSpec, &data, &length)) {
		LOG_ERRORF("audio", "Sound load error: %s: %s", path, SDL_GetError());
		return {};
	}
	Uint8* converted = nullptr;
	int convertedLength = 0;
	const bool ok =
		SDL_ConvertAudioSamples(&fileSpec, data, static_cast<int>(length), &spec, &converted, &convertedLength);
	SDL_free(data);
	if (!ok) {
		LOG_ERRORF("audio", "Sound convert error: %s: %s", path, SDL_GetError());
		return {};
	}
	Clip clip(converted, converted + convertedLength);
	SDL_free(converted);
	return clip;
}

int Mixer::Play(const Clip& clip, int channel, float volume, bool loop, int fadeMs, uint64_t nowMs) {
	if (!Open() || clip.empty())
		return -1;
	if (channel < 0) {
		for (int i = RESERVED; i < CHANNELS && channel < 0; i++)
			if (!Playing(i))
				channel = i;
		if (channel < 0)
			return -1;
	}
	if (!validChannel(channel))
		return -1;
	Channel& played = channels[static_cast<size_t>(channel)];
	played.volume = std::clamp(volume, 0.f, 1.f);
	played.fade = fadeMs > 0 ? GainRamp{.from = 0.f, .to = 1.f, .startMs = nowMs, .durationMs = fadeMs} : steady(1.f);
	applyGain(played);
	SDL_LockAudioStream(played.stream);
	SDL_ClearAudioStream(played.stream);
	played.loop = loop ? &clip : nullptr;
	SDL_PutAudioStreamData(played.stream, clip.data(), static_cast<int>(clip.size()));
	SDL_UnlockAudioStream(played.stream);
	return channel;
}

void Mixer::Halt(int channel) {
	if (!Open() || !validChannel(channel))
		return;
	Channel& halted = channels[static_cast<size_t>(channel)];
	SDL_LockAudioStream(halted.stream);
	halted.loop = nullptr;
	SDL_ClearAudioStream(halted.stream);
	SDL_UnlockAudioStream(halted.stream);
}

bool Mixer::Playing(int channel) const {
	if (!Open() || !validChannel(channel))
		return false;
	const Channel& asked = channels[static_cast<size_t>(channel)];
	SDL_LockAudioStream(asked.stream);
	const bool playing = asked.loop != nullptr || SDL_GetAudioStreamQueued(asked.stream) > 0;
	SDL_UnlockAudioStream(asked.stream);
	return playing;
}

void Mixer::SetVolume(int channel, float volume) {
	if (!Open())
		return;
	for (int i = 0; i < CHANNELS; i++)
		if (channel < 0 || channel == i) {
			channels[static_cast<size_t>(i)].volume = std::clamp(volume, 0.f, 1.f);
			applyGain(channels[static_cast<size_t>(i)]);
		}
}

float Mixer::Gain(int channel) const {
	return Open() && validChannel(channel) ? SDL_GetAudioStreamGain(channels[static_cast<size_t>(channel)].stream)
										   : 0.f;
}

bool Mixer::LoadMusic(const std::string& path) {
	if (!Open())
		return false;
	music.stopping = false;
	int error = 0;
	stb_vorbis* vorbis = stb_vorbis_open_filename(path.c_str(), &error, nullptr);
	if (vorbis == nullptr) {
		const char* why = error == VORBIS_file_open_failure ? "cannot open the file" : "not an Ogg Vorbis file";
		LOG_ERRORF("audio", "Music load error: %s: %s (stb_vorbis error %d)", path.c_str(), why, error);
		return false;
	}
	const stb_vorbis_info info = stb_vorbis_get_info(vorbis);
	const SDL_AudioSpec trackSpec{
		.format = SDL_AUDIO_F32,
		.channels = info.channels,
		.freq = static_cast<int>(info.sample_rate)
	};
	SDL_LockAudioStream(music.stream);
	closeMusic();
	music.vorbis = vorbis;
	music.channels = info.channels;
	SDL_SetAudioStreamFormat(music.stream, &trackSpec, nullptr);
	SDL_UnlockAudioStream(music.stream);
	return true;
}

void Mixer::FadeInMusic(int fadeMs, uint64_t nowMs) {
	if (!Open() || music.vorbis == nullptr)
		return;
	music.stopping = false;
	music.fade = fadeMs > 0 ? GainRamp{.from = 0.f, .to = 1.f, .startMs = nowMs, .durationMs = fadeMs} : steady(1.f);
	applyMusicGain();
	SDL_LockAudioStream(music.stream);
	SDL_ClearAudioStream(music.stream);
	stb_vorbis_seek_start(music.vorbis);
	music.playing = true;
	SDL_UnlockAudioStream(music.stream);
}

void Mixer::FadeOutMusic(int fadeMs, uint64_t nowMs) {
	if (!MusicPlaying())
		return;
	music.stopping = true;
	music.fade = GainRamp{.from = music.fade.At(nowMs), .to = 0.f, .startMs = nowMs, .durationMs = fadeMs};
	Update(nowMs);
}

void Mixer::HaltMusic() {
	if (!Open())
		return;
	music.stopping = false;
	SDL_LockAudioStream(music.stream);
	music.playing = false;
	SDL_ClearAudioStream(music.stream);
	SDL_UnlockAudioStream(music.stream);
}

bool Mixer::MusicPlaying() const {
	if (!Open())
		return false;
	SDL_LockAudioStream(music.stream);
	const bool playing = music.playing;
	SDL_UnlockAudioStream(music.stream);
	return playing;
}

void Mixer::SetMusicVolume(float volume) {
	music.volume = std::clamp(volume, 0.f, 1.f);
	applyMusicGain();
}

float Mixer::MusicGain() const { return Open() ? SDL_GetAudioStreamGain(music.stream) : 0.f; }

void Mixer::Update(uint64_t now) {
	lastUpdateMs = now;
	if (!Open())
		return;
	for (const Channel& channel : channels)
		applyGain(channel);
	applyMusicGain();
	if (music.stopping && music.fade.Done(now))
		HaltMusic();
}

void Mixer::CloseDevice() {
	if (device != 0)
		SDL_CloseAudioDevice(device);
	device = 0;
}

void Mixer::applyGain(const Channel& channel) const {
	if (channel.stream != nullptr)
		SDL_SetAudioStreamGain(channel.stream, channel.volume * channel.fade.At(lastUpdateMs));
}

void Mixer::applyMusicGain() const {
	if (music.stream != nullptr)
		SDL_SetAudioStreamGain(music.stream, music.volume * music.fade.At(lastUpdateMs));
}

// Under the stream's lock.
void Mixer::closeMusic() {
	music.playing = false;
	SDL_ClearAudioStream(music.stream);
	if (music.vorbis != nullptr)
		stb_vorbis_close(music.vorbis);
	music.vorbis = nullptr;
}

// The audio thread, under the stream's lock: a looped clip is put again while the device wants more.
void SDLCALL Mixer::refillLoop(void* userdata, SDL_AudioStream* stream, int additional, int /*total*/) {
	const Channel& channel = *static_cast<const Channel*>(userdata);
	if (channel.loop == nullptr || channel.loop->empty())
		return;
	const int size = static_cast<int>(channel.loop->size());
	for (int put = 0; put < additional; put += size)
		SDL_PutAudioStreamData(stream, channel.loop->data(), size);
}

// The audio thread, under the stream's lock: decodes what the device wants, from the start again at the end.
void SDLCALL Mixer::decodeMusic(void* userdata, SDL_AudioStream* stream, int additional, int /*total*/) {
	MusicTrack& track = *static_cast<MusicTrack*>(userdata);
	if (!track.playing || track.vorbis == nullptr || track.channels <= 0)
		return;
	constexpr int BUFFER_FLOATS = 4096;
	std::array<float, BUFFER_FLOATS> buffer{};
	bool rewound = false;
	while (additional > 0) {
		const int frames =
			stb_vorbis_get_samples_float_interleaved(track.vorbis, track.channels, buffer.data(), BUFFER_FLOATS);
		if (frames <= 0) {
			if (rewound) { // nothing decodes from the start either: a broken file, stop
				track.playing = false;
				return;
			}
			stb_vorbis_seek_start(track.vorbis);
			rewound = true;
			continue;
		}
		rewound = false;
		const int bytes = frames * track.channels * static_cast<int>(sizeof(float));
		SDL_PutAudioStreamData(stream, buffer.data(), bytes);
		additional -= bytes;
	}
}
