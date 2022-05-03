#include "./assert.hpp"

namespace mcmap {
namespace versions {
namespace assert_versions {
bool v2844(const nbt::NBT &chunk) {
  // Snapshot 21w43a
  return chunk.contains("sections")  // No sections mean no blocks
         && chunk.contains("Status") // Ensure the status is `full`
         && chunk["Status"].get<nbt::NBT::tag_string_t>() == "full";
}

#ifdef SNAPSHOT_SUPPORT
bool v2840(const nbt::NBT &chunk) {
  // Snapshot 21w42a
  return chunk.contains("Level") &&           // Level data is required
         chunk["Level"].contains("Sections")  // No sections mean no blocks
         && chunk["Level"].contains("Status") // Ensure the status is `full`
         && chunk["Level"]["Status"].get<nbt::NBT::tag_string_t>() == "full";
}
#endif

bool v1976(const nbt::NBT &chunk) {
  // From 1.14 onwards
  return chunk.contains("Level")                // Level data is required
         && chunk["Level"].contains("Sections") // No sections mean no blocks
         && chunk["Level"].contains("Status")   // Ensure the status is `full`
         && chunk["Level"]["Status"].get<nbt::NBT::tag_string_t>() == "full";
}

bool v1628(const nbt::NBT &chunk) {
  // From 1.13 onwards
  return chunk.contains("Level")                // Level data is required
         && chunk["Level"].contains("Sections") // No sections mean no blocks
         && chunk["Level"].contains("Status")   // Ensure the status is `full`
         && chunk["Level"]["Status"].get<nbt::NBT::tag_string_t>() ==
                "postprocessed";
}

bool catchall(const nbt::NBT &chunk) {
  logger::trace("Unsupported DataVersion: {}", chunk["DataVersion"].get<int>());
  return false;
}
} // namespace assert_versions
} // namespace versions
} // namespace mcmap
