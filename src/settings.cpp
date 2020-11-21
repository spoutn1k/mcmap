#include "./settings.h"
#include "logger.hpp"

#define ISPATH(p) (!(p).empty() && std::filesystem::exists((p)))

bool Settings::parseArgs(int argc, char **argv, Settings::WorldOptions *opts) {
#define MOREARGS(x) (argpos + (x) < argc)
#define NEXTARG argv[++argpos]
#define POLLARG(x) argv[argpos + (x)]
  int argpos = 0;
  while (MOREARGS(1)) {
    const char *option = NEXTARG;
    if (strcmp(option, "-from") == 0) {
      if (!MOREARGS(2) || !isNumeric(POLLARG(1)) || !isNumeric(POLLARG(2))) {
        logger::error("{} needs two integer arguments\n", option);
        return false;
      }
      opts->boundaries.minX = atoi(NEXTARG);
      opts->boundaries.minZ = atoi(NEXTARG);
    } else if (strcmp(option, "-to") == 0) {
      if (!MOREARGS(2) || !isNumeric(POLLARG(1)) || !isNumeric(POLLARG(2))) {
        logger::error("{} needs two integer arguments\n", option);
        return false;
      }
      opts->boundaries.maxX = atoi(NEXTARG);
      opts->boundaries.maxZ = atoi(NEXTARG);
    } else if (strcmp(option, "-max") == 0) {
      if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
        logger::error("{} needs an integer argument\n", option);
        return false;
      }
      const int height = atoi(NEXTARG);
      opts->boundaries.maxY =
          (height > MAX_TERRAIN_HEIGHT ? MAX_TERRAIN_HEIGHT : height);
    } else if (strcmp(option, "-min") == 0) {
      if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
        logger::error("{} needs an integer argument\n", option);
        return false;
      }
      const int height = atoi(NEXTARG);
      opts->boundaries.minY =
          (height < MIN_TERRAIN_HEIGHT ? MIN_TERRAIN_HEIGHT : height);
    } else if (strcmp(option, "-padding") == 0) {
      if (!MOREARGS(1) || !isNumeric(POLLARG(1)) || atoi(POLLARG(1)) < 0) {
        logger::error("{} needs an positive integer argument\n", option);
        return false;
      }
      opts->padding = atoi(NEXTARG);
    } else if (strcmp(option, "-nowater") == 0) {
      opts->hideWater = true;
    } else if (strcmp(option, "-nobeacons") == 0) {
      opts->hideBeacons = true;
    } else if (strcmp(option, "-shading") == 0) {
      opts->shading = true;
    } else if (strcmp(option, "-nether") == 0) {
      opts->dim = Dimension("the_nether");
    } else if (strcmp(option, "-end") == 0) {
      opts->dim = Dimension("the_end");
    } else if (strcmp(option, "-dimension") == 0 ||
               strcmp(option, "-dim") == 0) {
      if (!MOREARGS(1)) {
        logger::error("{} needs a dimension name or number\n", option);
        return false;
      }
      opts->dim = Dimension(NEXTARG);
    } else if (strcmp(option, "-file") == 0) {
      if (!MOREARGS(1)) {
        logger::error("{} needs one argument\n", option);
        return false;
      }
      opts->outFile = NEXTARG;
    } else if (strcmp(option, "-colors") == 0) {
      if (!MOREARGS(1)) {
        logger::error("{} needs one argument\n", option);
        return false;
      }
      opts->colorFile = NEXTARG;
      if (!ISPATH(opts->colorFile)) {
        logger::error("File {} does not exist\n", opts->colorFile.c_str());
        return false;
      }
    } else if (strcmp(option, "-dumpcolors") == 0) {
      opts->mode = Settings::DUMPCOLORS;
    } else if (strcmp(option, "-marker") == 0) {
      if (!MOREARGS(3) || !(isNumeric(POLLARG(1)) && isNumeric(POLLARG(2)))) {
        logger::error("{} needs three arguments: x z color\n", option);
        return false;
      }
      int x = atoi(NEXTARG), z = atoi(NEXTARG);
      opts->markers[opts->totalMarkers++] =
          Colors::Marker(x, z, std::string(NEXTARG));
    } else if (strcmp(option, "-nw") == 0) {
      opts->boundaries.orientation = NW;
    } else if (strcmp(option, "-sw") == 0) {
      opts->boundaries.orientation = SW;
    } else if (strcmp(option, "-ne") == 0) {
      opts->boundaries.orientation = NE;
    } else if (strcmp(option, "-se") == 0) {
      opts->boundaries.orientation = SE;
    } else if (strcmp(option, "-mb") == 0) {
      if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
        logger::error("{} needs an integer\n", option);
        return false;
      }
      opts->mem_limit = atoi(NEXTARG) * size_t(1024 * 1024);
    } else if (strcmp(option, "-tile") == 0) {
      if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
        logger::error("{} needs an integer\n", option);
        return false;
      }
      opts->tile_size = atoi(NEXTARG);
    } else if (strcmp(option, "-help") == 0 || strcmp(option, "-h") == 0) {
      return false;
    } else if (strcmp(option, "-verbose") == 0 || strcmp(option, "-v") == 0) {
      logger::level = logger::levels::DEBUG;
    } else if (strcmp(option, "-vv") == 0) {
      logger::level = logger::levels::DEEP_DEBUG;
    } else {
      opts->saveName = std::filesystem::path(option);
      if (!ISPATH(opts->saveName)) {
        logger::error("Error: File {} does not exist\n",
                      opts->saveName.c_str());
        return false;
      }
    }
  }

  if (opts->mode == RENDER) {
    // Check if the given save posesses the required dimension, must be done now
    // as the world path can be given after the dimension name, which messes up
    // regionDir()
    // TODO Check permissions and make ISPATH a real function
    if (!ISPATH(opts->regionDir())) {
      logger::error(
          "Cannot render dimension '{}' of world '{}': file '{}' does "
          "not exist\n",
          opts->dim.to_string(), opts->saveName.c_str(),
          opts->regionDir().c_str());
      return false;
    }

    // Scan the region directory and map the existing terrain in this set of
    // coordinates
    Terrain::Coordinates existingWorld;
    Terrain::scanWorldDirectory(opts->regionDir(), &existingWorld);

    if (opts->boundaries.isUndefined()) {
      // No boundaries were defined, import the whole existing world
      // No overwriting to preserve potential min/max data
      opts->boundaries.minX = existingWorld.minX;
      opts->boundaries.minZ = existingWorld.minZ;
      opts->boundaries.maxX = existingWorld.maxX;
      opts->boundaries.maxZ = existingWorld.maxZ;
    } else {
      // Restrict the map to draw to the existing terrain
      opts->boundaries.crop(existingWorld);
    }

    if (opts->boundaries.maxX < opts->boundaries.minX ||
        opts->boundaries.maxZ < opts->boundaries.minZ) {
      logger::debug("{}\n", opts->boundaries.to_string());
      logger::error("Nothing to render: -from X Z has to be <= -to X Z\n");
      return false;
    }

    if (opts->boundaries.maxX - opts->boundaries.minX < 0) {
      logger::error("Nothing to render: -min Y has to be < -max Y\n");
      return false;
    }

    if (opts->tile_size < 16) {
      logger::error("Cannot render tiles this small\n");
      return false;
    }
  }

  return true;
}
