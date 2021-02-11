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

using nbt::NBT;
using std::string;
using std::vector;

namespace Terrain {

typedef NBT Chunk;

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
  void loadRegion(const fs::path &regionFile, const int regionX,
                  const int regionZ);
  void loadChunk(const uint32_t offset, FILE *regionHandle, const int chunkX,
                 const int chunkZ, const fs::path &filename);

  // Chunk analysis methods - using the list of sections
  void stripChunk(vector<NBT> *);
  std::pair<short, short> importHeight(vector<NBT> *);
  void inflateChunk(vector<NBT> *);

  uint64_t chunkIndex(int64_t x, int64_t z) const {
    return (x - map.minX) + (z - map.minZ) * (map.maxX - map.minX + 1);
  }

  NBT &chunkAt(int64_t xPos, int64_t zPos);
};

} // namespace Terrain

typedef void (*sectionInterpreter)(const uint64_t, const std::vector<int64_t> *,
                                   std::array<uint8_t, 4096> &);

void sectionAtPre116(const uint64_t, const std::vector<int64_t> *,
                     std::array<uint8_t, 4096> &);
void sectionAtPost116(const uint64_t, const std::vector<int64_t> *,
                      std::array<uint8_t, 4096> &);

bool assertChunk(const NBT &);
#endif // WORLDLOADER_H_
