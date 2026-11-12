#include "../src/biome.h"
#include <gtest/gtest.h>

TEST(TestBiome, TestKnown) {
  ASSERT_TRUE(mcmap::biomes::is_known("minecraft:plains"));
  ASSERT_TRUE(mcmap::biomes::is_known("minecraft:sulfur_caves"));
  ASSERT_FALSE(mcmap::biomes::is_known("plains"));
  ASSERT_FALSE(mcmap::biomes::is_known("minecraft:nonexistent"));
}

TEST(TestBiome, TestSorted) {
  for (size_t i = 1; i < mcmap::biomes::known.size(); i++)
    ASSERT_LT(std::string(mcmap::biomes::known[i - 1]),
              std::string(mcmap::biomes::known[i]));
}

TEST(TestBiome, TestSuggestTypo) {
  const auto hints = mcmap::biomes::suggest("minecraft:plians");
  ASSERT_FALSE(hints.empty());
  ASSERT_EQ(hints[0], "minecraft:plains");
}

TEST(TestBiome, TestSuggestWithoutNamespace) {
  const auto hints = mcmap::biomes::suggest("cherry_grove");
  ASSERT_EQ(hints, std::vector<std::string>({"minecraft:cherry_grove"}));
}

TEST(TestBiome, TestSuggestPart) {
  const auto hints = mcmap::biomes::suggest("frozen_ocean");
  ASSERT_EQ(hints, std::vector<std::string>({"minecraft:frozen_ocean",
                                             "minecraft:deep_frozen_ocean"}));
}

TEST(TestBiome, TestSuggestExactFirst) {
  const auto hints = mcmap::biomes::suggest("ocean");
  ASSERT_GT(hints.size(), 1);
  ASSERT_EQ(hints[0], "minecraft:ocean");
}

TEST(TestBiome, TestSuggestNamespacedTypo) {
  const auto hints = mcmap::biomes::suggest("minecraft:snowy_plain");
  ASSERT_EQ(hints, std::vector<std::string>({"minecraft:snowy_plains"}));
}

TEST(TestBiome, TestSuggestNothing) {
  ASSERT_TRUE(mcmap::biomes::suggest("mymod:crystal_wastes").empty());
}
