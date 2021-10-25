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

  for (const auto &raw_section : data["Level"]["Sections"]) {
    Section section(raw_section, data_version);
    section.loadPalette(palette);
    sections.push_back(std::move(section));
  }
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
      || !chunk["Level"].contains("Status"))  // Ensure the status is `full`
    return false;

  long version = chunk["DataVersion"].get<long>();

  if (version > 1628) {
    return chunk["Level"]["Status"].get<nbt::NBT::tag_string_t>() == "full";
  }

  return chunk["Level"]["Status"].get<nbt::NBT::tag_string_t>() ==
         "postprocessed";
}

} // namespace mcmap

mcmap::Chunk::coordinates left_in(Map::Orientation o) {
  switch (o) {
  case Map::NW:
    return {0, 1};
  case Map::SW:
    return {1, 0};
  case Map::SE:
    return {0, -1};
  case Map::NE:
    return {-1, 0};
  }

  return {0, 0};
}

mcmap::Chunk::coordinates right_in(Map::Orientation o) {
  switch (o) {
  case Map::NW:
    return {1, 0};
  case Map::SW:
    return {0, -1};
  case Map::SE:
    return {-1, 0};
  case Map::NE:
    return {0, 1};
  }

  return {0, 0};
}
