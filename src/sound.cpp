#include "sound.h"
#include <SDL/SDL_mixer.h>
#include <cstdio>

namespace
{
    int g_soundInstanceCount = 0;
    bool g_audioOpened = false;
}


Sound::Sound()
{
    g_soundInstanceCount++;

    int audio_rate      = 22050;
    Uint16 audio_format = AUDIO_S16;
    int audio_channels  = 2;
    int audio_buffers   = 4096;
    
    int cur_rate; Uint16 cur_format; int cur_channels;
    if (!g_audioOpened && Mix_QuerySpec(&cur_rate, &cur_format, &cur_channels) == 0)
    {
        if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers))
        {
         	  printf(  "Unable to open audio!\n" );
        }
        else
        {
            g_audioOpened = true;
        }
    }
    else if (Mix_QuerySpec(&cur_rate, &cur_format, &cur_channels) != 0)
    {
        g_audioOpened = true;
    }

    OGG = 0;
    WAV = 0;
    data = NULL;
    Mdata = NULL;
}

Sound::~Sound()
{
     if(WAV)
	   Mix_FreeChunk(data);
     if(OGG)
	   Mix_FreeMusic(Mdata);
     WAV = 0;
     OGG = 0;

    g_soundInstanceCount--;
    if(g_audioOpened && g_soundInstanceCount == 0)
    {
        Mix_CloseAudio();
        g_audioOpened = false;
    }

    printf("Deleting sound %x \n",this);
}

bool Sound::LoadWAV(const char Filename[])
{
     if(WAV || OGG)
     {
	   printf("Sound load error:A sound file has already been loaded\n");
	   return 0;
     }
     
     data= Mix_LoadWAV(Filename);
          
     if(data == NULL)
     {
	   printf("Sound load error: failed loading [NULL, %s]\n", Mix_GetError());
	   
	   return 0;
     }
     
     WAV = 1;
     
     return 1;
}

bool Sound::LoadOGG(const char Filename[])
{
     if(WAV || OGG)
     {
	   printf("Sound load error:A sound file has already been loaded\n");
	   return 0;
     }
     
     Mdata= Mix_LoadMUS(Filename); 
 
     if(Mdata == NULL)
     {
	   printf("Sound load error: failed loading [NULL, %s]\n", Mix_GetError());
	   
	   return 0;
     }
     
     OGG = 1;
     
     return 1;
}

void Sound::Play()
{
     if(WAV)
	   Mix_PlayChannel(-1, data, 0);
     else if(OGG)
	   Mix_PlayMusic(Mdata, -1);
     else printf("Error playing sound : No sound was loaded\n");
}
