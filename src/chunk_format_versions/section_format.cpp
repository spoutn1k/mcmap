#include "section_format.hpp"

namespace mcmap {
namespace versions {
namespace block_states_versions {
void post116(const uint8_t index_length,
             const std::vector<int64_t> *blockStates,
             Section::block_array &buffer) {
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

void pre116(const uint8_t index_length, const std::vector<int64_t> *blockStates,
            Section::block_array &buffer) {
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
} // namespace block_states_versions

namespace init_versions {
void v1628(Section *target, const nbt::NBT &raw_section) {
  if (raw_section.contains("BlockStates") && raw_section.contains("Palette")) {
    target->palette =
        *raw_section["Palette"].get<const std::vector<nbt::NBT> *>();
    const nbt::NBT::tag_long_array_t *blockStates =
        raw_section["BlockStates"].get<const nbt::NBT::tag_long_array_t *>();

    // Remove the air that is default-constructed
    target->colors.clear();
    // Anticipate the color input from the palette's size
    target->colors.reserve(target->palette.size());

    // The length in bits of a block is the log2 of the palette's size or 4,
    // whichever is greatest. Ranges from 4 to 12.
    const uint8_t blockBitLength =
        std::max(uint8_t(ceil(log2(target->palette.size()))), uint8_t(4));

    // Parse the blockstates for block info
    block_states_versions::pre116(blockBitLength, blockStates, target->blocks);
  } else
    logger::trace("Section {} does not contain BlockStates or Palette !",
                  target->Y);
}

void v2534(Section *target, const nbt::NBT &raw_section) {
  if (raw_section.contains("BlockStates") && raw_section.contains("Palette")) {
    target->palette =
        *raw_section["Palette"].get<const std::vector<nbt::NBT> *>();
    const nbt::NBT::tag_long_array_t *blockStates =
        raw_section["BlockStates"].get<const nbt::NBT::tag_long_array_t *>();

    // Remove the air that is default-constructed
    target->colors.clear();
    // Anticipate the color input from the palette's size
    target->colors.reserve(target->palette.size());

    // The length in bits of a block is the log2 of the palette's size or 4,
    // whichever is greatest. Ranges from 4 to 12.
    const uint8_t blockBitLength =
        std::max(uint8_t(ceil(log2(target->palette.size()))), uint8_t(4));

    // Parse the blockstates for block info
    block_states_versions::post116(blockBitLength, blockStates, target->blocks);
  } else
    logger::trace("Section {} does not contain BlockStates or Palette !",
                  target->Y);
}

void v2567_MINEFLAYER(Section *target, const nbt::NBT &raw_section) {
  /* This code detects and handles error in terrain generated by mineflayer.
   * This should only be enabled on select binaries. */
  if (raw_section.contains("BlockStates") && raw_section.contains("Palette")) {
    target->palette =
        *raw_section["Palette"].get<const std::vector<nbt::NBT> *>();
    const nbt::NBT::tag_long_array_t *blockStates =
        raw_section["BlockStates"].get<const nbt::NBT::tag_long_array_t *>();

    // Remove the air that is default-constructed
    target->colors.clear();
    // Anticipate the color input from the palette's size
    target->colors.reserve(target->palette.size());

    // The length in bits of a block is the log2 of the palette's size or 4,
    // whichever is greatest. Ranges from 4 to 12.
    uint8_t blockBitLength =
        std::max(uint8_t(ceil(log2(target->palette.size()))), uint8_t(4));

    auto states_size = blockStates->size();
    auto inflation = 100 * (64 * states_size - 4096 * blockBitLength) /
                     (4096 * blockBitLength);
    if (inflation > 20) {
      logger::trace("Section {} in {} {}: bbl {}, bs {}, inflation {}%",
                    target->Y, target->parent_chunk_coordinates.x,
                    target->parent_chunk_coordinates.z, blockBitLength,
                    states_size, inflation);

      blockBitLength += 1;
    }

    // Parse the blockstates for block info
    block_states_versions::post116(blockBitLength, blockStates, target->blocks);
  } else
    logger::trace("Section {} of DataVersion 2567 does not contain BlockStates "
                  "or Palette !",
                  target->Y);
}

void v2840(Section *target, const nbt::NBT &raw_section) {
  if (raw_section.contains("block_states") &&
      raw_section["block_states"].contains("data") &&
      raw_section["block_states"].contains("palette")) {
    target->palette = *raw_section["block_states"]["palette"]
                           .get<const std::vector<nbt::NBT> *>();
    const nbt::NBT::tag_long_array_t *blockStates =
        raw_section["block_states"]["data"]
            .get<const nbt::NBT::tag_long_array_t *>();

    // Remove the air that is default-constructed
    target->colors.clear();
    // Anticipate the color input from the palette's size
    target->colors.reserve(target->palette.size());

    // The length in bits of a block is the log2 of the palette's size or 4,
    // whichever is greatest. Ranges from 4 to 12.
    const uint8_t blockBitLength =
        std::max(uint8_t(ceil(log2(target->palette.size()))), uint8_t(4));

    // Parse the blockstates for block info
    block_states_versions::post116(blockBitLength, blockStates, target->blocks);
  } else
    logger::trace("Section {} does not contain BlockStates or Palette !",
                  target->Y);
}

void v3100(Section *target, const nbt::NBT &raw_section) {
  // NEW in 1.19, some sections can omit the block_states array when only one
  // block is present in the palette to signify that the whole section is
  // filled with one block, so this checks for that special case

  if (raw_section.contains("block_states") &&
      raw_section["block_states"].contains("palette")) {
    target->palette = *raw_section["block_states"]["palette"]
                           .get<const std::vector<nbt::NBT> *>();
    // Remove the air that is default-constructed
    target->colors.clear();
    // Anticipate the color input from the palette's size
    target->colors.reserve(target->palette.size());

    if (raw_section["block_states"].contains("data")) {
      const nbt::NBT::tag_long_array_t *blockStates =
          raw_section["block_states"]["data"]
              .get<const nbt::NBT::tag_long_array_t *>();

      // The length in bits of a block is the log2 of the palette's size or 4,
      // whichever is greatest. Ranges from 4 to 12.
      const uint8_t blockBitLength =
          std::max(uint8_t(ceil(log2(target->palette.size()))), uint8_t(4));

      // Parse the blockstates for block info
      block_states_versions::post116(blockBitLength, blockStates,
                                     target->blocks);
    } else {
      target->blocks.fill(0);
    }
  } else
    logger::trace("Section {} does not contain a palette, aborting", target->Y);
}

void catchall(Section *, const nbt::NBT &) {
  logger::trace("Unsupported DataVersion");
}
} // namespace init_versions
} // namespace versions
} // namespace mcmap
