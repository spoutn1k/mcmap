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
#include <utility>

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
  // Final png width and height
  size_t width, height;

  // Pixel offset for block rendering
  uint8_t heightOffset;

  uint8_t padding;

  FILE *imageHandle;

  Colors::Palette palette;

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
  }

  bool create();
  bool save();
  void drawBlock(const size_t x, const size_t y, const NBT &block);
};

} // namespace PNG

#endif // DRAW_PNG_H_
