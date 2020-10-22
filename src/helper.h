#ifndef HELPER_H_
#define HELPER_H_

#include "./logger.h"
#include <algorithm>
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
struct Coordinates {
  int32_t minX, maxX, minZ, maxZ;
  uint8_t minY, maxY;
  Orientation orientation;

  Coordinates() {
    minX = maxX = minZ = maxZ = minY = maxY = 0;
    orientation = NW;
  }

  Coordinates(int32_t init) : Coordinates() {
    minX = maxX = minZ = maxZ = init;
  }

  void setUndefined() {
    minX = minZ = std::numeric_limits<int32_t>::max();
    maxX = maxZ = std::numeric_limits<int32_t>::min();
  }

  bool isUndefined() {
    return (minX == minZ && minX == std::numeric_limits<int32_t>::max() &&
            maxX == maxZ && maxX == std::numeric_limits<int32_t>::min());
  }

  void crop(const Coordinates &boundaries) {
    minX = std::max(minX, boundaries.minX);
    minZ = std::max(minZ, boundaries.minZ);
    maxX = std::min(maxX, boundaries.maxX);
    maxZ = std::min(maxZ, boundaries.maxZ);
  }

  std::string to_string() const {
    return fmt::format("x{} z{} to x{} z{}", minX, minZ, maxX, maxZ);
  }
};

void splitCoords(const Coordinates &original, Coordinates *&subCoords,
                 const uint16_t count);

#endif // HELPER_H_
