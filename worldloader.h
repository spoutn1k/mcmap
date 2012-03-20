#ifndef _WORLDLOADER_H_
#define _WORLDLOADER_H_

#include <cstdlib>
#include <stdint.h>

int getWorldFormat(const char *worldPath);
bool scanWorldDirectory(const char *fromPath);
bool loadTerrain(const char *fromPath, int &loadedChunks);
bool loadEntireTerrain();
uint64_t calcTerrainSize(const int chunksX, const int chunksZ);
void clearLightmap();
void calcBitmapOverdraw(int &left, int &right, int &top, int &bottom);
void loadBiomeMap(const char* path);
void uncoverNether();

#endif
