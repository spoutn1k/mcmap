#include "globals.h"

// Current window of world being rendered
int S_FROMX = UNDEFINED, S_FROMZ = UNDEFINED, S_TOX = UNDEFINED, S_TOZ = UNDEFINED;
size_t MAPSIZE_Z = 0, MAPSIZE_Y = 128, MAPSIZE_X = 0;

Orientation g_Orientation = East;
bool g_Nightmode = false;
bool g_Underground = false;
bool g_Skylight = false;
int g_Noise = 0;

uint8_t *g_Terrain = NULL, *g_Light = NULL;
