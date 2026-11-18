// The audio mixer on SDL3 streams (plan sdl3-audio): the fades' gain ramps and the channels' bookkeeping, on the
// dummy driver (silent).
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_timer.h>
#include "../../external/doctest/doctest.h"
#include "../../src/audio/mixer.h"
#include "../../src/core/paths.h"

namespace {
// The dummy driver plays nothing out loud; the hint is read when the audio subsystem starts (the Mixer).
Mixer& silentMixer() {
	SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
	static Mixer mixer;
	return mixer;
}

// Seconds of silence in the device format: long enough to stay queued for the whole test.
Clip silence(const Mixer& mixer, int seconds) {
	const SDL_AudioSpec& spec = mixer.Spec();
	return Clip(static_cast<size_t>(SDL_AUDIO_FRAMESIZE(spec) * spec.freq * seconds), 0);
}

void haltAll(Mixer& mixer) {
	for (int channel = 0; channel < Mixer::CHANNELS; channel++)
		mixer.Halt(channel);
}
} // namespace

TEST_CASE("GainRamp: a straight line from one level to another, steady before and after") {
	const GainRamp ramp{.from = 0.f, .to = 1.f, .startMs = 1000, .durationMs = 1500};
	CHECK(ramp.At(0) == doctest::Approx(0.f));
	CHECK(ramp.At(1000) == doctest::Approx(0.f));
	CHECK(ramp.At(1750) == doctest::Approx(0.5f));
	CHECK_FALSE(ramp.Done(2499));
	CHECK(ramp.Done(2500));
	CHECK(ramp.At(2500) == doctest::Approx(1.f));
	CHECK(ramp.At(9000) == doctest::Approx(1.f));
	const GainRamp out{.from = 0.8f, .to = 0.f, .startMs = 0, .durationMs = 1000};
	CHECK(out.At(250) == doctest::Approx(0.6f));
	const GainRamp none{.from = 0.3f, .to = 0.7f, .startMs = 50, .durationMs = 0};
	CHECK(none.Done(50));
	CHECK(none.At(50) == doctest::Approx(0.7f));
}

TEST_CASE("Mixer: an effect takes the first free channel past the reserved one, none free gives -1") {
	Mixer& mixer = silentMixer();
	REQUIRE(mixer.Open());
	haltAll(mixer);
	const Clip clip = silence(mixer, 5);
	for (int expected = Mixer::RESERVED; expected < Mixer::CHANNELS; expected++)
		CHECK(mixer.Play(clip, -1, 1.f, false, 0, 0) == expected);
	CHECK(mixer.Play(clip, -1, 1.f, false, 0, 0) == -1);
	CHECK_FALSE(mixer.Playing(0));

	mixer.Halt(3);
	CHECK_FALSE(mixer.Playing(3));
	CHECK(mixer.Play(clip, -1, 1.f, false, 0, 0) == 3);
	// The reserved channel plays only when asked for by number.
	CHECK(mixer.Play(clip, 0, 1.f, false, 0, 0) == 0);
	CHECK(mixer.Playing(0));
	CHECK(mixer.Play(Clip(), -1, 1.f, false, 0, 0) == -1); // nothing loaded
	mixer.Halt(-1); // nothing
	haltAll(mixer);
	for (int channel = 0; channel < Mixer::CHANNELS; channel++)
		CHECK_FALSE(mixer.Playing(channel));
}

TEST_CASE("Mixer: a looped clip plays until halted, even a short one") {
	Mixer& mixer = silentMixer();
	REQUIRE(mixer.Open());
	haltAll(mixer);
	const Clip shortClip(static_cast<size_t>(SDL_AUDIO_FRAMESIZE(mixer.Spec())), 0); // one frame
	CHECK(mixer.Play(shortClip, 0, 1.f, true, 0, 0) == 0);
	SDL_Delay(100); // the dummy device pulls the frame many times over
	CHECK(mixer.Playing(0));
	mixer.Halt(0);
	CHECK_FALSE(mixer.Playing(0));
}

TEST_CASE("Mixer: a channel's gain is its volume times its fade, stepped by Update") {
	Mixer& mixer = silentMixer();
	REQUIRE(mixer.Open());
	haltAll(mixer);
	mixer.Update(0);
	const Clip clip = silence(mixer, 5);
	const int channel = mixer.Play(clip, -1, 0.8f, false, 1000, 0);
	REQUIRE(channel >= 0);
	CHECK(mixer.Gain(channel) == doctest::Approx(0.f));
	mixer.Update(500);
	CHECK(mixer.Gain(channel) == doctest::Approx(0.4f));
	mixer.Update(1000);
	CHECK(mixer.Gain(channel) == doctest::Approx(0.8f));
	mixer.SetVolume(-1, 0.5f); // Options: every channel at once
	CHECK(mixer.Gain(channel) == doctest::Approx(0.5f));
	// A new play on the channel leaves no volume behind.
	CHECK(mixer.Play(clip, channel, 0.25f, false, 0, 1000) == channel);
	CHECK(mixer.Gain(channel) == doctest::Approx(0.25f));
	CHECK(mixer.Play(clip, Mixer::CHANNELS, 1.f, false, 0, 0) == -1); // no such channel
	haltAll(mixer);
}

TEST_CASE("Mixer: the music fades in, follows its volume, fades out and stops") {
	Mixer& mixer = silentMixer();
	REQUIRE(mixer.Open());
	mixer.Update(0);
	CHECK_FALSE(mixer.LoadMusic(MEDIA_DIR "sounds/music/no_such_track.ogg"));
	REQUIRE(mixer.LoadMusic(MEDIA_DIR "sounds/music/menu.ogg"));
	CHECK_FALSE(mixer.MusicPlaying());
	mixer.SetMusicVolume(0.5f);
	mixer.FadeInMusic(1500, 0);
	CHECK(mixer.MusicPlaying());
	CHECK(mixer.MusicGain() == doctest::Approx(0.f));
	mixer.Update(750);
	CHECK(mixer.MusicGain() == doctest::Approx(0.25f));
	mixer.Update(1500);
	CHECK(mixer.MusicGain() == doctest::Approx(0.5f));
	mixer.SetMusicVolume(1.f);
	CHECK(mixer.MusicGain() == doctest::Approx(1.f));

	mixer.FadeOutMusic(1500, 2000);
	mixer.Update(2750);
	CHECK(mixer.MusicPlaying()); // until the fade ends
	CHECK(mixer.MusicGain() == doctest::Approx(0.5f));
	mixer.Update(3500);
	CHECK_FALSE(mixer.MusicPlaying());

	// A fade-out cut short by a fade-in: back from silence.
	mixer.FadeInMusic(1000, 4000);
	mixer.FadeOutMusic(1000, 4500); // from half way up, down over a second
	mixer.Update(4750);
	CHECK(mixer.MusicGain() == doctest::Approx(0.375f));
	mixer.HaltMusic();
	CHECK_FALSE(mixer.MusicPlaying());
}
