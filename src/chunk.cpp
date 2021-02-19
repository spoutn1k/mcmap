#include "./chunk.h"

namespace mcmap {

Chunk::Chunk() : data_version(-1) {}

Chunk::Chunk(const nbt::NBT &data, const Colors::Palette &palette) : Chunk() {
  // If there is nothing to render
  if (data.is_end() || !assert_chunk(data))
    return;

  // This value is primordial: it states which version of minecraft the chunk
  // was created under, and we use it to know which interpreter to use later
  // in the sections
  data_version = data["DataVersion"].get<int>();

  for (const auto &section : data["Level"]["Sections"])
    sections.push_back(Section(section, data_version, palette));
}

Chunk::Chunk(Chunk &&other) { *this = std::move(other); }

Chunk &Chunk::operator=(Chunk &&other) {
  data_version = other.data_version;
  sections = std::move(other.sections);

  return *this;
}

bool Chunk::assert_chunk(const nbt::NBT &chunk) {
  if (chunk.is_end()                          // Catch uninitialized chunks
      || !chunk.contains("DataVersion")       // Dataversion is required
      || !chunk.contains("Level")             // Level data is required
      || !chunk["Level"].contains("Sections") // No sections mean no blocks
  )
    return false;

  return true;
}

} // namespace mcmap
