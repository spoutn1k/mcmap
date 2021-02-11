#include "./colors.h"
#include "./worldloader.h"
#include <nbt/nbt.hpp>

struct Section {
  int8_t Y;

  uint8_t beaconIndex;
  std::array<uint8_t, 4096> blocks;
  std::vector<const Colors::Block *> colors;
  const std::vector<nbt::NBT> *palette;

  Section() : beaconIndex(std::numeric_limits<uint8_t>::max()){};
  Section(const nbt::NBT &, const int, const Colors::Palette &);
  Section(Section &&other) { *this = std::move(other); }

  Section &operator=(Section &&other) {
    Y = other.Y;
    beaconIndex = other.beaconIndex;
    blocks = std::move(other.blocks);
    colors = std::move(other.colors);
    palette = std::move(other.palette);
    return *this;
  }

  void pickColors(const Colors::Palette &);

  inline bool empty() const {
    // The palette's first block is always air, even if no air is present.
    // Checking if it is empty is then as simple as checking if another block is
    // defined.

    return colors.size() < 2;
  }

  inline uint8_t block_at(uint8_t x, uint8_t y, uint8_t z) const {
    return blocks[x + 16 * z + 16 * 16 * y];
  }

  inline const Colors::Block *color_at(uint8_t x, uint8_t y, uint8_t z) const {
    return colors[blocks[x + 16 * z + 16 * 16 * y]];
  }

  inline const nbt::NBT &state_at(uint8_t x, uint8_t y, uint8_t z) const {
    return palette->operator[](blocks[x + 16 * z + 16 * 16 * y]);
  }
};
