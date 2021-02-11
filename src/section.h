#include "./colors.h"
#include "./worldloader.h"
#include <nbt/nbt.hpp>

struct Section {
  int8_t Y;

  uint8_t max_colors, beaconIndex;
  uint8_t blocks[4096];
  const std::vector<nbt::NBT> *palette;
  const Colors::Block *colors[256];

  Section() : max_colors(0), beaconIndex(std::numeric_limits<uint8_t>::max()){};
  Section(const nbt::NBT &, const int, const Colors::Palette &);

  Section(Section &&other) { *this = std::move(other); }

  Section &operator=(Section &&other) {
    Y = other.Y;
    max_colors = other.max_colors;
    beaconIndex = other.beaconIndex;
    memmove(blocks, other.blocks, 4096);
    memmove(colors, other.colors, 256 * sizeof(Colors::Block *));
    palette = std::move(other.palette);
    return *this;
  }

  void pickColors(const Colors::Palette &);

  inline bool empty() const { return max_colors < 2; }
};
