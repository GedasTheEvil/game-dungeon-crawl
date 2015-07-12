#ifndef TEXTUROS
#define TEXTUROS

#include <GL/gl.h>

struct TextureImage               // Create A Structure
{
          char    *data;   // Image Data (Up To 32 Bits)
          int     bpp;          // Image Color Depth In Bits Per Pixel.
          int     width;        // Image Width
          int     height;              // Image Height
          GLuint     texID;        // Texture ID Used To Select A Texture
} ; 

class Textura
{
private:
     TextureImage *texture;
     bool loaded;
public:
     Textura();
     ~Textura();
     int LoadTGA(const char *filename);
     int LoadBMP(const char *filename);
     void Bind();
     int ID();
};

#endif
