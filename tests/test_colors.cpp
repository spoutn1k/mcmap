#include "../src/colors.h"
#include <gtest/gtest.h>

TEST(TestColors, TestLoad) {
  Colors::Palette p;
  Colors::load("src/colors.json", &p);

  ASSERT_TRUE(p.size());
}
