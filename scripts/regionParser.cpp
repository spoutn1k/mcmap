#include "../src/region.h"
#include <filesystem>
#include <logger.hpp>

#define BUFFERSIZE 4096
#define REGIONSIZE 32
#define HEADER_SIZE REGIONSIZE *REGIONSIZE * 4

namespace fs = std::filesystem;

int main(int argc, char **argv) {
  fs::path region_file;

  if (argc < 2 || !fs::exists((region_file = fs::path(argv[1])))) {
    logger::error("Usage: {} <Region file>\n", argv[0]);
    return 1;
  }

  Region r(region_file);

  fmt::print("{:r}", r);
}
