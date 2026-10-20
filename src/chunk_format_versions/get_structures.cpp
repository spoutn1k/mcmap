#include "get_structures.hpp"

namespace mcmap {
namespace versions {
namespace structures_versions {
namespace {
void read_starts(const nbt::NBT &starts, std::vector<BoundingBox> &boxes) {
  for (const auto &start : starts) {
    const std::vector<BoundingBox> found = structure::pieces(start);
    boxes.insert(boxes.end(), found.begin(), found.end());
  }
}

void read_references(const nbt::NBT &references,
                     std::vector<Coordinates> &positions) {
  for (const auto &list : references) {
    const auto *packed = list.get<const nbt::NBT::tag_long_array_t *>();
    if (!packed)
      continue;

    for (const int64_t position : *packed)
      positions.push_back(structure::unpack(position));
  }
}

void read(const nbt::NBT &structures, const std::string &starts_key,
          std::vector<BoundingBox> &boxes,
          std::vector<Coordinates> &references) {
  if (structures.contains(std::string(starts_key)))
    read_starts(structures[starts_key], boxes);

  if (structures.contains("References"))
    read_references(structures["References"], references);
}
} // namespace

void v2844(const nbt::NBT &chunk, std::vector<BoundingBox> &boxes,
           std::vector<Coordinates> &references) {
  if (chunk.contains("structures"))
    read(chunk["structures"], "starts", boxes, references);
}

void v1628(const nbt::NBT &chunk, std::vector<BoundingBox> &boxes,
           std::vector<Coordinates> &references) {
  if (chunk.contains("Level") && chunk["Level"].contains("Structures"))
    read(chunk["Level"]["Structures"], "Starts", boxes, references);
}

void catchall(const nbt::NBT &chunk, std::vector<BoundingBox> &,
              std::vector<Coordinates> &) {
  logger::trace("Unsupported DataVersion: {}", chunk["DataVersion"].get<int>());
}
} // namespace structures_versions
} // namespace versions
} // namespace mcmap
