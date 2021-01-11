#include <filesystem>
#include <logger.hpp>

#define BUFFERSIZE 4096
#define REGIONSIZE 32
#define HEADER_SIZE REGIONSIZE *REGIONSIZE * 4

using std::filesystem::exists;
using std::filesystem::path;

SETUP_LOGGER;

uint32_t _ntohi(uint8_t *val) {
  return (uint32_t(val[0]) << 24) + (uint32_t(val[1]) << 16) +
         (uint32_t(val[2]) << 8) + (uint32_t(val[3]));
}

int main(int argc, char **argv) {
  char time[80];
  uint8_t locations[BUFFERSIZE], timestamps[BUFFERSIZE], data[5];
  uint32_t size = 0, read, chunkX, chunkZ, offset;
  time_t timestamp;
  size_t length;
  FILE *f;
  struct tm saved;

  if (argc < 2 || !exists(path(argv[1]))) {
    logger::error("Usage: {} <Region file>\n", argv[0]);
    return 1;
  }

  if (!(f = fopen(argv[1], "r"))) {
    logger::error("Error opening file: {}\n", strerror(errno));
    return 1;
  }

  if ((length = fread(locations, sizeof(uint8_t), HEADER_SIZE, f)) !=
      HEADER_SIZE) {
    logger::error("Error reading header, not enough bytes read.\n");
    fclose(f);
    return 1;
  }

  if ((length = fread(timestamps, sizeof(uint8_t), HEADER_SIZE, f)) !=
      HEADER_SIZE) {
    logger::error("Error reading header, not enough bytes read.\n");
    fclose(f);
    return 1;
  }

  logger::info("{}\t{}\t{}\t{}\t{}\t{}\n", "X", "Z", "Offset", "Size",
               "Compression", "Saved");

  for (int it = 0; it < REGIONSIZE * REGIONSIZE; it++) {
    // Bound check
    chunkX = it & 0x1f;
    chunkZ = it >> 5;
    size = 0;

    // Get the location of the data from the header
    offset = (_ntohi(locations + it * 4) >> 8);
    timestamp = _ntohi(timestamps + it * 4);

    if (offset) {
      fseek(f, offset * 4096, SEEK_SET);
      if ((read = fread(data, sizeof(uint8_t), 5, f)) == 5)
        size = _ntohi(data);
      else
        logger::error("Not enough data read for chunk {} {}\n", chunkX, chunkZ);

      saved = *localtime(&timestamp);
      strftime(time, 80, "%c", &saved);

    } else {
      size = 0;
      strcpy(time, "No data");
    }

    logger::info("{}\t{}\t{}\t{}\t{}\t{}\n", chunkX, chunkZ,
                 (offset ? std::to_string(offset) : "Not found"), size, data[4],
                 std::string(time));
  }

  fclose(f);
  return 0;
}
