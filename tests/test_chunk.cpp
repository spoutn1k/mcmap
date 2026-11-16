#include "../src/chunk.h"
#include "../src/include/nbt/parser.hpp"
#include "./samples.h"
#include <gtest/gtest.h>

TEST(TestChunkStatic, TestAssert) {
  nbt::NBT chunk;
  ASSERT_EQ(mcmap::Chunk::assert_chunk(chunk), mcmap::Chunk::UNRECOGNISED);
  nbt::parse(chunk_nbt, chunk_nbt_len, chunk);
  ASSERT_EQ(mcmap::Chunk::assert_chunk(chunk), mcmap::Chunk::FULL);
}

TEST(TestChunkStatic, TestAssertIncomplete) {
  nbt::NBT chunk;
  nbt::parse(chunk_nbt, chunk_nbt_len, chunk);

  chunk["Level"]["Status"] = nbt::NBT(std::string("features"), "Status");
  ASSERT_EQ(mcmap::Chunk::assert_chunk(chunk), mcmap::Chunk::INCOMPLETE);

  // A status spelled differently than the full one is a change in the format
  chunk["Level"]["Status"] =
      nbt::NBT(std::string("minecraft:features"), "Status");
  ASSERT_EQ(mcmap::Chunk::assert_chunk(chunk), mcmap::Chunk::UNRECOGNISED);

  chunk["Level"].erase("Status");
  ASSERT_EQ(mcmap::Chunk::assert_chunk(chunk), mcmap::Chunk::UNRECOGNISED);
}

class TestChunkCoordinates : public ::testing::Test {
protected:
  mcmap::Chunk::coordinates pos;
  TestChunkCoordinates() : pos({5, -8}) {}
};

TEST_F(TestChunkCoordinates, TestCreate) {
  ASSERT_EQ(pos.x, 5);
  ASSERT_EQ(pos.z, -8);
}

TEST_F(TestChunkCoordinates, TestAdd) {
  mcmap::Chunk::coordinates sum = pos + mcmap::Chunk::coordinates({-5, 8});

  ASSERT_EQ(sum.x, 0);
  ASSERT_EQ(sum.z, 0);
}

TEST_F(TestChunkCoordinates, TestEqual) {
  mcmap::Chunk::coordinates other = {0, 0};
  other.x = 5;
  other.z = -8;

  ASSERT_EQ(pos, other);
}

TEST_F(TestChunkCoordinates, TestOrder) {
  for (int i = -128; i < 129; i++) {
    for (int j = -128; j < 129; j++) {
      mcmap::Chunk::coordinates x = {i, j}, y = {j, i};
      // Ensure only one of x<y, y<x, x==y is true at any time
      ASSERT_NE((x < y) != (y < x), x == y);
    }
  }
}

TEST_F(TestChunkCoordinates, TestModulo) {
  mcmap::Chunk::coordinates trunc = pos % 7;
  ASSERT_EQ(trunc.x, 5);
  ASSERT_EQ(trunc.z, -1);
}

class TestChunk : public ::testing::Test {
protected:
  Colors::Palette colors;
  mcmap::Chunk chunk;
  TestChunk() {
    Colors::load(&colors);
    nbt::NBT data;
    nbt::parse(chunk_nbt, chunk_nbt_len, data);

    chunk = mcmap::Chunk(data, colors, {0, 0});
  }
};

TEST_F(TestChunk, TestValid) {
  ASSERT_TRUE(chunk.data_version);
  ASSERT_TRUE(chunk.valid());
}
TEST_F(TestChunk, TestCreate) {
  mcmap::Chunk chunk;

  ASSERT_FALSE(chunk.valid());
}
