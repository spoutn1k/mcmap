#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <stdint.h>
#include <cstdlib>

#define UNDEFINED 0x7FFFFFFF

enum Orientation {
	North,
	East,
	South,
	West
};

// see globals.cpp

extern int g_FromChunkX, g_FromChunkZ, g_ToChunkX, g_ToChunkZ;
extern size_t g_MapsizeY, g_MapsizeZ, g_MapsizeX;

extern int G_FROMX, G_FROMZ, G_TOX, G_TOZ;

extern Orientation g_Orientation;
extern bool g_Nightmode;
extern bool g_Underground;
extern bool g_Skylight;
extern int g_Noise;

extern uint8_t *g_Terrain, *g_Light;

#endif
