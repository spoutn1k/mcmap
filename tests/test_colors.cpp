#include "../src/colors.h"
#include <gtest/gtest.h>

TEST(TestColor, TestCreate) {
  Colors::Color c;

  ASSERT_TRUE(c.empty());

  Colors::Color water({7, 52, 200, 50});
  ASSERT_FALSE(water.empty());
  ASSERT_EQ(water.R, 7);
  ASSERT_EQ(water.G, 52);
  ASSERT_EQ(water.B, 200);
  ASSERT_EQ(water.ALPHA, 50);
}

TEST(TestColor, TestEmpty) {
  Colors::Color c;
  c.R = c.G = c.B = c.ALPHA;
  ASSERT_TRUE(c.empty());

  Colors::Color cR = c, cG = c, cB = c;
  cR.R = 1;
  cG.G = 1;
  cB.B = 1;

  ASSERT_FALSE(cR.empty());
  ASSERT_FALSE(cG.empty());
  ASSERT_FALSE(cB.empty());
}

TEST(TestColor, TestOpacity) {
  Colors::Color c;
  ASSERT_TRUE(c.transparent());
  ASSERT_FALSE(c.opaque());

  c.ALPHA = 1;
  ASSERT_FALSE(c.transparent());
  ASSERT_FALSE(c.opaque());

  c.ALPHA = 255;
  ASSERT_FALSE(c.transparent());
  ASSERT_TRUE(c.opaque());
}

TEST(TestBlock, TestCreate) {
  Colors::Block b;

  ASSERT_TRUE(b.type == Colors::FULL);
  ASSERT_TRUE(b.primary.empty());
  ASSERT_TRUE(b.secondary.empty());

  b = Colors::Block(Colors::drawSlab, {1, 2, 3, 4});
  ASSERT_TRUE(b.type == Colors::drawSlab);
  ASSERT_FALSE(b.primary.empty());
  ASSERT_TRUE(b.secondary.empty());

  b = Colors::Block(Colors::drawStair, {1, 2, 3, 4}, {5, 6, 7, 8});
  ASSERT_TRUE(b.type == Colors::drawStair);
  ASSERT_FALSE(b.primary.empty());
  ASSERT_FALSE(b.secondary.empty());
}

TEST(TestBlock, TestEqualOperator) {
  Colors::Block b1 = Colors::Block(Colors::drawBeam, {1, 2, 3, 4}), b2 = b1;

  ASSERT_EQ(b1, b2);
  b1.type = Colors::drawTransparent;
  ASSERT_NE(b1, b2);

  b2 = Colors::Block(Colors::drawTransparent, {1, 2, 3, 4}, {5, 6, 7, 8});
  ASSERT_NE(b1, b2);
}

TEST(TestBlock, TestJson) {
  Colors::Block b, translated;
  b = Colors::Block(Colors::drawStair, {1, 2, 3, 4}, {5, 6, 7, 8});
  translated = nlohmann::json(b).get<Colors::Block>();

  ASSERT_EQ(b, translated);
}

TEST(TestPalette, TestJson) {
  Colors::Palette p, translated;
  p.insert(std::pair("minecraft:water",
                     Colors::Block(Colors::FULL, {7, 52, 200, 50})));
  translated = nlohmann::json(p).get<Colors::Palette>();

  ASSERT_EQ(p, translated);
}

TEST(TestPalette, TestLoad) {
  Colors::Palette p;

  ASSERT_TRUE(Colors::load(&p));
  ASSERT_TRUE(p.size());
}

TEST(TestPalette, TestLoadFile) {
  Colors::Palette p;

  ASSERT_TRUE(Colors::load(&p));
  ASSERT_TRUE(p.size());
  ASSERT_TRUE(Colors::load("tests/nowater.json", &p));
  ASSERT_TRUE(p.size());
  ASSERT_TRUE(p.find("minecraft:water") != p.end());
  ASSERT_TRUE(p["minecraft:water"].primary.transparent());
}
