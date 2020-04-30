#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#define VERSION "2.5.0 (compatible with Minecraft up to 1.12.x)"

#include <stdint.h>
#include <cstdlib>
#include "block.h"

#define UNDEFINED 0x7FFFFFFF
#define MAX_MARKERS 200

enum Orientation {
	NORTH,
	EAST,
	SOUTH,
	WEST
};

// Global area of world being rendered
extern int g_TotalFromChunkX, g_TotalFromChunkZ, g_TotalToChunkX, g_TotalToChunkZ;
// size of that area in blocks (no offset)
extern size_t g_MapsizeZ, g_MapsizeX;

extern int g_WorldFormat;
extern Orientation g_Orientation; // North, West, South, East
extern bool g_Nightmode;
extern bool g_Underground;
extern bool g_BlendUnderground;
extern bool g_Skylight;
extern int g_Noise;
extern bool g_BlendAll; // If set, do not assume certain blocks (like grass) are always opaque
extern bool g_Hell, g_ServerHell; // rendering the nether
extern bool g_End;
extern bool g_NoWater; // render water clear

// For rendering biome colors properly, external png files are used
extern bool g_UseBiomes;
extern uint64_t g_BiomeMapSize;
extern uint8_t *g_Grasscolor, *g_Leafcolor, *g_TallGrasscolor;
extern uint16_t *g_BiomeMap;
extern int g_GrasscolorDepth, g_FoliageDepth;
extern uint8_t g_LastDoubleFlower;

// 3D arrays holding terrain/lightmap
extern uint8_t *g_Light;

// If output is to be split up (for google maps etc) this contains the path to output to, NULL otherwise
extern char *g_TilePath;

extern int8_t g_SectionMin, g_SectionMax;

#endif
