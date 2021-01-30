#include <gtest/gtest.h>
#include <map.hpp>

TEST(TestCoordinates, TestCreate) {
  Map::Coordinates<int8_t> minimap;
  ASSERT_TRUE(minimap.isUndefined());
}
