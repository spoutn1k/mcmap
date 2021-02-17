#include "./colors.h"
#include <nbt/nbt.hpp>

struct Section {
  // This statement makes the assumption a section contains at most 256
  // different blocks - this may be prove to be bold.
  using block_array = std::array<uint8_t, 4096>;
  using color_array = std::vector<const Colors::Block *>;
  using light_array = std::array<uint8_t, 2048>;

  // The vertical index of the section
  int8_t Y;

  block_array blocks;
  color_array colors;
  light_array lights;
  const std::vector<nbt::NBT> *palette;

  block_array::value_type beaconIndex;

  Section();
  Section(const nbt::NBT &, const int, const Colors::Palette &);
  Section(Section &&other) { *this = std::move(other); }

  Section &operator=(Section &&other) {
    Y = other.Y;

    beaconIndex = other.beaconIndex;

    blocks = std::move(other.blocks);
    lights = std::move(other.lights);
    colors = std::move(other.colors);
    palette = other.palette;
    return *this;
  }

  inline bool empty() const {
    // The palette's first block is always air, even if no air is present.
    // Checking if it is empty is then as simple as checking if another block is
    // defined.

    return colors.size() < 2;
  }

  inline block_array::value_type block_at(uint8_t x, uint8_t y,
                                          uint8_t z) const {
    return blocks[x + 16 * z + 16 * 16 * y];
  }

  inline color_array::value_type color_at(uint8_t x, uint8_t y,
                                          uint8_t z) const {
    return colors[blocks[x + 16 * z + 16 * 16 * y]];
  }

  inline light_array::value_type light_at(uint8_t x, uint8_t y,
                                          uint8_t z) const {
    const uint16_t index = x + 16 * z + 16 * 16 * y;

    return (index % 2 ? lights[index / 2] >> 4 : lights[index / 2] & 0x0f);
  }

  inline const nbt::NBT &state_at(uint8_t x, uint8_t y, uint8_t z) const {
    return palette->operator[](blocks[x + 16 * z + 16 * 16 * y]);
  }
};

void sectionAtPre116(const uint8_t, const std::vector<int64_t> *,
                     Section::block_array &);

void sectionAtPost116(const uint8_t, const std::vector<int64_t> *,
                      Section::block_array &);
