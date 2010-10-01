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

extern int S_FROMX, S_FROMZ, S_TOX, S_TOZ;
extern size_t MAPSIZE_Y, MAPSIZE_Z, MAPSIZE_X;

extern Orientation g_Orientation;
extern bool g_Nightmode;
extern bool g_Underground;
extern bool g_Skylight;
extern int g_Noise;

extern uint8_t *g_Terrain, *g_Light;

#endif
