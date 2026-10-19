#include "./region.h"
#include <algorithm>
#include <ctime>
#include <fstream>
#include <limits>
#include <nbt/parser.hpp>
#include <nbt/stream.hpp>
#include <nbt/writer.hpp>
#include <translator.hpp>

// The header is made of a 4K table of locations, then a 4K table of timestamps
#define HEADER_ENTRIES (REGIONSIZE * REGIONSIZE)
#define SECTOR_SIZE 4096
#define HEADER_SECTORS 2

// Compression types of chunk data
#define COMPRESSION_GZIP 1
#define COMPRESSION_ZLIB 2

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

size_t Region::index(const mcmap::Chunk::coordinates &coords) {
  // The header is ordered by z then x, using the coordinates of the chunk
  // relative to the region
  return (coords.x & (REGIONSIZE - 1)) +
         (coords.z & (REGIONSIZE - 1)) * REGIONSIZE;
}

Region::Region(const fs::path &_file) : file(_file) {
  locations.fill(Location());
  timestamps.fill(0);

  if (!fs::exists(file))
    return;

  std::ifstream regionData(file, std::ifstream::binary);
  std::array<uint8_t, HEADER_SECTORS * SECTOR_SIZE> header;

  if (!regionData.read(reinterpret_cast<char *>(header.data()),
                       header.size())) {
    logger::error("Error reading `{}` header.", _file.string());
    return;
  }

  for (size_t chunk = 0; chunk < HEADER_ENTRIES; chunk++) {
    locations[chunk].raw_data = translate<uint32_t>(&header[4 * chunk]);
    timestamps[chunk] = translate<uint32_t>(&header[SECTOR_SIZE + 4 * chunk]);
  }
}

bool Region::write_header() {
  // Create file if empty, as opening it in read/write mode requires it to exist
  if (!fs::exists(file))
    std::ofstream(file, std::ios::binary).close();

  std::fstream out(file, std::ios::in | std::ios::out | std::ios::binary);

  if (!out) {
    logger::error("Opening header for writing failed");
    return false;
  }

  std::array<uint8_t, HEADER_SECTORS * SECTOR_SIZE> header;

  for (size_t chunk = 0; chunk < HEADER_ENTRIES; chunk++) {
    translate(&header[4 * chunk], locations[chunk].raw_data);
    translate(&header[SECTOR_SIZE + 4 * chunk], timestamps[chunk]);
  }

  out.seekp(0, std::ios::beg);
  out.write(reinterpret_cast<char *>(header.data()), header.size());
  out.flush();

  if (!out) {
    logger::error("Writing header of `{}` failed", file.string());
    return false;
  }

  return true;
}

size_t Region::get_offset(uint8_t size) {
  // Get an offset at which `size` consecutive 4K sectors are free
  std::array<Location, REGIONSIZE * REGIONSIZE> sorted = locations;
  std::sort(sorted.begin(), sorted.end(), Location::order);

  // The first available sector is right after the header of 8K
  size_t sector = HEADER_SECTORS;

  // For every location in the sorted header
  for (const Location &location : sorted) {
    // Continue if the location is empty, does not code any space
    if (!location.raw_data)
      continue;

    // If there is enough space before this location, we found a pocket
    if (location.offset() >= sector + size) {
      logger::trace("Empty region of size {} at offset {}",
                    location.offset() - sector, sector);
      break;
    }

    // Put the sector at the end of the location's data. Overlapping locations
    // in corrupted headers must not move it backwards.
    sector = std::max<size_t>(sector, location.offset() + location.size());
  }

  return sector;
}

static bool decompressChunk(std::ifstream &region, const uint64_t offset,
                            std::vector<uint8_t> &output, uint64_t *length) {
  uint8_t header[5];

  region.seekg(offset, std::ios::beg);

  // Read the 5 bytes that give the size and type of data
  if (!region.read(reinterpret_cast<char *>(header), 5)) {
    logger::error("Reading chunk header at offset {} failed", offset);
    return false;
  }

  // The size includes the compression type byte
  const uint32_t size = translate<uint32_t>(header);
  const uint8_t compression = header[4];

  if (size < 2 || size - 1 > COMPRESSED_BUFFER) {
    logger::error("Invalid chunk size {} at offset {}", size, offset);
    return false;
  }

  if (compression != COMPRESSION_GZIP && compression != COMPRESSION_ZLIB) {
    logger::error("Unsupported chunk compression type {} at offset {}",
                  compression, offset);
    return false;
  }

  std::vector<uint8_t> compressed(size - 1);

  if (!region.read(reinterpret_cast<char *>(compressed.data()),
                   compressed.size())) {
    logger::error("Reading chunk data at offset {} failed", offset);
    return false;
  }

  z_stream zlibStream;
  memset(&zlibStream, 0, sizeof(z_stream));
  zlibStream.next_in = compressed.data();
  zlibStream.next_out = output.data();
  zlibStream.avail_in = compressed.size();
  zlibStream.avail_out = output.size();
  // Detect gzip or zlib headers automatically
  inflateInit2(&zlibStream, 32 + MAX_WBITS);

  int status = inflate(&zlibStream, Z_FINISH); // decompress in one step
  inflateEnd(&zlibStream);

  if (status != Z_STREAM_END) {
    logger::error("Decompressing chunk data failed: {}", zError(status));
    return false;
  }

  *length = zlibStream.total_out;
  return true;
}

bool Region::get_chunk(const mcmap::Chunk::coordinates &coords,
                       nbt::NBT &data) {
  std::vector<uint8_t> chunkBuffer(DECOMPRESSED_BUFFER);
  uint64_t length;
  const Location l = locations[index(coords)];

  if (!l.raw_data) {
    logger::debug("Chunk {} {} does not exist in region", coords.x, coords.z);
    return false;
  }

  std::ifstream region(file, std::ios::binary);

  if (!decompressChunk(region, uint64_t(l.offset()) * SECTOR_SIZE, chunkBuffer,
                       &length))
    return false;

  return nbt::parse(chunkBuffer.data(), length, data);
}

bool Region::add_chunk(const mcmap::Chunk::coordinates &coords,
                       const nbt::NBT &data) {
  // Serialize the NBT in memory. The buffer is the same size as the one used
  // to read chunks, so any chunk written can be read back.
  std::vector<uint8_t> chunk(DECOMPRESSED_BUFFER);
  nbt::io::ByteStreamWriter memory(chunk.data(), chunk.size());

  if (!nbt::put(memory, data)) {
    logger::error("Serializing chunk {} {} failed", coords.x, coords.z);
    return false;
  }

  // Compress the above
  std::vector<uint8_t> compressed(compressBound(memory.total_written));
  z_stream zlibStream;
  memset(&zlibStream, 0, sizeof(z_stream));
  zlibStream.next_in = chunk.data();
  zlibStream.next_out = compressed.data();
  zlibStream.avail_in = memory.total_written;
  zlibStream.avail_out = compressed.size();

  deflateInit(&zlibStream, Z_BEST_COMPRESSION);
  int zStatus = deflate(&zlibStream, Z_FINISH); // compress in one step
  deflateEnd(&zlibStream);

  if (zStatus != Z_STREAM_END) {
    logger::error("Compressing chunk data failed: {}", zError(zStatus));
    return false;
  }

  const uint32_t compressed_size = zlibStream.total_out;

  logger::trace("Compressed chunk {} {}: compressed size {}", coords.x,
                coords.z, compressed_size);

  // The data is prefixed by its size (4 bytes) and compression type (1 byte),
  // and the whole is padded to fill 4K sectors
  const size_t sectors = (compressed_size + 5 + SECTOR_SIZE - 1) / SECTOR_SIZE;

  if (sectors > std::numeric_limits<Location::size_type>::max()) {
    logger::error("Chunk {} {} is too big to fit in a region file", coords.x,
                  coords.z);
    return false;
  }

  std::vector<uint8_t> blob(sectors * SECTOR_SIZE, 0);
  // The size includes the compression type byte
  translate(blob.data(), uint32_t(compressed_size + 1));
  blob[4] = COMPRESSION_ZLIB;
  memcpy(blob.data() + 5, compressed.data(), compressed_size);

  // Get the minimum available offset for this chunk in the file. The chunk's
  // previous location is still marked as used, so if anything fails before the
  // header is updated, the previous data is left intact.
  const size_t offset = get_offset(sectors);

  // Make sure the file exists and has a header before writing data after it
  if (!fs::exists(file) && !write_header())
    return false;

  // in | out as not to erase the contents of the file, as the changes made are
  // localized
  std::fstream out(file, std::ios::in | std::ios::out | std::ios::binary);
  out.seekp(offset * SECTOR_SIZE, std::ios::beg);
  out.write(reinterpret_cast<char *>(blob.data()), blob.size());
  out.flush();

  if (!out) {
    logger::error("Writing chunk {} {} to `{}` failed", coords.x, coords.z,
                  file.string());
    return false;
  }

  out.close();

  // Point the header to the new data, releasing the previous location
  locations[index(coords)] = Location(offset, sectors);
  timestamps[index(coords)] = std::time(nullptr);

  return write_header();
}
