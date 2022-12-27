#include <filesystem>
#include <nlohmann/json.hpp>
#include <logger.hpp>

std::string info =
    "This program reads data from the file passed as an argument or stdin on "
    "UNIX, parses it as a JSON object and converts it to binary JSON (BSON) on "
    "stdout. This script is compiled before mcmap to convert the color file "
    "(defined in a JSON) in a format easily embeddable and smaller than a "
    "text file.";

#ifndef _WINDOWS
// If not on windows, allow piping
#include <unistd.h>
#endif

using nlohmann::json;
using std::filesystem::exists;
using std::filesystem::path;

int main(int argc, char **argv) {
  auto logger = spdlog::stderr_color_mt("json2bson");
  spdlog::set_default_logger(logger);

  if (argc > 2) {
    fmt::print(stderr, "Usage: {} [json file]\n{}\n", argv[0], info);

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
