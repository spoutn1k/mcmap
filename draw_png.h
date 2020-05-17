#ifndef DRAW_PNG_H_
#define DRAW_PNG_H_

// Separate them in case I ever implement 16bit rendering
#define CHANSPERPIXEL 4
#define BYTESPERCHAN 1
#define BYTESPERPIXEL 4

#include "./colors.h"
#include "./helper.h"
#include "./settings.h"
#include "./worldloader.h"
#include "colors.h"
#include "globals.h"
#include "helper.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <list>
#include <png.h>
#include <utility>
#ifndef _WIN32
#include <sys/stat.h>
#endif
#if defined(_WIN32) && !defined(__GNUC__)
#include <direct.h>
#endif

void createImageBuffer(const size_t width, const size_t height,
                       const bool splitUp);
bool createImage(FILE *fh, const size_t width, const size_t height,
                 const bool splitUp);
bool saveImage();
int loadImagePart(const int startx, const int starty, const int width,
                  const int height);
// void setPixel(const size_t x, const size_t y, Block& color, const float
// fsub); void blendPixel(const size_t x, const size_t y, const Block color,
// const float fsub);
bool saveImagePart();
bool discardImagePart();
bool composeFinalImage();

namespace PNG {

struct IsometricCanvas {
  size_t sizeX, sizeZ;
  uint8_t minY, maxY;
  Terrain::Orientation orientation;

  IsometricCanvas(const Terrain::Coordinates &coords,
                  const Settings::WorldOptions &options) {
    orientation = options.orientation;

    sizeX = coords.maxX - coords.minX;
    sizeZ = coords.maxZ - coords.minZ;

    if (orientation == Terrain::NE || orientation == Terrain::SW)
      std::swap(sizeX, sizeZ);

    minY = options.mapMinY;
    maxY = options.mapMaxY;
  }
};

struct Image {
  size_t width, height; // Final png width and height
  uint8_t heightOffset; // Pixel offset for block rendering
  uint8_t padding;      // Padding inside the image

  FILE *imageHandle;

  Colors::Palette palette;

  uint8_t *bytesBuffer; // The buffer where pixels are written
  size_t size;          // The size of the buffer

  uint32_t lineWidthChans; // Width of a single png line

  png_structp pngPtr;
  png_infop pngInfoPtr;

  Image(const std::filesystem::path file, const IsometricCanvas &canvas,
        const Colors::Palette &colors) {
    heightOffset = 3;
    padding = 5;
    width = (canvas.sizeX + canvas.sizeZ + padding) * 2;
    height = canvas.sizeX + canvas.sizeZ +
             (canvas.maxY - canvas.minY) * heightOffset + padding * 2;

    imageHandle = nullptr;
    imageHandle = fopen(file.c_str(), "wb");

    if (imageHandle == nullptr) {
      throw(std::runtime_error("Error opening '" + file.string() +
                               "' for writing: " + string(strerror(errno))));
    }

    if (!create()) {
      throw(std::runtime_error("Error allocating bitmap: " +
                               string(strerror(errno))));
    }

    palette = colors;
  }

  ~Image() {
    if (imageHandle)
      fclose(imageHandle);
    delete[] bytesBuffer;
  }

  bool create();
  bool save();
  void drawBlock(const size_t x, const size_t y, const NBT &block);
  inline uint8_t *pixel(size_t x, size_t y) {
    return &bytesBuffer[x * CHANSPERPIXEL + y * lineWidthChans];
  }

  typedef void (PNG::Image::*drawer)(const size_t, const size_t, const NBT &,
                                     const Colors::Block *);

  void drawFull(const size_t x, const size_t y, const NBT &metadata,
                const Colors::Block *color);

#define DEFINETYPE(STRING, CALLBACK)                                           \
  void CALLBACK(const size_t x, const size_t y, const NBT &metadata,           \
                const Colors::Block *color);
#include "./blocktypes.def"
#undef DEFINETYPE
};

} // namespace PNG

#endif // DRAW_PNG_H_
