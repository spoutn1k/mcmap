#include "./section.h"

const Colors::Block _void;

Section::Section(const nbt::NBT &raw_section, const int dataVersion,
                 const Colors::Palette &defined)
    : Section() {
  if (!raw_section.contains("Palette") ||
      !raw_section.contains("BlockStates")) {
    colors[max_colors++] = &_void;
    return;
  }

  palette = *raw_section["Palette"].get<const std::vector<NBT> *>();

  const uint32_t blockBitLength =
      std::max(uint32_t(ceil(log2(palette.size()))), uint32_t(4));

  const std::vector<int64_t> *blockStates =
      raw_section["BlockStates"].get<const std::vector<int64_t> *>();

  sectionInterpreter interpreter;

  if (dataVersion < 2534)
    interpreter = sectionAtPre116;
  else
    interpreter = sectionAtPost116;

  interpreter(blockBitLength, blockStates, blocks);

  pickColors(defined);
}

void Section::pickColors(const Colors::Palette &all) {
  for (auto &color : palette) {
    const string namespacedId = color["Name"].get<string>();
    auto defined = all.find(namespacedId);

    if (defined == all.end()) {
      logger::error("Color of block {} not found\n", namespacedId);
      colors[max_colors++] = &_void;
    } else {
      colors[max_colors++] = &defined->second;
      if (namespacedId == "minecraft:beacon")
        beaconIndex = max_colors - 1;
    }
  }
}
