#include <gtest/gtest.h>
#include <nbt/nbt.hpp>

TEST(TestNBT, TestCreate) {
  nbt::NBT test;
  ASSERT_TRUE(test.empty());
}
