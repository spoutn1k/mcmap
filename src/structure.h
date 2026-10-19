#pragma once

#include <cstdint>

namespace mcmap {

// Bounding box of a structure piece, in block coordinates, bounds included
struct BoundingBox {
  int32_t minX, minY, minZ, maxX, maxY, maxZ;

  inline bool contains(int32_t x, int32_t y, int32_t z) const {
    return minX <= x && x <= maxX && minY <= y && y <= maxY && minZ <= z &&
           z <= maxZ;
  }

  // Whether the box overlaps the column of the chunk at chunk coordinates
  // cX, cZ
  inline bool overlaps_chunk(int32_t cX, int32_t cZ) const {
    return (minX >> 4) <= cX && cX <= (maxX >> 4) && (minZ >> 4) <= cZ &&
           cZ <= (maxZ >> 4);
  }

  // Whether the box overlaps the blocks between minY and maxY
  inline bool overlaps_height(int32_t lower, int32_t upper) const {
    return minY <= upper && lower <= maxY;
  }
};

} // namespace mcmap
