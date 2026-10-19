#pragma once

#include "./colors.h"
#include "./diagnostics.h"
#include <2DCoordinates.hpp>
#include <nbt/nbt.hpp>

struct Section {
  // Each block is encoded on 16 bits as an index in the palette - where only 12 bits are necessary
  using block_array = std::array<uint16_t, 4096>;

  // Each index in the array above points to a block in the palette and a color in the following array
  using color_array = std::vector<const Colors::Block *>;
  using light_array = std::array<uint8_t, 2048>;
  // Since 1.18, biomes are stored per cell of 4x4x4 blocks as an index in the biome palette
  using biome_array = std::array<uint8_t, 64>;

  // The vertical index of the section
  int8_t Y;
  // Coordinates of the parent chunk, used only for debug
  Coordinates parent_chunk_coordinates;

  block_array blocks;
  color_array colors;
  light_array lights;
  std::vector<nbt::NBT> palette;

  biome_array biomes;
  std::vector<std::string> biome_palette;

  block_array::value_type beaconIndex;

  Section();
  Section(const nbt::NBT &, const int, const Coordinates = {0, 0});
  Section(Section &&other) { *this = std::move(other); }

  Section &operator=(Section &&other) {
    Y = other.Y;
    parent_chunk_coordinates = other.parent_chunk_coordinates;

    beaconIndex = other.beaconIndex;

    blocks = std::move(other.blocks);
    lights = std::move(other.lights);
    colors = std::move(other.colors);
    palette = std::move(other.palette);
    biomes = std::move(other.biomes);
    biome_palette = std::move(other.biome_palette);
    return *this;
  }

  inline bool empty() const {
    // Check the state of the section by looking at its palette: a section with only air does not need to be rendered
    return palette.empty() ||
           (palette.size() == 1 &&
            palette[0]["Name"].get<std::string>() == "minecraft:air");
  }

  inline block_array::value_type block_at(uint8_t x, uint8_t y,
                                          uint8_t z) const {
    return blocks[x + 16 * z + 16 * 16 * y];
  }

  inline color_array::value_type color_at(uint8_t x, uint8_t y,
                                          uint8_t z) const {
    return colors[blocks[x + 16 * z + 16 * 16 * y]];
  }

  inline light_array::value_type light_at(uint8_t x, uint8_t y,
                                          uint8_t z) const {
    const uint16_t index = x + 16 * z + 16 * 16 * y;

    return (index % 2 ? lights[index / 2] >> 4 : lights[index / 2] & 0x0f);
  }

  inline biome_array::value_type biome_at(uint8_t x, uint8_t y,
                                          uint8_t z) const {
    return biomes[(x >> 2) + 4 * (z >> 2) + 4 * 4 * (y >> 2)];
  }

  inline const nbt::NBT &state_at(uint8_t x, uint8_t y, uint8_t z) const {
    return palette[blocks[x + 16 * z + 16 * 16 * y]];
  }

  // Write the blocks and palette to a raw section, in the format of the given
  // DataVersion. Returns false if the format is not supported.
  bool to_nbt(nbt::NBT &, const int) const;

  void loadPalette(const Colors::Palette &, Diagnostics::Report * = nullptr);

};
