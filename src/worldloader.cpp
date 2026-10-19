#include "./worldloader.h"
#include <algorithm>
#include <nbt/parser.hpp>
#include <translator.hpp>
#include <zlib.h>

namespace Terrain {

Data::Chunk empty_chunk;

bool decompressChunk(const uint32_t offset, FILE *regionHandle,
                     uint8_t *chunkBuffer, uint64_t *length,
                     const std::filesystem::path &filename) {
  uint8_t zData[COMPRESSED_BUFFER];

  if (0 != FSEEK(regionHandle, offset, SEEK_SET)) {
    logger::debug("Accessing chunk data in file {} failed: {}",
                  filename.string(), strerror(errno));
    return false;
  }

  // Read the 5 bytes that give the size and type of data
  if (5 != fread(zData, sizeof(uint8_t), 5, regionHandle)) {
    logger::debug("Reading chunk size from region file {} failed: {}",
                  filename.string(), strerror(errno));
    return false;
  }

  // Read the size on the first 4 bytes, discard the type
  *length = translate<uint32_t>(zData);
  (*length)--; // Sometimes the data is 1 byte smaller

  if (fread(zData, sizeof(uint8_t), *length, regionHandle) != *length) {
    logger::debug("Not enough data for chunk: {}", strerror(errno));
    return false;
  }

  z_stream zlibStream;
  memset(&zlibStream, 0, sizeof(z_stream));
  zlibStream.next_in = (Bytef *)zData;
  zlibStream.next_out = (Bytef *)chunkBuffer;
  zlibStream.avail_in = *length;
  zlibStream.avail_out = DECOMPRESSED_BUFFER;
  inflateInit2(&zlibStream, 32 + MAX_WBITS);

  int status = inflate(&zlibStream, Z_FINISH); // decompress in one step
  inflateEnd(&zlibStream);

  if (status != Z_STREAM_END) {
    logger::debug("Decompressing chunk data failed: {}", zError(status));
    return false;
  }

  *length = zlibStream.total_out;
  return true;
}

bool Data::readChunk(const ChunkCoordinates coords, nbt::NBT &data) {
  FILE *regionHandle;
  uint8_t regionHeader[REGION_HEADER_SIZE], chunkBuffer[DECOMPRESSED_BUFFER];
  int32_t regionX = REGION(coords.x), regionZ = REGION(coords.z),
          cX = coords.x & 0x1f, cZ = coords.z & 0x1f;
  uint64_t length;

  std::filesystem::path regionFile = std::filesystem::path(regionDir) /=
      fmt::format("r.{}.{}.mca", regionX, regionZ);

  if (!std::filesystem::exists(regionFile)) {
    logger::trace("Region file r.{}.{}.mca does not exist, skipping ..",
                  regionX, regionZ);
    return false;
  }

  if (!(regionHandle = fopen(regionFile.string().c_str(), "rb"))) {
    logger::error("Opening region file `{}` failed: {}", regionFile.string(),
                  strerror(errno));
    return false;
  }

  // Then, we read the header (of size 4K) storing the chunks locations
  if (fread(regionHeader, sizeof(uint8_t), REGION_HEADER_SIZE, regionHandle) !=
      REGION_HEADER_SIZE) {
    logger::error("Region header too short in `{}`", regionFile.string());
    fclose(regionHandle);
    return false;
  }

  const uint32_t offset =
      (translate<uint32_t>(regionHeader + ((cZ << 5) + cX) * 4) >> 8) * 4096;

  if (!offset || !decompressChunk(offset, regionHandle, chunkBuffer, &length,
                                  regionFile)) {
    fclose(regionHandle);
    return false;
  }

  fclose(regionHandle);

  if (!nbt::parse(chunkBuffer, length, data)) {
    logger::trace("Chunk parsing failed for chunk {} {} in {}", coords.x,
                  coords.z, regionFile.string());
    return false;
  }

  return true;
}

void Data::loadChunk(const ChunkCoordinates coords) {
  nbt::NBT data;

  if (!readChunk(coords, data))
    return;

  if (!Chunk::assert_chunk(data)) {
    if (report && data.contains("DataVersion"))
      report->chunk_rejected(data["DataVersion"].get<int>(), coords);
    logger::trace("Chunk {} {} was rejected", coords.x, coords.z);
    return;
  }

  Chunk chunk(data, palette, coords, report);

  if (report)
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
