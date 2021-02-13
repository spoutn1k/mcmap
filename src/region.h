#include "./helper.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <logger.hpp>
#include <vector>

namespace fs = std::filesystem;

uint32_t _ntohi(uint8_t *val) {
  return (uint32_t(val[0]) << 24) + (uint32_t(val[1]) << 16) +
         (uint32_t(val[2]) << 8) + (uint32_t(val[3]));
}

struct Location {
  uint32_t raw_data;

  Location() : raw_data(0){};

  uint32_t offset() const { return raw_data >> 8; }

  uint8_t size() const { return raw_data & 0xff; }

  static bool order(const Location &lhs, const Location &rhs) {
    return lhs.offset() > rhs.offset();
  }
};

struct Region {
  fs::path file;
  int16_t rX, rZ;
  std::array<Location, REGIONSIZE * REGIONSIZE> locations;

  Region(const fs::path &_file) : file(_file) {
    locations.fill(Location());

    std::string buffer;
    const char delimiter = '.';

    std::stringstream ss(file.filename().string());
    std::getline(ss, buffer, delimiter); // This removes the 'r.'
    std::getline(ss, buffer, delimiter); // X in r.X.Z.mca

    rX = atoi(buffer.c_str());

    std::getline(ss, buffer, delimiter); // Z in r.X.Z.mca

    rZ = atoi(buffer.c_str());

    std::ifstream regionData(file, std::ifstream::binary);

    for (uint16_t chunk = 0; chunk < REGIONSIZE * REGIONSIZE; chunk++) {
      char buffer[4];
      regionData.read(buffer, 4);

      locations[chunk].raw_data = _ntohi((uint8_t *)buffer);

      if (!regionData) {
        logger::error("Error reading `{}` header.\n", _file.string());
        break;
      }
    }

    regionData.close();
  }

  void fragment() {
    std::array<Location, REGIONSIZE *REGIONSIZE> temporary = locations;

    std::sort(temporary.begin(), temporary.end(), Location::order);
  }
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
