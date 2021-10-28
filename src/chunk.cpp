#include "./chunk.h"

namespace mcmap {

namespace versions {
namespace assert {
bool v2844(const nbt::NBT &chunk) {
  // Snapshot 21w43a
  return chunk.contains("sections")  // No sections mean no blocks
         && chunk.contains("Status") // Ensure the status is `full`
         && chunk["Status"].get<nbt::NBT::tag_string_t>() == "full";
}

bool v2840(const nbt::NBT &chunk) {
  // Snapshot 21w42a
  return chunk.contains("Level") &&           // Level data is required
         chunk["Level"].contains("Sections")  // No sections mean no blocks
         && chunk["Level"].contains("Status") // Ensure the status is `full`
         && chunk["Level"]["Status"].get<nbt::NBT::tag_string_t>() == "full";
}

bool vbetween(const nbt::NBT &chunk) {
  return chunk.contains("Level")                // Level data is required
         && chunk["Level"].contains("Sections") // No sections mean no blocks
         && chunk["Level"].contains("Status")   // Ensure the status is `full`
         && chunk["Level"]["Status"].get<nbt::NBT::tag_string_t>() == "full";
}

bool v1628(const nbt::NBT &chunk) {
  // 1.13
  return chunk.contains("Level")                // Level data is required
         && chunk["Level"].contains("Sections") // No sections mean no blocks
         && chunk["Level"].contains("Status")   // Ensure the status is `full`
         && chunk["Level"]["Status"].get<nbt::NBT::tag_string_t>() ==
                "postprocessed";
}
} // namespace assert

namespace sections {
nbt::NBT v2844(const nbt::NBT &chunk) { return chunk["sections"]; }
nbt::NBT v1628(const nbt::NBT &chunk) { return chunk["Level"]["Sections"]; }
} // namespace sections
} // namespace versions

Chunk::Chunk() : data_version(-1) {}

Chunk::Chunk(const nbt::NBT &data, const Colors::Palette &palette) : Chunk() {
  // If there is nothing to render
  if (data.is_end() || !assert_chunk(data))
    return;

  // This value is primordial: it states which version of minecraft the chunk
  // was created under, and we use it to know which interpreter to use later
  // in the sections
  data_version = data["DataVersion"].get<int>();

  nbt::NBT sections_list;

  if (data_version >= 2844) {
    sections_list = versions::sections::v2844(data);
  } else {
    sections_list = versions::sections::v1628(data);
  }

  for (const auto &raw_section : sections_list) {
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
  if (chunk.is_end()                     // Catch uninitialized chunks
      || !chunk.contains("DataVersion")) // Dataversion is required
    return false;

  const int version = chunk["DataVersion"].get<int>();

  if (version >= 2844) {
    return versions::assert::v2844(chunk);
  } else if (version >= 2840) {
    return versions::assert::v2840(chunk);
  } else if (version > 1628) {
    return versions::assert::vbetween(chunk);
  } else {
    return versions::assert::v1628(chunk);
  }
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
