#include <utility>
#include <string>
#include <algorithm>
#include "./draw_png.h"
#include "./settings.h"
#include "./worldloader.h"
#include "./globals.h"
#include "./helper.h"

using std::string;

void printHelp(char *binary);
void render(const Terrain::Data& terrain,
        const PNG::Image& image,
        const IsometricCanvas& canvas,
        const Terrain::OrientedMap& world);


void _calcSplits(const Terrain::Coordinates& map,
        const Settings::WorldOptions& opts,
        Settings::ImageOptions* img_opts) {
    // Mem check
    uint64_t bitmapBytes = _calcImageSize(map, img_opts);
    if (opts.memlimit < bitmapBytes) {
        fprintf(stderr, "Not enough memory for image\n");
    }
}

void printHelp(char *binary) {
    printf("\nmcmap - an isometric minecraft map rendering tool.\n"
            "Version " VERSION " %dbit\n\n"
            "Usage: %s <options> WORLDPATH\n\n"
            "  -from X Z     coordinates of the block to start rendering at\n"
            "  -to X Z       coordinates of the block to stop rendering at\n"
            "  -min/max VAL  minimum/maximum Y index of blocks to render\n"
            "  -file NAME    output file to 'NAME'; default is output.png\n"
            "\n    WORLDPATH is the path of the desired Minecraft world.\n\n"
            "Examples:\n\n"
            "%s ~/.minecraft/saves/World1\n"
            "  - This would render your entire singleplayer world in slot 1\n"
            "%s -night -from -10 -10 -to 10 10 ~/.minecraft/saves/World1\n"
            "  - This would render the same world but at night, and only\n"
            "    from chunk (-10 -10) to chunk (10 10)\n"
            , 8*static_cast<int>(sizeof(size_t)), binary, binary, binary);
}

bool parseArgs(int argc, char** argv, Settings::WorldOptions* opts) {
#define MOREARGS(x) (argpos + (x) < argc)
#define NEXTARG argv[++argpos]
#define POLLARG(x) argv[argpos + (x)]
    int argpos = 0;
    while (MOREARGS(1)) {
        const char *option = NEXTARG;
        if (strcmp(option, "-from") == 0) {
            if (!MOREARGS(2)
                    || !isNumeric(POLLARG(1))
                    || !isNumeric(POLLARG(2))) {
                printf("Error: %s needs two integer arguments\n", option);
                return false;
            }
            opts->fromX = atoi(NEXTARG);
            opts->fromZ = atoi(NEXTARG);
        } else if (strcmp(option, "-to") == 0) {
            if (!MOREARGS(2)
                    || !isNumeric(POLLARG(1))
                    || !isNumeric(POLLARG(2))) {
                printf("Error: %s needs two integer arguments\n", option);
                return false;
            }
            opts->toX = atoi(NEXTARG) + 1;
            opts->toZ = atoi(NEXTARG) + 1;
        } else if (strcmp(option, "-max") == 0) {
            if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
                printf("Error: %s needs an integer argument\n", option);
                return false;
            }
            opts->mapMaxY = atoi(NEXTARG);
        } else if (strcmp(option, "-min") == 0) {
            if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
                printf("Error: %s needs an integer argument\n", option);
                return false;
            }
            opts->mapMinY = atoi(NEXTARG);
        } else if (strcmp(option, "-file") == 0) {
            if (!MOREARGS(1)) {
                printf("Error: %s needs one argument\n", option);
                return false;
            }
            opts->outFile = NEXTARG;
        } else if (strcmp(option, "-nw") == 0) {
            opts->orientation = Terrain::NW;
        } else if (strcmp(option, "-sw") == 0) {
            opts->orientation = Terrain::SW;
        } else if (strcmp(option, "-ne") == 0) {
            opts->orientation = Terrain::NE;
        } else if (strcmp(option, "-se") == 0) {
            opts->orientation = Terrain::SE;
        } else if (strcmp(option, "-3") == 0) {
            opts->offsetY = 3;
        } else if (strcmp(option, "-help") == 0 || strcmp(option, "-h") == 0) {
            return false;
        } else {
            opts->saveName = std::filesystem::path(option);
        }
    }

    opts->wholeworld = (opts->fromX == UNDEFINED || opts->toX == UNDEFINED);

    if (opts->saveName.empty() || !std::filesystem::exists(opts->saveName)) {
        printf("Error: No world given.\n");
        return false;
    }

    if (opts->toX <= opts->fromX || opts->toZ <= opts->fromZ) {
        printf("Nothing to render: -from X Z has to be <= -to X Z\n");
        return false;
    }

    if (opts->mapMaxY - opts->mapMinY < 1) {
        printf("Nothing to render: -min Y has to be < -max/-height Y\n");
        return false;
    }

    return true;
}

int main(int argc, char **argv) {
    Settings::WorldOptions opts;
    colorMap colors;

    printf("mcmap " VERSION " %dbit\n", 8*static_cast<int>(sizeof(size_t)));

    // Always same random seed, as this is only used for block noise,
    // which should give the same result for the same input every time
    srand(1337);

    if (argc < 2 || !parseArgs(argc, argv, &opts)) {
        printHelp(argv[0]);
        return 1;
    }

    if (!loadColors(colors)) {
        fprintf(stderr, "Could not load colors.\n");
        return 1;
    }

    if (opts.outFile.empty())
        opts.outFile = "output.png";

    Terrain::Coordinates coords;

    coords.minX = opts.fromX;
    coords.minZ = opts.fromZ;
    coords.maxX = opts.toX;
    coords.maxZ = opts.toZ;

    IsometricCanvas canvas(coords, opts);
    PNG::Image image(opts.outFile, canvas);

    std::filesystem::path saveFile(opts.saveName);
    saveFile /= "region";

    // The minecraft terrain to render
    Terrain::Data terrain(coords);
    _loadTerrain(terrain, saveFile);

    Terrain::OrientedMap world(coords, opts.orientation);

    render(terrain, image, canvas, world);
    saveImage();

    printf("Job complete.\n");
    return 0;
}

void render(const Terrain::Data& terrain,
        const PNG::Image& image,
        const IsometricCanvas& canvas,
        const Terrain::OrientedMap& world) {
    /* There are 3 sets of coordinates here:
     * - x, y, z: the coordinates of the dot on the virtual isometric map
     *   to be drawn, here named canvas;
     * - mapx, y, mapz: the coordinates of the corresponding block in the
     *   minecraft world, depending on the orientation of the map to be drawn;
     * - bitmapX, bitmapY: the position of the pixel in the resulting bitmap.
     *
     * The virtual map "canvas" is the link between the two other sets of
     * coordinates. Drawing the map MUST follow a special order to avoid
     * overwriting pixels when drawing: the horizontal order is as follows:
     *
     *   0
     *  3 1
     * 5 4 2
     *
     * The canvas allows to easily follow this pattern. The world block
     * and the position on the image are then calculated from the canvas
     * coordinates. */

    for (size_t x = 0; x < canvas.sizeX; x++) {
        for (size_t z = 0; z < canvas.sizeZ; z++) {
            const size_t bmpPosX = 2*(canvas.sizeZ - 1) + (x - z)*2
                + image.padding;

            // in some orientations, the axis are inverted in the world
            if (world.orientation == Terrain::NE
                    || world.orientation == Terrain::SW)
                std::swap(x, z);

            const int64_t worldX = world.coords.minX + x*world.vectorX;
            const int64_t worldZ = world.coords.minZ + z*world.vectorZ;

            // swap them back to avoid loop confusion
            if (world.orientation == Terrain::NE
                    || world.orientation == Terrain::SW)
                std::swap(x, z);

            const uint8_t maxHeight = heightAt(terrain, worldX, worldZ);

            for (uint8_t y = canvas.minY;
                    y < std::min(maxHeight, canvas.maxY); y++) {
                const size_t bmpPosY = image.height - 2 + x + z
                    - canvas.sizeX - canvas.sizeZ - y*image.heightOffset
                    - image.padding;
                Block block = Terrain::blockAt(terrain, worldX, worldZ, y);
                setPixel(bmpPosX, bmpPosY, block, 0);
            }
        }
    }

    return;
}
