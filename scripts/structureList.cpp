#include "../src/region.h"
#include "../src/savefile.h"
#include "../src/structure.h"
#include <algorithm>
#include <cmath>
#include <logger.hpp>

using mcmap::BoundingBox;
using ChunkCoordinates = mcmap::Chunk::coordinates;
using namespace mcmap::structure;

#define RADIUS_DEFAULT 128

struct Entry {
  std::string id;
  ChunkCoordinates start;
  BoundingBox bounds;
  size_t pieces;
  double distance;
};

void usage(const char *binary) {
  fmt::print(stderr,
             "Usage: {} [options] WORLD [X Z]\n\n"
             "List the structures of WORLD and the chunks they start in. If "
             "a block position\nX Z is given, only list the structures "
             "starting around it.\n\n"
             "  -dim[ension] NAME  dimension to use; default is overworld\n"
             "  -id ID             only list the structures of this id\n"
             "  -radius R (={})   radius around X Z, in blocks\n",
             binary, RADIUS_DEFAULT);
}

int main(int argc, char **argv) {
  auto logger = spdlog::stderr_color_mt("structureList");
  spdlog::set_default_logger(logger);

  Dimension dim("overworld");
  std::string filter;
  int32_t radius = RADIUS_DEFAULT;
  std::vector<std::string> positional;

  for (int i = 1; i < argc; i++) {
    const std::string option = argv[i];

    if ((option == "-dim" || option == "-dimension") && i + 1 < argc)
      dim = Dimension(argv[++i]);
    else if (option == "-id" && i + 1 < argc)
      filter = argv[++i];
    else if (option == "-radius" && i + 1 < argc && isNumeric(argv[i + 1]))
      radius = atoi(argv[++i]);
    else if (option == "-h" || option == "-help") {
      usage(argv[0]);
      return 0;
    } else
      positional.push_back(option);
  }

  const bool around = positional.size() == 3;

  if ((positional.size() != 1 && !around) ||
      (around && (!isNumeric(positional[1].c_str()) ||
                  !isNumeric(positional[2].c_str())))) {
    usage(argv[0]);
    return 1;
  }

  SaveFile save(positional[0]);

  if (!save.valid()) {
    logger::error("{} is not a save folder", positional[0]);
    return 1;
  }

  const fs::path directory = save.region(dim);

  if (directory.empty()) {
    logger::error("The world does not contain the dimension {}",
                  dim.to_string());
    return 1;
  }

  const int32_t X = around ? atoi(positional[1].c_str()) : 0,
                Z = around ? atoi(positional[2].c_str()) : 0;

  std::vector<Entry> entries;

  for (const auto &file : fs::directory_iterator(directory)) {
    const std::string name = file.path().filename().string();

    if (name.rfind("r.", 0) != 0 || file.path().extension() != ".mca")
      continue;

    const auto [rX, rZ] = Region::coordinates(file.path());

    // Skip the regions out of the radius
    if (around && ((rX << 9) + 511 < X - radius || (rX << 9) > X + radius ||
                   (rZ << 9) + 511 < Z - radius || (rZ << 9) > Z + radius))
      continue;

    Region region(file.path());

    for (int32_t z = 0; z < REGIONSIZE; z++) {
      for (int32_t x = 0; x < REGIONSIZE; x++) {
        const ChunkCoordinates pos = {(rX << 5) + x, (rZ << 5) + z};

        // Distance between the position and the center of the chunk
        const double distance =
            std::hypot((pos.x << 4) + 8 - X, (pos.z << 4) + 8 - Z);

        if (around && distance > radius)
          continue;

        nbt::NBT chunk;

        if (!region.locations[Region::index(pos)].raw_data ||
            !region.get_chunk(pos, chunk) || !mcmap::Chunk::assert_chunk(chunk))
          continue;

        nbt::NBT *data = compound(chunk);
        if (!data || !data->contains(starts_key(*data)))
          continue;

        const nbt::NBT &starts = (*data)[starts_key(*data)];

        for (auto it = starts.begin(); it != starts.end(); ++it) {
          if (!filter.empty() && it.key() != filter)
            continue;

          const std::vector<BoundingBox> boxes = pieces(*it);
          if (boxes.empty())
            continue;

          BoundingBox bounds = boxes.front();
          for (const auto &box : boxes)
            bounds = {std::min(bounds.minX, box.minX),
                      std::min(bounds.minY, box.minY),
                      std::min(bounds.minZ, box.minZ),
                      std::max(bounds.maxX, box.maxX),
                      std::max(bounds.maxY, box.maxY),
                      std::max(bounds.maxZ, box.maxZ)};

          entries.push_back({it.key(), pos, bounds, boxes.size(), distance});
        }
      }
    }
  }

  // Closest first around a position, else grouped by structure
  std::sort(entries.begin(), entries.end(),
            [around](const Entry &a, const Entry &b) {
              if (around)
                return a.distance < b.distance;
              if (a.id != b.id)
                return a.id < b.id;
              return a.start < b.start;
            });

  for (const auto &entry : entries) {
    fmt::print("{:<36} chunk {:>6} {:<6} {:>4} pieces  from {} {} {} to {} "
               "{} {}",
               entry.id, entry.start.x, entry.start.z, entry.pieces,
               entry.bounds.minX, entry.bounds.minY, entry.bounds.minZ,
               entry.bounds.maxX, entry.bounds.maxY, entry.bounds.maxZ);

    if (around)
      fmt::print("  ({:.0f} blocks away)", entry.distance);

    fmt::print("\n");
  }

  logger::info("{} structures found", entries.size());
  return 0;
}
