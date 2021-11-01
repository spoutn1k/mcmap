#include "./samples.h"
#include <gtest/gtest.h>
#include <nbt/parser.hpp>

#define BUFFERSIZE 2000000

TEST(TestNBT, TestCreateName) {
  nbt::NBT test;
  test.set_name("test NBT");
  ASSERT_EQ(test.get_name(), "test NBT");
}

TEST(TestNBT, TestCreateEnd) {
  nbt::NBT test;
  ASSERT_TRUE(test.empty());
}

TEST(TestNBT, TestCreateByte) {
  nbt::NBT test(std::numeric_limits<uint8_t>::max());

  ASSERT_FALSE(test.empty());
  ASSERT_EQ(test.get_type(), nbt::tag_type::tag_byte);
  ASSERT_EQ(test.get<uint8_t>(), std::numeric_limits<uint8_t>::max());
}

TEST(TestNBT, TestCreateShort) {
  nbt::NBT test(std::numeric_limits<uint16_t>::max());

  ASSERT_FALSE(test.empty());
  ASSERT_EQ(test.get_type(), nbt::tag_type::tag_short);
  ASSERT_EQ(test.get<uint16_t>(), std::numeric_limits<uint16_t>::max());
}

TEST(TestNBT, TestCreateInt) {
  nbt::NBT test(std::numeric_limits<uint32_t>::max());

  ASSERT_FALSE(test.empty());
  ASSERT_EQ(test.get_type(), nbt::tag_type::tag_int);
  ASSERT_EQ(test.get<uint32_t>(), std::numeric_limits<uint32_t>::max());
}

TEST(TestNBT, TestCreateLong) {
  nbt::NBT test(std::numeric_limits<uint64_t>::max());

  ASSERT_FALSE(test.empty());
  ASSERT_EQ(test.get_type(), nbt::tag_type::tag_long);
  ASSERT_EQ(test.get<uint64_t>(), std::numeric_limits<uint64_t>::max());
}

TEST(TestNBT, TestCreateFloat) {
  nbt::NBT test(std::numeric_limits<float>::max());

  ASSERT_FALSE(test.empty());
  ASSERT_EQ(test.get_type(), nbt::tag_type::tag_float);
  ASSERT_EQ(test.get<float>(), std::numeric_limits<float>::max());
}

TEST(TestNBT, TestCreateDouble) {
  nbt::NBT test(std::numeric_limits<double>::max());

  ASSERT_FALSE(test.empty());
  ASSERT_EQ(test.get_type(), nbt::tag_type::tag_double);
  ASSERT_EQ(test.get<double>(), std::numeric_limits<double>::max());
}

TEST(TestNBT, TestCreateString) {
  nbt::NBT test("key");

  ASSERT_FALSE(test.empty());
  ASSERT_EQ(test.get_type(), nbt::tag_type::tag_string);
  ASSERT_EQ(test.get<std::string>(), "key");
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

TEST(TestNBT, TestCreateList) {
  std::vector<std::string> elements = {"This", "is", "a", "test"};
  std::vector<nbt::NBT> list;

  for (auto &value : elements)
    list.push_back(nbt::NBT(value));

  nbt::NBT test = nbt::NBT(list);

  ASSERT_FALSE(test.empty());
  ASSERT_EQ(test.get_type(), nbt::tag_type::tag_list);

  for (size_t i = 0; i < elements.size(); i++)
    ASSERT_EQ(elements[i], test[i].get<std::string>());
}

TEST(TestNBT, TestCreateCompound) {
  std::vector<std::string> elements = {"This", "is", "a", "test"};
  std::map<std::string, nbt::NBT> compound;

  for (auto &value : elements)
    compound[value] = (nbt::NBT(value));

  nbt::NBT test = nbt::NBT(compound);

  ASSERT_FALSE(test.empty());
  ASSERT_EQ(test.get_type(), nbt::tag_type::tag_compound);

  // Iterators seem to be broken, have to delve into that
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
  nbt::NBT level;

  TestNBTParse() { nbt::parse(level_nbt, level_nbt_len, level); }
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

TEST(TestNBTIterator, TestEndIterator) {
  nbt::NBT end;

  ASSERT_EQ(end.begin(), end.end());
}

TEST(TestNBTIterator, TestByteIterator) {
  auto value = std::numeric_limits<uint8_t>::max();
  nbt::NBT container(value);

  auto it = container.begin();

  ASSERT_NE(it, container.end());
  ASSERT_EQ(it->get<uint8_t>(), value);
  ASSERT_EQ(++it, container.end());
}

TEST(TestNBTIterator, TestShortIterator) {
  auto value = std::numeric_limits<uint16_t>::max();
  nbt::NBT container(value);

  auto it = container.begin();

  ASSERT_NE(it, container.end());
  ASSERT_EQ(it->get<uint16_t>(), value);
  ASSERT_EQ(++it, container.end());
}

TEST(TestNBTIterator, TestIntIterator) {
  auto value = std::numeric_limits<uint32_t>::max();
  nbt::NBT container(value);

  auto it = container.begin();

  ASSERT_NE(it, container.end());
  ASSERT_EQ(it->get<uint32_t>(), value);
  ASSERT_EQ(++it, container.end());
}

TEST(TestNBTIterator, TestLongIterator) {
  auto value = std::numeric_limits<uint64_t>::max();
  nbt::NBT container(value);

  auto it = container.begin();

  ASSERT_NE(it, container.end());
  ASSERT_EQ(it->get<uint64_t>(), value);
  ASSERT_EQ(++it, container.end());
}

TEST(TestNBTIterator, TestFloatIterator) {
  auto value = 1.62786728f;
  nbt::NBT container(value);

  auto it = container.begin();

  ASSERT_NE(it, container.end());
  ASSERT_FLOAT_EQ(it->get<float>(), value);
  ASSERT_EQ(++it, container.end());
}

TEST(TestNBTIterator, TestDoubleIterator) {
  auto value = 1.62786728;
  nbt::NBT container(value);

  auto it = container.begin();

  ASSERT_NE(it, container.end());
  ASSERT_DOUBLE_EQ(it->get<double>(), value);
  ASSERT_EQ(++it, container.end());
}

TEST(TestNBTIterator, TestStringIterator) {
  auto value = "This is a nice string.";
  nbt::NBT container(value);

  auto it = container.begin();

  ASSERT_NE(it, container.end());
  ASSERT_EQ(it->get<nbt::NBT::tag_string_t>(), value);
  ASSERT_EQ(++it, container.end());
}

TEST(TestNBTIterator, TestByteArrayIterator) {
  std::vector<int8_t> value = {0, 1, 2, 3, 4};
  nbt::NBT container(value);

  auto it = container.begin();

  ASSERT_NE(it, container.end());
  std::vector<int8_t> data = *it->get<std::vector<int8_t> *>();
  for (std::vector<int8_t>::size_type i = 0; i < value.size(); i++)
    ASSERT_EQ(data[i], value[i]);
  ASSERT_EQ(++it, container.end());
}

TEST(TestNBTIterator, TestIntArrayIterator) {
  std::vector<int32_t> value = {0, 1, 2, 3, 4};
  nbt::NBT container(value);

  auto it = container.begin();

  ASSERT_NE(it, container.end());
  std::vector<int32_t> data = *it->get<std::vector<int32_t> *>();
  for (std::vector<int32_t>::size_type i = 0; i < value.size(); i++)
    ASSERT_EQ(data[i], value[i]);
  ASSERT_EQ(++it, container.end());
}

TEST(TestNBTIterator, TestLongArrayIterator) {
  std::vector<int64_t> value = {0, 1, 2, 3, 4};
  nbt::NBT container(value);

  auto it = container.begin();

  ASSERT_NE(it, container.end());
  std::vector<int64_t> data = *it->get<std::vector<int64_t> *>();
  for (std::vector<int64_t>::size_type i = 0; i < value.size(); i++)
    ASSERT_EQ(data[i], value[i]);
  ASSERT_EQ(++it, container.end());
}
