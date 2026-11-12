#pragma once

#include "./diagnostics.h"
#include "./section.h"
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

  coordinates position;
  version_t data_version;
  section_array_t sections;

  Chunk();
  Chunk(const nbt_t &, const Colors::Palette &, const coordinates,
        Diagnostics::Report * = nullptr);
  Chunk(Chunk &&);

  Chunk &operator=(Chunk &&);

  bool valid() const { return data_version != -1; }

  static bool assert_chunk(const nbt_t &);

  // The most recent DataVersion with a known chunk format
  static version_t latest_version();
};

} // namespace mcmap

mcmap::Chunk::coordinates left_in(Map::Orientation);
mcmap::Chunk::coordinates right_in(Map::Orientation);
