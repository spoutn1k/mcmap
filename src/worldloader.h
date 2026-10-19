#ifndef WORLDLOADER_H_
#define WORLDLOADER_H_

#include "./chunk.h"
#include "./helper.h"
#include <filesystem>
#include <map.hpp>
#include <nbt/nbt.hpp>

namespace Terrain {

struct Data {
  using Chunk = mcmap::Chunk;
  using ChunkCoordinates = mcmap::Chunk::coordinates;
  using ChunkStore = std::map<ChunkCoordinates, Chunk>;
  using StartStore = std::map<ChunkCoordinates, Chunk::box_array>;

  // The loaded chunks, organized as a map of coordinatesxchunk
  ChunkStore chunks;

  // Whether to resolve the structures overlapping the loaded chunks
  bool structures = false;
  // Structure starts read from the save, kept as they are shared by chunks
  StartStore starts;

  fs::path regionDir;
  const Colors::Palette &palette;

  // Where to record loading problems, if anywhere
  Diagnostics::Report *report;

  // Default constructor
  explicit Data(const std::filesystem::path &dir, const Colors::Palette &p,
                Diagnostics::Report *r = nullptr)
      : regionDir(dir), palette(p), report(r) {}

  // Chunk pre-processing methods
  void stripChunk(std::vector<nbt::NBT> *);
  void inflateChunk(std::vector<nbt::NBT> *);
  // Read and parse a chunk from its region file
  bool readChunk(const ChunkCoordinates, nbt::NBT &);

  // Chunk loading - should never be used, called by chunkAt in case of chunk fault
  void loadChunk(const ChunkCoordinates);

  // Bounding boxes of the structures starting in a chunk
  const Chunk::box_array &startsAt(const ChunkCoordinates);
  // Gather the bounding boxes of the structures overlapping a chunk
  void resolveStructures(Chunk &);

  // Access a chunk from the save file
  const Chunk &chunkAt(const ChunkCoordinates, const Map::Orientation, bool);

  // Mark a chunk as done and ready for deletion
  void free_chunk(const ChunkCoordinates);
};

} // namespace Terrain

#endif // WORLDLOADER_H_
