#include "worldloader.h"

NBT minecraft_air(nbt::tag_type::tag_end);

void Terrain::scanWorldDirectory(const std::filesystem::path &regionDir,
                                 Terrain::Coordinates *savedWorld) {
  const char delimiter = '.';
  std::string index;
  char buffer[4];
  int32_t regionX, regionZ;
  savedWorld->setUndefined();

  for (auto &region : std::filesystem::directory_iterator(regionDir)) {
    // This loop parses files with name 'r.x.y.mca', extracting x and y. This is
    // done by creating a string stream and using `getline` with '.' as a
    // delimiter.
    std::stringstream ss(region.path().filename().c_str());
    std::getline(ss, index, delimiter); // This removes the 'r.'
    std::getline(ss, index, delimiter);

    regionX = atoi(index.c_str());

    std::getline(ss, index, delimiter);

    regionZ = atoi(index.c_str());

    std::ifstream regionData(region.path());
    for (uint16_t chunk = 0; chunk < REGIONSIZE * REGIONSIZE; chunk++) {
      regionData.read(buffer, 4);

      if (*((uint32_t *)&buffer) == 0) {
        continue;
      }

      savedWorld->minX =
          std::min(savedWorld->minX, int32_t((regionX << 5) + (chunk & 0x1f)));
      savedWorld->maxX =
          std::max(savedWorld->maxX, int32_t((regionX << 5) + (chunk & 0x1f)));
      savedWorld->minZ =
          std::min(savedWorld->minZ, int32_t((regionZ << 5) + (chunk >> 5)));
      savedWorld->maxZ =
          std::max(savedWorld->maxZ, int32_t((regionZ << 5) + (chunk >> 5)));
    }
  }

  // Convert region indexes to blocks
  savedWorld->minX = savedWorld->minX << 4;
  savedWorld->minZ = savedWorld->minZ << 4;
  savedWorld->maxX = ((savedWorld->maxX + 1) << 4) - 1;
  savedWorld->maxZ = ((savedWorld->maxZ + 1) << 4) - 1;

  logger::debug("World spans from {}.{} to {}.{}\n", savedWorld->minX,
                savedWorld->minZ, savedWorld->maxX, savedWorld->maxZ);
}

void Terrain::Data::load(const std::filesystem::path &regionDir) {
  this->regionDir = regionDir;
}

void Terrain::Data::inflateChunk(vector<NBT> *sections) {
  // Some chunks are "hollow", empty sections being present between blocks.
  // Internally, minecraft does not store those empty sections, instead relying
  // on the section index (key "Y"). This routine creates empty sections where
  // they should be, to save a lot of time when rendering.
  //
  // This method ensure all sections from index 0 to the highest existing
  // section are inside the vector. This allows us to bypass al lot of checks
  // inside the critical drawing loop.

  // First of all, pad the beginning of the array if the lowest sections are
  // empty. This index is important and will be used later
  int8_t index = sections->front()["Y"].get<int8_t>();

  // We use `tag_end` to avoid initalizing too much stuff
  for (int i = index - 1; i > -1; i--)
    sections->insert(sections->begin(), NBT(nbt::tag_type::tag_end));

  // Then, go through the array and fill holes
  // As we check for the "Y" child in the compound, and did not add it
  // previously, the index MUST not change from the original first index
  vector<NBT>::iterator it = sections->begin() + index, next = it + 1;

  while (it != sections->end() && next != sections->end()) {
    uint8_t diff = next->operator[]("Y").get<int8_t>() - index - 1;

    if (diff) {
      while (diff--) {
        it = sections->insert(it + 1, NBT(nbt::tag_type::tag_end));
        ++index;
      }
    }

    // Increment both iterators
    next = ++it + 1;
    index++;
  }
}

void Terrain::Data::stripChunk(vector<NBT> *sections) {
  // Some chunks have a -1 section, we'll pop that real quick
  if (!sections->empty() && sections->front()["Y"].get<int8_t>() == -1) {
    sections->erase(sections->begin());
  }

  // Pop all the empty top sections
  while (!sections->empty() && !sections->back().contains("Palette")) {
    sections->pop_back();
  }
}

void Terrain::Data::cacheColors(vector<NBT> *sections) {
  // Complete the cache, to determine the colors to load
  for (auto section : *sections) {
    if (section.is_end() || !section.contains("Palette"))
      continue;

    for (auto block : *section["Palette"].get<vector<NBT> *>())
      cache.push_back(block["Name"].get<string>());
  }
}

uint16_t Terrain::Data::importHeight(vector<NBT> *sections) {
  const uint8_t chunkMin = sections->front()["Y"].get<int8_t>() << 4;
  const uint8_t chunkMax = (sections->back()["Y"].get<int8_t>() << 4) + 15;

  // If the chunk's height is the highest found, record it
  if (chunkMax > maxHeight())
    heightBounds = (chunkMax << 8) | heightBounds;

  return (chunkMax << 8) | chunkMin;
}

bool decompressChunk(const uint32_t offset, FILE *regionHandle,
                     uint8_t *chunkBuffer, uint64_t *length,
                     const std::filesystem::path &filename) {
  uint8_t zData[COMPRESSED_BUFFER];

  if (0 != fseek(regionHandle, offset, SEEK_SET)) {
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

bool assertChunk(const NBT &chunk) {
  if (chunk.is_end()                          // Catch uninitialized chunks
      || !chunk.contains("DataVersion")       // Dataversion is required
      || !chunk.contains("Level")             // Level data is required
      || !chunk["Level"].contains("Sections") // No sections mean no blocks
  )
    return false;

  return true;
}

void filterLevel(NBT &level) {
  // Erase unused NBT tags to save memory
  std::vector<std::string> blacklist = {
      "Entities",          "Structures",  "TileEntities",
      "TileTicks",         "LiquidTicks", "Lights",
      "LiquidsToBeTicked", "ToBeTicked",  "PostProcessing"};

  for (auto key : blacklist)
    level.erase(key);
}

void Terrain::Data::loadChunk(const uint32_t offset, FILE *regionHandle,
                              const int chunkX, const int chunkZ,
                              const std::filesystem::path &filename) {
  uint64_t length, chunkPos = chunkIndex(chunkX, chunkZ);

  // Buffers for chunk read from MCA files and decompression.
  uint8_t chunkBuffer[DECOMPRESSED_BUFFER];

  if (!offset ||
      !decompressChunk(offset, regionHandle, chunkBuffer, &length, filename))
    return;

  NBT chunk = NBT::parse(chunkBuffer, length);

  if (!assertChunk(chunk))
    return;

  filterLevel(chunk["Level"]);

  chunks[chunkPos] = std::move(chunk);
  vector<NBT> *sections =
      chunks[chunkPos]["Level"]["Sections"].get<vector<NBT> *>();

  // Strip the chunk of pointless sections
  stripChunk(sections);

  if (sections->empty()) {
    heightMap[chunkPos] = 0;
    return;
  }

  // Cache the blocks contained in the chunks, to load only the necessary
  // colors later on
  cacheColors(sections);

  // Analyze the sections vector for height info
  heightMap[chunkPos] = importHeight(sections);

  // Fill the chunk's holes with empty sections
  inflateChunk(sections);
}

void sectionAtPost116(const uint64_t index_length,
                      const std::vector<int64_t> *blockStates,
                      uint8_t *buffer) {
  // NEW in 1.16, longs are padded by 0s when a block cannot fit, so no more
  // overflow to deal with !

  for (uint16_t index = 0; index < 4096; index++) {
    // Determine how many indexes each long holds
    const uint8_t blocksPerLong = 64 / index_length;

    // Calculate where in the long array is the long containing the right index.
    const uint16_t longIndex = index / blocksPerLong;

    // Once we located a long, we have to know where in the 64 bits
    // the relevant block is located.
    const uint8_t padding = (index - longIndex * blocksPerLong) * index_length;

    // Bring the data to the first bits of the long, then extract it by bitwise
    // comparison
    const uint16_t blockIndex = ((*blockStates)[longIndex] >> padding) &
                                ((uint64_t(1) << index_length) - 1);

    buffer[index] = blockIndex;
  }
}

void sectionAtPre116(const uint64_t index_length,
                     const std::vector<int64_t> *blockStates, uint8_t *buffer) {
  // The `BlockStates` array contains data on the section's blocks. You have to
  // extract it by understanfing its structure.
  //
  // Although it is a array of long values, one must see it as an array of block
  // indexes, whose element size depends on the size of the Palette. This
  // routine locates the necessary long, extracts the block with bit
  // comparisons.
  //
  // The length of a block index has to be coded on the minimal possible size,
  // which is the logarithm in base2 of the size of the palette, or 4 if the
  // logarithm is smaller.

  for (uint16_t index = 0; index < 4096; index++) {

    // We skip the `position` first blocks, of length `size`, then divide by 64
    // to get the number of longs to skip from the array
    const uint16_t skip_longs = (index * index_length) >> 6;

    // Once we located the data in a long, we have to know where in the 64 bits
    // it is located. This is the remaining of the previous operation
    const int8_t padding = (index * index_length) & 63;

    // Sometimes the data of an index does not fit entirely into a long, so we
    // check if there is overflow
    const int8_t overflow =
        (padding + index_length > 64 ? padding + index_length - 64 : 0);

    // This complicated expression extracts the necessary bits from the current
    // long.
    //
    // Lets say we need the following bits in a long (not to scale):
    // 10011100111001110011100
    //    ^^^^^
    // We do this by shifting (>>) the data by padding, to get the relevant bits
    // on the end of the long:
    // ???????????????10011100
    //                   ^^^^^
    // We then apply a mask to get only the relevant bits:
    // ???????????????10011100
    // 00000000000000000011111 &
    // 00000000000000000011100 <- result
    //
    // The mask is made at the size of the data, using the formula (1 << n) - 1,
    // the resulting bitset is of the following shape: 0...01...1 with n 1s.
    //
    // If there is an overflow, the mask size is reduced, as not to catch noise
    // from the padding (ie the interrogation points earlier) that appear on
    // ARM32.
    uint16_t lower_data = ((*blockStates)[skip_longs] >> padding) &
                          ((uint64_t(1) << (index_length - overflow)) - 1);

    if (overflow > 0) {
      // The exact same process is used to catch the overflow from the next long
      const uint16_t upper_data =
          ((*blockStates)[skip_longs + 1]) & ((uint64_t(1) << overflow) - 1);
      // We then associate both values to create the final value
      lower_data = lower_data | (upper_data << (index_length - overflow));
    }

    // lower_data now contains the index in the palette
    buffer[index] = lower_data;
  }
}

Terrain::Chunk &Terrain::Data::chunkAt(int64_t xPos, int64_t zPos) {
  int32_t rX = REGION(xPos), rZ = REGION(zPos), cX = xPos & 0x1f,
          cZ = zPos & 0x1f;
  std::filesystem::path regionFile = std::filesystem::path(regionDir) /=
      "r." + std::to_string(rX) + "." + std::to_string(rZ) + ".mca";

  if (!std::filesystem::exists(regionFile)) {
    logger::deep_debug("Region file r.{}.{}.mca does not exist, skipping ..\n",
                       rX, rZ);
    return empty;
  }

  FILE *regionHandle;
  uint8_t regionHeader[REGION_HEADER_SIZE];

  if (!(regionHandle = fopen(regionFile.c_str(), "rb"))) {
    logger::error("Opening region file {} failed: {}\n", regionFile.c_str(),
                  strerror(errno));
    return empty;
  }

  // Then, we read the header (of size 4K) storing the chunks locations
  if (fread(regionHeader, sizeof(uint8_t), REGION_HEADER_SIZE, regionHandle) !=
      REGION_HEADER_SIZE) {
    logger::error("Region header too short in {}\n", regionFile.c_str());
    fclose(regionHandle);
    return empty;
  }

  const uint32_t offset =
      (_ntohl(regionHeader + ((cZ << 5) + cX) * 4) >> 8) * 4096;

  loadChunk(offset, regionHandle, xPos, zPos, regionFile);

  fclose(regionHandle);

  return chunks[chunkIndex(xPos, zPos)];
}
