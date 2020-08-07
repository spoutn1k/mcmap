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

  Coordinates map;          // The coordinates describing the 3D map
  uint32_t sizeX, sizeZ;    // The size of the 3D map
  uint8_t offsetX, offsetZ; // Offset of the first block in the first chunk

  uint32_t width, height; // Bitmap width and height
  uint16_t padding;       // Padding inside the image
  uint8_t heightOffset;   // Offset for block rendering

  uint8_t *bytesBuffer; // The buffer where pixels are written
  uint64_t size;        // The size of the buffer

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
                  const Colors::Palette &colors, const uint16_t padding = 0);

  ~IsometricCanvas() { delete[] bytesBuffer; }

  void setMarkers(uint8_t n, Colors::Marker (*array)[256]) {
    totalMarkers = n;
    markers = array;
  }

  // Cropping methods
  // Those getters return a value inferior to the actual underlying values
  // leaving out empty areas, to essentially 'crop' the canvas to fit perfectly
  // the image
  uint32_t getCroppedWidth() const;
  uint32_t getCroppedHeight() const;

  uint64_t getCroppedSize() const {
    return getCroppedWidth() * getCroppedHeight();
  }
  uint64_t getCroppedOffset() const;

  // Line indexes
  uint32_t firstLine() const;
  uint32_t lastLine() const;

  // Merging methods
  void merge(const IsometricCanvas &subCanvas);
  uint64_t calcAnchor(const IsometricCanvas &subCanvas);

  // Drawing methods
  // Helpers for position lookup
  void orientChunk(int32_t &x, int32_t &z);
  void orientSection(uint8_t &x, uint8_t &z);
  inline uint8_t *pixel(uint32_t x, uint32_t y) {
    return &bytesBuffer[(x + y * width) * BYTESPERPIXEL];
  }

  // Drawing entrypoints
  void renderTerrain(const Terrain::Data &);
  void renderChunk(const Terrain::Data &, const int64_t, const int64_t);
  void renderSection(const NBT &, const int64_t, const int64_t, const uint8_t,
                     sectionInterpreter);
  // Draw a block from virtual coords in the canvas
  void renderBlock(Colors::Block *, const uint32_t, const uint32_t,
                   const uint32_t, const NBT &metadata);

  // Empty section with only beams
  void renderBeamSection(const int64_t, const int64_t, const uint8_t);

  // This obscure typedef allows to create a member function pointer array
  // (ouch) to render different block types without a switch case
  typedef void (IsometricCanvas::*drawer)(const uint32_t, const uint32_t,
                                          const NBT &, const Colors::Block *);

  // The default block type, hardcoded
  void drawFull(const uint32_t, const uint32_t, const NBT &,
                const Colors::Block *);

  // The other block types are loaded at compile-time from the `blocktypes.def`
  // file, with some macro manipulation
#define DEFINETYPE(STRING, CALLBACK)                                           \
  void CALLBACK(const uint32_t, const uint32_t, const NBT &,                   \
                const Colors::Block *);
#include "./blocktypes.def"
#undef DEFINETYPE
};

#endif
