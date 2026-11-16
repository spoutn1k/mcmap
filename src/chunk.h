#pragma once

#include "./diagnostics.h"
#include "./section.h"
#include "./structure.h"
#include <2DCoordinates.hpp>
#include <map.hpp>
#include <nbt/nbt.hpp>

namespace mcmap {

struct Chunk {
  using nbt_t = nbt::NBT;
  using version_t = int32_t;
  using section_t = Section;
  using section_array_t = std::vector<section_t>;
  using coordinates = Coordinates;
  using box_array = std::vector<BoundingBox>;

  // Outcome of the format checks on raw chunk data
  enum State {
    // Fully generated, ready to be rendered
    FULL,
    // Understood, but still being generated: there is nothing to render yet
    INCOMPLETE,
    // Missing data the format requires, which hints at a change in the format
    UNRECOGNISED,
  };

  coordinates position;
  version_t data_version;
  section_array_t sections;

  // Not fully generated: only loaded when debugging, to be drawn as a see-through ghost
  bool ghost;

  // Bounding boxes of the pieces of the structures starting in this chunk
  box_array starts;
  // Chunks holding the starts of the structures overlapping this chunk
  std::vector<coordinates> references;
  // Bounding boxes overlapping this chunk, resolved by the world loader
  box_array structures;

  Chunk();
  Chunk(const nbt_t &, const Colors::Palette &, const coordinates,
        Diagnostics::Report * = nullptr);
  Chunk(Chunk &&);

  Chunk &operator=(Chunk &&);

  bool valid() const { return data_version != -1; }

  static State assert_chunk(const nbt_t &);

  // The most recent DataVersion with a known chunk format
  static version_t latest_version();

  // Read the structure starts and references of a raw chunk
  static void read_structures(const nbt_t &, box_array &,
                              std::vector<coordinates> &);
};

} // namespace mcmap

mcmap::Chunk::coordinates left_in(Map::Orientation);
mcmap::Chunk::coordinates right_in(Map::Orientation);
