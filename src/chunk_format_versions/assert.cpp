#include "./assert.hpp"

namespace mcmap {
namespace versions {
namespace assert_versions {
namespace {
// `level` holds the generation status and the sections of the chunk, under names that vary across versions; `full` is the status of a fully generated chunk
Chunk::State state(const nbt::NBT &level, const char *status_key,
                   const char *sections_key, const std::string &full) {
  if (!level.contains(status_key)) // The status is always saved
    return Chunk::UNRECOGNISED;

  const auto status = level[status_key].get<nbt::NBT::tag_string_t>();

  if (status == full)
    // No sections mean no blocks
    return level.contains(sections_key) ? Chunk::FULL : Chunk::UNRECOGNISED;

  // Any other status means the chunk is still being generated, as long as it is spelled like the full one: a change in the spelling (as when 1.20-pre5 namespaced the statuses) is a change in the format
  const bool namespaced = full.find(':') != std::string::npos;

  if ((status.find(':') != std::string::npos) != namespaced)
    return Chunk::UNRECOGNISED;

  return Chunk::INCOMPLETE;
}
} // namespace

Chunk::State v5128(const nbt::NBT &chunk) {
  // 26.4 snapshot, `Status` was renamed to `status`
  return state(chunk, "status", "sections", "minecraft:full");
}

Chunk::State v3458(const nbt::NBT &chunk) {
  // Minecraft 1.20-pre5, randomly changing things
  return state(chunk, "Status", "sections", "minecraft:full");
}

Chunk::State v2844(const nbt::NBT &chunk) {
  // Snapshot 21w43a
  return state(chunk, "Status", "sections", "full");
}

Chunk::State v1976(const nbt::NBT &chunk) {
  // From 1.14 onwards
  if (!chunk.contains("Level")) // Level data is required
    return Chunk::UNRECOGNISED;

  return state(chunk["Level"], "Status", "Sections", "full");
}

Chunk::State v1628(const nbt::NBT &chunk) {
  // From 1.13 onwards
  if (!chunk.contains("Level")) // Level data is required
    return Chunk::UNRECOGNISED;

  return state(chunk["Level"], "Status", "Sections", "postprocessed");
}

Chunk::State catchall(const nbt::NBT &chunk) {
  logger::trace("Unsupported DataVersion: {}", chunk["DataVersion"].get<int>());
  return Chunk::UNRECOGNISED;
}
} // namespace assert_versions
} // namespace versions
} // namespace mcmap
