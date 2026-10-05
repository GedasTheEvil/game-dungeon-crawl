#ifndef SOUND_H
#define SOUND_H

#include <SDL/SDL_mixer.h>

// Opens the SDL_mixer device while at least one instance lives. Base of every sound and music object.
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
	Mix_Chunk* chunk = nullptr;

  public:
	Sound() = default;
	~Sound();
	bool Load(const char filename[]);
	void Play() const;
};

// Looped background music (OGG).
class Music : AudioUser {
  private:
	Mix_Music* music = nullptr;

  public:
	Music() = default;
	~Music();
	bool Load(const char filename[]);
	void Play() const;
};

// Music and effects volume, 0-100 each (Options > Sound).
namespace Audio {
void SetVolumes(int music, int effects);
} // namespace Audio

#endif
