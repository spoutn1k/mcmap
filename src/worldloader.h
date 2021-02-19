#ifndef WORLDLOADER_H_
#define WORLDLOADER_H_

#include "./helper.h"
#include <filesystem>
#include <map.hpp>
#include <nbt/nbt.hpp>

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
  explicit Data(const World::Coordinates &coords,
                const std::filesystem::path &dir)
      : regionDir(dir) {
    map.minX = CHUNK(coords.minX);
    map.minZ = CHUNK(coords.minZ);
    map.maxX = CHUNK(coords.maxX);
    map.maxZ = CHUNK(coords.maxZ);
  }

  // Chunk pre-processing methods
  void stripChunk(std::vector<nbt::NBT> *);
  void inflateChunk(std::vector<nbt::NBT> *);

  // Chunk loading - should never be used, called by chunkAt in case of chunk
  // fault
  void loadChunk(const ChunkCoordinates);

  // Access a chunk from the save file
  const Chunk &chunkAt(const ChunkCoordinates);

  // Mark a chunk as done and ready for deletion
  void free_chunk(const ChunkCoordinates);
};

} // namespace Terrain

#endif // WORLDLOADER_H_
