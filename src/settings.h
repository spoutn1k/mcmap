#ifndef OPTIONS_H_
#define OPTIONS_H_

#include "./colors.h"
#include "./helper.h"
#include "./savefile.h"
#include <cstdint>
#include <filesystem>
#include <map.hpp>
#include <string>

#define UNDEFINED 0x7FFFFFFF

namespace Settings {

enum Action { RENDER, DUMPCOLORS, HELP };

struct WorldOptions {
  // Execution mode
  Action mode;

  // Files to use
  fs::path outFile, colorFile;

  // Map boundaries
  SaveFile save;
  Dimension dim;
  World::Coordinates boundaries;

  // Image settings
  uint16_t padding; // Should be enough
  bool hideWater, hideBeacons, shading, lighting;

  // Marker storage
  uint8_t totalMarkers;
  Colors::Marker markers[256];

  // Memory limits, legacy code for image splitting
  size_t mem_limit;
  size_t tile_size;

  WorldOptions()
      : mode(RENDER), outFile("output.png"), colorFile(""), save(),
        dim("overworld") {

    boundaries.setUndefined();
    boundaries.minY = mcmap::constants::min_y;
    boundaries.maxY = mcmap::constants::max_y;

    hideWater = hideBeacons = shading = lighting = false;
    padding = 5;

    totalMarkers = 0;

    mem_limit = 3500 * uint64_t(1024 * 1024);
    tile_size = 1024;
  }

  fs::path regionDir() const;
};

bool parseArgs(int argc, char **argv, Settings::WorldOptions *opts);

void to_json(json &j, const WorldOptions &o);

} // namespace Settings

#endif // OPTIONS_H_
