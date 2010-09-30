#ifndef _WORLDLOADER_H_
#define _WORLDLOADER_H_

#include <cstdlib>

bool scanWorldDirectory(const char *fromPath);
bool loadTerrain(const char *fromPath);
bool loadEntireTerrain();
size_t calcTerrainSize();

#endif
