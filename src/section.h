#include "./colors.h"
#include "./worldloader.h"
#include <nbt/nbt.hpp>

struct section {
  uint16_t beacon;
  uint8_t blocks[4096];
  const std::vector<nbt::NBT> *palette;
  Colors::Block *cache[256];

  section() : beacon(0) { memset(blocks, 0, 4096); };
  section(const nbt::NBT &, const int);

  void pickColors(const Colors::Palette &all);
};
