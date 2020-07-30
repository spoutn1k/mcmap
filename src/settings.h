#ifndef OPTIONS_H_
#define OPTIONS_H_

#include "./colors.h"
#include "./helper.h"
#include "./worldloader.h"
#include <cstdint>
#include <filesystem>
#include <string>
#define UNDEFINED 0x7FFFFFFF

namespace Settings {

enum Dimension {
  OVERWORLD,
  NETHER,
  END,
  CUSTOM,
};

struct WorldOptions {
  // Files to use
  std::filesystem::path saveName, outFile, colorFile;

  // Map boundaries
  Dimension dim;
  std::string customDim;
  Coordinates boundaries;
  uint16_t splits;

  // Image settings
  uint16_t padding; // Should be enough
  bool hideWater, hideBeacons, shading;

  // Marker storage
  uint8_t totalMarkers;
  Colors::Marker markers[256];

  // Memory limits, legacy code for image splitting
  int offsetY;
  uint64_t memlimit;
  bool memlimitSet, wholeworld;

  WorldOptions() {
    saveName = "";
    outFile = "output.png";
    colorFile = "colors.json";

    splits = 1;
    dim = OVERWORLD;
    boundaries.setUndefined();
    boundaries.minY = 0;
    boundaries.maxY = 255;

    offsetY = 3;
    hideWater = hideBeacons = shading = false;
    padding = 5;

    totalMarkers = 0;

    wholeworld = false;
    memlimit = 2000 * uint64_t(1024 * 1024);
    memlimitSet = false;
  }

  std::filesystem::path regionDir() {
    size_t sepIndex;
    std::string ns, id;
    switch (dim) {
    case NETHER:
      return std::filesystem::path(saveName) /= "DIM-1/region";
    case END:
      return std::filesystem::path(saveName) /= "DIM1/region";
    case CUSTOM:
      sepIndex = customDim.find_first_of(":/");
      if (sepIndex == std::string::npos) {
        if (customDim.substr(0, 3) == "DIM")
          return (std::filesystem::path(saveName) /= customDim) /= "region";
        else if (customDim == "0")
          break;
        else if (isNumeric(customDim.c_str()))
          return (std::filesystem::path(saveName) /= ("DIM" + customDim)) /= "region";
        ns = "minecraft";
        id = customDim;
      } else {
        ns = customDim.substr(0, sepIndex);
        id = customDim.substr(sepIndex + 1);
      }
      if (ns == "minecraft") {
        if (id == "overworld") 
          break;
        else if (id == "the_nether")
          return std::filesystem::path(saveName) /= "DIM-1/region";
        else if (id == "the_end")
          return std::filesystem::path(saveName) /= "DIM1/region";
      }
      return (((std::filesystem::path(saveName) /= "dimensions") /= ns) /= id) /= "region";
      break;
    default:
      break;
    }
    return std::filesystem::path(saveName) /= "region";
  }
};

bool parseArgs(int argc, char **argv, Settings::WorldOptions *opts);

} // namespace Settings

#endif // OPTIONS_H_
