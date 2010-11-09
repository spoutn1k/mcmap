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
extern int g_OffsetY; // y pixel offset in the final image for one y step in 3d array

extern Orientation g_Orientation; // North, West, South, East
extern bool g_Nightmode;
extern bool g_Underground;
extern bool g_BlendUnderground;
extern bool g_Skylight;
extern int g_Noise;
extern bool g_BlendAll; // If set, do not assume certain blocks (like grass) are always opaque
extern bool g_Hell, g_ServerHell; // rendering the nether

// For rendering biome colors properly, external png files are used
extern uint64_t g_BiomeMapSize;
extern uint8_t *g_Grasscolor, *g_Leafcolor;
extern uint16_t *g_BiomeMap;
extern int g_GrasscolorDepth, g_FoliageDepth;

// 3D arrays holding terrain/lightmap
extern uint8_t *g_Terrain, *g_Light;

#endif
