#include "get_structures.hpp"

namespace mcmap {
namespace versions {
namespace structures_versions {
namespace {
// Each structure start holds a list of pieces, each with a bounding box stored
// as [minX, minY, minZ, maxX, maxY, maxZ] in block coordinates
void read_starts(const nbt::NBT &starts, std::vector<BoundingBox> &boxes) {
  for (const auto &start : starts) {
    if (!start.is_compound() || !start.contains("Children"))
      continue;

    for (const auto &piece : start["Children"]) {
      if (!piece.is_compound() || !piece.contains("BB"))
        continue;

      const auto *bb = piece["BB"].get<const nbt::NBT::tag_int_array_t *>();
      if (bb && bb->size() == 6)
        boxes.push_back({(*bb)[0], (*bb)[1], (*bb)[2], (*bb)[3], (*bb)[4],
                         (*bb)[5]});
    }
  }
}

// References are lists of packed chunk positions pointing to the chunks
// holding the structure starts: x on the low 32 bits, z on the high 32 bits
void read_references(const nbt::NBT &references,
                     std::vector<Coordinates> &positions) {
  for (const auto &structure : references) {
    const auto *packed =
        structure.get<const nbt::NBT::tag_long_array_t *>();
    if (!packed)
      continue;

    for (const int64_t position : *packed)
      positions.push_back({int32_t(position & 0xffffffff),
                           int32_t(uint64_t(position) >> 32)});
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
