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
extern int g_ColormapFromX, g_ColormapToX, g_ColormapFromZ, g_ColormapToZ, g_GrassLineWidth, g_LeafLineWidth;
extern uint8_t *g_Grasscolor, *g_Leafcolor;

// 3D arrays holding terrain/lightmap
extern uint8_t *g_Terrain, *g_Light;

#endif
