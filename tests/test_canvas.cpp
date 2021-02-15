#include "../src/canvas.h"
#include <gtest/gtest.h>

TEST(TestCanvas, TestCreateDefault) {
  Canvas c1;

  ASSERT_TRUE(c1.type == Canvas::EMPTY);
  ASSERT_TRUE(c1.width() == c1.height() && c1.width() == 0);

  c1 = Canvas(Canvas::BYTES);

  ASSERT_TRUE(c1.type == Canvas::BYTES);
  ASSERT_TRUE(c1.width() == c1.height() && c1.width() == 0);

  c1 = Canvas(Canvas::CANVAS);

  ASSERT_TRUE(c1.type == Canvas::CANVAS);
  ASSERT_TRUE(c1.width() == c1.height() && c1.width() == 0);
}

TEST(TestCanvas, TestGetLine) {
  uint8_t buffer[1000];
  Canvas c1;

  ASSERT_FALSE(c1.getLine(&buffer[0], 1000, 0));

  c1 = Canvas(Canvas::BYTES);
  ASSERT_FALSE(c1.getLine(&buffer[0], 1000, 0));

  c1 = Canvas(Canvas::CANVAS);
  ASSERT_FALSE(c1.getLine(&buffer[0], 1000, 0));
}
