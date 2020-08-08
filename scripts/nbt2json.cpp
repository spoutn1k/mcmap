#include <filesystem>
#include <fmt/core.h>
#include <json.hpp>
#include <nbt/to_json.hpp>
#include <zlib.h>

#define BUFFERSIZE 2000000

using nlohmann::json;
using std::filesystem::exists;
using std::filesystem::path;

int main(int argc, char **argv) {
  if (argc < 2 || !exists(path(argv[1]))) {
    fmt::print(stderr, "Usage: {} <NBT file>\n", argv[0]);
    return 1;
  }

  gzFile f = gzopen(argv[1], "r");

  uint8_t buffer[BUFFERSIZE];
  size_t length = gzread(f, buffer, sizeof(uint8_t) * BUFFERSIZE);
  gzclose(f);

  json data = nbt::NBT::parse(buffer, length);

  fmt::print("{}", data.dump());

  return 0;
}
