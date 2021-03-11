#include "./helper.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <logger.hpp>
#include <vector>

namespace fs = std::filesystem;

struct Location {
  using offset_t = uint32_t;
  using size_t = uint8_t;

  uint32_t raw_data;

  Location() : raw_data(0){};

  offset_t offset() const { return raw_data >> 8; }
  size_t size() const { return raw_data & 0xff; }

  static bool order(const Location &lhs, const Location &rhs) {
    return lhs.offset() < rhs.offset();
  }
};

struct Region {
  static std::pair<int32_t, int32_t> coordinates(const fs::path &);

  fs::path file;
  std::array<Location, REGIONSIZE * REGIONSIZE> locations;

  Region() { locations.fill(Location()); }

  Region(const fs::path &);

  void write(const fs::path &);

  size_t get_offset(uint8_t);
};

template <> struct fmt::formatter<Region> {
  char presentation = 'r';

  constexpr auto parse(format_parse_context &ctx) {
    auto it = ctx.begin(), end = ctx.end();

    if (it != end && *it == 'r')
      presentation = *it++;

    // Check if reached the end of the range:
    if (it != end && *it != '}')
      throw format_error("invalid format");

    // Return an iterator past the end of the parsed range:
    return it;
  }

  template <typename FormatContext>
  auto format(const Region &r, FormatContext &ctx) {
    auto unknown = format_to(ctx.out(), "Contents of {}\n", r.file.string());
    for (int it = 0; it < REGIONSIZE * REGIONSIZE; it++) {
      unknown =
          format_to(ctx.out(), "{:02d}\t{:02d}\t{:04d}\t{:02d}\n", it & 0x1f,
                    it >> 5, r.locations[it].offset(), r.locations[it].size());
    }

    return unknown;
  }
};
