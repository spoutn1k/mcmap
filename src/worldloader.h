#ifndef WORLDLOADER_H_
#define WORLDLOADER_H_

#include "./colors.h"
#include "./helper.h"
#include "./include/nbt/nbt.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>
#include <zlib.h>

using nbt::NBT;
using std::string;
using std::vector;

void scanWorldDirectory(const std::filesystem::path &, Coordinates *);

namespace Terrain {

typedef NBT Chunk;
typedef Chunk *ChunkList;

using Coordinates = struct Coordinates;

struct Data {
  // The coordinates of the loaded chunks. This coordinates maps
  // the CHUNKS loaded, not the blocks
  Coordinates map;

  // The internal list of chunks, of size chunkLen
  ChunkList chunks;
  uint64_t chunkLen;

  // An array of bytes, one for each chunk
  // the first 4 bits are the highest section number,
  // the latter the lowest section number
  uint8_t *heightMap;

  // The extrema. The first 4 bits indicate the index of the highest section,
  // the last 4 the index of the lowest
  uint8_t heightBounds;

  vector<string> cache;

  // Default constructor
  explicit Data(const Terrain::Coordinates &coords) {
    map.minX = CHUNK(coords.minX);
    map.minZ = CHUNK(coords.minZ);
    map.maxX = CHUNK(coords.maxX);
    map.maxZ = CHUNK(coords.maxZ);

    chunkLen =
        uint64_t(map.maxX - map.minX + 1) * uint64_t(map.maxZ - map.minZ + 1);

    chunks = new Terrain::Chunk[chunkLen];
    heightMap = new uint8_t[chunkLen];

    heightBounds = 0x00;
  }

  ~Data() {
    delete[] heightMap;
    delete[] chunks;
  }

  // Chunk loading methods - only load should be useful
  void load(const std::filesystem::path &regionDir);
  void loadRegion(const std::filesystem::path &regionFile, const int regionX,
                  const int regionZ);
  void loadChunk(const uint32_t offset, FILE *regionHandle, const int chunkX,
                 const int chunkZ);

  // Chunk analysis methods - using the list of sections
  void stripChunk(vector<NBT> *);
  void cacheColors(vector<NBT> *);
  uint8_t importHeight(vector<NBT> *);
  void inflateChunk(vector<NBT> *);

  uint64_t chunkIndex(int64_t x, int64_t z) const {
    return (x - map.minX) + (z - map.minZ) * (map.maxX - map.minX + 1);
  }

  const NBT &chunkAt(int64_t xPos, int64_t zPos) const {
    return chunks[chunkIndex(xPos, zPos)];
  }

  uint8_t maxHeight() const { return heightBounds & 0xf0; }
  uint8_t minHeight() const { return (heightBounds & 0x0f) << 4; }

  uint8_t maxHeight(const int64_t x, const int64_t z) const {
    return heightMap[chunkIndex(CHUNK(x), CHUNK(z))] & 0xf0;
  }

  uint8_t minHeight(const int64_t x, const int64_t z) const {
    return (heightMap[chunkIndex(CHUNK(x), CHUNK(z))] & 0x0f) << 4;
  }

  uint8_t heightAt(int64_t xPos, int64_t zPos) const {
    return heightMap[chunkIndex(xPos, zPos)];
  }
};

} // namespace Terrain

typedef int16_t (*sectionInterpreter)(const uint64_t,
                                      const std::vector<int64_t> *, uint8_t,
                                      uint8_t, uint8_t);

int16_t blockAtPre116(const uint64_t, const std::vector<int64_t> *, uint8_t,
                      uint8_t, uint8_t);
int16_t blockAtPost116(const uint64_t, const std::vector<int64_t> *, uint8_t,
                       uint8_t, uint8_t);

#endif // WORLDLOADER_H_
