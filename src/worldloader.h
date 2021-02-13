#ifndef WORLDLOADER_H_
#define WORLDLOADER_H_

#include "./colors.h"
#include "./helper.h"
#include <cstdlib>
#include <filesystem>
#include <map.hpp>
#include <nbt/nbt.hpp>
#include <string>
#include <zlib.h>

namespace Terrain {

typedef nbt::NBT Chunk;

struct Data {
  // The coordinates of the loaded chunks. This coordinates maps
  // the CHUNKS loaded, not the blocks
  World::Coordinates map;

  // The internal list of chunks, of size chunkLen
  std::vector<Chunk> chunks;
  uint64_t chunkLen;

  fs::path regionDir;

  // Default constructor
  explicit Data(const World::Coordinates &coords) {
    map.minX = CHUNK(coords.minX);
    map.minZ = CHUNK(coords.minZ);
    map.maxX = CHUNK(coords.maxX);
    map.maxZ = CHUNK(coords.maxZ);

    chunkLen =
        uint64_t(map.maxX - map.minX + 1) * uint64_t(map.maxZ - map.minZ + 1);

    chunks.resize(chunkLen);
  }

  // Chunk loading methods - only load should be useful
  void load(const fs::path &regionDir);
  void loadChunk(const uint32_t offset, FILE *regionHandle, const int chunkX,
                 const int chunkZ, const fs::path &filename);

  // Chunk analysis methods - using the list of sections
  void stripChunk(std::vector<nbt::NBT> *);
  void inflateChunk(std::vector<nbt::NBT> *);

  uint64_t chunkIndex(int64_t x, int64_t z) const {
    return (x - map.minX) + (z - map.minZ) * (map.maxX - map.minX + 1);
  }

  nbt::NBT &chunkAt(int64_t xPos, int64_t zPos);
};

} // namespace Terrain

#endif // WORLDLOADER_H_
