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
bool g_BlendAll = false;
bool g_Hell = false, g_ServerHell = false;

int g_ColormapFromX = 0, g_ColormapToX = 0, g_ColormapFromZ = 0, g_ColormapToZ = 0, g_GrassLineWidth = 0, g_LeafLineWidth = 0;
uint8_t *g_Grasscolor = NULL, *g_Leafcolor = NULL;
uint8_t *g_Terrain = NULL, *g_Light = NULL;
