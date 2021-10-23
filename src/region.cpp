#include "./region.h"
#include <nbt/parser.hpp>
#include <nbt/stream.hpp>
#include <nbt/writer.hpp>

uint32_t _ntohi(uint8_t *val) {
  return (uint32_t(val[0]) << 24) + (uint32_t(val[1]) << 16) +
         (uint32_t(val[2]) << 8) + (uint32_t(val[3]));
}

std::pair<int32_t, int32_t> Region::coordinates(const fs::path &_file) {
  int16_t rX, rZ;
  std::string buffer;
  const char delimiter = '.';

  std::stringstream ss(_file.filename().string());
  std::getline(ss, buffer, delimiter); // This removes the 'r.'
  std::getline(ss, buffer, delimiter); // X in r.X.Z.mca

  rX = atoi(buffer.c_str());

  std::getline(ss, buffer, delimiter); // Z in r.X.Z.mca

  rZ = atoi(buffer.c_str());

  return {rX, rZ};
}

Region::Region(const fs::path &_file) : file(_file) {
  locations.fill(Location());

  if (!fs::exists(file))
    return;

  std::ifstream regionData(file, std::ifstream::binary);

  for (uint16_t chunk = 0; chunk < REGIONSIZE * REGIONSIZE; chunk++) {
    char buffer[4];
    regionData.read(buffer, 4);

    locations[chunk].raw_data = _ntohi((uint8_t *)buffer);

    if (!regionData) {
      logger::error("Error reading `{}` header.\n", _file.string());
      break;
    }
  }

  regionData.close();
}

void Region::write_header() {
  // Create file if empty
  if (!fs::exists(file))
    std::ofstream(file.string()).close();

  std::fstream out(file.c_str(),
                   std::ios::in | std::ios::out | std::ios::binary);

  if (!out) {
    logger::error("Opening header for writing failed\n");
    return;
  }

  out.seekp(0, std::ios::beg);

  for (uint16_t chunk = 0; chunk < REGIONSIZE * REGIONSIZE; chunk++) {
    uint32_t bytes = _ntohi((uint8_t *)&locations[chunk].raw_data);
    out.write((char *)&bytes, 4);
  }

  std::array<char, 4 * REGIONSIZE * REGIONSIZE> timestamps;
  timestamps.fill(0);

  out.write(&timestamps[0], 4 * REGIONSIZE * REGIONSIZE);

  out.close();
}

size_t Region::get_offset(uint8_t max_size) {
  // Get an offset at which the first empty 4K bytes blocks are free
  std::array<Location, REGIONSIZE *REGIONSIZE> sorted = locations;
  std::sort(sorted.begin(), sorted.end(), Location::order);

  // The first available block is right after the header of 8K
  size_t block = 2;
  auto it = sorted.begin();

  // For every location in the sorted header
  while (it != sorted.end()) {
    // Continue if the location is empty, does not code any space
    if (!it->raw_data) {
      it++;
      continue;
    }

    // If the next offset is not the current block, we just found a pocket
    if (it->offset() != block) {
      logger::deep_debug("Empty region of size {} at offset {}\n",
                         it->offset() - block, block);

      // Return if this pocket of blocks is big enough
      if (it->offset() - block >= max_size)
        break;
    }

    // Put the block at the end of the location's data
    block = it->offset() + it->size();

    it++;
  }

  return block;
}

bool decompressChunk(const uint32_t offset, std::ifstream &region,
                     uint8_t *chunkBuffer, uint64_t *length) {
  uint8_t zData[COMPRESSED_BUFFER];

  region.seekg(offset, std::ios::beg);

  // Read the 5 bytes that give the size and type of data
  region.read(reinterpret_cast<char *>(zData), 5);

  // Read the size on the first 4 bytes, discard the type
  *length = translate<uint32_t>(zData);
  (*length)--; // Sometimes the data is 1 byte smaller

  region.read(reinterpret_cast<char *>(zData), *length);

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

void Region::get_chunk(const mcmap::Chunk::coordinates &coords,
                       nbt::NBT &data) {
  uint8_t chunkBuffer[DECOMPRESSED_BUFFER];
  size_t length;
  Location l = locations[coords.x * REGIONSIZE + coords.z];

  if (!l.raw_data) {
    logger::error("Chunk {} {} does not exist in region\n", coords.x, coords.z);
    return;
  }

  std::ifstream region(file);

  decompressChunk(l.offset() * 4096, region, chunkBuffer, &length);

  region.close();

  nbt::parse(chunkBuffer, length, data);
}

void Region::add_chunk(const mcmap::Chunk::coordinates &coords,
                       const nbt::NBT &data) {
  uint8_t chunk[1024 * 1024];
  uint8_t compressed_chunk[65025]; // TODO set to 0
  uint32_t blob_size, inverted_blob_size;

  int zStatus;

  // Prepare a memory buffer for the NBT
  nbt::io::ByteStreamWriter memory(chunk, 1024 * 1024);
  put(memory, data);

  // Compress the above
  z_stream zlibStream;
  memset(&zlibStream, 0, sizeof(z_stream));
  zlibStream.next_in = (Bytef *)chunk;
  zlibStream.next_out = (Bytef *)compressed_chunk;
  zlibStream.avail_in = memory.total_written;
  zlibStream.avail_out = 65025;

  deflateInit(&zlibStream, Z_BEST_COMPRESSION);
  zStatus = deflate(&zlibStream, Z_FINISH); // compress in one step
  deflateEnd(&zlibStream);

  if (zStatus != Z_STREAM_END) {
    logger::info("Compressing chunk data failed: {}\n", zError(zStatus));
    return;
  }

  logger::deep_debug("Compressed chunk {} {}: compressed size {}\n", coords.x,
                     coords.z, zlibStream.total_out);

  // Add the compression type byte to the total size
  blob_size = zlibStream.total_out + 1;
  inverted_blob_size =
      translate<uint32_t>(reinterpret_cast<uint8_t *>(&blob_size));

  // Mark the chunk as erased, for its space to be available again
  // TODO Maybe 0 it out ?
  locations[coords.x * REGIONSIZE + coords.z] = Location();

  // The required amount of 4K blocks to store all of the data
  uint8_t blocks = std::ceil(double(zlibStream.total_out) / 4096);

  // Get the minimum available offset for this chunk in the file
  size_t offset = get_offset(blocks);

  // Set the location for this chunk
  locations[coords.x * REGIONSIZE + coords.z] = Location(offset, blocks);

  // Update the location and timestamp header
  write_header();

  // in | out as not to erase the contents of the file, as the changes made are
  // localized
  std::fstream out(file, std::ios::in | std::ios::out | std::ios::binary);
  out.seekp(offset * 4096, std::ios::beg);

  // Write: size, compression type, data
  out.write(reinterpret_cast<char *>(&inverted_blob_size), 4);
  out.put(2);
  out.write(reinterpret_cast<char *>(compressed_chunk), blocks * 4096 - 5);

  out.close();
}
