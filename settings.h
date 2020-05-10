#ifndef OPTIONS_H_
#define OPTIONS_H_

#include "./helper.h"
#include "./worldloader.h"
#include <cstdint>
#include <filesystem>
#define UNDEFINED 0x7FFFFFFF

namespace Settings {

struct WorldOptions {
  std::filesystem::path saveName, outFile, colorFile;

  bool wholeworld;

  // Map boundaries
  int fromX, fromZ, toX, toZ;
  int mapMinY, mapMaxY;
  int mapSizeY;
  Terrain::Orientation orientation;

  int offsetY;

  bool hideWater;

  // Memory limits, legacy code for image splitting
  uint64_t memlimit;
  bool memlimitSet;

  WorldOptions() {
    saveName = "";
    outFile = "output.png";
    colorFile = "colors.json";

    hideWater = false;

    fromX = fromZ = toX = toZ = UNDEFINED;

    mapMinY = 0;
    mapMaxY = 255;
    mapSizeY = mapMaxY - mapMinY;
    offsetY = 3;

    wholeworld = false;
    memlimit = 2000 * uint64_t(1024 * 1024);
    memlimitSet = false;

    orientation = Terrain::Orientation::NW;
  }
};

} // namespace Settings

bool parseArgs(int argc, char **argv, Settings::WorldOptions *opts);

#endif // OPTIONS_H_
