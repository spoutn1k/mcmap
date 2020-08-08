#include <filesystem>
#include <fmt/core.h>

#define BUFFERSIZE 2000000
#define REGIONSIZE 32
#define HEADER_SIZE REGIONSIZE *REGIONSIZE * 4

using std::filesystem::exists;
using std::filesystem::path;

uint32_t _ntohi(uint8_t *val) {
  return (uint32_t(val[0]) << 24) + (uint32_t(val[1]) << 16) +
         (uint32_t(val[2]) << 8) + (uint32_t(val[3]));
}

int main(int argc, char **argv) {
  uint8_t buffer[BUFFERSIZE];
  size_t length;
  FILE *f;

  if (argc < 2 || !exists(path(argv[1]))) {
    fmt::print(stderr, "Usage: {} <Region file>\n", argv[0]);
    return 1;
  }

  if (!(f = fopen(argv[1], "r"))) {
    fmt::print(stderr, "Error opening file: {}\n", strerror(errno));
    return 1;
  }

  if ((length = fread(buffer, sizeof(uint8_t), HEADER_SIZE, f)) !=
      HEADER_SIZE) {
    fmt::print(stderr, "Error reading header, not enough bytes read.\n");
    fclose(f);
    return 1;
  }

  fclose(f);

  for (int it = 0; it < REGIONSIZE * REGIONSIZE; it++) {
    // Bound check
    const int chunkX = it & 0x1f;
    const int chunkZ = it >> 5;

    // Get the location of the data from the header
    const uint32_t offset = (_ntohi(buffer + it * 4) >> 8) * 4096;

    fmt::print("Chunk {: >2}.{: >2} {:->68}\n", chunkX, chunkZ,
               " " + (offset ? std::to_string(offset) : "Not found"));
  }

  return 0;
}
