#include "./section.h"

const Colors::Block _void;

Section::Section(const nbt::NBT &raw_section, const int dataVersion,
                 const Colors::Palette &defined) {
  if (!raw_section.contains("Palette") ||
      !raw_section.contains("BlockStates")) {
    return;
  }

  palette = *raw_section["Palette"].get<const std::vector<NBT> *>();

  const uint32_t blockBitLength =
      std::max(uint32_t(ceil(log2(palette.size()))), uint32_t(4));

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

  pickColors(defined);
}

void Section::pickColors(const Colors::Palette &all) {
  uint16_t colorIndex = 0;

  for (auto &color : palette) {
    const string namespacedId = color["Name"].get<string>();
    auto defined = all.find(namespacedId);

    if (defined == all.end()) {
      logger::error("Color of block {} not found\n", namespacedId);
      colors[colorIndex++] = &_void;
    } else {
      colors[colorIndex++] = &defined->second;
      if (namespacedId == "minecraft:beacon")
        beaconIndex = colorIndex - 1;
    }
  }
}
