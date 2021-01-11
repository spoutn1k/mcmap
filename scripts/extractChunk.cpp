#include <filesystem>
#include <logger.hpp>
#include <zlib.h>

#define BUFFERSIZE 2000000
#define DECOMPRESSED_BUFFER BUFFERSIZE
#define REGIONSIZE 32
#define HEADER_SIZE REGIONSIZE *REGIONSIZE * 4

SETUP_LOGGER;

using std::filesystem::exists;
using std::filesystem::path;

uint32_t _ntohi(uint8_t *val) {
  return (uint32_t(val[0]) << 24) + (uint32_t(val[1]) << 16) +
         (uint32_t(val[2]) << 8) + (uint32_t(val[3]));
}

bool isNumeric(const char *str) {
  if (str[0] == '-' && str[1] != '\0') {
    ++str;
  }
  while (*str != '\0') {
    if (*str < '0' || *str > '9') {
      return false;
    }
    ++str;
  }
  return true;
}

int main(int argc, char **argv) {
  uint8_t buffer[BUFFERSIZE], data[DECOMPRESSED_BUFFER];
  size_t length;
  FILE *f;

  if (argc < 4 || !exists(path(argv[1])) || !isNumeric(argv[2]) ||
      !isNumeric(argv[3])) {
    logger::error("Usage: {} <Region file> <X> <Z>\n", argv[0]);
    return 1;
  }

  uint8_t x = atoi(argv[2]), z = atoi(argv[3]);

  if (x > 31 || z > 31) {
    logger::error("Invalid coordinates: {} {} must be 0 and 31\n", x, z);
    return 1;
  }

  if (isatty(STDOUT_FILENO)) {
    logger::error(
        "Not printing compressed data to a terminal, pipe to a file instead\n");
    return 1;
  }

  if (!(f = fopen(argv[1], "r"))) {
    logger::error("Error opening file: {}\n", strerror(errno));
    return 1;
  }

  if ((length = fread(buffer, sizeof(uint8_t), HEADER_SIZE, f)) !=
      HEADER_SIZE) {
    logger::error("Error reading header, not enough bytes read.\n");
    fclose(f);
    return 1;
  }

  const uint32_t offset =
      (_ntohi(buffer + (x + z * REGIONSIZE) * 4) >> 8) * 4096;

  if (!offset) {
    logger::error("Error: Chunk not found\n");
    fclose(f);
    return 1;
  }

  if (0 != fseek(f, offset, SEEK_SET)) {
    logger::error("Accessing chunk data in file {} failed: {}\n", argv[1],
                  strerror(errno));
    fclose(f);
    return 1;
  }

  // Read the 5 bytes that give the size and type of data
  if (5 != fread(buffer, sizeof(uint8_t), 5, f)) {
    logger::error("Reading chunk size from region file {} failed: {}\n",
                  argv[1], strerror(errno));
    fclose(f);
    return 1;
  }

  length = _ntohi(buffer);
  length--; // Sometimes the data is 1 byte smaller

  if (fread(buffer, sizeof(uint8_t), length, f) != length) {
    logger::error("Not enough data for chunk: {}\n", strerror(errno));
    fclose(f);
    return 1;
  }

  fclose(f);

  z_stream zlibStream;
  memset(&zlibStream, 0, sizeof(z_stream));
  zlibStream.next_in = (Bytef *)buffer;
  zlibStream.next_out = (Bytef *)data;
  zlibStream.avail_in = length;
  zlibStream.avail_out = DECOMPRESSED_BUFFER;
  inflateInit2(&zlibStream, 32 + MAX_WBITS);

  int status = inflate(&zlibStream, Z_FINISH); // decompress in one step
  inflateEnd(&zlibStream);

  if (status != Z_STREAM_END) {
    logger::error("Decompressing chunk data failed: {}\n", zError(status));
    return 1;
  }

  length = zlibStream.total_out;

  int outfd = dup(STDOUT_FILENO);
  close(STDOUT_FILENO);
  gzFile out = gzdopen(outfd, "w");
  gzwrite(out, data, length);
  gzclose(out);

  return 0;
}
