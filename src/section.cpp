#include "./section.h"
#include <compat.hpp>
#include <functional>

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
  }
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
  }
}

#ifdef SNAPSHOT_SUPPORT
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
  }
}
#endif

void catchall(Section *, const nbt::NBT &) {
  logger::deep_debug("Unsupported DataVersion\n");
}
} // namespace init_versions

std::map<int, std::function<void(Section *, const nbt::NBT &)>> init = {
#ifdef SNAPSHOT_SUPPORT
    {2840, init_versions::v2840},
#endif
    {2534, init_versions::v2534},
    {1628, init_versions::v1628},
    {0, init_versions::catchall},
};

} // namespace versions

const Colors::Block _void;

Section::Section() : colors{&_void} {
  // The `colors` array needs to contain at least a color to have a defined
  // behavious when uninitialized. `color_at` is called 4096x per section, it is
  // critical for it to avoid if-elses.

  // This is set to the maximum index available as not to trigger a beacon
  // detection by error
  beaconIndex = std::numeric_limits<block_array::value_type>::max();

  // Make sure all the blocks are air - thanks to the default value in `colors`
  blocks.fill(std::numeric_limits<block_array::value_type>::min());
  lights.fill(std::numeric_limits<light_array::value_type>::min());
}

void Section::loadPalette(const Colors::Palette &defined) {
  // Pick the colors from the Palette
  for (const auto &color : palette) {
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
}

Section::Section(const nbt::NBT &raw_section, const int dataVersion)
    : Section() {

  // Get data from the NBT
  Y = raw_section["Y"].get<int8_t>();

  auto init_it = compatible(versions::init, dataVersion);

  if (init_it != versions::init.end()) {
    init_it->second(this, raw_section);
  }

  // Iron out potential corruption errors
  for (block_array::reference index : blocks) {
    if (index > palette.size() - 1) {
      logger::deep_debug("Malformed section: block is undefined in palette\n");
      index = 0;
    }
  }

  // Import lighting data if present
  if (raw_section.contains("BlockLight")) {
    const nbt::NBT::tag_byte_array_t *blockLights =
        raw_section["BlockLight"].get<const nbt::NBT::tag_byte_array_t *>();

    if (blockLights->size() == 2048) {
      for (nbt::NBT::tag_byte_array_t::size_type i = 0; i < blockLights->size();
           i++) {
        lights[i] = blockLights->at(i);
      }
    }
  }
}

void Section::to_nbt(nbt::NBT &section) {
  std::vector<uint64_t> blockstates;

  if (!palette.size())
    return;

  section["Palette"] = nbt::NBT(this->palette, "Palette");

  const uint8_t blockBitLength =
      std::max(uint8_t(ceil(log2(this->palette.size()))), uint8_t(4));

  const std::vector<uint64_t>::size_type blocksPerLong =
      sizeof(uint64_t) * 8 / blockBitLength;

  const std::vector<uint64_t>::size_type states =
      ceil(float(this->blocks.size()) / blocksPerLong);

  blockstates.resize(states);

  auto block_i = this->blocks.begin();

  for (std::vector<uint64_t>::size_type index = 0; index < states; index++) {
    uint64_t blockstate = 0;

    for (std::vector<uint64_t>::size_type b = 0; b < blocksPerLong; b++) {
      blockstate = blockstate | (uint64_t(*block_i++) << (blockBitLength * b));
    }

    blockstates[index] = blockstate;
  }

  section["BlockStates"] = nbt::NBT(blockstates, "BlockStates");
}
