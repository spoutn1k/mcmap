#include "./VERSION"
#include "./draw_png.h"
#include "./helper.h"
#include "./settings.h"
#include "./worldloader.h"
#include <algorithm>
#include <string>
#include <utility>

using std::string;

void printHelp(char *binary);
void render(IsometricCanvas *canvas, const Terrain::Data &world);

void printHelp(char *binary) {
  printf("\nmcmap - an isometric minecraft map rendering tool.\n"
         "Version " VERSION " %dbit\n\n"
         "Usage: %s <options> WORLDPATH\n\n"
         "  -from X Z        coordinates of the block to start rendering at\n"
         "  -to X Z          coordinates of the block to stop rendering at\n"
         "  -min/max VAL     minimum/maximum Y index of blocks to render\n"
         "  -file NAME       output file; default is 'output.png'\n"
         "  -colors NAME     color file to use; default is 'colors.json'\n"
         "  -nw -ne -se -sw  the orientation of the map\n"
         "  -nether          render the nether\n"
         "  -end             render the end\n"
         "  -nowater         do not render water\n"
         "  -nobeacons       do not render beacon beams\n",
         8 * static_cast<int>(sizeof(size_t)), binary);
}

int main(int argc, char **argv) {
  Settings::WorldOptions options;
  Colors::Palette colors;

  printf("mcmap " VERSION " %dbit\n", 8 * static_cast<int>(sizeof(size_t)));

  // Always same random seed, as this is only used for block noise,
  // which should give the same result for the same input every time
  srand(1337);

  if (argc < 2 || !parseArgs(argc, argv, &options)) {
    printHelp(argv[0]);
    return 1;
  }

  // Get the relevant options from the options parsed
  Terrain::Coordinates coords = options.boundaries;
  const std::filesystem::path regionDir = options.regionDir();
  Colors::load(options.colorFile, &colors);

  // This is the canvas on which the final image will be rendered
  IsometricCanvas finalCanvas(coords, colors, options.padding);

  // Prepare the sub-regions to render
  // This could be bypassed when the program is run in single-threaded mode, but
  // it works just fine when run in single threaded, so why bother making huge
  // if-elses ?
  Terrain::Coordinates *subCoords = new Terrain::Coordinates[options.splits];
  splitCoords(coords, subCoords, options.splits);

#pragma omp parallel shared(finalCanvas)
  {
#pragma omp for ordered schedule(static)
    for (size_t i = 0; i < options.splits; i++) {
      // Load the minecraft terrain to render
      Terrain::Data world(subCoords[i]);
      world.load(regionDir);

      // Cap the height to avoid having a ridiculous image height
      subCoords[i].minY = std::max(subCoords[i].minY, world.minHeight());
      subCoords[i].maxY = std::min(subCoords[i].maxY, world.maxHeight());

      // Pre-cache the colors used in the part of the world loaded to squeeze a
      // few milliseconds of color lookup
      Colors::Palette localColors;
      Colors::filter(colors, world.cache, &localColors);

      // Overwrite water if asked to
      // TODO expand this to other blocks
      if (options.hideWater)
        localColors["minecraft:water"] = Colors::Block();
      if (options.hideBeacons)
        localColors["mcmap:beacon_beam"] = Colors::Block();

      // Draw the terrain fragment
      IsometricCanvas canvas(subCoords[i], localColors);
      canvas.drawTerrain(world);

#pragma omp ordered
      {
        // Merge the terrain fragment into the final canvas. The ordered
        // directive in the pragma is primordial, as the merging algorithm
        // cannot merge terrain when not in order.
        finalCanvas.merge(canvas);
      }
    }
  }

  delete[] subCoords;

  PNG::Image(options.outFile, &finalCanvas).save();
  printf("Job complete.\n");
  return 0;
}
