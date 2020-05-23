#include "worldloader.h"

NBT minecraft_air(nbt::tag_type::tag_end);

enum renderTypes {
  SKIP = 0,
  PRE116,
  POST116,
};

const NBT &blockAtEmpty(const NBT &, uint8_t, uint8_t, uint8_t);
const NBT &blockAtPre116(const NBT &, uint8_t, uint8_t, uint8_t);
const NBT &blockAtPost116(const NBT &, uint8_t, uint8_t, uint8_t);

const NBT &(*getBlock[3])(const NBT &, uint8_t, uint8_t, uint8_t) = {
    blockAtEmpty, blockAtPre116, blockAtPost116};

void Terrain::Data::load(const std::filesystem::path &regionDir) {
  // Parse all the necessary region files
  for (int16_t rx = REGION(map.minX); rx < REGION(map.maxX) + 1; rx++) {
    for (int16_t rz = REGION(map.minZ); rz < REGION(map.maxZ) + 1; rz++) {
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
  int8_t index = *(sections->front()["Y"].get<int8_t *>());

  // We use `tag_end` to avoid initalizing too much stuff
  for (int i = index - 1; i > -1; i--)
    sections->insert(sections->begin(), NBT(nbt::tag_type::tag_end));

  // Then, go through the array and fill holes
  // As we check for the "Y" child in the compound, and did not add it
  // previously, the index MUST not change from the original first index
  vector<NBT>::iterator it = sections->begin() + index, next = it + 1;

  while (it != sections->end() && next != sections->end()) {
    uint8_t diff = *(next->operator[]("Y").get<int8_t *>()) - index - 1;

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

void Terrain::Data::tagSections(vector<NBT> *sections) {
  // The sole purpose of this chunk analysis section is retro-compatibility
  // with 1.13-1.15 versions. In 1.16 the section format changed, and worlds may
  // contain sections with multiple formats. We tag chunks here for performance.

  for (auto it = sections->begin(); it != sections->end(); it++) {
    // First, skip it if you can
    if (!it->is_compound())
      return;

    if (!it->contains("Palette")) {
      it->operator[]("_type") = NBT(renderTypes::SKIP);
      continue;
    }

    // Block index size
    const uint64_t length =
        std::max((uint64_t)ceil(log2(it->operator[]("Palette").size())), 4ul);

    // Pre-1.16, no padding was added to the BlockStates longs, meaning that the
    // entire data fits on exactly 16*16*16*length/64 longs (+1 if there is
    // overflow). This simple check looks at the size of the array to guess what
    // type it is.
    if (it->operator[]("BlockStates").size() == uint64_t(ceil(length * 64l))) {
      it->operator[]("_type") = NBT(renderTypes::PRE116);
      continue;
    };

    it->operator[]("_type") = NBT(renderTypes::POST116);
  }
}

void Terrain::Data::stripChunk(vector<NBT> *sections) {
  // Some chunks have a -1 section, we'll pop that real quick
  if (!sections->empty() && *sections->front()["Y"].get<int8_t *>() == -1) {
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
      cache.push_back(*block["Name"].get<string *>());
  }
}

uint8_t Terrain::Data::importHeight(vector<NBT> *sections) {
  // If there are sections in the chunk
  const uint8_t chunkMin = *sections->front()["Y"].get<int8_t *>();
  const uint8_t chunkHeight = (*sections->back()["Y"].get<int8_t *>() + 1) << 4;

  // If the chunk's height is the highest found, record it
  if (chunkHeight > (heightBounds & 0xf0))
    heightBounds = chunkHeight | (heightBounds & 0x0f);

  return chunkHeight | chunkMin;
}

bool decompressChunk(const uint32_t offset, FILE *regionHandle,
                     uint8_t *chunkBuffer, uint64_t *length) {
  uint8_t zData[COMPRESSED_BUFFER];

  if (0 != fseek(regionHandle, offset, SEEK_SET)) {
    // printf("Error seeking to chunk\n");
    return false;
  }

  // Read the 5 bytes that give the size and type of data
  if (5 != fread(zData, sizeof(uint8_t), 5, regionHandle)) {
    // printf("Error reading chunk size from region file\n");
    return false;
  }

  *length = _ntohl(zData);
  // len--; // This dates from Zahl's, no idea of its purpose

  if (fread(zData, sizeof(uint8_t), *length, regionHandle) != *length) {
    // printf("Not enough input for chunk\n");
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
    printf("Error decompressing chunk: %s\n", zError(status));
    return false;
  }

  *length = zlibStream.total_out;
  return true;
}

void Terrain::Data::loadChunk(const uint32_t offset, FILE *regionHandle,
                              const int chunkX, const int chunkZ) {
  uint64_t length, chunkPos = chunkIndex(chunkX, chunkZ);

  // Buffers for chunk read from MCA files and decompression.
  uint8_t chunkBuffer[DECOMPRESSED_BUFFER];

  if (!offset || !decompressChunk(offset, regionHandle, chunkBuffer, &length))
    return;

  NBT chunk = NBT::parse(chunkBuffer, length);

  chunks[chunkPos] = std::move(chunk["Level"]["Sections"]);
  vector<NBT> *sections = chunks[chunkPos].get<vector<NBT> *>();

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

  // Check for sections of older versions of minecraft
  tagSections(sections);

  // Fill the chunk's holes with empty sections
  inflateChunk(sections);
}

const NBT &blockAtEmpty(const NBT &, uint8_t, uint8_t, uint8_t) {
  return minecraft_air;
}

const NBT &blockAtPost116(const NBT &section, uint8_t x, uint8_t z, uint8_t y) {
  // The `BlockStates` array contains data on the section's blocks. You have
  // to extract it by understanfing its structure.
  //
  // Although it is a array of long values, one must see it as an array of
  // block indexes, whose element size depends on the size of the Palette.
  // This routine locates the necessary long, extracts the block with bit
  // comparisons, and cross-references it in the palette to get the block
  // name.
  //
  // NEW in 1.16, longs are padded by 0s when a block cannot fit.

  const vector<int64_t> *blockStates =
      section["BlockStates"].get<const vector<int64_t> *>();
  const uint64_t index = (x & 0x0f) + ((z & 0x0f) + (y & 0x0f) * 16) * 16;

  // The length of a block index has to be coded on the minimal possible size,
  // which is the logarithm in base2 of the size of the palette, or 4 if the
  // logarithm is smaller.
  const uint64_t length =
      std::max((uint64_t)ceil(log2(section["Palette"].size())), 4ul);

  // First, determine how many blocks are in each long. There is an implicit
  // `floor` here, needed later.
  const uint8_t blocksPerLong = 64 / length;

  // Next, calculate where in the long array is the long containing the block.
  const uint64_t longIndex = index / blocksPerLong;

  // Once we located a long, we have to know where in the 64 bits
  // the relevant block is located.
  const uint64_t padding = (index - longIndex * blocksPerLong) * length;

  // Bring the data to the first bits of the long, then extract it by bitwise
  // comparison
  const uint64_t blockIndex =
      ((*blockStates)[longIndex] >> padding) & ((1l << length) - 1);

  // Lower data now contains the index in the palette
  return section["Palette"][blockIndex];
}

const NBT &blockAtPre116(const NBT &section, uint8_t x, uint8_t z, uint8_t y) {
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
  return section["Palette"][lower_data];
}

const NBT &Terrain::Data::block(const int32_t x, const int32_t z,
                                const int32_t y) const {

  const NBT &chunk = chunks[chunkIndex(CHUNK(x), CHUNK(z))];

  if (chunk.is_end())
    return minecraft_air;

  const NBT &section = chunk[y >> 4];

  if (!section.is_end() && section.contains("_type"))
    return (*getBlock[*section["_type"].get<const int8_t *>()])(section, x, z,
                                                                y);

  return minecraft_air;
}
