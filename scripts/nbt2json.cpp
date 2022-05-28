#include <filesystem>
#include <json.hpp>
#include <logger.hpp>
#include <nbt/parser.hpp>
#include <nbt/to_json.hpp>
#include <zlib.h>

std::string info =
    "Convert a NBT file (compressed or not) into a JSON file. This operation "
    "is destructive and cannot be reversed. Extremely useful to easily "
    "diagnose errors due to format changes.";

#define BUFFERSIZE 2000000

using nlohmann::json;
using std::filesystem::exists;
using std::filesystem::path;

int main(int argc, char **argv) {
  if (argc > 2 || (argc == 2 && !exists(path(argv[1]))) ||
      (argc > 2 && !nbt::assert_NBT(argv[1]))) {
    fmt::print(stderr, "Usage: {} [NBT file]\n{}\n", argv[0], info);
    return 1;
  }

  gzFile f;

#ifndef _WINDOWS
  if (argc == 1)
    f = gzdopen(STDIN_FILENO, "r");
  else
#endif
    f = gzopen(argv[1], "r");

  if (!f) {
    logger::error("Error opening file: {}\n", strerror(errno));
    return 1;
  }

  uint8_t buffer[BUFFERSIZE];
  size_t length = gzread(f, buffer, sizeof(uint8_t) * BUFFERSIZE);
  gzclose(f);

  nbt::NBT data;

  if (!nbt::parse(buffer, length, data)) {
    logger::error("Error parsing data !\n");
    return 1;
  } else
    fmt::print("{}", json(data).dump());

  return 0;
}
