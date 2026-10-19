#include "../structure.h"
#include <2DCoordinates.hpp>
#include <nbt/nbt.hpp>

namespace mcmap {
namespace versions {
namespace structures_versions {
void v2844(const nbt::NBT &, std::vector<BoundingBox> &,
           std::vector<Coordinates> &);
void v1628(const nbt::NBT &, std::vector<BoundingBox> &,
           std::vector<Coordinates> &);
void catchall(const nbt::NBT &, std::vector<BoundingBox> &,
              std::vector<Coordinates> &);
} // namespace structures_versions
} // namespace versions
} // namespace mcmap
