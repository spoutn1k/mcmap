#include "./samples.h"
#include <gtest/gtest.h>
#include <nbt/hash.hpp>
#include <nbt/parser.hpp>
#include <nbt/writer.hpp>

namespace {
bool round_trip(const nbt::NBT &data, nbt::NBT &result) {
  std::vector<uint8_t> buffer(chunk_nbt_len * 2);
  nbt::io::ByteStreamWriter output(buffer.data(), buffer.size());

  if (!nbt::put(output, data))
    return false;

  return nbt::parse(buffer.data(), output.total_written, result);
}
} // namespace

TEST(TestNBTWriter, TestRoundTripChunk) {
  nbt::NBT chunk, result;
  nbt::parse(chunk_nbt, chunk_nbt_len, chunk);

  ASSERT_TRUE(round_trip(chunk, result));
  ASSERT_EQ(nbt::hash(result), nbt::hash(chunk));
}

TEST(TestNBTWriter, TestRoundTripScalars) {
  nbt::NBT data(nbt::NBT::tag_compound_t({
      {"short", nbt::NBT(int16_t(-12345), "short")},
      {"int", nbt::NBT(int32_t(-123456789), "int")},
      {"long", nbt::NBT(int64_t(-1234567890123456789), "long")},
      {"float", nbt::NBT(1.5f, "float")},
      {"double", nbt::NBT(-2.25, "double")},
  }));
  nbt::NBT result;

  ASSERT_TRUE(round_trip(data, result));
  ASSERT_EQ(result["short"].get<int16_t>(), -12345);
  ASSERT_EQ(result["int"].get<int32_t>(), -123456789);
  ASSERT_EQ(result["long"].get<int64_t>(), -1234567890123456789);
  ASSERT_EQ(result["float"].get<float>(), 1.5f);
  ASSERT_EQ(result["double"].get<double>(), -2.25);
}

TEST(TestNBTWriter, TestBufferTooSmall) {
  nbt::NBT chunk;
  nbt::parse(chunk_nbt, chunk_nbt_len, chunk);

  std::vector<uint8_t> buffer(1024);
  nbt::io::ByteStreamWriter output(buffer.data(), buffer.size());

  ASSERT_FALSE(nbt::put(output, chunk));
}

TEST(TestNBTWriter, TestIdenticalBytes) {
  // Compounds keep their keys in order, so the output matches the source
  nbt::NBT chunk;
  nbt::parse(chunk_nbt, chunk_nbt_len, chunk);

  std::vector<uint8_t> buffer(chunk_nbt_len * 2);
  nbt::io::ByteStreamWriter output(buffer.data(), buffer.size());

  ASSERT_TRUE(nbt::put(output, chunk));
  ASSERT_EQ(output.total_written, chunk_nbt_len);
  ASSERT_TRUE(std::equal(chunk_nbt, chunk_nbt + chunk_nbt_len, buffer.begin()));
}

TEST(TestNBTWriter, TestCompoundOrder) {
  nbt::NBT compound(nbt::NBT::tag_compound_t({
      {"zebra", nbt::NBT(int8_t(1), "zebra")},
      {"apple", nbt::NBT(int8_t(2), "apple")},
  }));
  compound["mango"] = nbt::NBT(int8_t(3), "mango");
  // Assigning an existing key keeps its position
  compound["zebra"] = nbt::NBT(int8_t(4), "zebra");
  // Duplicates are ignored on insertion, like std::map
  compound.insert({"apple", nbt::NBT(int8_t(5), "apple")});

  std::vector<std::string> keys;
  for (const auto &element : *compound.get<const nbt::NBT::tag_compound_t *>())
    keys.push_back(element.first);

  ASSERT_EQ(keys, std::vector<std::string>({"zebra", "apple", "mango"}));
  ASSERT_EQ(compound["zebra"].get<int8_t>(), 4);
  ASSERT_EQ(compound["apple"].get<int8_t>(), 2);

  compound.erase("apple");
  ASSERT_FALSE(compound.contains("apple"));
  ASSERT_EQ(compound.size(), 2);
}

TEST(TestNBTWriter, TestKeys) {
  // Elements are written under their key in the compound, whatever their own name
  nbt::NBT data = nbt::NBT(nbt::NBT::tag_compound_t());
  data["unnamed"] = nbt::NBT(int8_t(5));
  data["renamed"] = nbt::NBT(int8_t(6), "other");
  // Compounds created on access have no name either
  data["outer"]["inner"] = nbt::NBT(int32_t(7));
  nbt::NBT result;

  ASSERT_TRUE(round_trip(data, result));
  ASSERT_EQ(result.size(), 3);
  ASSERT_EQ(result["unnamed"].get<int8_t>(), 5);
  ASSERT_EQ(result["renamed"].get<int8_t>(), 6);
  ASSERT_EQ(result["outer"]["inner"].get<int32_t>(), 7);
}

TEST(TestNBTWriter, TestMixedList) {
  // The type of a list's elements is written once, so a list mixing types cannot be written
  nbt::NBT data(nbt::NBT::tag_compound_t(
      {{"list", nbt::NBT(nbt::NBT::tag_list_t({nbt::NBT(int8_t(1)),
                                               nbt::NBT(std::string("two"))}),
                         "list")}}));

  std::vector<uint8_t> buffer(1024);
  nbt::io::ByteStreamWriter output(buffer.data(), buffer.size());

  ASSERT_FALSE(nbt::put(output, data));
}

TEST(TestNBTWriter, TestRoundTripArrays) {
  // Longer than the batches arrays are written in, with values of both signs
  std::vector<int8_t> bytes(10000);
  std::vector<int32_t> ints(3000);
  std::vector<int64_t> longs(3000);

  for (size_t i = 0; i < bytes.size(); i++)
    bytes[i] = int8_t(i * 37);
  for (size_t i = 0; i < ints.size(); i++)
    ints[i] = int32_t(uint32_t(i) * 2654435761u);
  for (size_t i = 0; i < longs.size(); i++)
    longs[i] = int64_t(uint64_t(i) * 0x9e3779b97f4a7c15ULL);

  nbt::NBT data(nbt::NBT::tag_compound_t({
      {"bytes", nbt::NBT(bytes, "bytes")},
      {"ints", nbt::NBT(ints, "ints")},
      {"longs", nbt::NBT(longs, "longs")},
  }));
  nbt::NBT result;

  ASSERT_TRUE(round_trip(data, result));
  ASSERT_EQ(*result["bytes"].get<const nbt::NBT::tag_byte_array_t *>(), bytes);
  ASSERT_EQ(*result["ints"].get<const nbt::NBT::tag_int_array_t *>(), ints);
  ASSERT_EQ(*result["longs"].get<const nbt::NBT::tag_long_array_t *>(), longs);
}
