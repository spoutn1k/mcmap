#pragma once

#include <nbt/nbt.hpp>

inline std::size_t combine(std::size_t seed, std::size_t h) noexcept {
  seed ^= h + 0x9e3779b9 + (seed << 6U) + (seed >> 2U);
  return seed;
}

namespace nbt {
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

  case tag_type::tag_compound: {
    auto seed = combine(type, n.size());
    const auto *elements = n.template get<const nbt::NBT::tag_compound_t *>();
    for (const auto &element : *elements) {
      auto h = std::hash<string_t>{}(element.first);
      h = combine(h, hash(element.second));
      seed = seed + h;
    }
    return seed;
  }

  case tag_type::tag_byte_array: {
    auto seed = combine(type, n.size());
    const std::vector<int8_t> *values =
        n.template get<const std::vector<int8_t> *>();
    for (const auto &element : *values) {
      const auto h = std::hash<int8_t>{}(element);
      seed = combine(seed, h);
    }
    return seed;
  }

  case tag_type::tag_int_array: {
    auto seed = combine(type, n.size());
    const std::vector<int32_t> *values =
        n.template get<const std::vector<int32_t> *>();
    for (const auto &element : *values) {
      const auto h = std::hash<int32_t>{}(element);
      seed = combine(seed, h);
    }
    return seed;
  }

  case tag_type::tag_long_array: {
    auto seed = combine(type, n.size());
    const std::vector<int64_t> *values =
        n.template get<const std::vector<int64_t> *>();
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
