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
  Coordinates map;     // The coordinates describing the 3D map
  size_t sizeX, sizeZ; // The size of the 3D map

  size_t width, height; // Bitmap width and height
  size_t padding;       // Padding inside the image
  uint8_t heightOffset; // Offset for block rendering

  uint8_t *bytesBuffer; // The buffer where pixels are written
  size_t size;          // The size of the buffer

  Colors::Palette palette; // The colors to use when drawing

  IsometricCanvas(const Terrain::Coordinates &coords,
                  const Colors::Palette &colors, const size_t padding = 0)
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
    this->padding = 2 + padding;

    sizeX = map.maxX - map.minX;
    sizeZ = map.maxZ - map.minZ;

    if (map.orientation == NE || map.orientation == SW)
      std::swap(sizeX, sizeZ);

    // The isometrical view of the terrain implies that the width of each chunk
    // equals 16 blocks per side. Each block is overlapped so is 2 pixels wide.
    // => A chunk's width equals its size on each side times 2.
    // By generalizing this formula, the entire map's size equals the sum of its
    // length on both the horizontal axis times 2.
    width = (sizeX + sizeZ + this->padding) * 2;

    height = sizeX + sizeZ + (map.maxY - map.minY) * heightOffset +
             this->padding * 2;

    size = uint64_t(width * BYTESPERPIXEL) * uint64_t(height);
    bytesBuffer = new uint8_t[size];
    memset(bytesBuffer, 0, size);

    palette = colors;
  }

  ~IsometricCanvas() { delete[] bytesBuffer; }

  // Cropping methods
  // Those getters return a value inferior to the actual underlying values
  // leaving out empty areas, to essentially 'crop' the canvas to fit perfectly
  // the image
  size_t getCroppedWidth() const;
  size_t getCroppedHeight() const;
  size_t getCroppedSize() const {
    return getCroppedWidth() * getCroppedHeight();
  }
  size_t getCroppedOffset() const;
  size_t firstLine() const;
  size_t lastLine() const;

  // Merging methods
  void merge(const IsometricCanvas &subCanvas);
  size_t calcAnchor(const IsometricCanvas &subCanvas);

  // Drawing methods
  // Helpers for position lookup
  void translate(size_t x, size_t z, int64_t *worldX, int64_t *worldZ);
  inline uint8_t *pixel(size_t x, size_t y) {
    return &bytesBuffer[(x + y * width) * BYTESPERPIXEL];
  }

  // Drawing entrypoints
  void drawTerrain(const Terrain::Data &);
  void drawBlock(const size_t, const size_t, const NBT &);
  inline void drawBlock(const size_t, const size_t, const size_t, const NBT &);

  // This obscure typedef allows to create a member function pointer array
  // (ouch) to render different block types without a switch case
  typedef void (IsometricCanvas::*drawer)(const size_t, const size_t,
                                          const NBT &, const Colors::Block *);

  // The default block type, hardcoded
  void drawFull(const size_t, const size_t, const NBT &, const Colors::Block *);

  // The other block types are loaded at compile-time from the `blocktypes.def`
  // file, with some macro manipulation
#define DEFINETYPE(STRING, CALLBACK)                                           \
  void CALLBACK(const size_t, const size_t, const NBT &, const Colors::Block *);
#include "./blocktypes.def"
#undef DEFINETYPE
};
