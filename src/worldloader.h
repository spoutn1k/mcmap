#ifndef WORLDLOADER_H_
#define WORLDLOADER_H_

#include "./colors.h"
#include "./helper.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <nbt/nbt.hpp>
#include <stdint.h>
#include <string>
#include <vector>
#include <zlib.h>

using nbt::NBT;
using std::string;
using std::vector;

namespace Terrain {

typedef NBT Chunk;
typedef Chunk *ChunkList;

using Coordinates = Coordinates<int32_t>;

void scanWorldDirectory(const std::filesystem::path &, Coordinates *);

struct Data {
  // The coordinates of the loaded chunks. This coordinates maps
  // the CHUNKS loaded, not the blocks
  Coordinates map;

  // The internal list of chunks, of size chunkLen
  ChunkList chunks;
  uint64_t chunkLen;

  // An array of bytes, one for each chunk
  // the first 8 bits are the highest block to render,
  // the latter the lowest section number
  uint16_t *heightMap;

  // The global version of the values above. The first 8 bits indicate the
  // highest block to render, the last 8 the lowest block
  uint16_t heightBounds;

  vector<string> cache;

  std::filesystem::path regionDir;
  Chunk empty;

  // Default constructor
  explicit Data(const Terrain::Coordinates &coords) : heightBounds(0) {
    map.minX = CHUNK(coords.minX);
    map.minZ = CHUNK(coords.minZ);
    map.maxX = CHUNK(coords.maxX);
    map.maxZ = CHUNK(coords.maxZ);

    chunkLen =
        uint64_t(map.maxX - map.minX + 1) * uint64_t(map.maxZ - map.minZ + 1);

    chunks = new Terrain::Chunk[chunkLen];
    heightMap = new uint16_t[chunkLen];
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
                 const int chunkZ, const std::filesystem::path &filename);

  // Chunk analysis methods - using the list of sections
  void stripChunk(vector<NBT> *);
  void cacheColors(vector<NBT> *);
  uint16_t importHeight(vector<NBT> *);
  void inflateChunk(vector<NBT> *);

  uint64_t chunkIndex(int64_t x, int64_t z) const {
    return (x - map.minX) + (z - map.minZ) * (map.maxX - map.minX + 1);
  }

  NBT &chunkAt(int64_t xPos, int64_t zPos);

  uint8_t maxHeight() const { return 255; }
  uint8_t minHeight() const { return 0; }

  uint8_t maxHeight(const int64_t x, const int64_t z) const {
    return heightMap[chunkIndex(x, z)] >> 8;
  }

  uint8_t minHeight(const int64_t x, const int64_t z) const {
    return heightMap[chunkIndex(x, z)] & 0xff;
  }
};

} // namespace Terrain

typedef void (*sectionInterpreter)(const uint64_t, const std::vector<int64_t> *,
                                   uint8_t *);

void sectionAtPre116(const uint64_t, const std::vector<int64_t> *, uint8_t *);
void sectionAtPost116(const uint64_t, const std::vector<int64_t> *, uint8_t *);

bool assertChunk(const NBT &);
#endif // WORLDLOADER_H_
