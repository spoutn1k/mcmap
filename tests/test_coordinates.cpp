#include <gtest/gtest.h>
#include <map.hpp>

TEST(TestCoordinates, TestCreateDefault) {
  Map::Coordinates<int8_t> minimap;
  ASSERT_TRUE(minimap.isUndefined());
}

TEST(TestCoordinates, TestCreateCopy) {
  Map::Coordinates<int8_t> original(-100, 0, -100, 100, 255, 100, Map::SE),
      copy;

  copy = original;

  ASSERT_EQ(original.minX, copy.minX);
  ASSERT_EQ(original.minZ, copy.minZ);
  ASSERT_EQ(original.maxX, copy.maxX);
  ASSERT_EQ(original.maxZ, copy.maxZ);
  ASSERT_EQ(original.orientation, copy.orientation);
}

TEST(TestCoordinates, TestCrop) {
  Map::Coordinates<uint32_t> original(-1000, 0, -1000, 100, 255, 100),
      copy(-100, 0, -100, 1000, 255, 1000),
      intended(-100, 0, -100, 100, 255, 100);

  original.crop(copy);
  ASSERT_EQ(original, intended);
}

TEST(TestCoordinates, TestAdd) {
  Map::Coordinates<uint32_t> original(-1000, 0, -1000, 100, 255, 100),
      copy(-100, 0, -100, 1000, 255, 1000),
      intended(-1000, 0, -1000, 1000, 255, 1000);

  original += copy;
  ASSERT_EQ(original, intended);
}
