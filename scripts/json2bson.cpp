#include <filesystem>
#include <json.hpp>
#include <logger.hpp>

#ifndef _WINDOWS
// If not on windows, allow piping
#include <unistd.h>
#endif

using nlohmann::json;
using std::filesystem::exists;
using std::filesystem::path;

int main(int argc, char **argv) {
  if (argc > 2) {
    logger::error("Usage: {} [json file]", argv[0]);

    return 1;
  }

  json data;
  FILE *f;

#ifndef _WINDOWS
  if (argc == 1)
    f = fdopen(STDIN_FILENO, "r");
  else
#endif
    f = fopen(argv[1], "r");

  if (!f) {
    logger::error("{}: Error opening {}: {}", argv[0], argv[1],
                  strerror(errno));
    return 1;
  }

  try {
    data = json::parse(f);
  } catch (const json::parse_error &err) {
    logger::error("{}: Error parsing {}: {}", argv[0], argv[1], err.what());
    fclose(f);
    return 1;
  }
  fclose(f);

  std::vector<uint8_t> bson_vector = json::to_bson(data);

  fmt::print("{{");
  for (auto byte : bson_vector)
    fmt::print("{:#x}, ", byte);
  fmt::print("}}\n");

  return 0;
}
