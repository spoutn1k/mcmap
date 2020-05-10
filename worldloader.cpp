#include "worldloader.h"

uint8_t zData[COMPRESSED_BUFFER];
uint8_t chunkBuffer[DECOMPRESSED_BUFFER];

void Terrain::Data::load(const std::filesystem::path &regionDir) {
  // Parse all the necessary region files
  for (int8_t rx = REGION(map.minX); rx < REGION(map.maxX) + 1; rx++) {
    for (int8_t rz = REGION(map.minZ); rz < REGION(map.maxZ) + 1; rz++) {
      std::filesystem::path regionFile = std::filesystem::path(regionDir) /=
          "r." + std::to_string(rx) + "." + std::to_string(rz) + ".mca";

      if (!std::filesystem::exists(regionFile)) {
        fprintf(stderr, "Region file %s does not exist, skipping ..\n",
                regionFile.c_str());
        continue;
      }

      loadRegion(regionFile, rx, rz);
    }
  }
}

void Terrain::Data::loadRegion(const std::filesystem::path &regionFile,
                               const int regionX, const int regionZ) {
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
  zlibStream.avail_out = DECOMPRESSED_BUFFER;
  inflateInit2(&zlibStream, 32 + MAX_WBITS);

  int status = inflate(&zlibStream, Z_FINISH); // decompress in one step
  inflateEnd(&zlibStream);

  if (status != Z_STREAM_END) {
    printf("Error decompressing chunk: %s\n", zError(status));
    return;
  }

  len = zlibStream.total_out;

  NBT tree = NBT::parse(chunkBuffer, len);

  // Strip the chunk of pointless sections
  size_t chunkPos = chunkIndex(chunkX, chunkZ);
  try {
    chunks[chunkPos] = std::move(tree["Level"]["Sections"]);
    vector<NBT> *sections = chunks[chunkPos].get<vector<NBT> *>();

    // Some chunks have a -1 section, we'll pop that real quick
    if (!sections->empty() && *sections->front()["Y"].get<int8_t *>() == -1) {
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
      vector<NBT> *blocks = section["Palette"].get<vector<NBT> *>();
      for (auto block : *blocks) {
        string *id = block["Name"].get<string *>();
        cache.insert(std::pair<std::string, uint8_t>(*id, 0));
      }
    }

    // Analyze the sections vector for height info
    if (size_t nSections = sections->size()) {
      // If there are sections in the chunk
      const uint8_t chunkMin =
          nSections ? *sections->front()["Y"].get<int8_t *>() : 0;
      const uint8_t chunkHeight = (nSections + chunkMin) << 4;

      heightMap[chunkPos] = chunkHeight | chunkMin;

      // If the chunk's height is the highest found, record it
      if (chunkHeight > (heightBounds & 0xf0))
        heightBounds = chunkHeight | (heightBounds & 0x0f);

    } else {
      // If there are no sections, max = min = 0
      heightMap[chunkPos] = 0;
    }

  } catch (const std::invalid_argument &e) {
    fprintf(stderr, "Err: %s\n", e.what());
    return;
  }
}

size_t Terrain::Data::chunkIndex(int64_t x, int64_t z) const {
  return (x - map.minX) + (z - map.minZ) * (map.maxX - map.minX + 1);
}

string blockAt(const NBT &section, uint8_t x, uint8_t z, uint8_t y) {
  // The `BlockStates` array contains data on the section's blocks. You have to
  // extract it by understanfing its structure.
  //
  // Although it is a array of long values, one must see it as an array of block
  // indexes, whose element size depends on the size of the Palette. This
  // routine locates the necessary long, extracts the block with bit
  // comparisons, and cross-references it in the palette to get the block name.
  const vector<int64_t> *blockStates =
      section["BlockStates"].get<const vector<int64_t> *>();
  const uint64_t index = (x & 0x0f) + ((z & 0x0f) + (y & 0x0f) * 16) * 16;

  // The length of a block index has to be coded on the minimal possible size,
  // which is the logarithm in base2 of the size of the palette, or 4 if the
  // logarithm is smaller.
  const uint64_t length =
      std::max((uint64_t)ceil(log2(section["Palette"].size())), (uint64_t)4);

  // We skip the `position` first blocks, of length `size`, then divide by 64 to
  // get the number of longs to skip from the array
  const uint64_t skip_longs = index * length >> 6;

  // Once we located the data in a long, we have to know where in the 64 bits it
  // is located. This is the remaining of the previous operation
  const int64_t padding = index * length & 63;

  // Craft a mask from the length of the block index and the padding, the apply
  // it to the long
  const uint64_t mask = ((1l << length) - 1) << padding;
  uint64_t lower_data = ((*blockStates)[skip_longs] & mask) >> padding;

  // Sometimes the length of the index does not fall entirely into a long, so
  // here we check if there is overflow and extract it too
  const int64_t overflow = padding + length - 64;
  if (overflow > 0) {
    const uint64_t upper_data =
        (*blockStates)[skip_longs + 1] & ((1l << overflow) - 1);
    lower_data = lower_data | upper_data << (length - overflow);
  }

  // Lower data now contains the index in the palette
  const string *id =
      section["Palette"][lower_data]["Name"].get<const string *>();
  return *id;
}

string Terrain::Data::block(const int32_t x, const int32_t z,
                            const int32_t y) const {
  const size_t index = chunkIndex(CHUNK(x), CHUNK(z));

  const NBT &section = chunks[index][y >> 4];
  if (section.contains("Palette"))
    return blockAt(section, x, z, y);
  return "minecraft:air";
}

uint8_t Terrain::Data::maxHeight() const { return heightBounds & 0xf0; }

uint8_t Terrain::Data::maxHeight(const int64_t x, const int64_t z) const {
  return heightMap[chunkIndex(CHUNK(x), CHUNK(z))] & 0xf0;
}

uint8_t Terrain::Data::minHeight() const { return (heightBounds & 0x0f) << 4; }

uint8_t Terrain::Data::minHeight(const int64_t x, const int64_t z) const {
  return (heightMap[chunkIndex(CHUNK(x), CHUNK(z))] & 0x0f) << 4;
}
