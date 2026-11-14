#include "../src/region.h"
#include "./samples.h"
#include <fstream>
#include <gtest/gtest.h>
#include <nbt/hash.hpp>
#include <nbt/parser.hpp>

class TestRegion : public ::testing::Test {
protected:
  fs::path file;
  nbt::NBT chunk;

  TestRegion() {
    file = fs::temp_directory_path() / "mcmap_test_region.mca";
    fs::remove(file);

    nbt::parse(chunk_nbt, chunk_nbt_len, chunk);
  }

  ~TestRegion() { fs::remove(file); }

  // A chunk that does not compress, of a size close to `size` once compressed
  static nbt::NBT incompressible(size_t size) {
    std::vector<int8_t> payload(size);
    for (auto &byte : payload)
      byte = rand();

    return nbt::NBT(
        nbt::NBT::tag_compound_t({{"payload", nbt::NBT(payload, "payload")}}));
  }
};

TEST_F(TestRegion, TestIndex) {
  ASSERT_EQ(Region::index({0, 0}), 0);
  ASSERT_EQ(Region::index({1, 0}), 1);
  ASSERT_EQ(Region::index({0, 1}), 32);
  ASSERT_EQ(Region::index({31, 31}), 1023);
  // Absolute coordinates, including negative ones
  ASSERT_EQ(Region::index({33, -31}), 33);
  ASSERT_EQ(Region::index({-1, -1}), 1023);
}

TEST_F(TestRegion, TestMissingChunk) {
  Region region(file);
  nbt::NBT data;

  ASSERT_FALSE(region.get_chunk({0, 0}, data));
}

TEST_F(TestRegion, TestRoundTrip) {
  Region region(file);
  ASSERT_TRUE(region.add_chunk({1, 0}, chunk));
  ASSERT_NE(region.locations[1].raw_data, 0);
  ASSERT_EQ(region.locations[32].raw_data, 0);

  Region reopened(file);
  nbt::NBT data;
  ASSERT_TRUE(reopened.get_chunk({1, 0}, data));
  ASSERT_EQ(nbt::hash(data), nbt::hash(chunk));
}

TEST_F(TestRegion, TestSectorBoundaries) {
  // Payloads whose compressed size crosses a 4K boundary, header included
  srand(0);

  for (size_t size = 4000; size < 4150; size++) {
    fs::remove(file);
    nbt::NBT payload = incompressible(size);

    Region region(file);
    ASSERT_TRUE(region.add_chunk({0, 0}, payload));

    Region reopened(file);
    nbt::NBT data;
    ASSERT_TRUE(reopened.get_chunk({0, 0}, data)) << "payload of " << size;
    ASSERT_EQ(nbt::hash(data), nbt::hash(payload));
    ASSERT_EQ(fs::file_size(file) % 4096, 0);
  }
}

TEST_F(TestRegion, TestLargeChunk) {
  // Bigger than 64K once compressed
  nbt::NBT payload = incompressible(100000);

  Region region(file);
  ASSERT_TRUE(region.add_chunk({0, 0}, payload));

  Region reopened(file);
  nbt::NBT data;
  ASSERT_TRUE(reopened.get_chunk({0, 0}, data));
  ASSERT_EQ(nbt::hash(data), nbt::hash(payload));
}

TEST_F(TestRegion, TestReplace) {
  nbt::NBT bigger = incompressible(20000);

  Region region(file);
  ASSERT_TRUE(region.add_chunk({0, 0}, chunk));
  ASSERT_TRUE(region.add_chunk({1, 0}, chunk));
  // Replacing a chunk with bigger data must not overwrite its neighbour
  ASSERT_TRUE(region.add_chunk({0, 0}, bigger));

  Region reopened(file);
  nbt::NBT first, second;
  ASSERT_TRUE(reopened.get_chunk({0, 0}, first));
  ASSERT_TRUE(reopened.get_chunk({1, 0}, second));
  ASSERT_EQ(nbt::hash(first), nbt::hash(bigger));
  ASSERT_EQ(nbt::hash(second), nbt::hash(chunk));
}

TEST_F(TestRegion, TestTimestamps) {
  Region region(file);
  region.timestamps[5] = 12345;
  ASSERT_TRUE(region.add_chunk({0, 0}, chunk));
  ASSERT_NE(region.timestamps[0], 0);

  Region reopened(file);
  ASSERT_EQ(reopened.timestamps[0], region.timestamps[0]);
  ASSERT_EQ(reopened.timestamps[5], 12345);
}

TEST_F(TestRegion, TestTooBigToReadBack) {
  // Compresses to more than readers accept, while fitting in the sectors of a region
  nbt::NBT payload = incompressible(COMPRESSED_BUFFER + 50000);

  Region region(file);
  ASSERT_TRUE(region.add_chunk({0, 0}, chunk));
  ASSERT_FALSE(region.add_chunk({0, 0}, payload));

  // The previous chunk is still there
  Region reopened(file);
  nbt::NBT data;
  ASSERT_TRUE(reopened.get_chunk({0, 0}, data));
  ASSERT_EQ(nbt::hash(data), nbt::hash(chunk));
}

TEST_F(TestRegion, TestNotACompound) {
  // What a failed get_chunk leaves behind
  nbt::NBT empty;

  Region region(file);
  ASSERT_TRUE(region.add_chunk({0, 0}, chunk));
  ASSERT_FALSE(region.add_chunk({0, 0}, empty));
  ASSERT_FALSE(region.add_chunk({0, 0}, nbt::NBT(int32_t(7))));

  // The previous chunk is still there
  Region reopened(file);
  nbt::NBT data;
  ASSERT_TRUE(reopened.get_chunk({0, 0}, data));
  ASSERT_EQ(nbt::hash(data), nbt::hash(chunk));
}

TEST_F(TestRegion, TestUnreadableHeader) {
  // A file too short to hold a header is not mistaken for an empty region
  const std::vector<char> truncated(1000, 0x2a);
  std::ofstream(file, std::ios::binary)
      .write(truncated.data(), truncated.size());

  Region region(file);
  ASSERT_FALSE(region.header_valid);
  ASSERT_FALSE(region.add_chunk({0, 0}, chunk));
  ASSERT_FALSE(region.write_header());

  // The file is left as it was
  ASSERT_EQ(fs::file_size(file), truncated.size());
}

TEST_F(TestRegion, TestEmptyFile) {
  // An empty file is a region without chunks
  std::ofstream(file, std::ios::binary).close();

  Region region(file);
  ASSERT_TRUE(region.header_valid);
  ASSERT_TRUE(region.add_chunk({0, 0}, chunk));

  Region reopened(file);
  nbt::NBT data;
  ASSERT_TRUE(reopened.get_chunk({0, 0}, data));
  ASSERT_EQ(nbt::hash(data), nbt::hash(chunk));
}
