#include "./chunk.h"
#include <functional>

namespace mcmap {

namespace versions {
namespace assert_versions {
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
  logger::deep_debug("Unsupported DataVersion: {}\n",
                     chunk["DataVersion"].get<int>());
  return false;
}
} // namespace assert_versions

namespace sections_versions {
nbt::NBT v2844(const nbt::NBT &chunk) { return chunk["sections"]; }
nbt::NBT v1628(const nbt::NBT &chunk) { return chunk["Level"]["Sections"]; }
nbt::NBT catchall(const nbt::NBT &chunk) {
  logger::deep_debug("Unsupported DataVersion: {}\n",
                     chunk["DataVersion"].get<int>());
  return nbt::NBT(std::vector<nbt::NBT>());
}
} // namespace sections_versions

std::map<int, std::function<bool(const nbt::NBT &)>> assert = {
    {2844, assert_versions::v2844}, {2840, assert_versions::v2840},
    {1976, assert_versions::v1976}, {1628, assert_versions::v1628},
    {0, assert_versions::catchall},
};

std::map<int, std::function<nbt::NBT(const nbt::NBT &)>> sections = {
    {2844, sections_versions::v2844},
    {1628, sections_versions::v1628},
    {0, sections_versions::catchall},
};

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

  auto sections_it = compatible(versions::sections, data_version);

  if (sections_it != versions::sections.end()) {
    sections_list = sections_it->second(data);

    for (const auto &raw_section : sections_list) {
      Section section(raw_section, data_version);
      section.loadPalette(palette);
      sections.push_back(std::move(section));
    }
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

  auto assert_it = compatible(versions::assert, version);

  if (assert_it == versions::assert.end())
    return false;

  return assert_it->second(chunk);
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
