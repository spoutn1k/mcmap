#pragma once

#include <2DCoordinates.hpp>
#include <cstdint>
#include <nbt/nbt.hpp>
#include <string>
#include <vector>

namespace mcmap {

// Bounding box of a structure piece, in block coordinates, bounds included
struct BoundingBox {
  int32_t minX, minY, minZ, maxX, maxY, maxZ;

  inline bool contains(int32_t x, int32_t y, int32_t z) const {
    return minX <= x && x <= maxX && minY <= y && y <= maxY && minZ <= z &&
           z <= maxZ;
  }

  // Whether the box overlaps the column of the chunk at chunk coordinates cX, cZ
  inline bool overlaps_chunk(int32_t cX, int32_t cZ) const {
    return (minX >> 4) <= cX && cX <= (maxX >> 4) && (minZ >> 4) <= cZ &&
           cZ <= (maxZ >> 4);
  }

  // Whether the box overlaps the blocks between minY and maxY
  inline bool overlaps_height(int32_t lower, int32_t upper) const {
    return minY <= upper && lower <= maxY;
  }
};

namespace structure {
// References are packed chunk positions pointing to the chunks holding the structure starts: x on the low 32 bits, z on the high 32 bits
int64_t pack(const Coordinates &);
Coordinates unpack(int64_t);

// Compound holding the structure starts and references of a raw chunk, or nullptr; it moved out of the `Level` compound in 1.18
nbt::NBT *compound(nbt::NBT &chunk);

// Key of the structure starts in the compound above, renamed in 1.18
std::string starts_key(const nbt::NBT &compound);

// Bounding boxes of the pieces of a structure start, stored as [minX, minY, minZ, maxX, maxY, maxZ]
std::vector<BoundingBox> pieces(const nbt::NBT &start);
} // namespace structure

} // namespace mcmap
