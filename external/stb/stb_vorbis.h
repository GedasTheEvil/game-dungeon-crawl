// stb_vorbis's declarations only (it ships as one .c file): what the game's mixer (src/audio/mixer.cpp) includes.
// The implementation is compiled once, in stb.cpp.
#ifndef STB_VORBIS_H
#define STB_VORBIS_H
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"
#undef STB_VORBIS_HEADER_ONLY
#endif
