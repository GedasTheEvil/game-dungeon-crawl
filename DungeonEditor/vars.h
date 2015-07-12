#ifndef GameVars
#define GameVars

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>
#include <GL/glut.h>    // Header File For The GLUT Library 
#include <GL/gl.h>	// Header File For The OpenGL32 Library
#include <GL/glu.h>	// Header File For The GLu32 Library
#include <unistd.h>     // Header file for sleeping.
#include <stdio.h>      // Header file for standard file i/o.
#include <stdlib.h>     // Header file for malloc/free.
#include <math.h>       // Header file for trigonometric functions.
#include <stdarg.h>
#include <string.h>

     // 	mouse
     #define wheel_up    4
     #define wheel_down  3
     #define RMB         2
     #define MMB         1
     #define LMB         0

     #define intex 4
     #define buttons 32

     #define items 0x90
     
     int DungeonNo = 0;
     
     int Atr1 = 0;
     int Atr2 = 0;
     
     int selA1  = 0;
     int selA1V = 0;
     float selMN  = 0;
     
     char AtText1[5]= {0};
     int  AtText1L = 0;
     char AtText2[7] = {0};
     int  AtText2L = 0;
     
     char AtText3[6] = {0};
     int  AtText3L = 0;
     
     typedef struct 
     {
	   int type;
	   int Atr;
	   int AtrV;
     }cell;
     
     cell map[1881];
     
     int selectedB = 0;
     
     
     //Objects:
     #define Wall     0
     #define Empty    1
     #define Door     2
     #define Death    3
     #define Monster  4
     #define Spike    5
     #define Ladder   6
     #define Area3D   7
     #define Treasure 8
     #define Ankh     9
    
     int mouseX, mouseY;

     //end
     #define DegRad 0.0174532925
      int loaded ;
      int texres ;
      int resX, resY;
      unsigned int display;

  
      int window;

      GLuint	base;	// Font Display List

  
     //	text inputs
      char TextKey[32];
      int  inputSw ;
      int tl ;
     //	!text inputs

     struct Image {
	   unsigned long sizeX;
	   unsigned long sizeY;
	   char *data;
     };
     typedef struct Image Image;
     
     typedef struct			// Create A Structure
     {
	   GLubyte	*imageData;	// Image Data (Up To 32 Bits)
	   GLuint	bpp;		// Image Color Depth In Bits Per Pixel.
	   GLuint	width;		// Image Width
	   GLuint	height;		// Image Height
	   GLuint	texID;		// Texture ID Used To Select A Texture
     } TextureImage;	
     
      TextureImage  texttex  =  {NULL, 1, 2, 3, 4};
      TextureImage  nullt    =  {NULL, 1, 2, 3, 4};
      TextureImage  Mat[12]   = {{NULL, 1, 2, 3, 4}};
      TextureImage Button[2] = {{NULL, 1, 2, 3, 4}};
    
      int aaaa  ;// 0;
      int ccc   ;// 0;
      float aVX, aVZ , sVX, sVZ;
      
      

#endif
