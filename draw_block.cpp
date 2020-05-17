/**
 * This file contains functions to draw blocks to a png file
 */

#include "./draw_png.h"

PNG::Image::drawer blockRenderers[] = {
    &PNG::Image::drawFull,
#define DEFINETYPE(STRING, CALLBACK) &PNG::Image::CALLBACK,
#include "./blocktypes.def"
#undef DEFINETYPE
};

void PNG::Image::drawBlock(const size_t x, const size_t y,
                           const NBT &blockData) {
  if (x < 0 || x > width - 1)
    throw std::range_error("Invalid x: " + std::to_string(x) + "/" +
                           std::to_string(width));

  if (y < 0 || y > height - 1)
    throw std::range_error("Invalid y: " + std::to_string(y) + "/" +
                           std::to_string(height));

  if (!blockData.contains("Name"))
    return;

  const string &name = *(blockData["Name"].get<const string *>());
  auto defined = palette.find(name);

  if (defined == palette.end()) {
    fprintf(stderr, "Error getting color of block %s\n", name.c_str());
    return;
  }

  const Colors::Block *blockColor = &(defined->second);

  if (blockColor->primary.empty())
    return;

  // Then call the function registered with the block's type
  (this->*blockRenderers[blockColor->type])(x, y, blockData, blockColor);
}

inline void blend(uint8_t *const destination, const uint8_t *const source) {
  if (destination[PALPHA] == 0 || source[PALPHA] == 255) {
    memcpy(destination, source, BYTESPERPIXEL);
    return;
  }
#define BLEND(ca, aa, cb)                                                      \
  uint8_t(((size_t(ca) * size_t(aa)) + (size_t(255 - aa) * size_t(cb))) / 255)
  destination[0] = BLEND(source[0], source[PALPHA], destination[0]);
  destination[1] = BLEND(source[1], source[PALPHA], destination[1]);
  destination[2] = BLEND(source[2], source[PALPHA], destination[2]);
  destination[PALPHA] +=
      (size_t(source[PALPHA]) * size_t(255 - destination[PALPHA])) / 255;
}

inline void addColor(uint8_t *const color, const uint8_t *const add) {
  const float v2 = (float(add[PALPHA]) / 255.0f);
  const float v1 = (1.0f - (v2 * .2f));
  color[0] = clamp(uint16_t(float(color[0]) * v1 + float(add[0]) * v2));
  color[1] = clamp(uint16_t(float(color[1]) * v1 + float(add[1]) * v2));
  color[2] = clamp(uint16_t(float(color[2]) * v1 + float(add[2]) * v2));
}

void PNG::Image::drawHead(const size_t x, const size_t y, const NBT &,
                          const Colors::Block *block) {
  /* Small block centered
   * |    |
   * |    |
   * | PP |
   * | DL | */
  uint8_t *pos = pixel(x + 1, y + 2);
  memcpy(pos, &block->primary, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &block->primary, BYTESPERPIXEL);

  pos = pixel(x + 1, y + 3);
  memcpy(pos, &block->dark, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &block->light, BYTESPERPIXEL);
}

void PNG::Image::drawThin(const size_t x, const size_t y, const NBT &,
                          const Colors::Block *block) {
  /* Overwrite the block below's top layer
   * |    |
   * |    |
   * |    |
   * |XXXX|
   *   XX   */
  uint8_t *pos = pixel(x, y + 3);
  for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL) {
    memcpy(pos, &block->primary, BYTESPERPIXEL);
  }
#ifndef LEGACY
  pos = pixel(x + 1, y + 4);
  memcpy(pos, &block->primary, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &block->primary, BYTESPERPIXEL);
#endif
}

void PNG::Image::drawHidden(const size_t, const size_t, const NBT &,
                            const Colors::Block *) {
  return;
}

void PNG::Image::drawTransparent(const size_t x, const size_t y, const NBT &,
                                 const Colors::Block *block) {
  // Avoid the dark/light edges for a clearer look through
  for (uint8_t i = 0; i < 4; i++)
    for (uint8_t j = 0; j < 3; j++)
      blend(pixel(x + i, y + j), (uint8_t *)&block->primary);
}

void PNG::Image::drawTorch(const size_t x, const size_t y, const NBT &,
                           const Colors::Block *block) {
  /* TODO Callback to handle the orientation
   * Print the secondary on top of two primary
   * |    |
   * |  S |
   * |  P |
   * |  P | */
  uint8_t *pos = pixel(x + 2, y + 1);
  memcpy(pos, &block->secondary, BYTESPERPIXEL);
  pos = pixel(x + 2, y + 2);
#ifdef LEGACY
  memcpy(pos, &block->secondary, BYTESPERPIXEL);
#else
  memcpy(pos, &block->primary, BYTESPERPIXEL);
  pos = pixel(x + 2, y + 3);
  memcpy(pos, &block->primary, BYTESPERPIXEL);
#endif
}

void PNG::Image::drawPlant(const size_t x, const size_t y, const NBT &,
                           const Colors::Block *block) {
  /* Print a plant-like block
   * TODO Make that nicer ?
   * |    |
   * | X X|
   * |  X |
   * | X  | */
  uint8_t *pos = pixel(x, y + 1);
  memcpy(pos + (CHANSPERPIXEL), &block->primary, BYTESPERPIXEL);
  memcpy(pos + (CHANSPERPIXEL * 3), &block->primary, BYTESPERPIXEL);
  pos = pixel(x + 2, y + 2);
  memcpy(pos, &block->primary, BYTESPERPIXEL);
  pos = pixel(x + 1, y + 3);
  memcpy(pos, &block->primary, BYTESPERPIXEL);
}

void PNG::Image::drawFire(const size_t x, const size_t y, const NBT &,
                          const Colors::Block *const color) {
  // This basically just leaves out a few pixels
  // Top row
  uint8_t *pos = pixel(x, y);
  blend(pos, (uint8_t *)&color->light);
  blend(pos + CHANSPERPIXEL * 2, (uint8_t *)&color->dark);
  // Second and third row
  for (size_t i = 1; i < 3; ++i) {
    pos = pixel(x, y + i);
    blend(pos, (uint8_t *)&color->dark);
    blend(pos + (CHANSPERPIXEL * i), (uint8_t *)&color->primary);
    blend(pos + (CHANSPERPIXEL * 3), (uint8_t *)&color->light);
  }
  // Last row
  pos = pixel(x, y + 3);
  blend(pos + (CHANSPERPIXEL * 2), (uint8_t *)&color->light);
}

void PNG::Image::drawOre(const size_t x, const size_t y, const NBT &,
                         const Colors::Block *color) {
  /* Print a vein with the secondary in the block
   * |PPPS|
   * |DDSL|
   * |DSLS|
   * |SDLL| */

  // Top row
  uint8_t *pos = pixel(x, y);
  for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL)
    memcpy(pos, (i == 3 ? &color->secondary : &color->primary), BYTESPERPIXEL);

  // Second row
  pos = pixel(x, y + 1);
  memcpy(pos, &color->dark, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &color->dark, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 2, &color->secondary, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 3, &color->light, BYTESPERPIXEL);
  // Third row
  pos = pixel(x, y + 2);
  memcpy(pos, &color->dark, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &color->secondary, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 2, &color->light, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 3, &color->secondary, BYTESPERPIXEL);
  // Last row
  pos = pixel(x, y + 3);
  memcpy(pos, &color->secondary, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &color->dark, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 2, &color->light, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 3, &color->light, BYTESPERPIXEL);
}

void PNG::Image::drawGrown(const size_t x, const size_t y, const NBT &,
                           const Colors::Block *color) {
  /* Print the secondary color on top
   * |SSSS|
   * |DSSL|
   * |DDLL|
   * |DDLL| */

  // First determine how much the color has to be lightened up or darkened
  // The brighter the color, the stronger the impact
  int sub = (float(color->primary.BRIGHTNESS) / 323.0f + .21f);

  Colors::Color L(color->secondary);
  Colors::Color D(color->secondary);
  L.modColor(sub - 15);
  D.modColor(sub - 25);

  // Top row
  uint8_t *pos = pixel(x, y);
  for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL) {
    memcpy(pos, &color->secondary, BYTESPERPIXEL);
  }

  // Second row
  pos = pixel(x, y + 1);
  memcpy(pos, &color->dark, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &D, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 2, &L, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 3, &color->light, BYTESPERPIXEL);

  // Third row
  pos = pixel(x, y + 2);
  memcpy(pos, &color->dark, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &color->dark, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 2, &color->light, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 3, &color->light, BYTESPERPIXEL);

  // Last row
  pos = pixel(x, y + 3);
  memcpy(pos, &color->dark, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &color->dark, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 2, &color->light, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 3, &color->light, BYTESPERPIXEL);
}

void PNG::Image::drawRod(const size_t x, const size_t y, const NBT &,
                         const Colors::Block *const color) {
  /* A full fat rod
   * | PP |
   * | DL |
   * | DL |
   * | DL | */
  uint8_t *pos = pixel(x + 1, y);
  memcpy(pos, &color->primary, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &color->primary, BYTESPERPIXEL);
  for (int i = 1; i < 4; i++) {
    pos = pixel(x + 1, y + i);
    memcpy(pos, &color->dark, BYTESPERPIXEL);
    memcpy(pos + CHANSPERPIXEL, &color->light, BYTESPERPIXEL);
  }
}

void PNG::Image::drawSlab(const size_t x, const size_t y, const NBT &metadata,
                          const Colors::Block *color) {
  /* This one has a hack to make it look like a gradual step up:
   * The second layer has primary colors to make the difference less
   * obvious.
   * |    |
   * |PPPP|
   * |DPPL|
   * |DDLL| */

  bool top = false;
#define SLAB_OFFSET (top ? 0 : 1)
  if (*metadata["Properties"]["type"].get<const string *>() == "top")
    top = true;

  uint8_t *pos = pixel(x, y + SLAB_OFFSET);
  for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL) {
    memcpy(pos, &color->primary, BYTESPERPIXEL);
  }

  pos = pixel(x, y + SLAB_OFFSET + 1);
  memcpy(pos, &color->dark, BYTESPERPIXEL);
  if (top) {
    memcpy(pos + CHANSPERPIXEL, &color->dark, BYTESPERPIXEL);
    memcpy(pos + CHANSPERPIXEL * 2, &color->light, BYTESPERPIXEL);
  } else {
    memcpy(pos + CHANSPERPIXEL, &color->primary, BYTESPERPIXEL);
    memcpy(pos + CHANSPERPIXEL * 2, &color->primary, BYTESPERPIXEL);
  }
  memcpy(pos + CHANSPERPIXEL * 3, &color->light, BYTESPERPIXEL);

  pos = pixel(x, y + SLAB_OFFSET + 2);
  for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL) {
    memcpy(pos, (i < 2 ? &color->dark : &color->light), BYTESPERPIXEL);
  }
#undef SLAB_OFFSET
}

void PNG::Image::drawWire(const size_t x, const size_t y, const NBT &,
                          const Colors::Block *color) {
  uint8_t *pos = pixel(x + 1, y + 2);
  memcpy(pos, &color->primary, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &color->primary, BYTESPERPIXEL);
}

/*
        void setUpStep(const size_t x, const size_t y, const uint8_t * const
   color, const uint8_t * const light, const uint8_t * const dark) { uint8_t
   *pos = pixel(x, y); for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL) {
                        memcpy(pos, color, BYTESPERPIXEL);
                }
                pos = pixel(x, y+1);
                for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL) {
                        memcpy(pos, color, BYTESPERPIXEL);
                }
        }
*/

void PNG::Image::drawFull(const size_t x, const size_t y, const NBT &,
                          const Colors::Block *color) {
  // Sets pixels around x,y where A is the anchor
  // T = given color, D = darker, L = lighter
  // A T T T
  // D D L L
  // D D L L
  // D D L L

  // Ordinary blocks are all rendered the same way
  if (color->primary.ALPHA == 255) { // Fully opaque - faster
    // Top row
    uint8_t *pos = pixel(x, y);
    for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL) {
      memcpy(pos, &color->primary, BYTESPERPIXEL);
    }
    // Second row
    pos = pixel(x, y + 1);
    for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL) {
      memcpy(pos, (i < 2 ? &color->dark : &color->light), BYTESPERPIXEL);
    }
    // Third row
    pos = pixel(x, y + 2);
    for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL) {
      memcpy(pos, (i < 2 ? &color->dark : &color->light), BYTESPERPIXEL);
    }
    // Last row
    pos = pixel(x, y + 3);
    for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL) {
      memcpy(pos, (i < 2 ? &color->dark : &color->light), BYTESPERPIXEL);
      // The weird check here is to get the pattern right, as the noise should
      // be stronger every other row, but take into account the isometric
      // perspective
    }
  } else { // Not opaque, use slower blending code
    // Top row
    uint8_t *pos = pixel(x, y);
    for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL) {
      blend(pos, (uint8_t *)&color->primary);
    }
    // Second row
    pos = pixel(x, y + 1);
    for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL) {
      blend(pos, (i < 2 ? (uint8_t *)&color->dark : (uint8_t *)&color->light));
    }
    // Third row
    pos = pixel(x, y + 2);
    for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL) {
      blend(pos, (i < 2 ? (uint8_t *)&color->dark : (uint8_t *)&color->light));
    }
    // Last row
    pos = pixel(x, y + 3);
    for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL) {
      blend(pos, (i < 2 ? (uint8_t *)&color->dark : (uint8_t *)&color->light));
    }
  }
  // The above two branches are almost the same, maybe one could just create a
  // function pointer and...
}
