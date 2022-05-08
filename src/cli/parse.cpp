#include "./parse.h"

#define ISPATH(p) (!(p).empty() && std::filesystem::exists((p)))

bool parseArgs(int argc, char **argv, Settings::WorldOptions *opts) {
#define MOREARGS(x) (argpos + (x) < argc)
#define NEXTARG argv[++argpos]
#define POLLARG(x) argv[argpos + (x)]
  int argpos = 0;
  while (MOREARGS(1)) {
    const char *option = NEXTARG;
    if (strcmp(option, "-from") == 0) {
      if (!MOREARGS(2) || !isNumeric(POLLARG(1)) || !isNumeric(POLLARG(2))) {
        logger::error("{} needs two integer arguments", option);
        return false;
      }
      opts->boundaries.minX = atoi(NEXTARG);
      opts->boundaries.minZ = atoi(NEXTARG);
    } else if (strcmp(option, "-to") == 0) {
      if (!MOREARGS(2) || !isNumeric(POLLARG(1)) || !isNumeric(POLLARG(2))) {
        logger::error("{} needs two integer arguments", option);
        return false;
      }
      opts->boundaries.maxX = atoi(NEXTARG);
      opts->boundaries.maxZ = atoi(NEXTARG);
    } else if (strcmp(option, "-centre") == 0 ||
               strcmp(option, "-center") == 0) {
      if (!MOREARGS(2) || !isNumeric(POLLARG(1)) || !isNumeric(POLLARG(2))) {
        logger::error("{} needs two integer arguments", option);
        return false;
      }
      opts->boundaries.cenX = atoi(NEXTARG);
      opts->boundaries.cenZ = atoi(NEXTARG);
    } else if (strcmp(option, "-radius") == 0) {
      if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
        logger::error("{} needs an integer argument", option);
        return false;
      }
      opts->boundaries.radius = atoi(NEXTARG);
    } else if (strcmp(option, "-max") == 0) {
      if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
        logger::error("{} needs an integer argument", option);
        return false;
      }
      const int height = atoi(NEXTARG);
      opts->boundaries.maxY =
          std::min(height, static_cast<int>(mcmap::constants::max_y));
    } else if (strcmp(option, "-min") == 0) {
      if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
        logger::error("{} needs an integer argument", option);
        return false;
      }
      const int height = atoi(NEXTARG);
      opts->boundaries.minY =
          std::max(height, static_cast<int>(mcmap::constants::min_y));
    } else if (strcmp(option, "-padding") == 0) {
      if (!MOREARGS(1) || !isNumeric(POLLARG(1)) || atoi(POLLARG(1)) < 0) {
        logger::error("{} needs an positive integer argument", option);
        return false;
      }
      opts->padding = atoi(NEXTARG);
    } else if (strcmp(option, "-nowater") == 0) {
      opts->hideWater = true;
    } else if (strcmp(option, "-nobeacons") == 0) {
      opts->hideBeacons = true;
    } else if (strcmp(option, "-shading") == 0) {
      opts->shading = true;
    } else if (strcmp(option, "-lighting") == 0) {
      opts->lighting = true;
    } else if (strcmp(option, "-nether") == 0) {
      opts->dim = Dimension("the_nether");
    } else if (strcmp(option, "-end") == 0) {
      opts->dim = Dimension("the_end");
    } else if (strcmp(option, "-dimension") == 0 ||
               strcmp(option, "-dim") == 0) {
      if (!MOREARGS(1)) {
        logger::error("{} needs a dimension name or number", option);
        return false;
      }
      opts->dim = Dimension(NEXTARG);
    } else if (strcmp(option, "-file") == 0) {
      if (!MOREARGS(1)) {
        logger::error("{} needs one argument", option);
        return false;
      }
      opts->outFile = NEXTARG;
    } else if (strcmp(option, "-colors") == 0) {
      if (!MOREARGS(1)) {
        logger::error("{} needs one argument", option);
        return false;
      }
      opts->colorFile = NEXTARG;
      if (!ISPATH(opts->colorFile)) {
        logger::error("File {} does not exist", opts->colorFile.string());
        return false;
      }
    } else if (strcmp(option, "-dumpcolors") == 0) {
      opts->mode = Settings::DUMPCOLORS;
    } else if (strcmp(option, "-marker") == 0) {
      if (!MOREARGS(3) || !(isNumeric(POLLARG(1)) && isNumeric(POLLARG(2)))) {
        logger::error("{} needs three arguments: x z color", option);
        return false;
      }
      int x = atoi(NEXTARG), z = atoi(NEXTARG);
      opts->markers[opts->totalMarkers++] =
          Colors::Marker(x, z, std::string(NEXTARG));
    } else if (strcmp(option, "-nw") == 0) {
      opts->boundaries.orientation = Map::NW;
    } else if (strcmp(option, "-sw") == 0) {
      opts->boundaries.orientation = Map::SW;
    } else if (strcmp(option, "-ne") == 0) {
      opts->boundaries.orientation = Map::NE;
    } else if (strcmp(option, "-se") == 0) {
      opts->boundaries.orientation = Map::SE;
    } else if (strcmp(option, "-mb") == 0) {
      if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
        logger::error("{} needs an integer", option);
        return false;
      }
      opts->mem_limit = atoi(NEXTARG) * size_t(1024 * 1024);
    } else if (strcmp(option, "-tile") == 0) {
      if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
        logger::error("{} needs an integer", option);
        return false;
      }
      opts->tile_size = atoi(NEXTARG);
    } else if (strcmp(option, "-fragment") == 0) {
      if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
        logger::error("{} needs an integer", option);
        return false;
      }
      opts->fragment_size = atoi(NEXTARG);
    } else if (strcmp(option, "-help") == 0 || strcmp(option, "-h") == 0) {
      opts->mode = Settings::HELP;
      return false;
    } else if (strcmp(option, "-verbose") == 0 || strcmp(option, "-v") == 0) {
      // logger::level = logger::levels::DEBUG;
    } else if (strcmp(option, "-vv") == 0) {
      // logger::level = logger::levels::DEEP_DEBUG;
    } else {
      opts->save = SaveFile(option);
    }
  }

  if (opts->boundaries.circleDefined()) {
    // Generate the min/max coordinates based on our centre and the radius.
    // Add a little padding for good luck.
    int paddedRadius = 1.2 * opts->boundaries.radius;

    opts->boundaries.minX = opts->boundaries.cenX - paddedRadius;
    opts->boundaries.maxX = opts->boundaries.cenX + paddedRadius;
    opts->boundaries.minZ = opts->boundaries.cenZ - paddedRadius;
    opts->boundaries.maxZ = opts->boundaries.cenZ + paddedRadius;

    // We use the squared radius many times later; calculate it once here.
    opts->boundaries.rsqrd = opts->boundaries.radius * opts->boundaries.radius;
  }

  if (opts->mode == Settings::RENDER) {
    // Check if the given save posesses the required dimension, must be done now
    // as the world path can be given after the dimension name, which messes up
    // regionDir()
    if (!opts->save.valid()) {
      logger::error("Given folder does not seem to be a save file");
      return false;
    }

    // Scan the region directory and map the existing terrain in this set of
    // coordinates
    World::Coordinates existingWorld = opts->save.getWorld(opts->dim);

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
      logger::debug("Processed boundaries: {}", opts->boundaries.to_string());
      logger::error("Nothing to render: -from X Z has to be <= -to X Z");
      return false;
    }

    if (opts->boundaries.maxX - opts->boundaries.minX < 0) {
      logger::error("Nothing to render: -min Y has to be < -max Y");
      return false;
    }

    if (opts->fragment_size < 16) {
      logger::error("Cannot render map fragments this small");
      return false;
    }

    if (opts->tile_size) {
      // In case tiling output has been queried
      // Forbid padding
      if (opts->padding != Settings::PADDING_DEFAULT && opts->padding) {
        logger::error("Cannot pad tiled output !");
        return false;
      }

      // Change output.png to output by default
      if (opts->outFile == Settings::OUTPUT_DEFAULT)
        opts->outFile = Settings::OUTPUT_TILED_DEFAULT;

      // Get absolute path towards file
      if (opts->outFile.is_relative())
        opts->outFile = fs::absolute(opts->outFile);

      std::error_code dir_creation_error;
      fs::create_directory(opts->outFile, dir_creation_error);
      if (dir_creation_error) {
        logger::error("Failed to create directory {}: {}",
                      opts->outFile.string().c_str(),
                      dir_creation_error.message());
        return false;
      }
    }
  }

  return true;
}
