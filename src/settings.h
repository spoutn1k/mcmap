#ifndef OPTIONS_H_
#define OPTIONS_H_

#include "./colors.h"
#include "./helper.h"
#include "./worldloader.h"
#include <cstdint>
#include <filesystem>
#include <string>
#define UNDEFINED 0x7FFFFFFF

#include "savefile.h"

namespace Settings {

enum actions { RENDER, DUMPCOLORS, HELP };

struct WorldOptions {
  // Execution mode
  int mode;

  // Files to use
  std::filesystem::path saveName, outFile, colorFile;

  // Map boundaries
  Dimension dim;
  std::string customDim;
  Terrain::Coordinates boundaries;

  // Image settings
  uint16_t padding; // Should be enough
  bool hideWater, hideBeacons, shading;

  // Marker storage
  uint8_t totalMarkers;
  Colors::Marker markers[256];

  // Memory limits, legacy code for image splitting
  size_t mem_limit;
  size_t tile_size;

  WorldOptions() : mode(RENDER), saveName(""), colorFile(""), dim("overworld") {
    outFile = "output.png";

    boundaries.setUndefined();
    boundaries.minY = 0;
    boundaries.maxY = 255;

    hideWater = hideBeacons = shading = false;
    padding = 5;

    totalMarkers = 0;

    mem_limit = 3500 * uint64_t(1024 * 1024);
    tile_size = 1024;
  }

  std::filesystem::path regionDir() { return dim.regionDir(saveName); }
};

bool parseArgs(int argc, char **argv, Settings::WorldOptions *opts);

} // namespace Settings

#endif // OPTIONS_H_
