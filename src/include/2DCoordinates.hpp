#pragma once

#include <initializer_list>
#include <stdint.h>
#include <type_traits>

struct Coordinates {
  int32_t x, z;

  template <typename I, std::enable_if_t<std::is_integral<I>::value, int> = 0>
  Coordinates(std::initializer_list<I> l) {
    x = *l.begin();
    z = *(l.begin() + 1);
  }

  Coordinates() {
    x = int32_t();
    z = int32_t();
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
