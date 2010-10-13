#ifndef _WORLDLOADER_H_
#define _WORLDLOADER_H_

#include <cstdlib>

bool scanWorldDirectory(const char *fromPath);
bool loadTerrain(const char *fromPath);
bool loadEntireTerrain();
size_t calcTerrainSize(int chunksX, int chunksZ);
void clearLightmap();
void calcBitmapOverdraw(int &left, int &right, int &top, int &bottom);

#endif
