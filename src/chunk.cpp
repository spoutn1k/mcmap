#include "./chunk.h"
#include "./chunk_format_versions/assert.hpp"
#include "./chunk_format_versions/get_section.hpp"
#include <compat.hpp>
#include <functional>

namespace mcmap {

namespace versions {
std::map<int, std::function<bool(const nbt::NBT &)>> assert = {
    {3458, assert_versions::v3458}, {2844, assert_versions::v2844},
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

Chunk::Chunk(const nbt::NBT &data, const Colors::Palette &palette,
             const coordinates pos)
    : Chunk() {
  position = pos;

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
      Section section(raw_section, data_version, this->position);
      section.loadPalette(palette);
      sections.push_back(std::move(section));
    }
  }
}

Chunk::Chunk(Chunk &&other) { *this = std::move(other); }

Chunk &Chunk::operator=(Chunk &&other) {
  position = other.position;
  data_version = other.data_version;
  sections = std::move(other.sections);

  return *this;
}

bool Chunk::assert_chunk(const nbt::NBT &chunk) {
  if (chunk.is_end()                     // Catch uninitialized chunks
      || !chunk.contains("DataVersion")) // Dataversion is required
  {
    logger::trace("Chunk is empty or invalid");
    return false;
  }

  const int version = chunk["DataVersion"].get<int>();

  auto assert_it = compatible(versions::assert, version);

  if (assert_it == versions::assert.end()) {
    logger::trace("Unsupported chunk version: {}", version);
    return false;
  }

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
