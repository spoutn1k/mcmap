#ifndef CANVAS_H_
#define CANVAS_H_

#include "./helper.h"
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
  bool shading;

  Coordinates map;     // The coordinates describing the 3D map
  size_t sizeX, sizeZ; // The size of the 3D map

  size_t width, height; // Bitmap width and height
  uint16_t padding;     // Padding inside the image
  uint8_t heightOffset; // Offset for block rendering

  uint8_t *bytesBuffer; // The buffer where pixels are written
  size_t size;          // The size of the buffer

  uint64_t nXChunks, nZChunks;

  Colors::Palette palette;         // The colors to use when drawing
  Colors::Block water, beaconBeam; // Cached colors for easy access

  // Those arrays are chunk-based values, that get overwritten at every new
  // chunk
  uint8_t numBeacons = 0, beacons[256];
  uint8_t localMarkers = 0, totalMarkers = 0;
  // Markers inside the chunk:
  // 8 bits for the index inside markers, 4 bits for x, 4 bits for z
  uint16_t chunkMarkers[256];
  Colors::Marker (*markers)[256];

  float *brightnessLookup;

  IsometricCanvas(const Terrain::Coordinates &coords,
                  const Colors::Palette &colors, const uint16_t padding = 0)
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

    nXChunks = CHUNK(map.maxX) - CHUNK(map.minX) + 1;
    nZChunks = CHUNK(map.maxZ) - CHUNK(map.minZ) + 1;

    sizeX = nXChunks << 4;
    sizeZ = nZChunks << 4;

    if (map.orientation == NE || map.orientation == SW) {
      std::swap(nXChunks, nZChunks);
      std::swap(sizeX, sizeZ);
    }

    // The isometrical view of the terrain implies that the width of each chunk
    // equals 16 blocks per side. Each block is overlapped so is 2 pixels wide.
    // => A chunk's width equals its size on each side times 2.
    // By generalizing this formula, the entire map's size equals the sum of its
    // length on both the horizontal axis times 2.
    width = (sizeX + sizeZ + this->padding) * 2;

    height = sizeX + sizeZ + ((13 << 4) - map.minY) * heightOffset +
             this->padding * 2;

    size = uint64_t(width * BYTESPERPIXEL) * uint64_t(height);
    bytesBuffer = new uint8_t[size];
    memset(bytesBuffer, 0, size);

    // Setting and pre-caching colors
    palette = colors;

    auto beamColor = colors.find("mcmap:beacon_beam");
    if (beamColor != colors.end())
      beaconBeam = beamColor->second;

    auto waterColor = colors.find("minecraft:water");
    if (waterColor != colors.end())
      water = waterColor->second;

    // Set to true to use shading later on
    shading = false;
    // Precompute the shading profile. The values are arbitrary, and will go
    // through Colors::Color.modcolor further down the code. The 255 array
    // represents the entire world height. This profile is linear, going from
    // -100 at height 0 to 100 at height 255. This replaced a convoluted formula
    // that did a much better job of higlighting overground terrain, but would
    // look weird in other dimensions.
    // Legacy formula: ((100.0f / (1.0f + exp(- (1.3f * (float(y) *
    // MIN(g_MapsizeY, 200) / g_MapsizeY) / 16.0f) + 6.0f))) - 91)
    brightnessLookup = new float[255];
    for (int y = 0; y < 255; ++y)
      brightnessLookup[y] = -100 + 200 * float(y) / 255;
  }

  ~IsometricCanvas() { delete[] bytesBuffer; }

  void setMarkers(uint8_t n, Colors::Marker (*array)[256]) {
    totalMarkers = n;
    markers = array;
  }

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
  void drawChunk(const Terrain::Data &, const int64_t, const int64_t);
  void drawSection(const NBT &, const int64_t, const int64_t, const uint8_t,
                   sectionInterpreter);
  void drawChunk(const NBT &);
  void drawBeams(const int64_t, const int64_t, const uint8_t);
  void drawBlock(const size_t, const size_t, const NBT &);
  inline void drawBlock(const size_t, const size_t, const size_t, const NBT &);
  void drawBlock(Colors::Block *, const size_t, const size_t, const size_t,
                 const NBT &);

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

#endif
