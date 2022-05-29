#include <gtest/gtest.h>
#include <nbt/hash.hpp>

using SimpleTypes =
    testing::Types<int8_t, int16_t, int32_t, int64_t, float, double>;
template <class> struct TestNBTHashScalar : testing::Test {};
TYPED_TEST_SUITE(TestNBTHashScalar, SimpleTypes);

TYPED_TEST(TestNBTHashScalar, hashnstuff) {
  using test_type = TypeParam;
  nbt::NBT min(std::numeric_limits<test_type>::min()),
      max(std::numeric_limits<test_type>::max());

  ASSERT_TRUE(nbt::hash(min));
  ASSERT_TRUE(nbt::hash(max));
  ASSERT_NE(nbt::hash(min), nbt::hash(max));

  nbt::NBT other(std::numeric_limits<test_type>::max());
  ASSERT_EQ(nbt::hash(max), nbt::hash(other));
}

using ArrayScalars = testing::Types<uint8_t, int32_t, int64_t>;
template <class> struct TestNBTHashScalarArray : testing::Test {};
TYPED_TEST_SUITE(TestNBTHashScalarArray, ArrayScalars);

TYPED_TEST(TestNBTHashScalarArray, hashnstuff) {
  using test_type = TypeParam;
  std::vector<test_type> bytes = {0, 1, 2}, bigger = {3, 4, 5},
                         other = {0, 1, 2}, mixed = {2, 1, 0};

  nbt::NBT a(bytes), b(bigger), c(other), d(mixed);

  ASSERT_TRUE(nbt::hash(a));

  ASSERT_NE(nbt::hash(a), nbt::hash(b));
  ASSERT_EQ(nbt::hash(a), nbt::hash(c));
  ASSERT_NE(nbt::hash(a), nbt::hash(d));
  ASSERT_NE(nbt::hash(b), nbt::hash(c));
  ASSERT_NE(nbt::hash(b), nbt::hash(d));
  ASSERT_NE(nbt::hash(c), nbt::hash(d));
}

TEST(TestNBTHash, TestHashString) {
  nbt::NBT key("key"), value("value"), param("value");

  ASSERT_TRUE(nbt::hash(key));
  ASSERT_TRUE(nbt::hash(value));
  ASSERT_TRUE(nbt::hash(param));

  ASSERT_NE(nbt::hash(key), nbt::hash(value));
  ASSERT_NE(nbt::hash(key), nbt::hash(param));
  ASSERT_EQ(nbt::hash(value), nbt::hash(param));
}

TEST(TestNBTHash, TestHashList) {
  nbt::NBT a = nbt::NBT({nbt::NBT("This"), nbt::NBT("is"), nbt::NBT("a"),
                         nbt::NBT("test")}),
           b = nbt::NBT({nbt::NBT("Another"), nbt::NBT("set"), nbt::NBT("of"),
                         nbt::NBT("strings")}),
           c = nbt::NBT({nbt::NBT("This"), nbt::NBT("is"), nbt::NBT("a"),
                         nbt::NBT("test")}),
           d = nbt::NBT({nbt::NBT("This"), nbt::NBT("test"), nbt::NBT("is"),
                         nbt::NBT("a")});
  ASSERT_TRUE(nbt::hash(a));
  ASSERT_TRUE(nbt::hash(b));
  ASSERT_TRUE(nbt::hash(c));
  ASSERT_TRUE(nbt::hash(d));

  ASSERT_NE(nbt::hash(a), nbt::hash(b));
  ASSERT_EQ(nbt::hash(a), nbt::hash(c));
  ASSERT_NE(nbt::hash(a), nbt::hash(d));
  ASSERT_NE(nbt::hash(b), nbt::hash(c));
  ASSERT_NE(nbt::hash(b), nbt::hash(d));
  ASSERT_NE(nbt::hash(c), nbt::hash(d));
}

TEST(TestNBTHash, TestHashCompound) {
  nbt::NBT
      a = nbt::NBT({{"block_states", nbt::NBT(std::vector<long>({0, 0, 0}))},
                    {"palette", nbt::NBT("air")}}),

      b = nbt::NBT(
          {{"block_states", nbt::NBT(std::vector<long>({1, 0, 0}))},
           {"palette", nbt::NBT("air")}}),

      c = nbt::NBT({{"block_states", nbt::NBT(std::vector<long>({0, 0, 0}))},
                    {"palette", nbt::NBT("stone")}}),

      d = nbt::NBT({{"block_states", nbt::NBT(std::vector<long>({0, 0, 0}))},
                    {"palette", nbt::NBT("air")}}),

      e = nbt::NBT({{"palette", nbt::NBT("air")},
                    {"block_states", nbt::NBT(std::vector<long>({0, 0, 0}))}});

  ASSERT_TRUE(nbt::hash(a));
  ASSERT_TRUE(nbt::hash(b));
  ASSERT_TRUE(nbt::hash(c));
  ASSERT_TRUE(nbt::hash(d));
  ASSERT_TRUE(nbt::hash(e));

  ASSERT_NE(nbt::hash(a), nbt::hash(b));
  ASSERT_NE(nbt::hash(a), nbt::hash(c));
  ASSERT_EQ(nbt::hash(a), nbt::hash(d));
  ASSERT_EQ(nbt::hash(a), nbt::hash(e));
  ASSERT_NE(nbt::hash(b), nbt::hash(c));
  ASSERT_NE(nbt::hash(b), nbt::hash(d));
  ASSERT_NE(nbt::hash(b), nbt::hash(e));
  ASSERT_NE(nbt::hash(c), nbt::hash(d));
  ASSERT_NE(nbt::hash(c), nbt::hash(e));
  ASSERT_EQ(nbt::hash(d), nbt::hash(e));
}
