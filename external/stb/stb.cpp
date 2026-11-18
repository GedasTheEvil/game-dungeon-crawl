// The stb implementations (images, the Ogg Vorbis decoder), in a translation unit of their own: built with the game
// and the tools, not checked by clang-tidy (make tidy runs on src/ only).
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// The music's Ogg Vorbis decoder (plan sdl3-audio), for the game's mixer (src/audio/mixer.cpp).
#include "stb_vorbis.c"
