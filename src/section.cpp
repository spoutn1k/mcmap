#include "./section.h"
#include "./chunk_format_versions/section_format.hpp"
#include <compat.hpp>
#include <functional>

namespace mcmap {
namespace versions {
std::map<int, std::function<void(Section *, const nbt::NBT &)>> init = {
    {2840, init_versions::v2840},
    {2534, init_versions::v2534},
    {1628, init_versions::v1628},
    {0, init_versions::catchall},
};
} // namespace versions
} // namespace mcmap

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
      logger::error("Color of block {} not found", namespacedId);
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

  auto init_it = compatible(mcmap::versions::init, dataVersion);

  if (init_it != mcmap::versions::init.end()) {
    init_it->second(this, raw_section);
  }

  // Iron out potential corruption errors
  for (block_array::reference index : blocks) {
    if (index > palette.size() - 1) {
      logger::trace("Malformed section: block is undefined in palette");
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
