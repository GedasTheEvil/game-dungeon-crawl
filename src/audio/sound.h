#ifndef SOUND_H
#define SOUND_H

#include <string>
#include <vector>

// Opens the audio device (the Mixer, mixer.h) while at least one instance lives. Base of every sound and music object.
class AudioUser {
  protected:
	AudioUser();
	~AudioUser();

  public:
	AudioUser(const AudioUser&) = delete;
	AudioUser& operator=(const AudioUser&) = delete;
};

// A WAV effect.
class Sound : AudioUser {
  private:
	std::vector<unsigned char> clip; // PCM in the device format (Mixer::LoadWav); empty: not loaded

  public:
	Sound() = default;
	bool Load(const char filename[]);
	// The channel it plays on, for Stop; -1: none (not loaded, no free channel). gain: of the effects volume, 0..1.
	int Play(float gain = 1.f) const;
	static void Stop(int channel); // -1: nothing
};

// Looped background music (OGG), one file at a time (plan zone-music), at the music volume.
class Music : AudioUser {
  private:
	std::string wanted; // the file asked for last
	std::string loaded; // the file in the mixer's music decoder
	bool loadedOk = false; // false: loaded failed to open

  public:
	Music() = default;
	~Music();
	// Each update: the file playing goes on; another fades the one playing out, then fades in, looped. Also steps
	// every fade of the mixer (Mixer::Update): the game calls it once per update tick.
	void Play(const std::string& filename);
};

// A looped background sound on a channel of its own (plan zone-looks: the zone's ambience), at the effects volume.
class Ambience : AudioUser {
  private:
	std::vector<unsigned char> clip; // the loop, which its channel reads while it plays
	std::string playing; // the file looping now

  public:
	Ambience() = default;
	~Ambience();
	// Fades the file in, looped; the one already playing goes on. Empty: silence.
	void Play(const std::string& filename);
};

// Music and effects volume, 0-100 each (Options > Sound).
namespace Audio {
void SetVolumes(int music, int effects);
// Closes the audio device (its thread ends), keeps the mixer: sounds still play, unheard. For a scenario batch,
// before it forks (Scenario::loadBatch).
void CloseDevice();
} // namespace Audio

#endif
