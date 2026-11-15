#include "./worldloader.h"
#include "./region.h"
#include <algorithm>
#include <cstdlib>
#include <nbt/parser.hpp>
#include <translator.hpp>
#include <zlib.h>

namespace Terrain {

Data::Chunk empty_chunk;

// Debug mode: also load the chunks that are not fully generated, for them to be drawn as ghosts
const bool ghost_chunks = std::getenv("MCMAP_GHOST_CHUNKS") != nullptr;

bool Data::readChunk(const ChunkCoordinates coords, nbt::NBT &data) {
  std::filesystem::path regionFile = std::filesystem::path(regionDir) /=
      fmt::format("r.{}.{}.mca", REGION(coords.x), REGION(coords.z));

  if (!std::filesystem::exists(regionFile)) {
    logger::trace("Region file r.{}.{}.mca does not exist, skipping ..",
                  REGION(coords.x), REGION(coords.z));
    return false;
  }

  if (!Region(regionFile).get_chunk(coords, data)) {
    logger::trace("Reading chunk {} {} from {} failed", coords.x, coords.z,
                  regionFile.string());
    return false;
  }

  return true;
}

void Data::loadChunk(const ChunkCoordinates coords) {
  nbt::NBT data;

  if (!readChunk(coords, data))
    return;

  switch (Chunk::assert_chunk(data)) {
  case Chunk::FULL:
    break;

  case Chunk::INCOMPLETE:
    // Unfinished chunks surround every explored area, they are not a sign of an unsupported format
    logger::trace("Chunk {} {} is not fully generated", coords.x, coords.z);

    if (!ghost_chunks)
      return;
    break;

  case Chunk::UNRECOGNISED:
    if (report && data.contains("DataVersion"))
      report->chunk_rejected(data["DataVersion"].get<int>(), coords);
    logger::trace("Chunk {} {} was rejected", coords.x, coords.z);
    return;
  }

  Chunk chunk(data, palette, coords, report);

  // Ghosts are left out for the diagnostics to be the same with or without them
  if (report && !chunk.ghost)
    report->chunk_loaded(chunk.data_version);

  if (structures) {
    starts[coords] = chunk.starts;
    resolveStructures(chunk);
  }

  chunks[coords] = std::move(chunk);
}

const Data::Chunk::box_array &Data::startsAt(const ChunkCoordinates coords) {
  auto query = starts.find(coords);

  if (query == starts.end()) {
    // Only the structure data is needed, skip decoding the sections
    nbt::NBT data;
    Chunk::box_array boxes;
    std::vector<ChunkCoordinates> unused;

    if (readChunk(coords, data))
      Chunk::read_structures(data, boxes, unused);

    query = starts.emplace(coords, std::move(boxes)).first;
  }

  return query->second;
}

void Data::resolveStructures(Chunk &chunk) {
  // The chunk's own starts are not always referenced by the chunk itself
  std::vector<ChunkCoordinates> sources = chunk.references;
  sources.push_back(chunk.position);

  std::sort(sources.begin(), sources.end());
  sources.erase(std::unique(sources.begin(), sources.end()), sources.end());

  for (const auto &source : sources)
    for (const auto &box : startsAt(source))
      if (box.overlaps_chunk(chunk.position.x, chunk.position.z))
        chunk.structures.push_back(box);
}

const Data::Chunk &Data::chunkAt(const ChunkCoordinates coords,
                                 const Map::Orientation o, bool surround) {
  if (chunks.find(coords) == chunks.end())
    loadChunk(coords);

  if (surround) {
    ChunkCoordinates left = coords + left_in(o);
    ChunkCoordinates right = coords + right_in(o);

    if (chunks.find(left) == chunks.end())
      loadChunk(left);
    if (chunks.find(right) == chunks.end())
      loadChunk(right);
  }

  auto query = chunks.find(coords);
  if (query == chunks.end()) {
    logger::trace("Chunk loading failed for {} {}", coords.x, coords.z);
    return empty_chunk;
  }

  return query->second;
}

void Data::free_chunk(const ChunkCoordinates coords) {
  auto query = chunks.find(coords);

  if (query != chunks.end())
    chunks.erase(query);
}

} // namespace Terrain
