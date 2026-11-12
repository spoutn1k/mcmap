#include "../src/include/nbt/parser.hpp"
#include "../src/section.h"
#include "./samples.h"
#include <gtest/gtest.h>

class TestSection : public ::testing::Test {
protected:
  Colors::Palette colors;
  nbt::NBT sections;
  int dataVersion;

  TestSection() {
    nbt::NBT chunk;
    nbt::parse(chunk_nbt, chunk_nbt_len, chunk);

    dataVersion = chunk["DataVersion"].get<int>();
    sections = chunk["Level"]["Sections"];

    Colors::load(&colors);
  }
};

TEST_F(TestSection, TestCreateEmpty) {
  Section s;

  for (uint8_t x = 0; x < 16; x++)
    for (uint8_t y = 0; y < 16; y++)
      for (uint8_t z = 0; z < 16; z++)
        ASSERT_EQ(s.color_at(x, y, z)->primary.ALPHA, 0);
}

TEST_F(TestSection, TestCreateFromEmpty) {
  Section s(sections[0], dataVersion);
  s.loadPalette(colors);

  for (uint8_t x = 0; x < 16; x++)
    for (uint8_t y = 0; y < 16; y++)
      for (uint8_t z = 0; z < 16; z++)
        ASSERT_EQ(s.color_at(x, y, z)->primary.ALPHA, 0);
}

TEST_F(TestSection, TestColorPresence) {
  auto query = colors.find("minecraft:bedrock");
  bool presence = false;

  Colors::Block bedrock = query->second;
  Section s(sections[1], dataVersion);
  s.loadPalette(colors);

  ASSERT_NE(s.colors.size(), 0);

  for (auto &block_color : s.colors)
    presence |= (*block_color == bedrock);

  ASSERT_TRUE(presence);
}

TEST_F(TestSection, TestColor) {
  auto query = colors.find("minecraft:bedrock");

  Colors::Block bedrock = query->second;
  Section s(sections[1], dataVersion);
  s.loadPalette(colors);

  for (uint8_t x = 0; x < 16; x++)
    for (uint8_t z = 0; z < 16; z++)
      ASSERT_EQ(*s.color_at(x, 0, z), bedrock);
}

TEST_F(TestSection, TestMetadata) {
  Section s(sections[1], dataVersion);
  s.loadPalette(colors);

  const nbt::NBT &state = s.state_at(0, 0, 0);

  ASSERT_TRUE(state.contains("Name"));
  ASSERT_EQ(state["Name"].get<string>(), "minecraft:bedrock");
}

namespace {
nbt::NBT biome_section(const std::vector<std::string> &names,
                       const std::vector<int64_t> &data) {
  std::vector<nbt::NBT> palette;
  for (const auto &name : names)
    palette.push_back(nbt::NBT(name));

  nbt::NBT::tag_compound_t biomes = {{"palette", nbt::NBT(palette, "palette")}};
  if (!data.empty())
    biomes["data"] = nbt::NBT(data, "data");

  return nbt::NBT(nbt::NBT::tag_compound_t(
      {{"Y", nbt::NBT(int8_t(0), "Y")}, {"biomes", nbt::NBT(biomes, "biomes")}}));

  }
} // namespace

TEST_F(TestSection, TestBiomeSingle) {
  Section s(biome_section({"minecraft:plains"}, {}), 3100);

  ASSERT_EQ(s.biome_palette, std::vector<std::string>({"minecraft:plains"}));
  for (uint8_t x = 0; x < 16; x++)
    for (uint8_t y = 0; y < 16; y++)
      for (uint8_t z = 0; z < 16; z++)
        ASSERT_EQ(s.biome_at(x, y, z), 0);
}

TEST_F(TestSection, TestBiomeCells) {
  // 3 entries need 2 bits: 32 cells per long, cell index x + 4z + 16y
  std::vector<int64_t> data(2, 0);
  const auto set = [&](size_t cell, uint64_t value) {
    data[cell / 32] |= int64_t(value << ((cell % 32) * 2));
  };
  set(0, 1);           // x 0-3, y 0-3, z 0-3
  set(3 + 4 * 2, 2);   // x 12-15, y 0-3, z 8-11
  set(1 + 16 * 3, 2);  // x 4-7, y 12-15, z 0-3

  Section s(biome_section({"minecraft:plains", "minecraft:forest",
                           "minecraft:desert"},
                          data),
            5009);

  ASSERT_EQ(s.biome_at(0, 0, 0), 1);
  ASSERT_EQ(s.biome_at(3, 3, 3), 1);
  ASSERT_EQ(s.biome_at(4, 0, 0), 0);
  ASSERT_EQ(s.biome_at(13, 2, 9), 2);
  ASSERT_EQ(s.biome_at(5, 15, 1), 2);
  ASSERT_EQ(s.biome_at(15, 15, 15), 0);
}

TEST_F(TestSection, TestBiomeLegacy) {
  Section s(sections[1], dataVersion);

  ASSERT_TRUE(s.biome_palette.empty());
}
