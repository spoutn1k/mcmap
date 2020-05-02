#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <cstdint>
#include <filesystem>
#include "worldloader.h"
#define UNDEFINED 0x7FFFFFFF

namespace Settings {

struct WorldOptions {
    std::filesystem::path saveName, outFile, colorfile;

    bool wholeworld;

    // Map boundaries
    int fromX, fromZ, toX, toZ;
    int mapMinY, mapMaxY;
    int mapSizeY;
    Terrain::Orientation orientation;

    int offsetY;

    // Memory limits, legacy code for image splitting
    uint64_t memlimit;
    bool memlimitSet;

    WorldOptions() {
        saveName = "";
        outFile = "";
        colorfile = "";

        wholeworld = false;

        fromX = fromZ = toX = toZ = UNDEFINED;

        mapMinY = 0;
        mapMaxY = 255;
        mapSizeY = mapMaxY - mapMinY;
        offsetY = 3;

        memlimit = 2000 * uint64_t(1024 * 1024);
        memlimitSet = false;

        orientation = Terrain::Orientation::NW;
    }
};

struct ImageOptions {
    // Final png width and height
    int width, height;

    // Pixel offset for block rendering
    int heightOffset;

    // Legacy
    bool splitImage;
    int numSplitsX, numSplitsZ;
    int cropLeft, cropRight, cropTop, cropBottom;

    ImageOptions() {
        height = width = 0;

        heightOffset = 3;

        splitImage = false;
        numSplitsX = numSplitsZ = 0;
        cropLeft = cropRight = cropTop = cropBottom = 0;
    }
};

}  // namespace Settings

#endif  // OPTIONS_H_