#pragma once

#include "./section.h"
#include <map.hpp>
#include <nbt/nbt.hpp>

struct Coordinates {
  int32_t x, z;

  template <typename I, std::enable_if_t<std::is_integral<I>::value, int> = 0>
  Coordinates(std::initializer_list<I> l) {
    x = *l.begin();
    z = *(l.begin() + 1);
  }

  Coordinates operator+(const Coordinates &other) const {
    return {x + other.x, z + other.z};
  }

  bool operator<(const Coordinates &other) const {
    // Code from <bits/stl_pair.h>
    return x < other.x || (!(other.x < x) && z < other.z);
  }

  bool operator==(const Coordinates &other) const {
    return x == other.x && z == other.z;
  }

  bool operator!=(const Coordinates &other) const {
    return !this->operator==(other);
  }

  template <typename I, std::enable_if_t<std::is_integral<I>::value, int> = 0>
  Coordinates operator%(const I &m) const {
    return {x % m, z % m};
  }
};

namespace mcmap {

struct Chunk {
  using nbt_t = nbt::NBT;
  using version_t = int32_t;
  using section_t = Section;
  using section_array_t = std::vector<section_t>;
  using coordinates = Coordinates;

  version_t data_version;
  section_array_t sections;

  Chunk();
  Chunk(const nbt_t &, const Colors::Palette &);
  Chunk(Chunk &&);

  Chunk &operator=(Chunk &&);

  bool valid() const { return data_version != -1; }

  static bool assert_chunk(const nbt_t &);
};

} // namespace mcmap

mcmap::Chunk::coordinates left_in(Map::Orientation);
mcmap::Chunk::coordinates right_in(Map::Orientation);
