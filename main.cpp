#include <sys/stat.h>
#include <filesystem>
#include <string>
#include <cstdio>
#include <cstdlib>
#include "./draw_png.h"
#include "./settings.h"
#include "./worldloader.h"
#include "./globals.h"

using std::string;

bool parseArgs(int, char**, struct cli_options& opts);
void printHelp(char *binary);
void render(Settings::WorldOptions&,
        Settings::ImageOptions&,
        Terrain::Coordinates&);

void _calcSplits(Terrain::Coordinates& map,
        Settings::WorldOptions& opts,
        Settings::ImageOptions& img_opts) {
    // Mem check
    uint64_t bitmapBytes = _calcImageSize(map, img_opts);
    if (opts.memlimit < bitmapBytes) {
        fprintf(stderr, "Not enough memory for image\n");
    }
}

void printHelp(char *binary) {
    printf( "\nmcmap - an isometric minecraft map rendering tool.\n"
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

bool parseArgs(int argc, char** argv, Settings::WorldOptions& opts) {
#define MOREARGS(x) (argpos + (x) < argc)
#define NEXTARG argv[++argpos]
#define POLLARG(x) argv[argpos + (x)]
    int argpos = 0;
    while (MOREARGS(1)) {
        const char *option = NEXTARG;
        if (strcmp(option, "-from") == 0) {
            if (!MOREARGS(2) || !isNumeric(POLLARG(1)) || !isNumeric(POLLARG(2))) {
                printf("Error: %s needs two integer arguments, ie: %s -10 5\n", option, option);
                return false;
            }
            opts.fromX = atoi(NEXTARG);
            opts.fromZ = atoi(NEXTARG);
        } else if (strcmp(option, "-to") == 0) {
            if (!MOREARGS(2) || !isNumeric(POLLARG(1)) || !isNumeric(POLLARG(2))) {
                printf("Error: %s needs two integer arguments, ie: %s -5 20\n", option, option);
                return false;
            }
            opts.toX = atoi(NEXTARG) + 1;
            opts.toZ = atoi(NEXTARG) + 1;
        } else if (strcmp(option, "-max") == 0) {
            if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
                printf("Error: %s needs an integer argument, ie: %s 100\n", option, option);
                return false;
            }
            opts.mapMaxY = atoi(NEXTARG);
        } else if (strcmp(option, "-min") == 0) {
            if (!MOREARGS(1) || !isNumeric(POLLARG(1))) {
                printf("Error: %s needs an integer argument, ie: %s 50\n", option, option);
                return false;
            }
            opts.mapMinY = atoi(NEXTARG);
        } else if (strcmp(option, "-mem") == 0) {
            if (!MOREARGS(1) || !isNumeric(POLLARG(1)) || atoi(POLLARG(1)) <= 0) {
                printf("Error: %s needs a positive integer argument, ie: %s 1000\n", option, option);
                return false;
            }
            opts.memlimitSet = true;
            opts.memlimit = size_t (atoi(NEXTARG)) * size_t (1024 * 1024);
        } else if (strcmp(option, "-file") == 0) {
            if (!MOREARGS(1)) {
                printf("Error: %s needs one argument, ie: %s myworld.bmp\n", option, option);
                return false;
            }
            opts.outFile = NEXTARG;
        } else if (strcmp(option, "-colors") == 0) {
            if (!MOREARGS(1)) {
                printf("Error: %s needs one argument, ie: %s colors.txt\n", option, option);
                return false;
            }
            opts.colorfile = NEXTARG;
        } else if (strcmp(option, "-nw") == 0) {
            opts.orientation = Terrain::NW;
        } else if (strcmp(option, "-sw") == 0) {
            opts.orientation = Terrain::SW;
        } else if (strcmp(option, "-ne") == 0) {
            opts.orientation = Terrain::NE;
        } else if (strcmp(option, "-se") == 0) {
            opts.orientation = Terrain::SE;
        } else if (strcmp(option, "-3") == 0) {
            opts.offsetY = 3;
        } else if (strcmp(option, "-help") == 0 || strcmp(option, "-h") == 0) {
            return false;
        } else {
            opts.saveName = (char *) option;
        }
    }

    opts.wholeworld = (opts.fromX == UNDEFINED || opts.toX == UNDEFINED);

    if (opts.saveName == NULL) {
        printf("Error: No world given. Please add the path to your world to the command line.\n");
        return false;
    }

    if (opts.toX <= opts.fromX || opts.toZ <= opts.fromZ) {
        printf("Nothing to render: -from X Z has to be <= -to X Z\n");
        return false;
    }

    if (opts.mapMaxY - opts.mapMinY < 1) {
        printf("Nothing to render: -min Y has to be < -max/-height Y\n");
        return false;
    }

    return true;
}

int main(int argc, char **argv) {
    Settings::WorldOptions opts;
    Settings::ImageOptions img_opts;
    colorMap colors;

    printf("mcmap " VERSION " %dbit\n", 8*static_cast<int>(sizeof(size_t)));

    if (argc < 2 || !parseArgs(argc, argv, opts)) {
        printHelp(argv[0]);
        return 1;
    }

    if (!loadColors(colors)) {
        fprintf(stderr, "Could not load colors.\n");
        return 1;
    }

    Terrain::Coordinates coords;

    coords.minX = opts.fromX;
    coords.minZ = opts.fromZ;
    coords.maxX = opts.toX - 1;
    coords.maxZ = opts.toZ - 1;

    if (sizeof(size_t) < 8 && opts.memlimit > 1800 * uint64_t(1024 * 1024)) {
        opts.memlimit = 1800 * uint64_t(1024 * 1024);
    }

    _calcSplits(coords, opts, img_opts);

    // Always same random seed, as this is only used for block noise, which should give the same result for the same input every time
    srand(1337);

    if (opts.outFile == NULL) {
        opts.outFile = (char *) "output.png";
    }

    // open output file only if not doing the tiled output
    FILE *fileHandle = NULL;
    fileHandle = fopen(opts.outFile, (img_opts.splitImage ? "w+b" : "wb"));

    if (fileHandle == NULL) {
        printf("Error opening '%s' for writing.\n", opts.outFile);
        return 1;
    }

    // This writes out the bitmap header and pre-allocates space if disk caching is used
    if (!createImage(fileHandle, img_opts.width, img_opts.height, img_opts.splitImage)) {
        printf("Error allocating bitmap. Check if you have enough free disk space.\n");
        return 1;
    }

    render(opts, img_opts, coords);
    saveImage();

    if (fileHandle)
        fclose(fileHandle);

    printf("Job complete.\n");
    return 0;
}

struct IsometricCanvas {
    size_t sizeX, sizeZ;
    uint8_t minY, maxY;
    Terrain::Orientation orientation;

    IsometricCanvas(Terrain::Coordinates& coords, Settings::WorldOptions& options) {
        orientation = options.orientation;

        sizeX = coords.maxX - coords.minX;
        sizeZ = coords.maxZ - coords.minZ;

        if (orientation == Terrain::NE || orientation == Terrain::SW)
            std::swap(sizeX, sizeZ);

        minY = options.mapMinY;
        maxY = options.mapMaxY;
    }
};

void render(Settings::WorldOptions& opts, Settings::ImageOptions& image, Terrain::Coordinates& coords) {
    Terrain::Data terrain(coords);
    Terrain::OrientedMap world(coords, opts.orientation);
    IsometricCanvas canvas(coords, opts);

    std::filesystem::path saveFile(opts.saveName);
    saveFile /= "region";

    _loadTerrain(terrain, saveFile);

    /* There are 3 sets of coordinates here:
     * - x, y, z: the coordinates of the dot on the virtual isometric map
     *   to be drawn, here named canvas;
     * - mapx, y, mapz: the coordinates of the corresponding block in the
     *   minecraft world, depending on the orientation of the map to be drawn;
     * - bitmapX, bitmapY: the position of the pixel in the resulting bitmap.
     *
     * The virtual map "canvas" is the link between the two other sets of
     * coordinates. Drawing the map MUST follow a special order to avoid
     * overwriting pixels when drawing: the order is as follows:
     *
     *   0
     *  3 1
     * 5 4 2
     *
     * The canvas allows to easily follow this pattern. The world block
     * and the position on the image are then calculated from the canvas
     * coordinates. */

    for (size_t x = 0; x < canvas.sizeX + 1; x++) {
        for (size_t z = 0; z < canvas.sizeZ + 1; z++) {
            const size_t bmpPosX = 2*canvas.sizeZ + (x - z)*2;

            // in some orientations, the axis are inverted in the world
            if (world.orientation == Terrain::NE || world.orientation == Terrain::SW)
                std::swap(x, z);

            const int64_t worldX = world.coords.minX + x*world.vectorX;
            const int64_t worldZ = world.coords.minZ + z*world.vectorZ;

            // swap them back to avoid loop confusion
            if (world.orientation == Terrain::NE || world.orientation == Terrain::SW)
                std::swap(x, z);

            const uint8_t maxHeight = heightAt(terrain, worldX, worldZ);

            for (uint8_t y = canvas.minY; y < std::min(maxHeight, canvas.maxY); y++) {
                const size_t bmpPosY = image.height - 4 - canvas.sizeX - canvas.sizeZ - y*opts.offsetY + x + z;
                Block block = Terrain::blockAt(terrain, worldX, worldZ, y);
                setPixel(bmpPosX, bmpPosY, block, 0);
            }
        }
    }

    return;
}
