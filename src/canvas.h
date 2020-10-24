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

  Terrain::Coordinates map; // The coordinates describing the 3D map
  uint32_t sizeX, sizeZ;    // The size of the 3D map
  uint8_t offsetX, offsetZ; // Offset of the first block in the first chunk

  uint32_t width, height; // Bitmap width and height
  uint8_t heightOffset;   // Offset of the first block in the first chunk

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

  IsometricCanvas();

  ~IsometricCanvas() { delete[] bytesBuffer; }

  void setColors(const Colors::Palette &);
  void setMap(const Terrain::Coordinates &);
  void setMarkers(uint8_t n, Colors::Marker (*array)[256]) {
    totalMarkers = n;
    markers = array;
  }

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
  size_t getLine(uint8_t *, size_t, uint64_t) const;
};

struct CompositeCanvas {
  // A sparse canvas made with smaller canvasses
  //
  // To render multiple canvasses made by threads, we compose an image from them
  // directly. This object allows to do so. It is given a list of canvasses, and
  // can be read as an image (made out of lines, with a height and width) that
  // is composed of the canvasses, without actually using any more memory.
  //
  // This is done by keeping track of the offset of each sub-canvas from the top
  // left of the image. When reading a line, it is composed of the lines of each
  // sub-canvas, with the appropriate offset.
  //
  // +-------------------+
  // |Composite Canvas   |
  // |+------------+     |
  // ||Canvas 1    |     |
  // ||    +------------+|
  // ||    |Canvas 2    ||
  // ||====|============|| < Read line
  // ||    |            ||
  // |+----|            ||
  // |     |            ||
  // |     +------------+|
  // +-------------------+

  uint64_t width, height;   // Properties of the final image
  Terrain::Coordinates map; // Virtual isometric map it draws

  struct Position {
    // Struct holding metadata about where the subCanvas is to be drawn.
    uint32_t offsetX, offsetY;        // Offsets to draw the image
    const IsometricCanvas *subCanvas; // Canvas to draw
  };

  std::vector<Position>
      subCanvasses; // Sorted list of Positions to draw the final image

  CompositeCanvas(const std::vector<IsometricCanvas> &);

  std::string to_string();

  size_t getLine(uint8_t *, size_t, uint64_t) const;
};

#endif
