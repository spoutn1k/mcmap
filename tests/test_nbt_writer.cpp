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
