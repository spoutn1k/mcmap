#ifndef CANVAS_H_
#define CANVAS_H_

#include "./helper.h"
#include "./section.h"
#include "./worldloader.h"
#include <stdint.h>

#define CHANSPERPIXEL 4
#define BYTESPERCHAN 1
#define BYTESPERPIXEL 4

struct Beam {
  uint8_t position;
  const Colors::Block *color;

  Beam() : position(0), color(nullptr){};
  Beam(uint8_t x, uint8_t z, const Colors::Block *c)
      : position((x << 4) + z), color(c){};

  inline uint8_t x() const { return position >> 4; }
  inline uint8_t z() const { return position & 0x0f; }

  inline bool column(uint8_t x, uint8_t z) const {
    return position == ((x << 4) + z);
  }

  Beam &operator=(Beam &&other) {
    position = other.position;
    color = other.color;
    return *this;
  }
};

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

  uint32_t nXChunks, nZChunks;

  Colors::Palette palette;  // The colors to use when drawing
  Colors::Block air, water, // fire, earth. Teh four nations lived in harmoiny
      beaconBeam;           // Cached colors for easy access

  // TODO bye bye
  uint8_t totalMarkers = 0;
  Colors::Marker (*markers)[256];

  float *brightnessLookup;

  Section sections[16];

  // In-chunk variables
  uint32_t chunkX;
  uint32_t chunkZ;
  int8_t yPos, minSection, maxSection;

  // Beams in the chunk being rendered
  uint8_t beamNo = 0;
  Beam beams[256];

  uint8_t orientedX, orientedZ, y;

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
  void renderTerrain(Terrain::Data &);
  void renderChunk(Terrain::Data &);
  void renderSection();
  // Draw a block from virtual coords in the canvas
  void renderBlock(const Colors::Block *, const uint32_t, const uint32_t,
                   const uint32_t, const NBT &metadata);

  // Empty section with only beams
  void renderBeamSection(const int64_t, const int64_t, const uint8_t);

  const Colors::Block *nextBlock();
};

#endif
