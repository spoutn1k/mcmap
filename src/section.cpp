#include "./section.h"

section::section(const nbt::NBT &raw_section, const int dataVersion) {
  palette = raw_section["Palette"].get<const std::vector<NBT> *>();

  const uint32_t blockBitLength =
      std::max(uint32_t(ceil(log2(palette->size()))), uint32_t(4));

  const std::vector<int64_t> *blockStates =
      raw_section["BlockStates"].get<const std::vector<int64_t> *>();

  sectionInterpreter interpreter;

  if (dataVersion < 2534)
    interpreter = blockAtPre116;
  else
    interpreter = blockAtPost116;

  for (int i = 0; i < 4096; i++)
    blocks[i] = interpreter(blockBitLength, blockStates, i % 16, (i / 16) % 16,
                            i / 256);
}

/*
void section::pickColors(const Colors::Palette &all) {
  uint16_t colorIndex = 0;
  const Colors::Block *air;

  for (auto &color : *palette) {
    const string namespacedId = color["Name"].get<string>();
    auto defined = all.find(namespacedId);

    if (defined == all.end()) {
      logger::error("Color of block {} not found\n", namespacedId);
      cache[colorIndex++] = &air;
    } else {
      cache[colorIndex++] = &defined->second;
      if (namespacedId == "minecraft:beacon")
        beacon = colorIndex - 1;
    }
  }
};
*/
