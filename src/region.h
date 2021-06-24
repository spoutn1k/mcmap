#pragma once
#include "./chunk.h"
#include "./helper.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <logger.hpp>
#include <vector>

struct Location {
  using offset_type = uint32_t;
  using size_type = uint8_t;

  uint32_t raw_data;

  Location() : raw_data(0){};
  Location(offset_type off, size_type size) : raw_data((off << 8) | size){};

  offset_type offset() const { return raw_data >> 8; }
  size_type size() const { return raw_data & 0xff; }

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

  void write_header();

  void add_chunk(const mcmap::Chunk::coordinates &, const nbt::NBT &);

private:
  size_t get_offset(uint8_t);
};
