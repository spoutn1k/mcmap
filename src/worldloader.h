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

struct Data {
  using Chunk = nbt::NBT;
  using ChunkCoordinates = std::pair<int32_t, int32_t>;
  using ChunkStore = std::map<ChunkCoordinates, Chunk>;

  // The coordinates of the loaded chunks. This coordinates maps
  // the CHUNKS loaded, not the blocks
  World::Coordinates map;

  // The loaded chunks, organized as a map of coordinatesxchunk
  ChunkStore chunks;

  fs::path regionDir;

  // Default constructor
  explicit Data(const World::Coordinates &coords) {
    map.minX = CHUNK(coords.minX);
    map.minZ = CHUNK(coords.minZ);
    map.maxX = CHUNK(coords.maxX);
    map.maxZ = CHUNK(coords.maxZ);
  }

  // Chunk loading methods - only load should be useful
  void load(const fs::path &regionDir);
  void loadChunk(const ChunkCoordinates);

  // Chunk analysis methods - using the list of sections
  void stripChunk(std::vector<nbt::NBT> *);
  void inflateChunk(std::vector<nbt::NBT> *);

  Chunk &chunkAt(const ChunkCoordinates);
};

} // namespace Terrain

#endif // WORLDLOADER_H_
