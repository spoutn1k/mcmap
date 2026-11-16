#include "./chunk.h"
#include "./chunk_format_versions/assert.hpp"
#include "./chunk_format_versions/get_section.hpp"
#include "./chunk_format_versions/get_structures.hpp"
#include <compat.hpp>
#include <functional>

namespace mcmap {

namespace versions {
std::map<int, std::function<Chunk::State(const nbt::NBT &)>> assert = {
    {5128, assert_versions::v5128}, {3458, assert_versions::v3458},
    {2844, assert_versions::v2844}, {1976, assert_versions::v1976},
    {1628, assert_versions::v1628}, {0, assert_versions::catchall},
};

std::map<int, std::function<nbt::NBT(const nbt::NBT &)>> sections = {
    {2844, sections_versions::v2844},
    {1628, sections_versions::v1628},
    {0, sections_versions::catchall},
};

std::map<int, std::function<void(const nbt::NBT &, std::vector<BoundingBox> &,
                                 std::vector<Coordinates> &)>>
    structures = {
        {2844, structures_versions::v2844},
        {1628, structures_versions::v1628},
        {0, structures_versions::catchall},
};

} // namespace versions

// The see-through color of every block of the chunks that are not fully generated
const Colors::Block ghost_color(Colors::BlockTypes::drawTransparent,
                                "#e0e8ff14");

Chunk::Chunk() : data_version(-1), ghost(false) {}

Chunk::Chunk(const nbt::NBT &data, const Colors::Palette &palette,
             const coordinates pos, Diagnostics::Report *report)
    : Chunk() {
  position = pos;

  const State state = assert_chunk(data);

  // If there is nothing to render
  if (state == UNRECOGNISED)
    return;

  // Incomplete chunks are only loaded when debugging, see Terrain::Data::loadChunk
  ghost = state == INCOMPLETE;

  // This value is primordial: it states which version of minecraft the chunk was created under, and we use it to know which interpreter to use later in the sections
  data_version = data["DataVersion"].get<int>();

  nbt::NBT sections_list;

  auto sections_it = compatible(versions::sections, data_version);

  if (sections_it != versions::sections.end()) {
    sections_list = sections_it->second(data);

    for (const auto &raw_section : sections_list) {
      Section section(raw_section, data_version, this->position);
      section.loadPalette(palette, report);

      // Ghosts are drawn with a single color, swapped in for every visible block of the palette
      if (ghost)
        for (auto &color : section.colors)
          if (!color->primary.transparent())
            color = &ghost_color;

      sections.push_back(std::move(section));
    }
  }

  read_structures(data, starts, references);
}

Chunk::Chunk(Chunk &&other) { *this = std::move(other); }

Chunk &Chunk::operator=(Chunk &&other) {
  position = other.position;
  data_version = other.data_version;
  sections = std::move(other.sections);
  ghost = other.ghost;
  starts = std::move(other.starts);
  references = std::move(other.references);
  structures = std::move(other.structures);

  return *this;
}

Chunk::version_t Chunk::latest_version() {
  return versions::assert.rbegin()->first;
}

Chunk::State Chunk::assert_chunk(const nbt::NBT &chunk) {
  if (chunk.is_end()                     // Catch uninitialized chunks
      || !chunk.contains("DataVersion")) // Dataversion is required
  {
    logger::trace("Chunk is empty or invalid");
    return UNRECOGNISED;
  }

  const int version = chunk["DataVersion"].get<int>();

  auto assert_it = compatible(versions::assert, version);

  if (assert_it == versions::assert.end()) {
    logger::trace("Unsupported chunk version: {}", version);
    return UNRECOGNISED;
  }

  return assert_it->second(chunk);
}

void Chunk::read_structures(const nbt::NBT &chunk, box_array &boxes,
                            std::vector<coordinates> &refs) {
  if (chunk.is_end() || !chunk.contains("DataVersion"))
    return;

  auto structures_it =
      compatible(versions::structures, chunk["DataVersion"].get<int>());

  if (structures_it != versions::structures.end())
    structures_it->second(chunk, boxes, refs);
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
