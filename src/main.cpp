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
#ifndef DISABLE_OMP
      "  -splits VAL         render with VAL threads\n"
#endif
      "  -marker X Z color   draw a marker at X Z of the desired color\n"
      "  -padding VAL        padding to use around the image (default 5)\n"
      "  -h[elp]             display an option summary\n"
      "  -v[erbose]          toggle debug mode\n"
      "  -dumpcolors         dump a json with all defined colors\n",
      binary);
}

int main(int argc, char **argv) {
  Settings::WorldOptions options;
  Colors::Palette colors;

  // Always same random seed, as this is only used for block noise,
  // which should give the same result for the same input every time
  srand(1337);

  if (argc < 2 || !parseArgs(argc, argv, &options)) {
    printHelp(argv[0]);
    return 1;
  }

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
  size_t capacity = memory_capacity(options.mem_limit, tiles[0].footprint(),
                                    tiles.size(), omp_get_max_threads());

#ifndef DISABLE_OMP
#pragma omp parallel shared(fragments, capacity)
#endif
  {
#ifndef DISABLE_OMP
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
        if (true || i >= capacity) {
          std::filesystem::path temporary =
              fmt::format("/tmp/{}.png", canvas.map.to_string());
          canvas.save(temporary, 0);

          fragments[i] = std::move(ImageCanvas(canvas.map, temporary));
        } else
          fragments[i] = std::move(canvas);
      } else {
        // If the canvas was empty, increase the capacity to reflect the free
        // space
        if (i < capacity && capacity != std::numeric_limits<size_t>::max())
          capacity++;
      }
    }
  }

  CompositeCanvas merged(std::move(fragments));
  logger::debug("{}\n{}/{} canvasses cached\n", merged.to_string(),
                tiles.size() - capacity, tiles.size());

  if (merged.save(options.outFile, options.padding))
    logger::info("Job complete.\n");

  return 0;
}
