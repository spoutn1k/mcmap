#pragma once
#include <fmt/core.h>
#include <vector>

namespace Map {

enum Orientation {
  NW = 0,
  SW,
  SE,
  NE,
};

template <typename Integer,
          std::enable_if_t<std::is_integral<Integer>::value, int> = 0>
struct Coordinates {
  Integer minX, maxX, minZ, maxZ;
  Integer cenX, cenZ, radius, rsqrd;
  int16_t minY, maxY;
  Orientation orientation;

  Coordinates() {
    setUndefined();
    orientation = NW;
  }

  Coordinates(Integer _minX, int16_t _minY, Integer _minZ, Integer _maxX,
              int16_t _maxY, Integer _maxZ, Orientation o = NW)
      : minX(_minX), maxX(_maxX), minZ(_minZ), maxZ(_maxZ), minY(_minY),
        maxY(_maxY), orientation(o) {}

  void setUndefined() {
    minX = minZ = std::numeric_limits<Integer>::max();
    maxX = maxZ = std::numeric_limits<Integer>::min();
    minY = std::numeric_limits<int16_t>::max();
    maxY = std::numeric_limits<int16_t>::min();
    cenX = cenZ = radius = std::numeric_limits<Integer>::max();
  }

  bool isUndefined() const {
    return (minX == minZ && minX == std::numeric_limits<Integer>::max() &&
            maxX == maxZ && maxX == std::numeric_limits<Integer>::min());
  }

  // Only have a circle if we have all three parts.
  bool circleDefined() const {
    return (cenX != std::numeric_limits<Integer>::max() &&
            cenZ != std::numeric_limits<Integer>::max() &&
            radius != std::numeric_limits<Integer>::max());
  }

  void setMaximum() {
    setUndefined();
    std::swap(minX, maxX);
    std::swap(minZ, maxZ);
    std::swap(minY, maxY);
  }

  void crop(const Coordinates &boundaries) {
    minX = std::max(minX, boundaries.minX);
    minZ = std::max(minZ, boundaries.minZ);
    maxX = std::min(maxX, boundaries.maxX);
    maxZ = std::min(maxZ, boundaries.maxZ);
  }

  std::string to_string() const {
    std::string str_orient = "ERROR";
    switch (orientation) {
    case NW:
      str_orient = "North-West";
      break;
    case SW:
      str_orient = "South-West";
      break;
    case SE:
      str_orient = "South-East";
      break;
    case NE:
      str_orient = "North-East";
      break;
    }

#ifndef _WINDOWS
    const std::string format_str = "{}.{}.{} ~> {}.{}.{} ({})";
#else
    const std::string format_str = "{}.{}.{}.{}.{}.{}.{}";
#endif

    return fmt::format(format_str, minX, minZ, minY, maxX, maxZ, maxY,
                       str_orient);
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

  void tile(std::vector<Coordinates<Integer>> &fragments, size_t size) const {
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
    // to the full map's top corner => we compare the minX and minZ
    // coordinates
    //
    // From there, the map's top corner is sizeZ pizels from the edge, and the
    // sub-canvasses' edge is at sizeZ' pixels from its top corner.
    //
    // By adding up those elements we get the delta between the edge of the
    // full image and the edge of the partial image.
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

  template <typename Other,
            std::enable_if_t<std::is_integral<Other>::value, int> = 0>
  bool operator==(const Coordinates<Other> &other) const {
    return (minX == other.minX && minZ == other.minZ && maxX == other.maxX &&
            maxZ == other.maxZ && minY == other.minY && maxY == other.maxY &&
            orientation == other.orientation);
  }
};

} // namespace Map

namespace World {
using Coordinates = Map::Coordinates<int32_t>;
}
