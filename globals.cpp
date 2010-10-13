#include "globals.h"

// Current window of world being rendered
int g_FromChunkX = UNDEFINED, g_FromChunkZ = UNDEFINED, g_ToChunkX = UNDEFINED, g_ToChunkZ = UNDEFINED;
size_t g_MapsizeZ = 0, g_MapsizeY = 128, g_MapsizeX = 0;

Orientation g_Orientation = East;
bool g_Nightmode = false;
bool g_Underground = false;
bool g_BlendUnderground = false;
bool g_Skylight = false;
int g_Noise = 0;

uint8_t *g_Terrain = NULL, *g_Light = NULL;
