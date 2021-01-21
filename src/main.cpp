#include "./VERSION"
#include "./canvas.h"
#include "./helper.h"
#include "./settings.h"
#include "./worldloader.h"
#include <algorithm>
#include <limits>
#include <omp.h>
#include <string>
#include <utility>

using std::string;

SETUP_LOGGER

#define SUCCESS 0
#define ERROR 1
#define CACHE "cache"

#ifdef _OPENMP
#define THREADS omp_get_max_threads()
#else
#define THREADS 1
#endif

void printHelp(char *binary) {
  logger::info(
      "Usage: {} <options> WORLDPATH\n\n"
      "  -from X Z           coordinates of the block to start rendering at\n"
      "  -to X Z             coordinates of the block to stop rendering at\n"
      "  -min/max VAL        minimum/maximum Y index of blocks to render\n"
      "  -file NAME          output file; default is 'output.png'\n"
      "  -colors NAME        color file to use; default is 'colors.json'\n"
      "  -nw -ne -se -sw     the orientation of the map\n"
      "  -nether             render the nether\n"
      "  -end                render the end\n"
      "  -dim[ension] NAME   render a dimension by namespaced ID\n"
      "  -nowater            do not render water\n"
      "  -nobeacons          do not render beacon beams\n"
      "  -shading            toggle shading (brightens blocks depending on "
      "height)\n"
      "  -mb int (=3500)     use the specified amount of memory (in MB)\n"
      "  -tile int (=1024)   render terrain in tiles of the specified size\n"
      "  -marker X Z color   draw a marker at X Z of the desired color\n"
      "  -padding int (=5)   padding to use around the image\n"
      "  -h[elp]             display an option summary\n"
      "  -v[erbose]          toggle debug mode (-vv for more)\n"
      "  -dumpcolors         dump a json with all defined colors\n",
      binary);
}

int main(int argc, char **argv) {
  Settings::WorldOptions options;
  Colors::Palette colors;

  if (argc < 2 || !parseArgs(argc, argv, &options)) {
    printHelp(argv[0]);
    return ERROR;
  }

  // Load colors from the text segment
  Colors::load(&colors);

  // If requested, load colors from file
  if (!options.colorFile.empty())
    Colors::load(options.colorFile, &colors);

  if (options.mode == Settings::DUMPCOLORS) {
    logger::info("{}", json(colors).dump());
    return 0;
  } else {
    logger::info(VERSION " {}bit (" COMMENT ")\n",
                 8 * static_cast<int>(sizeof(size_t)));
  }

  // Get the relevant options from the options parsed
  Terrain::Coordinates coords = options.boundaries;
  const std::filesystem::path regionDir = options.regionDir();

  // Overwrite water if asked to
  // TODO expand this to other blocks
  if (options.hideWater)
    colors["minecraft:water"] = Colors::Block();
  if (options.hideBeacons)
    colors["mcmap:beacon_beam"] = Colors::Block();

  std::vector<Terrain::Coordinates> tiles;
  coords.tile(tiles, options.tile_size);

  std::vector<Canvas> fragments(tiles.size());

  // This value represents the amount of canvasses that can fit in memory at
  // once to avoid going over the limit of RAM
  Counter<size_t> capacity = memory_capacity(
      options.mem_limit, tiles[0].footprint(), tiles.size(), THREADS);

  if (!capacity)
    return ERROR;

  logger::debug("Memory capacity: {} tiles - {} tiles scheduled\n",
                size_t(capacity), tiles.size());

  // If caching is needed, ensure the cache directory is available
  if (capacity < tiles.size())
    if (!prepare_cache(CACHE))
      return ERROR;

#ifdef _OPENMP
#pragma omp parallel shared(fragments, capacity)
#endif
  {
#ifdef _OPENMP
#pragma omp for ordered schedule(dynamic)
#endif
    for (std::vector<Terrain::Coordinates>::size_type i = 0; i < tiles.size();
         i++) {
      IsometricCanvas canvas;
      canvas.setMap(tiles[i]);
      canvas.setColors(colors);

      // Load the minecraft terrain to render
      Terrain::Data world(tiles[i]);
      world.load(regionDir);

      // Cap the height to avoid having a ridiculous image height
      tiles[i].minY = std::max(tiles[i].minY, world.minHeight());
      tiles[i].maxY = std::min(tiles[i].maxY, world.maxHeight());

      // Draw the terrain fragment
      canvas.shading = options.shading;
      canvas.setMarkers(options.totalMarkers, &options.markers);
      canvas.renderTerrain(world);

      if (!canvas.empty()) {
        if (i >= capacity) {
          std::filesystem::path temporary =
              fmt::format("{}/{}.png", CACHE, canvas.map.to_string());
          canvas.save(temporary);

          fragments[i] = std::move(ImageCanvas(canvas.map, temporary));
        } else
          fragments[i] = std::move(canvas);
      } else {
        // If the canvas was empty, increase the capacity to reflect the free
        // space
        if (i < capacity)
          ++capacity;
      }
    }
  }

  CompositeCanvas merged(std::move(fragments));
  logger::debug("{}\n", merged.to_string());

  if (merged.save(options.outFile, options.padding))
    logger::info("Job complete.\n");

  return SUCCESS;
}
