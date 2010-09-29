#include <stdint.h>
#include <cstdlib>

int S_FROMX = 0, S_FROMZ = 0, S_TOX = 0, S_TOZ = 0;
int MAPSIZE_Y = 0, MAPSIZE_Z = 0, MAPSIZE_X = 0;
bool g_Nightmode = false;
bool g_Underground = false;
uint8_t *g_Terrain = NULL, *g_Light = NULL;
