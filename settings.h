#ifndef OPTIONS_H_
#define OPTIONS_H_

#include "./helper.h"
#include "./worldloader.h"
#include <cstdint>
#include <filesystem>
#define UNDEFINED 0x7FFFFFFF

namespace Settings {

enum Dimension {
  OVERWORLD,
  NETHER,
  END,
};

struct WorldOptions {
  std::filesystem::path saveName, outFile, colorFile;

  bool wholeworld;

  // Map boundaries
  Coordinates boundaries;
  size_t splits;
  int mapSizeY;
  Orientation orientation;

  int offsetY;

  bool hideWater;

  // Memory limits, legacy code for image splitting
  uint64_t memlimit;
  bool memlimitSet;

  Dimension dim;

  WorldOptions() : boundaries(UNDEFINED) {
    saveName = "";
    dim = OVERWORLD;
    outFile = "output.png";
    colorFile = "colors.json";

    hideWater = false;

    splits = 1;
    boundaries.minY = 0;
    boundaries.maxY = 255;
    offsetY = 3;

    wholeworld = false;
    memlimit = 2000 * uint64_t(1024 * 1024);
    memlimitSet = false;
  }

  std::filesystem::path regionDir() {
    switch (dim) {
    case NETHER:
      return std::filesystem::path(saveName) /= "DIM-1/region";
    case END:
      return std::filesystem::path(saveName) /= "DIM1/region";
    default:
      break;
    }
    return std::filesystem::path(saveName) /= "region";
  }
};

bool parseArgs(int argc, char **argv, Settings::WorldOptions *opts);

} // namespace Settings

#endif // OPTIONS_H_
