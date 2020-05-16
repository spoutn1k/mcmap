#ifndef WORLDLOADER_H_
#define WORLDLOADER_H_

#include "./colors.h"
#include "./helper.h"
#include "./nbt/nbt.hpp"
#include <cstdlib>
#include <filesystem>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>
#include <zlib.h>

using nbt::NBT;
using std::string;
using std::vector;

namespace Terrain {

typedef NBT Chunk;
typedef Chunk *ChunkList;

enum Orientation {
  NW,
  SW,
  NE,
  SE,
};

using Coordinates = struct Coordinates;

struct Data {
  // The coordinates of the loaded chunks. This coordinates maps
  // the CHUNKS loaded, not the blocks
  Coordinates map;

  // The internal list of chunks, of size chunkLen
  ChunkList chunks;
  size_t chunkLen;

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

    chunkLen = (map.maxX - map.minX + 1) * (map.maxZ - map.minZ + 1);

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
  void tagSections(vector<NBT> *);
  void stripChunk(vector<NBT> *);
  void cacheColors(vector<NBT> *);
  uint8_t importHeight(vector<NBT> *);
  void inflateChunk(vector<NBT> *);

  size_t chunkIndex(int64_t x, int64_t z) const {
    return (x - map.minX) + (z - map.minZ) * (map.maxX - map.minX + 1);
  }

  uint8_t maxHeight() const { return heightBounds & 0xf0; }
  uint8_t minHeight() const { return (heightBounds & 0x0f) << 4; }

  uint8_t maxHeight(const int64_t x, const int64_t z) const {
    return heightMap[chunkIndex(CHUNK(x), CHUNK(z))] & 0xf0;
  }

  uint8_t minHeight(const int64_t x, const int64_t z) const {
    return (heightMap[chunkIndex(CHUNK(x), CHUNK(z))] & 0x0f) << 4;
  }

  const NBT &block(const int32_t x, const int32_t z, const int32_t y) const;
};

struct OrientedMap {
  Terrain::Coordinates bounds;
  struct Terrain::Data terrain;
  int8_t vectorX, vectorZ;
  Terrain::Orientation orientation;

  OrientedMap(const Terrain::Coordinates &map,
              const Terrain::Orientation direction)
      : terrain(map) {
    bounds = map;
    vectorX = vectorZ = 1;
    orientation = direction;
    reshape(orientation);
  }

  void reshape(const Terrain::Orientation orientation) {
    switch (orientation) {
    case NW:
      // This is the default. No changes
      break;
    case SW:
      std::swap(bounds.maxZ, bounds.minZ);
      vectorZ = -1;
      break;
    case NE:
      std::swap(bounds.maxX, bounds.minX);
      vectorX = -1;
      break;
    case SE:
      std::swap(bounds.maxX, bounds.minX);
      std::swap(bounds.maxZ, bounds.minZ);
      vectorX = vectorZ = -1;
      break;
    }
  }
};

} // namespace Terrain

#endif // WORLDLOADER_H_
