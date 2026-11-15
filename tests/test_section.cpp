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

  const BlockState &state = s.state_at(0, 0, 0);

  ASSERT_TRUE(state.recognized);
  ASSERT_EQ(state.id, "minecraft:bedrock");
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

  return nbt::NBT(
      nbt::NBT::tag_compound_t({{"Y", nbt::NBT(int8_t(0), "Y")},
                                {"biomes", nbt::NBT(biomes, "biomes")}}));
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
  set(0, 1);          // x 0-3, y 0-3, z 0-3
  set(3 + 4 * 2, 2);  // x 12-15, y 0-3, z 8-11
  set(1 + 16 * 3, 2); // x 4-7, y 12-15, z 0-3

  Section s(
      biome_section(
          {"minecraft:plains", "minecraft:forest", "minecraft:desert"}, data),
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

namespace {
BlockState block(const std::string &name) { return BlockState(name); }

nbt::NBT empty_section() {
  return nbt::NBT(nbt::NBT::tag_compound_t({{"Y", nbt::NBT(int8_t(0), "Y")}}));
}

// Compare two sections block by block, as their palettes may be ordered differently
void expect_same_blocks(const Section &a, const Section &b) {
  for (uint8_t x = 0; x < 16; x++)
    for (uint8_t y = 0; y < 16; y++)
      for (uint8_t z = 0; z < 16; z++)
        ASSERT_TRUE(a.state_at(x, y, z) == b.state_at(x, y, z));
}

const int versions[] = {1628, 2534, 2840, 3100, 5009};
} // namespace

TEST_F(TestSection, TestToNbtRoundTrip) {
  for (const auto &raw : sections) {
    Section original(raw, dataVersion);
    if (original.palette.empty())
      continue;

    for (const int version : versions) {
      nbt::NBT written = empty_section();
      ASSERT_TRUE(original.to_nbt(written, version));

      Section result(written, version);
      expect_same_blocks(original, result);
    }
  }
}

TEST_F(TestSection, TestToNbtWideIndexes) {
  // 40 entries need 6 bits, which do not divide 64, and 4096 blocks do not fill the last long exactly
  Section original;
  for (int i = 0; i < 40; i++)
    original.palette.push_back(block("minecraft:block_" + std::to_string(i)));
  for (size_t i = 0; i < original.blocks.size(); i++)
    original.blocks[i] = (i * 7) % 40;

  for (const int version : versions) {
    nbt::NBT written = empty_section();
    ASSERT_TRUE(original.to_nbt(written, version));

    Section result(written, version);
    expect_same_blocks(original, result);
  }
}

TEST_F(TestSection, TestToNbtDropsUnused) {
  Section original;
  original.palette = {block("minecraft:air"), block("minecraft:stone"),
                      block("minecraft:dirt")};
  original.blocks.fill(2);

  // Since 1.18, single block sections have no data
  for (const int version : {2840, 3100}) {
    nbt::NBT written = empty_section();
    ASSERT_TRUE(original.to_nbt(written, version));
    ASSERT_EQ(written["block_states"]["palette"].size(), 1);
    ASSERT_FALSE(written["block_states"].contains("data"));

    Section result(written, version);
    expect_same_blocks(original, result);
  }

  // Before, the data is always there
  nbt::NBT written = empty_section();
  ASSERT_TRUE(original.to_nbt(written, 2534));
  ASSERT_TRUE(written.contains("BlockStates"));
}

TEST_F(TestSection, TestToNbtUnsupported) {
  Section original(sections[1], dataVersion);
  nbt::NBT written = empty_section();

  ASSERT_FALSE(original.to_nbt(written, 1000));
}

TEST_F(TestSection, TestToNbtPost5009Palette) {
  Section original;
  original.palette = {block("minecraft:air"), block("minecraft:stone")};
  original.blocks[1] = 1;

  // Blocks without properties are bare ids
  nbt::NBT written = empty_section();
  ASSERT_TRUE(original.to_nbt(written, 5009));
  for (const auto &entry : written["block_states"]["palette"])
    ASSERT_TRUE(entry.is_string());

  // Mixed with blocks with properties, they are wrapped under the empty key
  BlockState slab = block("minecraft:stone_slab");
  slab.properties["type"] = nbt::NBT(std::string("top"), "type");
  original.palette.push_back(slab);
  original.blocks[2] = 2;

  written = empty_section();
  ASSERT_TRUE(original.to_nbt(written, 5009));
  const auto &palette = written["block_states"]["palette"];
  ASSERT_EQ(palette.size(), 3);
  for (const auto &entry : palette)
    ASSERT_TRUE(entry.is_compound());
  ASSERT_EQ(palette[0][""].get<std::string>(), "minecraft:air");
  ASSERT_EQ(palette[2]["id"].get<std::string>(), "minecraft:stone_slab");
  ASSERT_EQ(palette[2]["properties"]["type"].get<std::string>(), "top");

  Section result(written, 5009);
  expect_same_blocks(original, result);
}

TEST_F(TestSection, TestToNbtKeepsUnknownData) {
  // Keys and entries this code does not know about survive a conversion in both directions
  Section original;
  BlockState tagged = block("minecraft:stone");
  tagged.extra["custom"] = nbt::NBT(int32_t(42), "custom");
  original.palette = {
      block("minecraft:air"), tagged,
      BlockState::from_nbt(nbt::NBT(nbt::NBT::tag_compound_t(
          {{"block", nbt::NBT(std::string("minecraft:stone"), "block")}})))};
  original.blocks[1] = 1;
  original.blocks[2] = 2;
  ASSERT_FALSE(original.palette[2].recognized);

  for (const int version : {2840, 5009}) {
    nbt::NBT written = empty_section();
    ASSERT_TRUE(original.to_nbt(written, version));

    Section result(written, version);
    expect_same_blocks(original, result);
    ASSERT_EQ(result.palette[1].extra.at("custom").get<int32_t>(), 42);
    ASSERT_EQ(result.palette[2].raw["block"].get<std::string>(),
              "minecraft:stone");
  }

  // Converting from one format to the other keeps everything too
  nbt::NBT modern = empty_section();
  ASSERT_TRUE(original.to_nbt(modern, 5009));
  Section read(modern, 5009);

  nbt::NBT legacy = empty_section();
  ASSERT_TRUE(read.to_nbt(legacy, 2840));
  Section result(legacy, 2840);
  expect_same_blocks(original, result);
}

TEST_F(TestSection, TestToNbtMixedPalette) {
  // An unrecognized entry that is not a compound cannot share the palette's list with the others
  Section original;
  original.palette = {block("minecraft:air"),
                      BlockState::from_nbt(nbt::NBT(int8_t(7)))};
  original.blocks[1] = 1;

  for (const int version : {2534, 2840, 5009}) {
    nbt::NBT written = empty_section();
    ASSERT_FALSE(original.to_nbt(written, version));
    // The section is left as it was
    ASSERT_EQ(written.size(), 1);
  }
}
