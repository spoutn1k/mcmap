#include "globals.h"

// Current window of world being rendered
int g_FromChunkX = UNDEFINED, g_FromChunkZ = UNDEFINED, g_ToChunkX = UNDEFINED, g_ToChunkZ = UNDEFINED;
size_t g_MapsizeZ = 0, g_MapsizeY = 128, g_MapminY = 0, g_MapsizeX = 0;
int g_OffsetY = 2;

Orientation g_Orientation = East;
bool g_Nightmode = false;
bool g_Underground = false;
bool g_BlendUnderground = false;
bool g_Skylight = false;
int g_Noise = 0;
bool g_BlendAll = false;
bool g_Hell = false, g_ServerHell = false;

bool g_UseBiomes = false;
uint64_t g_BiomeMapSize = 0;
uint8_t *g_Grasscolor = NULL, *g_Leafcolor = NULL;
uint16_t *g_BiomeMap = NULL;
int g_GrasscolorDepth = 0, g_FoliageDepth = 0;

uint8_t *g_Terrain = NULL, *g_Light = NULL;
