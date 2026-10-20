#include "./structure.h"

namespace mcmap {
namespace structure {

int64_t pack(const Coordinates &pos) {
  return int64_t(uint32_t(pos.x)) | (int64_t(pos.z) << 32);
}

Coordinates unpack(int64_t packed) {
  return {int32_t(packed & 0xffffffff), int32_t(uint64_t(packed) >> 32)};
}

nbt::NBT *compound(nbt::NBT &chunk) {
  if (chunk.contains("structures"))
    return &chunk["structures"];

  if (chunk.contains("Level") && chunk["Level"].contains("Structures"))
    return &chunk["Level"]["Structures"];

  return nullptr;
}

std::string starts_key(const nbt::NBT &compound) {
  return compound.contains("Starts") ? "Starts" : "starts";
}

std::vector<BoundingBox> pieces(const nbt::NBT &start) {
  std::vector<BoundingBox> boxes;

  if (!start.is_compound() || !start.contains("Children"))
    return boxes;

  for (const auto &piece : start["Children"]) {
    if (!piece.is_compound() || !piece.contains("BB"))
      continue;

    const auto *bb = piece["BB"].get<const nbt::NBT::tag_int_array_t *>();
    if (bb && bb->size() == 6)
      boxes.push_back(
          {(*bb)[0], (*bb)[1], (*bb)[2], (*bb)[3], (*bb)[4], (*bb)[5]});
  }

  return boxes;
}

} // namespace structure
} // namespace mcmap
