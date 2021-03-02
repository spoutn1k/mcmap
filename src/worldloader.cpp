#include "./worldloader.h"
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
    logger::debug("Accessing chunk data in file {} failed: {}\n",
                  filename.string(), strerror(errno));
    return false;
  }

  // Read the 5 bytes that give the size and type of data
  if (5 != fread(zData, sizeof(uint8_t), 5, regionHandle)) {
    logger::debug("Reading chunk size from region file {} failed: {}\n",
                  filename.string(), strerror(errno));
    return false;
  }

  // Read the size on the first 4 bytes, discard the type
  *length = translate<uint32_t>(zData);
  (*length)--; // Sometimes the data is 1 byte smaller

  if (fread(zData, sizeof(uint8_t), *length, regionHandle) != *length) {
    logger::debug("Not enough data for chunk: {}\n", strerror(errno));
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
    logger::debug("Decompressing chunk data failed: {}\n", zError(status));
    return false;
  }

  *length = zlibStream.total_out;
  return true;
}

void Data::loadChunk(const ChunkCoordinates coords) {
  FILE *regionHandle;
  uint8_t regionHeader[REGION_HEADER_SIZE], chunkBuffer[DECOMPRESSED_BUFFER];
  int32_t regionX = REGION(coords.x), regionZ = REGION(coords.z),
          cX = coords.x & 0x1f, cZ = coords.z & 0x1f;
  uint64_t length;

  std::filesystem::path regionFile = std::filesystem::path(regionDir) /=
      fmt::format("r.{}.{}.mca", regionX, regionZ);

  if (!std::filesystem::exists(regionFile)) {
    logger::deep_debug("Region file r.{}.{}.mca does not exist, skipping ..\n",
                       regionX, regionZ);
    return;
  }

  if (!(regionHandle = fopen(regionFile.string().c_str(), "rb"))) {
    logger::error("Opening region file `{}` failed: {}\n", regionFile.string(),
                  strerror(errno));
    return;
  }

  // Then, we read the header (of size 4K) storing the chunks locations
  if (fread(regionHeader, sizeof(uint8_t), REGION_HEADER_SIZE, regionHandle) !=
      REGION_HEADER_SIZE) {
    logger::error("Region header too short in `{}`\n", regionFile.string());
    fclose(regionHandle);
    return;
  }

  const uint32_t offset =
      (translate<uint32_t>(regionHeader + ((cZ << 5) + cX) * 4) >> 8) * 4096;

  if (!offset || !decompressChunk(offset, regionHandle, chunkBuffer, &length,
                                  regionFile)) {
    fclose(regionHandle);
    return;
  }

  nbt::NBT data;

  if (!nbt::parse(chunkBuffer, length, data) || !Chunk::assert_chunk(data)) {
    fclose(regionHandle);
    return;
  }

  fclose(regionHandle);

  chunks[coords] = Chunk(data, palette);
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
  if (query == chunks.end())
    return empty_chunk;

  return query->second;
}

void Data::free_chunk(const ChunkCoordinates coords) {
  auto query = chunks.find(coords);

  if (query != chunks.end())
    chunks.erase(query);
}

} // namespace Terrain
