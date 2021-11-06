#include <cmath>
#include <compat.hpp>
#include <gtest/gtest.h>

TEST(TestCompat, TestReturn) {
  std::map<int, int> versions = {
      {0, 0}, {1, 0}, {2, 0}, {4, 0}, {8, 0}, {16, 0},
  };

  std::map<int, int>::const_iterator it;

  it = compatible(versions, 0);
  ASSERT_NE(it, versions.end());
  ASSERT_EQ(it->first, 0);

  for (int i = 1; i < 20; i++) {
    it = compatible(versions, i);
    ASSERT_NE(it, versions.end());
    ASSERT_EQ(it->first, 1 << int(std::log2(double(i))));
  }
}
