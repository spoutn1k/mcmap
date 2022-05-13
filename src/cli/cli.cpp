#include "../mcmap.h"
#include "./parse.h"

#define SUCCESS 0
#define ERROR 1

#ifdef SNAPSHOT_SUPPORT
#define SNAPSHOT " Snapshots enabled "
#else
#define SNAPSHOT ""
#endif

void printHelp(char *binary) {
  fmt::print(
      "Usage: {} <options> WORLDPATH\n\n"
      "  -from X Z             coordinates of the block to start rendering at\n"
      "  -to X Z               coordinates of the block to stop rendering at\n"
      "  -min/max VAL          minimum/maximum Y index of blocks to render\n"
      "  -file NAME            output file; default is 'output.png'\n"
      "  -colors NAME          color file to use; default is 'colors.json'\n"
      "  -nw -ne -se -sw       the orientation of the map\n"
      "  -nether               render the nether\n"
      "  -end                  render the end\n"
      "  -dim[ension] NAME     render a dimension by namespaced ID\n"
      "  -nowater              do not render water\n"
      "  -nobeacons            do not render beacon beams\n"
      "  -shading              toggle shading (brightens depending on height)\n"
      "  -lighting             toggle lighting (brightens depending on light)\n"
      "  -mb int (=3500)       use the specified amount of memory (in MB)\n"
      "  -fragment int (=1024) render terrain in tiles of the specified size\n"
      "  -tile int (=0)        if not 0, will create split output of the "
      "desired tile size\n"
      "  -marker X Z color     draw a marker at X Z of the desired color\n"
      "  -padding int (=5)     padding to use around the image\n"
      "  -h[elp]               display an option summary\n"
      "  -v[erbose]            toggle debug mode (-vv for more)\n"
      "  -dumpcolors           dump a json with all defined colors\n"
      "  -radius VAL           radius of the circular render\n"
      "  -centre|-center X Z   coordinates of the centre of circular render\n",
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
    Colors::load(&colors, options.colorFile);

  if (options.mode == Settings::DUMPCOLORS) {
    fmt::print("{}", json(colors).dump());
    return 0;
  } else {
    fmt::print("{}\n", mcmap::version());
  }

  // Overwrite water if asked to
  // TODO expand this to other blocks
  if (options.hideWater)
    colors["minecraft:water"] = Colors::Block();
  if (options.hideBeacons)
    colors["mcmap:beacon_beam"] = Colors::Block();

  if (!mcmap::render(options, colors, Progress::Status::ascii)) {
    logger::error("Error rendering terrain.");
    return ERROR;
  }

  fmt::print("Job complete.\n");
  return SUCCESS;
}
