#ifndef _WORLDLOADER_H_
#define _WORLDLOADER_H_

#include "nbt.h"
#include "block.h"
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>
#include <stdint.h>

namespace Terrain {
	typedef NBT_Tag Chunk;
	typedef Chunk* chunkList;

	struct Coordinates {
		int32_t minX;
		int32_t minZ;
		int32_t maxX;
		int32_t maxZ;
	};

	struct Data {
		struct Coordinates map; // The position of the loaded chunks
		chunkList chunks;
	};

	Block blockAt(Terrain::Data&, int32_t x, int32_t z, int32_t y);
}

int getWorldFormat(const char *worldPath);
bool scanWorldDirectory(const char *fromPath);
bool loadTerrain(const char *fromPath, int &loadedChunks);
NBT* loadChunk(const char *fromPath, int x, int z);
void freeTerrain();
bool loadEntireTerrain();
uint64_t calcTerrainSize(const int chunksX, const int chunksZ);
void clearLightmap();
void calcBitmapOverdraw(int &left, int &right, int &top, int &bottom);
void loadBiomeMap(const char* path);
void uncoverNether();

void _loadTerrain(Terrain::Data&, std::filesystem::path, Terrain::Coordinates&);

#endif
