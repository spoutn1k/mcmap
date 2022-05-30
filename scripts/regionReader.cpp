#include <filesystem>
#include <logger.hpp>
#include <nbt/parser.hpp>
#include <nbt/to_json.hpp>
#include <translator.hpp>
#include <zlib.h>

std::string info = "This program will output all the information present in a "
                   "region header from the file passed as an argument.";

#define BUFFERSIZE 4096
#define REGIONSIZE 32
#define COMPRESSED_BUFFER 500 * 1024
#define DECOMPRESSED_BUFFER 1000 * 1024
#define HEADER_SIZE REGIONSIZE *REGIONSIZE * 4

using std::filesystem::exists;
using std::filesystem::path;

uint32_t _ntohi(uint8_t *val) {
  return (uint32_t(val[0]) << 24) + (uint32_t(val[1]) << 16) +
         (uint32_t(val[2]) << 8) + (uint32_t(val[3]));
}

bool decompressChunk(FILE *regionHandle, uint8_t *chunkBuffer, uint64_t *length,
                     const std::filesystem::path &filename) {
  uint8_t zData[COMPRESSED_BUFFER];

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

int main(int argc, char **argv) {
  char time[80];
  uint8_t locations[BUFFERSIZE], timestamps[BUFFERSIZE],
      chunkBuffer[DECOMPRESSED_BUFFER];
  uint32_t chunkX, chunkZ, offset;
  time_t timestamp;
  size_t length;
  FILE *f;
  struct tm saved;

  auto logger = spdlog::stderr_color_mt("regionReader");
  spdlog::set_default_logger(logger);
  spdlog::set_level(spdlog::level::info);

  if (argc < 2 || !exists(path(argv[1]))) {
    fmt::print("Usage: {} <Region file>\n{}\n", argv[0], info);
    return 1;
  }

  if (!(f = fopen(argv[1], "r"))) {
    logger::error("Error opening file: {}", strerror(errno));
    return 1;
  }

  if ((length = fread(locations, sizeof(uint8_t), HEADER_SIZE, f)) !=
      HEADER_SIZE) {
    logger::error("Error reading header, not enough bytes read.");
    fclose(f);
    return 1;
  }

  if ((length = fread(timestamps, sizeof(uint8_t), HEADER_SIZE, f)) !=
      HEADER_SIZE) {
    logger::error("Error reading header, not enough bytes read.");
    fclose(f);
    return 1;
  }

  fmt::print("{}\t{}\t{}\t{}\t{}\n", "X", "Z", "Last Saved", "DataVersion",
             "Status");

  for (int it = 0; it < REGIONSIZE * REGIONSIZE; it++) {
    // Bound check
    chunkX = it & 0x1f;
    chunkZ = it >> 5;

    // Get the location of the data from the header
    offset = (_ntohi(locations + it * 4) >> 8);
    timestamp = _ntohi(timestamps + it * 4);

    auto data_version = 0;
    std::string status = "unknown";

    if (offset) {
      nbt::NBT nbt_data;

      fseek(f, offset * 4096, SEEK_SET);
      if (decompressChunk(f, chunkBuffer, &length, argv[1])) {
        if (nbt::parse(chunkBuffer, length, nbt_data)) {
          if (nbt_data.contains("DataVersion")) {
            data_version = nbt_data["DataVersion"].get<int>();
          }
          if (nbt_data.contains("Status")) {
            status = nbt_data["Status"].get<std::string>();
          }
        }
      }

      saved = *localtime(&timestamp);
      strftime(time, 80, "%c", &saved);
    } else {
      strcpy(time, "No data for chunk");
    }

    fmt::print("{}\t{}\t{}\t{}\t{}\n", chunkX, chunkZ, std::string(time),
               data_version, status);
  }

  fclose(f);
  return 0;
}
