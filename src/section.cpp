#include "./section.h"
#include "./chunk_format_versions/section_format.hpp"
#include <algorithm>
#include <compat.hpp>
#include <functional>

namespace mcmap {
namespace versions {
std::map<int, std::function<void(Section *, const nbt::NBT &)>> init = {
    {5009, init_versions::v5009}, {3100, init_versions::v3100},
    {2840, init_versions::v2840}, {2534, init_versions::v2534},
    {1628, init_versions::v1628}, {0, init_versions::catchall},
};

std::map<int, std::function<void(Section *, const nbt::NBT &)>> biomes = {
    {2836, biomes_versions::v2836},
    {0, biomes_versions::catchall},
};
} // namespace versions
} // namespace mcmap

const Colors::Block _void;

Section::Section() : colors{&_void} {
  // The `colors` array needs to contain at least a color to have a defined behavious when uninitialized. `color_at` is called 4096x per section, it is critical for it to avoid if-elses.

  // This is set to the maximum index available as not to trigger a beacon detection by error
  beaconIndex = std::numeric_limits<block_array::value_type>::max();

  // Make sure all the blocks are air - thanks to the default value in `colors`
  blocks.fill(std::numeric_limits<block_array::value_type>::min());
  lights.fill(std::numeric_limits<light_array::value_type>::min());
  biomes.fill(0);
}

void Section::loadPalette(const Colors::Palette &defined,
                          Diagnostics::Report *report) {
  // Pick the colors from the Palette
  for (const auto &color : palette) {
    const string namespacedId = color["Name"].get<string>();
    auto query = defined.find(namespacedId);

    if (query == defined.end()) {
      logger::debug("Color of block {} not found", namespacedId);
      if (report)
        report->missing_color(namespacedId);
      colors.push_back(&_void);
    } else {
      colors.push_back(&query->second);
      if (namespacedId == "minecraft:beacon")
        beaconIndex = colors.size() - 1;
    }
  }
}

Section::Section(const nbt::NBT &raw_section, const int dataVersion,
                 const Coordinates chunk)
    : Section() {

  // Get data from the NBT
  Y = raw_section["Y"].get<int8_t>();
  this->parent_chunk_coordinates = chunk;

  auto init_it = compatible(mcmap::versions::init, dataVersion);

  if (init_it != mcmap::versions::init.end()) {
    init_it->second(this, raw_section);
  }

  auto biomes_it = compatible(mcmap::versions::biomes, dataVersion);

  if (biomes_it != mcmap::versions::biomes.end())
    biomes_it->second(this, raw_section);

  // Iron out potential corruption errors
  for (block_array::reference index : blocks) {
    if (index > palette.size() - 1) {
      logger::trace("Malformed section: block is undefined in palette");
      index = 0;
    }
  }

  for (biome_array::reference index : biomes) {
    if (index >= biome_palette.size()) {
      logger::trace("Malformed section: biome is undefined in palette");
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

namespace {
// Pack block indexes of `bits` bits into longs. Before 1.16, indexes are packed back to back and can span two longs; since then, longs are padded with 0s when an index cannot fit.
std::vector<uint64_t> pack(const Section::block_array &indexes,
                           const uint8_t bits, const bool spanning) {
  std::vector<uint64_t> states;

  if (spanning) {
    states.resize((indexes.size() * bits + 63) / 64, 0);

    for (size_t i = 0; i < indexes.size(); i++) {
      const size_t bit = i * bits;
      const uint8_t padding = bit & 63;
      const uint64_t value = indexes[i];

      states[bit >> 6] |= value << padding;

      // Write the overflowing bits at the start of the next long
      if (padding + bits > 64)
        states[(bit >> 6) + 1] |= value >> (64 - padding);
    }
  } else {
    const size_t blocksPerLong = 64 / bits;
    states.resize((indexes.size() + blocksPerLong - 1) / blocksPerLong, 0);

    for (size_t i = 0; i < indexes.size(); i++)
      states[i / blocksPerLong] |= uint64_t(indexes[i])
                                   << ((i % blocksPerLong) * bits);
  }

  return states;
}

// Since 26.3-snapshot-7, a block without properties is stored as its bare id, and the others as a compound with an `id` and `properties`. Lists cannot mix types, so in a palette with both the bare ids are wrapped in a compound under the empty key.
std::vector<nbt::NBT> post5009(const std::vector<nbt::NBT> &legacy) {
  const bool mixed =
      std::any_of(legacy.begin(), legacy.end(), [](const nbt::NBT &entry) {
        return entry.contains("Properties");
      });

  std::vector<nbt::NBT> entries;

  for (const auto &entry : legacy) {
    const nbt::NBT id(entry["Name"].get<nbt::NBT::tag_string_t>());

    if (!entry.contains("Properties")) {
      entries.push_back(mixed ? nbt::NBT(nbt::NBT::tag_compound_t({{"", id}}))
                              : id);
      continue;
    }

    nbt::NBT properties = entry["Properties"];
    properties.set_name("properties");

    nbt::NBT::tag_compound_t modern;
    modern["id"] = nbt::NBT(id.get<nbt::NBT::tag_string_t>(), "id");
    modern["properties"] = properties;
    entries.push_back(nbt::NBT(modern));
  }

  return entries;
}
} // namespace

bool Section::to_nbt(nbt::NBT &section, const int dataVersion) const {
  if (dataVersion < 1628) {
    logger::error("Writing sections of DataVersion {} is not supported",
                  dataVersion);
    return false;
  }

  if (palette.empty())
    return true;

  // Drop the palette entries no block uses, keeping the order of the others
  std::vector<bool> used(palette.size(), false);

  for (const auto index : blocks) {
    if (index >= palette.size()) {
      logger::error("Section {}: block is undefined in palette", Y);
      return false;
    }

    used[index] = true;
  }

  std::vector<block_array::value_type> remap(palette.size());
  std::vector<nbt::NBT> compacted;

  for (size_t i = 0; i < palette.size(); i++) {
    if (used[i]) {
      remap[i] = compacted.size();
      compacted.push_back(palette[i]);
    }
  }

  block_array indexes;
  for (size_t i = 0; i < blocks.size(); i++)
    indexes[i] = remap[blocks[i]];

  // The length in bits of a block is the log2 of the palette's size or 4, whichever is greatest
  uint8_t bits = 4;
  while ((size_t(1) << bits) < compacted.size())
    bits++;

  if (dataVersion < 2840) {
    section["Palette"] = nbt::NBT(compacted, "Palette");
    section["BlockStates"] =
        nbt::NBT(pack(indexes, bits, dataVersion < 2534), "BlockStates");
    return true;
  }

  if (!section.contains("block_states") ||
      !section["block_states"].is_compound())
    section["block_states"] =
        nbt::NBT(nbt::NBT::tag_compound_t(), "block_states");

  nbt::NBT &block_states = section["block_states"];
  block_states["palette"] = nbt::NBT(
      dataVersion >= 5009 ? post5009(compacted) : compacted, "palette");

  // Since 1.19, a section filled with a single block has no data array
  if (dataVersion >= 3100 && compacted.size() == 1)
    block_states.erase("data");
  else
    block_states["data"] = nbt::NBT(pack(indexes, bits, false), "data");

  return true;
}
