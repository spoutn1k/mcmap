#include "get_section.hpp"

namespace mcmap {
namespace versions {
namespace sections_versions {
nbt::NBT v2844(const nbt::NBT &chunk) { return chunk["sections"]; }
nbt::NBT v1628(const nbt::NBT &chunk) { return chunk["Level"]["Sections"]; }
nbt::NBT catchall(const nbt::NBT &chunk) {
  logger::trace("Unsupported DataVersion: {}", chunk["DataVersion"].get<int>());
  return nbt::NBT(std::vector<nbt::NBT>());
}
} // namespace sections_versions
} // namespace versions
} // namespace mcmap
