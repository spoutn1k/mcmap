#include "../src/helper.h"
#include <gtest/gtest.h>

TEST(TestCoordinates, TestCreate) {
  Coordinates<int8_t> minimap;
  ASSERT_TRUE(minimap.isUndefined());
}
