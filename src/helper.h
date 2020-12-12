#ifndef HELPER_H_
#define HELPER_H_

#include <algorithm>
#include <filesystem>
#include <logger.hpp>
#include <stdint.h>
#include <string>

#define CHUNKSIZE 16
#define REGIONSIZE 32
#define MIN_TERRAIN_HEIGHT 0
#define MAX_TERRAIN_HEIGHT 255

#define REGION_HEADER_SIZE REGIONSIZE *REGIONSIZE * 4
#define DECOMPRESSED_BUFFER 1000 * 1024
#define COMPRESSED_BUFFER 500 * 1024

#define CHUNK(x) ((x) >> 4)
#define REGION(x) ((x) >> 5)

uint8_t clamp(int32_t val);
bool isNumeric(const char *str);

uint32_t _ntohl(uint8_t *val);

size_t memory_capacity(size_t, size_t, size_t, size_t);
bool prepare_cache(const std::filesystem::path &);

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
    return fmt::format("{}.{}.{} ~> {}.{}.{}", minX, minZ, minY, maxX, maxZ,
                       maxY);
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

  size_t footprint() const {
    Integer width = (sizeX() + sizeZ()) * 2;
    Integer height = sizeX() + sizeZ() + (maxY - minY + 1) * 3 - 1;

#define BYTESPERPIXEL 4
    return width * height * BYTESPERPIXEL;
#undef BYTESPERPIXEL
  }

  void tile(std::vector<Coordinates<Integer>> &fragments, size_t size) {
    for (Integer x = minX; x <= maxX; x += size) {
      for (Integer z = minZ; z <= maxZ; z += size) {
        Coordinates<Integer> fragment = *this;

        fragment.minX = x;
        fragment.maxX = std::min(Integer(x + size - 1), maxX);

        fragment.minZ = z;
        fragment.maxZ = std::min(Integer(z + size - 1), maxZ);

        fragments.push_back(fragment);
      }
    }
  }

  // The following methods are used to get the position of the map in an image
  // made by another (englobing) map, called the referential

  inline Integer offsetX(const Coordinates<Integer> &referential) const {
    // This formula is thought around the top corner' position.
    //
    // The top corner's postition of the sub-map is influenced by its distance
    // to the full map's top corner => we compare the minX and minZ coordinates
    //
    // From there, the map's top corner is sizeZ pizels from the edge, and the
    // sub-canvasses' edge is at sizeZ' pixels from its top corner.
    //
    // By adding up those elements we get the delta between the edge of the full
    // image and the edge of the partial image.
    Coordinates<Integer> oriented = this->orient(Orientation::NW);

    return 2 * (referential.sizeZ() - oriented.sizeZ() -
                (referential.minX - oriented.minX) +
                (referential.minZ - oriented.minZ));
  }

  inline Integer offsetY(const Coordinates<Integer> &referential) const {
    // This one is simpler, the vertical distance being equal to the distance
    // between top corners.
    Coordinates<Integer> oriented = this->orient(Orientation::NW);

    return oriented.minX - referential.minX + oriented.minZ - referential.minZ;
  }

  Coordinates<Integer> &operator+=(const Coordinates<Integer> &other) {
    minX = std::min(other.minX, minX);
    minZ = std::min(other.minZ, minZ);
    maxX = std::max(other.maxX, maxX);
    maxZ = std::max(other.maxZ, maxZ);
    minY = std::min(other.minY, minY);
    maxY = std::max(other.maxY, maxY);

    return *this;
  }
};

template <typename UInteger,
          std::enable_if_t<std::is_unsigned<UInteger>::value, int> = 0>
struct Counter {
  UInteger counter;

  Counter(UInteger value = 0) : counter(value) {}

  Counter<UInteger> &operator++() {
    counter < std::numeric_limits<UInteger>::max() ? ++counter : counter;
    return *this;
  }

  operator UInteger() { return counter; }
};

#endif // HELPER_H_
