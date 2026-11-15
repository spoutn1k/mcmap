#include "../src/block_state.h"
#include <gtest/gtest.h>

namespace {
nbt::NBT compound(const nbt::NBT::tag_compound_t &elements) {
  return nbt::NBT(elements);
}

nbt::NBT string(const std::string &value, const std::string &name = "") {
  return nbt::NBT(value, name);
}
} // namespace

TEST(TestBlockState, TestReadLegacy) {
  BlockState state = BlockState::from_nbt(compound(
      {{"Name", string("minecraft:oak_log", "Name")},
       {"Properties",
        nbt::NBT(nbt::NBT::tag_compound_t({{"axis", string("x", "axis")}}),
                 "Properties")}}));

  ASSERT_TRUE(state.recognized);
  ASSERT_EQ(state.id, "minecraft:oak_log");
  ASSERT_EQ(state.property("axis"), "x");
  ASSERT_EQ(state.property("missing", "fallback"), "fallback");
  ASSERT_TRUE(state.extra.empty());
}

TEST(TestBlockState, TestReadModern) {
  BlockState bare = BlockState::from_nbt(string("minecraft:stone"));
  BlockState wrapped =
      BlockState::from_nbt(compound({{"", string("minecraft:stone", "")}}));
  BlockState full = BlockState::from_nbt(compound(
      {{"id", string("minecraft:oak_log", "id")},
       {"properties",
        nbt::NBT(nbt::NBT::tag_compound_t({{"axis", string("x", "axis")}}),
                 "properties")}}));

  ASSERT_EQ(bare, BlockState("minecraft:stone"));
  ASSERT_EQ(wrapped, BlockState("minecraft:stone"));
  ASSERT_EQ(full.id, "minecraft:oak_log");
  ASSERT_EQ(full.property("axis"), "x");
}

TEST(TestBlockState, TestUnrecognized) {
  nbt::NBT entry = compound({{"what", string("is this", "what")}});
  BlockState state = BlockState::from_nbt(entry);

  ASSERT_FALSE(state.recognized);
  ASSERT_EQ(state.id, "minecraft:air");
  ASSERT_EQ(state.to_nbt_pre5009()["what"].get<std::string>(), "is this");
  ASSERT_EQ(state.to_nbt_post5009()["what"].get<std::string>(), "is this");
  ASSERT_FALSE(state.can_be_bare());
}

TEST(TestBlockState, TestWriteLayouts) {
  BlockState plain("minecraft:stone"), log("minecraft:oak_log");
  log.properties["axis"] = string("x", "axis");
  log.extra["custom"] = nbt::NBT(int8_t(1), "custom");

  nbt::NBT legacy = log.to_nbt_pre5009();
  ASSERT_EQ(legacy["Name"].get<std::string>(), "minecraft:oak_log");
  ASSERT_EQ(legacy["Properties"]["axis"].get<std::string>(), "x");
  ASSERT_EQ(legacy["custom"].get<int8_t>(), 1);
  ASSERT_EQ(plain.to_nbt_pre5009()["Name"].get<std::string>(),
            "minecraft:stone");

  nbt::NBT modern = log.to_nbt_post5009();
  ASSERT_EQ(modern["id"].get<std::string>(), "minecraft:oak_log");
  ASSERT_EQ(modern["properties"]["axis"].get<std::string>(), "x");
  ASSERT_EQ(modern["custom"].get<int8_t>(), 1);

  ASSERT_TRUE(plain.to_nbt_post5009(true).is_string());
  ASSERT_EQ(plain.to_nbt_post5009()[""].get<std::string>(), "minecraft:stone");

  ASSERT_EQ(BlockState::from_nbt(legacy), log);
  ASSERT_EQ(BlockState::from_nbt(modern), log);
}

TEST(TestBlockState, TestBareRefused) {
  // Asking for a bare id does not drop the properties of an entry that cannot be bare
  BlockState log("minecraft:oak_log");
  log.properties["axis"] = string("x", "axis");

  nbt::NBT entry = log.to_nbt_post5009(true);
  ASSERT_TRUE(entry.is_compound());
  ASSERT_EQ(BlockState::from_nbt(entry), log);
}

TEST(TestBlockState, TestEquality) {
  BlockState stairs("minecraft:oak_stairs"), reordered("minecraft:oak_stairs");
  stairs.properties["facing"] = string("north", "facing");
  stairs.properties["half"] = string("top", "half");
  reordered.properties["half"] = string("top", "half");
  reordered.properties["facing"] = string("north", "facing");

  // The order of the properties does not matter
  ASSERT_EQ(stairs, reordered);

  reordered.properties["half"] = string("bottom", "half");
  ASSERT_NE(stairs, reordered);

  // Extra keys whose hashes used to be equal
  BlockState a("minecraft:stone"), b("minecraft:stone");
  a.extra["a"] = nbt::NBT(int32_t(0), "a");
  a.extra["b"] = nbt::NBT(int32_t(1), "b");
  b.extra["a"] = nbt::NBT(int32_t(1), "a");
  b.extra["b"] = nbt::NBT(int32_t(2), "b");
  ASSERT_NE(a, b);

  ASSERT_EQ(BlockState::from_nbt(nbt::NBT(int8_t(7))),
            BlockState::from_nbt(nbt::NBT(int8_t(7))));
  ASSERT_NE(BlockState::from_nbt(nbt::NBT(int8_t(7))),
            BlockState::from_nbt(nbt::NBT(int8_t(8))));
}
