#pragma once
#include <limits>
#include <type_traits>

template <typename UInteger,
          std::enable_if_t<std::is_unsigned<UInteger>::value, int> = 0>
struct Counter {
  UInteger counter;

  Counter(UInteger value = 0) : counter(value) {}

  Counter<UInteger> &operator++() {
    counter < std::numeric_limits<UInteger>::max() ? ++counter : counter;
    return *this;
  }

  operator UInteger() { return counter; }
};
