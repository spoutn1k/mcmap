#pragma once
#include "./chunk.h"
#include "./helper.h"
#include <cstdint>
#include <filesystem>
#include <logger.hpp>

struct Location {
  using offset_type = uint32_t;
  using size_type = uint8_t;

  uint32_t raw_data;

  Location() : raw_data(0) {};
  Location(offset_type off, size_type size) : raw_data((off << 8) | size){};

  offset_type offset() const { return raw_data >> 8; }
  size_type size() const { return raw_data & 0xff; }

  static bool order(const Location &lhs, const Location &rhs) {
    return lhs.offset() < rhs.offset();
  }
};

struct Region {
  static std::pair<int32_t, int32_t> coordinates(const fs::path &);

  // Index of a chunk in the header; accepts absolute chunk coordinates
  static size_t index(const mcmap::Chunk::coordinates &);

  fs::path file;
  std::array<Location, REGIONSIZE * REGIONSIZE> locations;
  std::array<uint32_t, REGIONSIZE * REGIONSIZE> timestamps;

  Region() {
    locations.fill(Location());
    timestamps.fill(0);
  }

  Region(const fs::path &);

  bool write_header();

  // Both return true on success
  bool get_chunk(const mcmap::Chunk::coordinates &, nbt::NBT &);
  bool add_chunk(const mcmap::Chunk::coordinates &, const nbt::NBT &);

private:
  size_t get_offset(uint8_t);
};
