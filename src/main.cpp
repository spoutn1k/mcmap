#include "./VERSION"
#include "./draw_png.h"
#include "./helper.h"
#include "./logger.h"
#include "./settings.h"
#include "./worldloader.h"
#include <algorithm>
#include <string>
#include <utility>

using std::string;

void printHelp(char *binary);
void render(IsometricCanvas *canvas, const Terrain::Data &world);

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

  // This is the canvas on which the final image will be rendered
  IsometricCanvas finalCanvas(coords, colors, options.padding);

  // Prepare the sub-regions to render
  // This could be bypassed when the program is run in single-threaded mode, but
  // it works just fine when run in single threaded, so why bother making huge
  // if-elses ?
  Terrain::Coordinates *subCoords = new Terrain::Coordinates[options.splits];
  splitCoords(coords, subCoords, options.splits);

#ifndef DISABLE_OMP
#pragma omp parallel shared(finalCanvas)
#endif
  {
#ifndef DISABLE_OMP
#pragma omp for ordered schedule(static)
#endif
    for (uint16_t i = 0; i < options.splits; i++) {
      // Load the minecraft terrain to render
      Terrain::Data world(subCoords[i]);
      world.load(regionDir);

      // Cap the height to avoid having a ridiculous image height
      subCoords[i].minY = std::max(subCoords[i].minY, world.minHeight());
      subCoords[i].maxY = std::min(subCoords[i].maxY, world.maxHeight());

      // Draw the terrain fragment
      IsometricCanvas canvas(subCoords[i], colors);
      canvas.shading = options.shading;
      canvas.setMarkers(options.totalMarkers, &options.markers);
      canvas.renderTerrain(world);

#ifndef DISABLE_OMP
#pragma omp ordered
#endif
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
  logger::info("Job complete.\n");

  return 0;
}
