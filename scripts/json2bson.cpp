#include <filesystem>
#include <fmt/core.h>
#include <json.hpp>

using nlohmann::json;
using std::filesystem::exists;
using std::filesystem::path;

int main(int argc, char **argv) {
  if (argc < 2 || !exists(path(argv[1]))) {
    fmt::print(stderr, "Usage: {} <json file>\n", argv[0]);
    return 1;
  }

  json data;
  FILE *f = fopen(argv[1], "r");

  try {
    data = json::parse(f);
  } catch (const json::parse_error &err) {
    fmt::print(stderr, "Error parsing {}: {}\n", argv[1], err.what());
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
