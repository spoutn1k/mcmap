#include <logger.hpp>

#define CHUNK(x) (((x) >> 4) & 0xf)
#define REGION(x) ((x) >> 9)

std::string info = "This program's purpose is to locate a chunk in the world. "
                   "Give it a block coordinate X and Z, and it will output its "
                   "region file and associated coordinates. This is useful "
                   "when used in conjunction with the extractChunk program.";

int main(int argc, char **argv) {

  auto logger = spdlog::stderr_color_mt("chunkPos");
  spdlog::set_default_logger(logger);

  if (argc < 3) {
    fmt::print(stderr, "Usage: {} <X> <Z>\n{}\n", argv[0], info);
    return 1;
  }

  int32_t x = atoi(argv[1]), z = atoi(argv[2]);

  int8_t rX = REGION(x), rZ = REGION(z);
  uint8_t cX = CHUNK(x), cZ = CHUNK(z);

  fmt::print("Block is in chunk {} {} in r.{}.{}.mca\n", cX, cZ, rX, rZ);

  return 0;
}
