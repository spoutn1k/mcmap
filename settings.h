#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <cstdint>
#define UNDEFINED 0x7FFFFFFF

namespace Settings {

struct WorldOptions {
    char *saveName, *outFile, *colorfile;
    // char *texturefile = nullptr;
    // char *biomepath = nullptr;

    bool wholeworld;
    // bool dumpColors = false;
    // bool infoOnly = false;

    // Map boundaries
    int fromX, fromZ, toX, toZ;
    int mapMinY, mapMaxY;
    int mapSizeY;

    int offsetY;

    // Memory limits, legacy code for image splitting
    uint64_t memlimit;
    bool memlimitSet;

    WorldOptions() {
        saveName = nullptr;
        outFile = nullptr;
        colorfile = nullptr;

        wholeworld = false;

        fromX = fromZ = toX = toZ = UNDEFINED;

        mapMinY = 0;
        mapMaxY = 256;
        mapSizeY = mapMaxY - mapMinY;
        offsetY = 3;

        memlimit = 2000 * uint64_t(1024 * 1024);
        memlimitSet = false;
    }
};

struct ImageOptions {
    // Final png width and height
    int bitmapX, bitmapY;

    // Pixel offset for block rendering
    int heightOffset;

    // Legacy
    bool splitImage;
    int numSplitsX, numSplitsZ;
    int cropLeft, cropRight, cropTop, cropBottom;

    ImageOptions() {
        bitmapX = bitmapY = 0;

        heightOffset = 3;

        splitImage = false;
        numSplitsX = numSplitsZ = 0;
        cropLeft = cropRight = cropTop = cropBottom = 0;
    }
};

}  // namespace Settings

#endif  // OPTIONS_H_
