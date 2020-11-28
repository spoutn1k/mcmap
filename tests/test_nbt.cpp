#include <gtest/gtest.h>
#include <logger.hpp>
#include <nbt/nbt.hpp>
#include <zlib.h>

#define BUFFERSIZE 2000000
#define NBT_FILE "tests/level.dat"

TEST(TestNBT, TestCreate) {
  nbt::NBT test;
  ASSERT_TRUE(test.empty());
}

TEST(TestNBT, TestCreateByte) {
  nbt::NBT test(uint8_t(4));

  ASSERT_FALSE(test.empty());
  ASSERT_EQ(test.get_type(), nbt::tag_type::tag_byte);
}

TEST(TestNBT, TestCreateString) {
  nbt::NBT test("key");

  ASSERT_FALSE(test.empty());
  ASSERT_EQ(test.get_type(), nbt::tag_type::tag_string);
}

TEST(TestNBT, TestCreateByteArray) {
  std::vector<int8_t> array = {98, 97, 10, 99};
  std::vector<uint8_t> uarray = {98, 97, 10, 99};
  std::vector<char> carray = {98, 97, 10, 99};

  nbt::NBT test(array);
  ASSERT_FALSE(test.empty());
  ASSERT_EQ(test.get_type(), nbt::tag_type::tag_byte_array);

  std::vector<int8_t> *data = test.get<std::vector<int8_t> *>();
  for (size_t i = 0; i < array.size(); i++)
    ASSERT_EQ(array.at(i), data->at(i));

  nbt::NBT utest(uarray);
  ASSERT_FALSE(test.empty());
  ASSERT_EQ(test.get_type(), nbt::tag_type::tag_byte_array);

  std::vector<int8_t> *udata = utest.get<std::vector<int8_t> *>();
  for (size_t i = 0; i < uarray.size(); i++)
    ASSERT_EQ(uarray.at(i), udata->at(i));

  nbt::NBT ctest(carray);
  ASSERT_FALSE(test.empty());
  ASSERT_EQ(test.get_type(), nbt::tag_type::tag_byte_array);

  std::vector<int8_t> *cdata = ctest.get<std::vector<int8_t> *>();
  for (size_t i = 0; i < carray.size(); i++)
    ASSERT_EQ(carray.at(i), cdata->at(i));
}

TEST(TestNBT, TestCreateIntArray) {
  std::vector<int> array = {98, 97, 10, 99};

  nbt::NBT test(array);
  ASSERT_FALSE(test.empty());
  ASSERT_EQ(test.get_type(), nbt::tag_type::tag_int_array);

  std::vector<int> *data = test.get<std::vector<int> *>();
  for (size_t i = 0; i < array.size(); i++)
    ASSERT_EQ(array.at(i), data->at(i));
}

TEST(TestNBT, TestCreateLongArray) {
  std::vector<long> array = {98, 97, 10, 99};

  nbt::NBT test(array);
  ASSERT_FALSE(test.empty());
  ASSERT_EQ(test.get_type(), nbt::tag_type::tag_long_array);

  std::vector<long> *data = test.get<std::vector<long> *>();
  for (size_t i = 0; i < array.size(); i++)
    ASSERT_EQ(array.at(i), data->at(i));
}

class TestNBTParse : public ::testing::Test {
protected:
  std::vector<uint8_t> buffer;
  size_t length;
  nbt::NBT level;

  TestNBTParse() {

    buffer.reserve(BUFFERSIZE);

    gzFile f;
    f = gzopen(NBT_FILE, "r");

    if (!f) {
      logger::error("Error opening " NBT_FILE ": {}\n", strerror(errno));
    } else {
      length = gzread(f, &buffer[0], sizeof(uint8_t) * BUFFERSIZE);
      level = nbt::NBT::parse(&buffer[0], length);
    }
  }
};

TEST_F(TestNBTParse, TestParse) {
  ASSERT_EQ(level.get_type(), nbt::tag_type::tag_compound);
  ASSERT_TRUE(level.size());
  ASSERT_EQ(level["Data"].get_type(), nbt::tag_type::tag_compound);
}

TEST_F(TestNBTParse, TestAccessByte) {
  uint8_t value = level["Data"]["Difficulty"].get<uint8_t>();

  ASSERT_TRUE(value);
}

TEST_F(TestNBTParse, TestAccessShort) {
  uint16_t value = level["Data"]["Difficulty"].get<uint16_t>();

  ASSERT_TRUE(value);
}

TEST_F(TestNBTParse, TestAccessInt) {
  int32_t value = level["Data"]["DataVersion"].get<int32_t>();

  ASSERT_TRUE(value);
}

TEST_F(TestNBTParse, TestAccessLong) {
  uint64_t value = level["Data"]["DayTime"].get<uint64_t>();

  ASSERT_TRUE(value);
}

TEST_F(TestNBTParse, TestAccessFloat) {
  float value = level["Data"]["BorderSafeZone"].get<float>();

  ASSERT_TRUE(value);
}

TEST_F(TestNBTParse, TestAccessDouble) {
  double value = level["Data"]["BorderSafeZone"].get<double>();

  ASSERT_TRUE(value);
}

TEST_F(TestNBTParse, TestCompoundIterator) {
  for (auto &el : level)
    ASSERT_EQ(el.get_name(), "Data");
}

TEST_F(TestNBTParse, TestListIterator) {
  nbt::NBT gateways = level["Data"]["DragonFight"]["Gateways"];
  std::vector<int> angles;

  for (auto &el : gateways) {
    ASSERT_EQ(el.get_name(), "");
    angles.push_back(el.get<int>());
  }

  ASSERT_TRUE(angles.size());
  ASSERT_NE(find(angles.begin(), angles.end(), 0), angles.end());
  ASSERT_NE(find(angles.begin(), angles.end(), 1), angles.end());
}
