#ifndef OPTIONS_H_
#define OPTIONS_H_

#include "./colors.h"
#include "./helper.h"
#include "./marker.h"
#include "./savefile.h"
#include <cstdint>
#include <filesystem>
#include <map.hpp>
#include <string>

#define UNDEFINED 0x7FFFFFFF

namespace Settings {
const string OUTPUT_DEFAULT = "output.png";
const string OUTPUT_TILED_DEFAULT = "output";
const size_t PADDING_DEFAULT = 5;
const size_t TILE_SIZE_DEFAULT = 0;
const size_t ZOOM_LEVELS_DEFAULT = 3;

enum Action { RENDER, DUMPCOLORS, HELP };

struct WorldOptions {
  // Execution mode
  Action mode;

  // Files to use
  fs::path outFile, colorFile, markerFile;

  // Map boundaries
  SaveFile save;
  Dimension dim;
  World::Coordinates boundaries;

  // Image settings
  uint16_t padding;
  bool hideWater, hideBeacons, shading, lighting;
  size_t tile_size; // 0 means no tiling
  uint8_t zoom_levels;

  // Memory limits
  size_t mem_limit;
  size_t fragment_size;

  WorldOptions()
      : mode(RENDER), outFile(OUTPUT_DEFAULT), colorFile(""), save(),
        dim("overworld") {

    boundaries.setUndefined();
    boundaries.minY = mcmap::constants::min_y;
    boundaries.maxY = mcmap::constants::max_y;

    hideWater = hideBeacons = shading = lighting = false;
    padding = PADDING_DEFAULT;
    tile_size = TILE_SIZE_DEFAULT;
    zoom_levels = ZOOM_LEVELS_DEFAULT;

    // Default 3.5G of memory maximum
    mem_limit = 3500 * uint64_t(1024 * 1024);
    // Render whole regions at once
    fragment_size = 1024;
  }

  fs::path regionDir() const { return save.region(dim); }
};

void to_json(json &j, const WorldOptions &o);

} // namespace Settings

#endif // OPTIONS_H_
