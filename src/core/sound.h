#ifndef SoundH
#define SoundH

#include <SDL/SDL_mixer.h>

class Sound {
  private:
	Mix_Chunk* data;
	Mix_Music* Mdata;
	bool WAV;
	bool OGG;

  public:
	Sound();
	~Sound();
	bool LoadWAV(const char filename[]);
	bool LoadOGG(const char filename[]);
	void Play();
};

#endif
