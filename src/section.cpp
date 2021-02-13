#include "./section.h"

const Colors::Block _void;

Section::Section(const nbt::NBT &raw_section, const int dataVersion,
                 const Colors::Palette &defined)
    : Section() {
  if (!raw_section.contains("Palette") ||
      !raw_section.contains("BlockStates")) {
    // If the section is empty or invalid, make sure all the blocks are air
    colors.push_back(&_void);
    blocks.fill(0);
    return;
  }

  // Get data from the NBT
  Y = raw_section["Y"].get<int8_t>();
  palette = raw_section["Palette"].get<const std::vector<nbt::NBT> *>();
  const std::vector<int64_t> *blockStates =
      raw_section["BlockStates"].get<const std::vector<int64_t> *>();

  // Anticipate the color input from the palette's size
  colors.reserve(palette->size());

  // The length in bits of a block is the log2 of the palette's size or 4,
  // whichever is greatest.
  const uint32_t blockBitLength =
      std::max(uint32_t(ceil(log2(palette->size()))), uint32_t(4));

  // Parse the blockstates for block info
  if (dataVersion < 2534)
    sectionAtPre116(blockBitLength, blockStates, blocks);
  else
    sectionAtPost116(blockBitLength, blockStates, blocks);

  // Pick the colors from the Palette
  for (auto &color : *palette) {
    const string namespacedId = color["Name"].get<string>();
    auto query = defined.find(namespacedId);

    if (query == defined.end()) {
      logger::error("Color of block {} not found\n", namespacedId);
      colors.push_back(&_void);
    } else {
      colors.push_back(&query->second);
      if (namespacedId == "minecraft:beacon")
        beaconIndex = colors.size() - 1;
    }
  }

  // Iron out potential corruption errors
  for (uint8_t &index : blocks) {
    if (index > colors.size() - 1) {
      logger::deep_debug("Malformed section: block is undefined in palette\n");
      index = 0;
    }
  }
}

void sectionAtPost116(const uint64_t index_length,
                      const std::vector<int64_t> *blockStates,
                      std::array<uint8_t, 4096> &buffer) {
  // NEW in 1.16, longs are padded by 0s when a block cannot fit, so no more
  // overflow to deal with !

  for (uint16_t index = 0; index < 4096; index++) {
    // Determine how many indexes each long holds
    const uint8_t blocksPerLong = 64 / index_length;

    // Calculate where in the long array is the long containing the right index.
    const uint16_t longIndex = index / blocksPerLong;

    // Once we located a long, we have to know where in the 64 bits
    // the relevant block is located.
    const uint8_t padding = (index - longIndex * blocksPerLong) * index_length;

    // Bring the data to the first bits of the long, then extract it by bitwise
    // comparison
    const uint16_t blockIndex = ((*blockStates)[longIndex] >> padding) &
                                ((uint64_t(1) << index_length) - 1);

    buffer[index] = blockIndex;
  }
}

void sectionAtPre116(const uint64_t index_length,
                     const std::vector<int64_t> *blockStates,
                     std::array<uint8_t, 4096> &buffer) {
  // The `BlockStates` array contains data on the section's blocks. You have to
  // extract it by understanfing its structure.
  //
  // Although it is a array of long values, one must see it as an array of block
  // indexes, whose element size depends on the size of the Palette. This
  // routine locates the necessary long, extracts the block with bit
  // comparisons.
  //
  // The length of a block index has to be coded on the minimal possible size,
  // which is the logarithm in base2 of the size of the palette, or 4 if the
  // logarithm is smaller.

  for (uint16_t index = 0; index < 4096; index++) {

    // We skip the `position` first blocks, of length `size`, then divide by 64
    // to get the number of longs to skip from the array
    const uint16_t skip_longs = (index * index_length) >> 6;

    // Once we located the data in a long, we have to know where in the 64 bits
    // it is located. This is the remaining of the previous operation
    const int8_t padding = (index * index_length) & 63;

    // Sometimes the data of an index does not fit entirely into a long, so we
    // check if there is overflow
    const int8_t overflow =
        (padding + index_length > 64 ? padding + index_length - 64 : 0);

    // This complicated expression extracts the necessary bits from the current
    // long.
    //
    // Lets say we need the following bits in a long (not to scale):
    // 10011100111001110011100
    //    ^^^^^
    // We do this by shifting (>>) the data by padding, to get the relevant bits
    // on the end of the long:
    // ???????????????10011100
    //                   ^^^^^
    // We then apply a mask to get only the relevant bits:
    // ???????????????10011100
    // 00000000000000000011111 &
    // 00000000000000000011100 <- result
    //
    // The mask is made at the size of the data, using the formula (1 << n) - 1,
    // the resulting bitset is of the following shape: 0...01...1 with n 1s.
    //
    // If there is an overflow, the mask size is reduced, as not to catch noise
    // from the padding (ie the interrogation points earlier) that appear on
    // ARM32.
    uint16_t lower_data = ((*blockStates)[skip_longs] >> padding) &
                          ((uint64_t(1) << (index_length - overflow)) - 1);

    if (overflow > 0) {
      // The exact same process is used to catch the overflow from the next long
      const uint16_t upper_data =
          ((*blockStates)[skip_longs + 1]) & ((uint64_t(1) << overflow) - 1);
      // We then associate both values to create the final value
      lower_data = lower_data | (upper_data << (index_length - overflow));
    }

    // lower_data now contains the index in the palette
    buffer[index] = lower_data;
  }
}
