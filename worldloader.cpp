#include "worldloader.h"
#include <bitset>

using std::string;
using std::vector;
namespace fs = std::filesystem;

uint8_t zData[5 * 4096];
uint8_t chunkBuffer[1000 * 1024];

void Terrain::Data::load(const fs::path &regionDir) {
  // Parse all the necessary region files
  for (int8_t rx = REGION(map.minX); rx < REGION(map.maxX) + 1; rx++) {
    for (int8_t rz = REGION(map.minZ); rz < REGION(map.maxZ) + 1; rz++) {
      fs::path regionFile = fs::path(regionDir) /=
          "r." + std::to_string(rx) + "." + std::to_string(rz) + ".mca";

      if (!fs::exists(regionFile)) {
        fprintf(stderr, "Region file %s does not exist, skipping ..\n",
                regionFile.c_str());
        continue;
      }

      loadRegion(regionFile, rx, rz);
    }
  }
}

void Terrain::Data::loadRegion(const fs::path &regionFile, const int regionX,
                               const int regionZ) {
  FILE *regionHandle;
  uint8_t regionHeader[REGION_HEADER_SIZE];

  if (!(regionHandle = fopen(regionFile.c_str(), "rb"))) {
    printf("Error opening region file %s\n", regionFile.c_str());
    return;
  }
  // Then, we read the header (of size 4K) storing the chunks locations

  if (fread(regionHeader, sizeof(uint8_t), REGION_HEADER_SIZE, regionHandle) !=
      REGION_HEADER_SIZE) {
    printf("Header too short in %s\n", regionFile.c_str());
    fclose(regionHandle);
    return;
  }

  // For all the chunks in the file
  for (int it = 0; it < REGIONSIZE * REGIONSIZE; it++) {
    // Bound check
    const int chunkX = (regionX << 5) + (it & 0x1f);
    const int chunkZ = (regionZ << 5) + (it >> 5);
    if (chunkX < map.minX || chunkX > map.maxX || chunkZ < map.minZ ||
        chunkZ > map.maxZ) {
      // Chunk is not in bounds
      continue;
    }

    // Get the location of the data from the header
    const uint32_t offset = (_ntohl(regionHeader + it * 4) >> 8) * 4096;

    loadChunk(offset, regionHandle, chunkX, chunkZ);
  }

  fclose(regionHandle);
}

void Terrain::Data::loadChunk(const uint32_t offset, FILE *regionHandle,
                              const int chunkX, const int chunkZ) {
  if (!offset) {
    // Chunk does not exist
    // printf("Chunk does not exist !\n");
    return;
  }

  if (0 != fseek(regionHandle, offset, SEEK_SET)) {
    // printf("Error seeking to chunk\n");
    return;
  }

  // Read the 5 bytes that give the size and type of data
  if (5 != fread(zData, sizeof(uint8_t), 5, regionHandle)) {
    // printf("Error reading chunk size from region file\n");
    return;
  }

  uint32_t len = _ntohl(zData);
  // len--; // This dates from Zahl's, no idea of its purpose

  if (fread(zData, sizeof(uint8_t), len, regionHandle) != len) {
    // printf("Not enough input for chunk\n");
    return;
  }

  z_stream zlibStream;
  memset(&zlibStream, 0, sizeof(z_stream));
  zlibStream.next_in = (Bytef *)zData;
  zlibStream.next_out = (Bytef *)chunkBuffer;
  zlibStream.avail_in = len;
  zlibStream.avail_out = 1000 * 1024;
  inflateInit2(&zlibStream, 32 + MAX_WBITS);

  int status = inflate(&zlibStream, Z_FINISH); // decompress in one step
  inflateEnd(&zlibStream);

  if (status != Z_STREAM_END) {
    printf("Error decompressing chunk: %s\n", zError(status));
    return;
  }

  len = zlibStream.total_out;

  nbt::NBT tree = nbt::NBT::parse(chunkBuffer, len);

  // Strip the chunk of pointless sections
  size_t chunkPos = chunkIndex(chunkX, chunkZ);
  try {
    chunks[chunkPos] = tree["Level"]["Sections"];
    vector<nbt::NBT> *sections = chunks[chunkPos].get<vector<nbt::NBT> *>();

    // Some chunks have a -1 section, we'll pop that real quick
    if (!sections->empty() && !sections->front().contains("Palette")) {
      sections->erase(sections->begin());
    }

    // Pop all the empty top sections
    while (!sections->empty() && !sections->back().contains("Palette")) {
      sections->pop_back();
    }

    // Complete the cache, to determine the colors to load
    for (auto section : *sections) {
      if (!section.contains("Palette"))
        continue;

      string blockID;
      vector<nbt::NBT> *blocks = section["Palette"].get<vector<nbt::NBT> *>();
      for (auto block : *blocks) {
        string *id = block["Name"].get<string *>();
        cache.insert(std::pair<std::string, uint8_t>(*id, 0));
      }
    }

    uint8_t chunkHeight = sections->size() << 4;
    heightMap[chunkPos] = chunkHeight;

    // If the chunk's height is the highest found, record it
    if (chunkHeight > (heightBounds & 0xf0))
      heightBounds = chunkHeight | (heightBounds & 0x0f);

  } catch (const std::invalid_argument &e) {
    fprintf(stderr, "Err: %s\n", e.what());
    return;
  }
}

size_t Terrain::Data::chunkIndex(int64_t x, int64_t z) const {
  return (x - map.minX) + (z - map.minZ) * (map.maxX - map.minX + 1);
}

string blockAt(const nbt::NBT &section, uint8_t x, uint8_t z, uint8_t y) {
  /* Get a block string from the raw section data */
  const vector<int64_t> *data =
      section["BlockStates"].get<const vector<int64_t> *>();
  const uint64_t position = (x & 0x0f) + ((z & 0x0f) + (y & 0x0f) * 16) * 16;

  const uint64_t size =
      std::max((uint64_t)ceil(log2(section["Palette"].size())), (uint64_t)4);

  // The number of longs to skip
  const uint64_t skip = position * size / 64;

  // The bits to skip in the first non-skipped long
  const int64_t remain = position * size - 64 * skip;

  // The bits in the second non-skipped long
  const int64_t overflow = remain + size - 64;

  const uint64_t mask = ((1l << size) - 1) << remain;
  uint64_t lower_data = ((*data)[skip] & mask) >> remain;

  if (overflow > 0) {
    const uint64_t upper_data = (*data)[skip + 1] & ((1l << overflow) - 1);
    lower_data = lower_data | upper_data << (size - overflow);
  }

  const string *id =
      section["Palette"][lower_data]["Name"].get<const string *>();
  return *id;
}

string Terrain::Data::block(const int32_t x, const int32_t z,
                            const int32_t y) const {
  const size_t index = chunkIndex(CHUNK(x), CHUNK(z));
  const uint8_t sectionY = y >> 4;
  try {
    const nbt::NBT section = chunks[index][sectionY];
    if (section.contains("Palette"))
      return blockAt(section, x, z, y);
    return "minecraft:air";
  } catch (std::exception &e) {
    printf("Got air because: %s (%d.%d.%d)\n", e.what(), x, z, y);
    return "minecraft:air";
  }
}

uint8_t Terrain::Data::maxHeight() const { return heightBounds & 0xf0; }

uint8_t Terrain::Data::maxHeight(const int64_t x, const int64_t z) const {
  return heightMap[chunkIndex(CHUNK(x), CHUNK(z))] & 0xf0;
}

uint8_t Terrain::Data::minHeight() const { return (heightBounds & 0x0f) << 4; }

uint8_t Terrain::Data::minHeight(const int64_t x, const int64_t z) const {
  return (heightMap[chunkIndex(CHUNK(x), CHUNK(z))] & 0x0f) << 4;
}
