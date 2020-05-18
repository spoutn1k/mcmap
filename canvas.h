#include "./helper.h"
#include "./settings.h"
#include "./worldloader.h"
#include <stdint.h>

#define CHANSPERPIXEL 4
#define BYTESPERCHAN 1
#define BYTESPERPIXEL 4

// Isometric canvas
// This structure holds the final bitmap data, a 2D array of pixels. It is
// created with a set of 3D coordinates, and translate every block drawn into a
// 2D position.
struct IsometricCanvas {
  Coordinates map;

  size_t sizeX, sizeZ; // The size of the 3D map

  size_t width, height; // Bitmap width and height
  uint8_t heightOffset; // Offset for block rendering
  uint8_t padding;      // Padding inside the image

  uint8_t *bytesBuffer; // The buffer where pixels are written
  size_t size;          // The size of the buffer

  Colors::Palette palette;

  IsometricCanvas(const Terrain::Coordinates &coords,
                  const Colors::Palette &colors)
      : map(coords) {
    // This is a legacy setting, changing how the map is drawn. It can be 2 or
    // 3; it means that a block is drawn with a 2 or 3 pixel offset over the
    // block under it. This changes the orientation of the map: but it totally
    // changes the drawing of special blocks, and as no special cases can be
    // made easily, I set it to 3 for now.
    heightOffset = 3;

    // Minimal padding; as a block is drawn as a square of 4*4, and that on the
    // edge half of it is covered, the blocks on the edges stick out by 2
    // pixels on each side. We add a padding of 2 on the entire image to balance
    // that.
    padding = 2;

    sizeX = map.maxX - map.minX;
    sizeZ = map.maxZ - map.minZ;

    if (map.orientation == NE || map.orientation == SW)
      std::swap(sizeX, sizeZ);

    // The isometrical view of the terrain implies that the width of each chunk
    // equals 16 blocks per side. Each block is overlapped so is 2 pixels wide.
    // => A chunk's width equals its size on each side times 2.
    // By generalizing this formula, the entire map's size equals the sum of its
    // length on both the horizontal axis times 2.
    width = (sizeX + sizeZ + padding) * 2;

    height = sizeX + sizeZ + (map.maxY - map.minY) * heightOffset + padding * 2;

    size = uint64_t(width * BYTESPERPIXEL) * uint64_t(height);
    bytesBuffer = new uint8_t[size];
    memset(bytesBuffer, 0, size);

    palette = colors;
  }

  ~IsometricCanvas() { delete[] bytesBuffer; }

  void merge(const IsometricCanvas &subCanvas);

  void translate(size_t x, size_t z, int64_t *worldX, int64_t *worldZ) {
    switch (map.orientation) {
    case NW:
      *worldX = map.minX + x;
      *worldZ = map.minZ + z;
      break;
    case SW:
      std::swap(x, z);
      *worldX = map.minX + x;
      *worldZ = map.maxZ - z;
      break;
    case NE:
      std::swap(x, z);
      *worldX = map.maxX - x;
      *worldZ = map.minZ + z;
      break;
    case SE:
      *worldX = map.maxX - x;
      *worldZ = map.maxZ - z;
      break;
    }
  }

  inline uint8_t *pixel(size_t x, size_t y) {
    return &bytesBuffer[(x + y * width) * BYTESPERPIXEL];
  }

  void drawTerrain(const Terrain::Data &terrain);
  void drawBlock(const size_t x, const size_t y, const NBT &block);
  inline void drawBlock(const size_t x, const size_t z, const size_t y,
                        const NBT &block) {
    const size_t bmpPosX = 2 * (sizeZ - 1) + (x - z) * 2 + padding;
    const size_t bmpPosY = height - 2 + x + z - sizeX - sizeZ -
                           (y - map.minY) * heightOffset - padding;

    drawBlock(bmpPosX, bmpPosY, block);
  }

  typedef void (IsometricCanvas::*drawer)(const size_t, const size_t,
                                          const NBT &, const Colors::Block *);

  void drawFull(const size_t x, const size_t y, const NBT &metadata,
                const Colors::Block *color);

#define DEFINETYPE(STRING, CALLBACK)                                           \
  void CALLBACK(const size_t x, const size_t y, const NBT &metadata,           \
                const Colors::Block *color);
#include "./blocktypes.def"
#undef DEFINETYPE
};
