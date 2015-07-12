#include "timer.h"
#include <SDL/SDL_timer.h>
#include <SDL/SDL.h>

timer::timer()
{
     if ( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
     {
	  printf(  "Video/Audio initialization failed: %s\n", SDL_GetError( ) );
     }
     
     time_start = SDL_GetTicks();
     ticks = CDefTime;
}

timer::timer(int defT)
{
     if ( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
     {
	  printf(  "Video/Audio initialization failed: %s\n", SDL_GetError( ) );
     }
     
     time_start = SDL_GetTicks();
     ticks = defT;
}

timer::~timer()
{
     SDL_Quit( );
     printf("Deleting Timer %x \n",this);
}

bool timer::TimePassed()
{
     int xxx = SDL_GetTicks();
     if(xxx - time_start >= ticks)
     {
	   time_start = SDL_GetTicks();
	   return 1;
	   
     }
     return 0;
}

bool timer::TimePassed(bool noRepeat)
{
     int xxx = SDL_GetTicks();
     if(xxx - time_start >= ticks)
     {
	   if(!noRepeat)
		 time_start = SDL_GetTicks();
	   return 1;
	   
     }
     return 0;
}

void timer::Reset()
{
     time_start = SDL_GetTicks();
}