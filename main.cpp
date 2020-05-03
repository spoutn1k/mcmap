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
void render(const PNG::Image& image,
        const PNG::IsometricCanvas& canvas,
        const Terrain::OrientedMap& world);

void printHelp(char *binary) {
    printf("\nmcmap - an isometric minecraft map rendering tool.\n"
            "Version " VERSION " %dbit\n\n"
            "Usage: %s <options> WORLDPATH\n\n"
            "  -from X Z        coordinates of the block to start rendering at\n"
            "  -to X Z          coordinates of the block to stop rendering at\n"
            "  -min/max VAL     minimum/maximum Y index of blocks to render\n"
            "  -file NAME       output file to 'NAME'; default is output.png\n"
            "  -nw -ne -se -sw  the orientation of the map\n"
            "  -nowater         do not render water\n"
            "\n    WORLDPATH is the path of the desired Minecraft world.\n\n"
            "Examples:\n\n"
            "%s ~/.minecraft/saves/World1\n"
            "  - This would render your entire singleplayer world in slot 1\n"
            "%s -night -from -10 -10 -to 10 10 ~/.minecraft/saves/World1\n"
            "  - This would render the same world but at night, and only\n"
            "    from chunk (-10 -10) to chunk (10 10)\n"
            , 8*static_cast<int>(sizeof(size_t)), binary, binary, binary);
}

int main(int argc, char **argv) {
    Settings::WorldOptions options;
    Colors::Palette colors;

    printf("mcmap " VERSION " %dbit\n", 8*static_cast<int>(sizeof(size_t)));

    // Always same random seed, as this is only used for block noise,
    // which should give the same result for the same input every time
    srand(1337);

    if (argc < 2 || !parseArgs(argc, argv, &options)) {
        printHelp(argv[0]);
        return 1;
    }

    Terrain::Coordinates coords;

    coords.minX = options.fromX;
    coords.minZ = options.fromZ;
    coords.maxX = options.toX;
    coords.maxZ = options.toZ;

    std::filesystem::path saveFile(options.saveName);
    saveFile /= "region";

    // The minecraft terrain to render
    Terrain::OrientedMap world(coords, options.orientation);
    world.terrain.load(saveFile);

    if (!Colors::load(options.colorFile, world.terrain.cache, &colors))
        return 1;

    // Overwrite water if asked to
    // TODO expand this to other blocks
    if (options.hideWater)
        colors["minecraft:water"] = Colors::Block();

    PNG::IsometricCanvas canvas(coords, options);
    // Cap the height of the canvas to avoid having a ridiculous height
    canvas.maxY = std::min(canvas.maxY, world.terrain.maxHeight());
    PNG::Image image(options.outFile, canvas, colors);

    render(image, canvas, world);
    saveImage();

    printf("Job complete.\n");
    return 0;
}

void render(const PNG::Image& image,
        const PNG::IsometricCanvas& canvas,
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

    for (size_t x = 0; x < canvas.sizeX + 1; x++) {
        for (size_t z = 0; z < canvas.sizeZ + 1; z++) {
            const size_t bmpPosX = 2*(canvas.sizeZ - 1) + (x - z)*2
                + image.padding;

            // in some orientations, the axis are inverted in the world
            if (world.orientation == Terrain::NE
                    || world.orientation == Terrain::SW)
                std::swap(x, z);

            const int64_t worldX = world.bounds.minX + x*world.vectorX;
            const int64_t worldZ = world.bounds.minZ + z*world.vectorZ;

            // swap them back to avoid loop confusion
            if (world.orientation == Terrain::NE
                    || world.orientation == Terrain::SW)
                std::swap(x, z);

            const uint8_t maxHeight = world.terrain.maxHeight(worldX, worldZ);
            const uint8_t minHeight = world.terrain.minHeight(worldX, worldZ);

            for (uint8_t y = std::max(minHeight, canvas.minY);
                    y < std::min(maxHeight, canvas.maxY); y++) {
                const size_t bmpPosY = image.height - 2 + x + z
                    - canvas.sizeX - canvas.sizeZ - y*image.heightOffset
                    - image.padding;
                const string block = world.terrain.block(worldX, worldZ, y); 
                image.setPixel(bmpPosX, bmpPosY, block);
            }
        }
    }

    return;
}
