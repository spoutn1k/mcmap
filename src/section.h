#include "./colors.h"
#include "./worldloader.h"
#include <nbt/nbt.hpp>

struct Section {
  uint8_t max_colors, beaconIndex;
  uint8_t blocks[4096];
  std::vector<nbt::NBT> palette;
  const Colors::Block *colors[256];

  Section() : max_colors(0), beaconIndex(std::numeric_limits<uint8_t>::max()){};
  Section(const nbt::NBT &, const int, const Colors::Palette &);
  Section(Section &&other) : palette(std::move(other.palette)) {
    memmove(blocks, other.blocks, 4096);
  };

  void pickColors(const Colors::Palette &);

  inline bool empty() const { return palette.size() < 2; }

  Section &operator=(Section &&other) {
    beaconIndex = other.beaconIndex;
    memmove(blocks, other.blocks, 4096);
    memmove(colors, other.colors, 256 * sizeof(Colors::Block *));
    palette = std::move(other.palette);
    return *this;
  }
};
