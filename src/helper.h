#ifndef HELPER_H_
#define HELPER_H_

#include <algorithm>
#include <logger.hpp>
#include <stdint.h>
#include <string>

#define CHUNKSIZE 16
#define REGIONSIZE 32
#define MIN_TERRAIN_HEIGHT 0
#define MAX_TERRAIN_HEIGHT 255

#define REGION_HEADER_SIZE REGIONSIZE *REGIONSIZE * 4
#define DECOMPRESSED_BUFFER 1000 * 1024
#define COMPRESSED_BUFFER 100 * 1024

#define CHUNK(x) ((x) >> 4)
#define REGION(x) ((x) >> 5)

uint8_t clamp(int32_t val);
bool isNumeric(const char *str);

uint32_t _ntohl(uint8_t *val);

enum Orientation {
  NW = 0,
  SW,
  SE,
  NE,
};

// A simple coordinates structure
template <typename Integer,
          std::enable_if_t<std::is_integral<Integer>::value, int> = 0>
struct Coordinates {
  Integer minX, maxX, minZ, maxZ;
  uint8_t minY, maxY;
  Orientation orientation;

  Coordinates() {
    minX = maxX = minZ = maxZ = minY = maxY = 0;
    orientation = NW;
  }

  Coordinates(Integer init) : Coordinates() {
    minX = maxX = minZ = maxZ = init;
  }

  void setUndefined() {
    minX = minZ = std::numeric_limits<Integer>::max();
    maxX = maxZ = std::numeric_limits<Integer>::min();
    minY = std::numeric_limits<uint8_t>::max();
    maxY = std::numeric_limits<uint8_t>::min();
  }

  bool isUndefined() {
    return (minX == minZ && minX == std::numeric_limits<Integer>::max() &&
            maxX == maxZ && maxX == std::numeric_limits<Integer>::min());
  }

  void crop(const Coordinates &boundaries) {
    minX = std::max(minX, boundaries.minX);
    minZ = std::max(minZ, boundaries.minZ);
    maxX = std::min(maxX, boundaries.maxX);
    maxZ = std::min(maxZ, boundaries.maxZ);
  }

  std::string to_string() const {
    return fmt::format("x{} z{} y{} to x{} z{} y{}", minX, minZ, minY, maxX,
                       maxZ, maxY);
  }

  void rotate() {
    std::swap(minX, maxX);
    minX = -minX;
    maxX = -maxX;
    std::swap(minX, minZ);
    std::swap(maxX, maxZ);
  };

  Coordinates<Integer> orient(Orientation o) const {
    Coordinates<Integer> oriented = *this;

    for (int i = 0; i < (4 + o - orientation) % 4; i++)
      oriented.rotate();

    oriented.orientation = o;

    return oriented;
  };

  inline Integer sizeX() const { return maxX - minX + 1; }
  inline Integer sizeZ() const { return maxZ - minZ + 1; }
};

void splitCoords(const Coordinates<int32_t> &original,
                 Coordinates<int32_t> *subCoords, const uint16_t count);

#endif // HELPER_H_
