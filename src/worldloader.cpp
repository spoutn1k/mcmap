#include "./worldloader.h"
#include <nbt/parser.hpp>
#include <zlib.h>

namespace Terrain {

Data::Chunk empty_chunk;

void Data::inflateChunk(std::vector<nbt::NBT> *sections) {
  // Some chunks are "hollow", empty sections being present between blocks.
  // Internally, minecraft does not store those empty sections, instead relying
  // on the section index (key "Y"). This routine creates empty sections where
  // they should be, to save a lot of time when rendering.

  int8_t index = sections->front()["Y"].get<int8_t>();
  std::vector<nbt::NBT>::iterator it = sections->begin(), next = it + 1;

  while (it != sections->end() && next != sections->end()) {
    int8_t diff = next->operator[]("Y").get<int8_t>() - index - 1;

    if (diff) {
      while (diff--) {
        it = sections->insert(it + 1, nbt::NBT(nbt::tag_type::tag_end));
        ++index;
      }
    }

    next = ++it + 1;
    index++;
  }
}

void Data::stripChunk(std::vector<nbt::NBT> *sections) {
  // Remove sections with no blocks from the edges of the map
  while (!sections->empty() && !sections->front().contains("Palette"))
    sections->erase(sections->begin());

  // Pop all the empty top sections
  while (!sections->empty() && !sections->back().contains("Palette"))
    sections->pop_back();
}

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

  *length = _ntohl(zData);
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

bool assertChunk(const nbt::NBT &chunk) {
  if (chunk.is_end()                          // Catch uninitialized chunks
      || !chunk.contains("DataVersion")       // Dataversion is required
      || !chunk.contains("Level")             // Level data is required
      || !chunk["Level"].contains("Sections") // No sections mean no blocks
  )
    return false;

  return true;
}

void filterLevel(nbt::NBT &level) {
  // Erase unused NBT tags to save memory
  std::vector<std::string> blacklist = {
      "Entities",          "Structures",  "TileEntities",
      "TileTicks",         "LiquidTicks", "Lights",
      "LiquidsToBeTicked", "ToBeTicked",  "PostProcessing"};

  for (auto key : blacklist)
    level.erase(key);
}

void Data::loadChunk(const ChunkCoordinates coords) {
  int32_t regionX = REGION(coords.first), regionZ = REGION(coords.second),
          cX = coords.first & 0x1f, cZ = coords.second & 0x1f;

  std::filesystem::path regionFile = std::filesystem::path(regionDir) /=
      fmt::format("r.{}.{}.mca", regionX, regionZ);

  if (!std::filesystem::exists(regionFile)) {
    logger::deep_debug("Region file r.{}.{}.mca does not exist, skipping ..\n",
                       regionX, regionZ);
    return;
  }

  FILE *regionHandle;
  uint8_t regionHeader[REGION_HEADER_SIZE];

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
      (_ntohl(regionHeader + ((cZ << 5) + cX) * 4) >> 8) * 4096;

  uint64_t length;

  // Buffers for chunk read from MCA files and decompression.
  uint8_t chunkBuffer[DECOMPRESSED_BUFFER];

  if (!offset || !decompressChunk(offset, regionHandle, chunkBuffer, &length,
                                  regionFile)) {
    fclose(regionHandle);
    return;
  }

  nbt::NBT chunk;

  if (!nbt::parse(chunkBuffer, length, chunk) || !assertChunk(chunk)) {
    fclose(regionHandle);
    return;
  }

  fclose(regionHandle);

  filterLevel(chunk["Level"]);

  std::vector<nbt::NBT> *sections =
      chunk["Level"]["Sections"].get<std::vector<nbt::NBT> *>();

  // Strip the chunk of pointless sections
  stripChunk(sections);

  if (sections->empty())
    return;

  // Fill the chunk's holes with empty sections
  inflateChunk(sections);

  chunks[coords] = std::move(chunk);
}

const Data::Chunk &Data::chunkAt(const ChunkCoordinates coords) {
  loadChunk(coords);

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
