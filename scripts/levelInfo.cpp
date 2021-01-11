#include <filesystem>
#include <logger.hpp>
#include <map>
#include <nbt/nbt.hpp>
#include <zlib.h>

namespace fs = std::filesystem;
using namespace nbt;

#define BUFFERSIZE 1024 * 1024
SETUP_LOGGER;

uint8_t buffer[BUFFERSIZE];

bool assert_save(fs::path root) {
  fs::path level = root / "level.dat", region = root / "region";

  std::map<fs::path, fs::file_type> requirements = {
      {root, fs::file_type::directory},
      {level, fs::file_type::regular},
      {region, fs::file_type::directory}};

  for (auto &r : requirements) {
    if (fs::status(r.first).type() != r.second) {
      logger::debug("File '{}' is of an unexpected format ({}/{})\n",
                    r.first.string(), fs::status(r.first).type(), r.second);
      return false;
    }
  }

  return true;
}

int main(int argc, char **argv) {
  gzFile f;
  fs::path root, level;

  logger::level = logger::levels::DEBUG;

  if (argc < 2) {
    logger::error("Usage: {} <level>\n", argv[0]);
    return 1;
  }

  root = fs::path(argv[1]);
  level = root / "level.dat";

  if (!assert_save(root)) {
    logger::error("File '{}' is not a save folder\n", root.string());
    return 1;
  }

  if (!(f = gzopen(level.c_str(), "r"))) {
    logger::error("Error opening file: {}\n", strerror(errno));
    return 1;
  }

  size_t length = gzread(f, buffer, sizeof(uint8_t) * BUFFERSIZE);

  NBT data = NBT::parse(buffer, length)["Data"];

  logger::info("Name: {}\n", data["LevelName"].get<std::string>());
  logger::info("Last played: {}\n", data["LastPlayed"].get<long>());

  return 0;
}
