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
    logger::level = logger::levels::DEEP_DEBUG;
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
  Section s(sections[0], dataVersion, colors);

  for (uint8_t x = 0; x < 16; x++)
    for (uint8_t y = 0; y < 16; y++)
      for (uint8_t z = 0; z < 16; z++)
        ASSERT_EQ(s.color_at(x, y, z)->primary.ALPHA, 0);
}

TEST_F(TestSection, TestColorPresence) {
  auto query = colors.find("minecraft:bedrock");
  bool presence = false;

  Colors::Block bedrock = query->second;
  Section s(sections[1], dataVersion, colors);

  ASSERT_NE(s.colors.size(), 0);

  for (auto &block_color : s.colors)
    presence |= (*block_color == bedrock);

  ASSERT_TRUE(presence);
}

TEST_F(TestSection, TestColor) {
  auto query = colors.find("minecraft:bedrock");

  Colors::Block bedrock = query->second;
  Section s(sections[1], dataVersion, colors);

  for (uint8_t x = 0; x < 16; x++)
    for (uint8_t z = 0; z < 16; z++)
      ASSERT_EQ(*s.color_at(x, 0, z), bedrock);
}

TEST_F(TestSection, TestMetadata) {
  Section s(sections[1], dataVersion, colors);

  const nbt::NBT &state = s.state_at(0, 0, 0);

  ASSERT_TRUE(state.contains("Name"));
  ASSERT_EQ(state["Name"].get<string>(), "minecraft:bedrock");
}
