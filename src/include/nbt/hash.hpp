#pragma once

#include <nbt/nbt.hpp>

namespace nbt {
// Finalizer of splitmix64: every bit of the input affects every bit of the output
inline std::uint64_t mix(std::uint64_t x) noexcept {
  x ^= x >> 30U;
  x *= 0xbf58476d1ce4e5b9ULL;
  x ^= x >> 27U;
  x *= 0x94d049bb133111ebULL;
  x ^= x >> 31U;
  return x;
}

// std::hash is the identity on integers with most standard libraries, and small values combined without mixing cancel each other out: `h` and the result are mixed so that summing or chaining combined hashes does not collide
inline std::size_t combine(std::size_t seed, std::size_t h) noexcept {
  return static_cast<std::size_t>(mix(seed ^ (mix(h) + 0x9e3779b97f4a7c15ULL)));
}

template <typename NBTType> std::size_t hash(const NBTType &n);

// Hash of a compound's contents, the same as the compound's, for contents held outside of an NBT. Entries are summed so that the order of the keys does not matter.
inline std::size_t hash(const NBT::tag_compound_t &elements) {
  auto seed = combine(static_cast<std::size_t>(tag_type::tag_compound),
                      elements.size());
  for (const auto &element : elements) {
    auto h = std::hash<NBT::tag_string_t>{}(element.first);
    h = combine(h, hash(element.second));
    seed = seed + h;
  }
  return seed;
}

template <typename NBTType> std::size_t hash(const NBTType &n) {
  using string_t = typename NBTType::tag_string_t;

  const auto type = static_cast<std::size_t>(n.get_type());

  switch (n.get_type()) {
  case tag_type::tag_byte: {
    const auto h = std::hash<int8_t>{}(n.template get<int8_t>());
    return combine(type, h);
  }

  case tag_type::tag_short: {
    const auto h = std::hash<int16_t>{}(n.template get<int16_t>());
    return combine(type, h);
  }

  case tag_type::tag_int: {
    const auto h = std::hash<int32_t>{}(n.template get<int32_t>());
    return combine(type, h);
  }

  case tag_type::tag_long: {
    const auto h = std::hash<int64_t>{}(n.template get<int64_t>());
    return combine(type, h);
  }

  case tag_type::tag_float: {
    const auto h = std::hash<float>{}(n.template get<float>());
    return combine(type, h);
  }

  case tag_type::tag_double: {
    const auto h = std::hash<double>{}(n.template get<double>());
    return combine(type, h);
  }

  case tag_type::tag_string: {
    const auto h = std::hash<string_t>{}(n.template get<string_t>());
    return combine(type, h);
  }

  case tag_type::tag_list: {
    auto seed = combine(type, n.size());
    for (const auto &element : n) {
      seed = combine(seed, hash(element));
    }
    return seed;
  }

  case tag_type::tag_compound:
    return hash(*n.template get<const typename NBTType::tag_compound_t *>());

  case tag_type::tag_byte_array: {
    auto seed = combine(type, n.size());
    const auto *values =
        n.template get<const typename NBTType::tag_byte_array_t *>();
    for (const auto &element : *values) {
      const auto h = std::hash<int8_t>{}(element);
      seed = combine(seed, h);
    }
    return seed;
  }

  case tag_type::tag_int_array: {
    auto seed = combine(type, n.size());
    const auto *values =
        n.template get<const typename NBTType::tag_int_array_t *>();
    for (const auto &element : *values) {
      const auto h = std::hash<int32_t>{}(element);
      seed = combine(seed, h);
    }
    return seed;
  }

  case tag_type::tag_long_array: {
    auto seed = combine(type, n.size());
    const auto *values =
        n.template get<const typename NBTType::tag_long_array_t *>();
    for (const auto &element : *values) {
      const auto h = std::hash<int64_t>{}(element);
      seed = combine(seed, h);
    }
    return seed;
  }

  case tag_type::tag_end:
    return combine(type, 0);

  default:
    break;
  }

  return 0;
}
} // namespace nbt
