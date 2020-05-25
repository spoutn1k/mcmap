/**
 * This file contains functions to draw blocks to a png file
 */

#include "./canvas.h"
#ifdef CLOCK
#include <ctime>
#endif

//  ____                       _
// / ___|_ __ ___  _ __  _ __ (_)_ __   __ _
//| |   | '__/ _ \| '_ \| '_ \| | '_ \ / _` |
//| |___| | | (_) | |_) | |_) | | | | | (_| |
// \____|_|  \___/| .__/| .__/|_|_| |_|\__, |
//                |_|   |_|            |___/
// The following methods are related to the cropping mechanism.

size_t IsometricCanvas::getCroppedWidth() const {
  // Not implemented, returns the actual width. Might come back to this but it
  // is not as interesting as the height.
  return width;
}

size_t IsometricCanvas::firstLine() const {
  // Tip: Return -7 for a freaky glichy look
  // return -7;

  // We search for the first non-empty line
  size_t line = 0;

  for (size_t row = 0; row < height && !line; row++)
    for (size_t pixel = 0; pixel < width && !line; pixel++)
      if (*(bytesBuffer + (pixel + row * width) * BYTESPERPIXEL))
        line = row;

  return line - (padding - 2);
}

size_t IsometricCanvas::lastLine() const {
  // We search for the last non-empty line
  size_t line = 0;

  for (size_t row = height - 1; row > 0 && !line; row--)
    for (size_t pixel = 0; pixel < width && !line; pixel++)
      if (*(bytesBuffer + (pixel + row * width) * BYTESPERPIXEL))
        line = row;

  return line + (padding - 2);
}

size_t IsometricCanvas::getCroppedHeight() const {
  return lastLine() - firstLine() + 1;
}

size_t IsometricCanvas::getCroppedOffset() const {
  // The first line to render in the cropped view of the canvas, as an offset
  // from the beginning of the byte buffer
  return firstLine() * width * BYTESPERPIXEL;
}

// ____                     _
//|  _ \ _ __ __ ___      _(_)_ __   __ _
//| | | | '__/ _` \ \ /\ / / | '_ \ / _` |
//| |_| | | | (_| |\ V  V /| | | | | (_| |
//|____/|_|  \__,_| \_/\_/ |_|_| |_|\__, |
//                                  |___/
// The following methods are used to draw the map into the canvas' 2D buffer

void IsometricCanvas::translate(size_t xCanvasPos, size_t zCanvasPos,
                                int64_t *xPos, int64_t *zPos) {
  switch (map.orientation) {
  case NW:
    *xPos = (map.minX >> 4) + xCanvasPos;
    *zPos = (map.minZ >> 4) + zCanvasPos;
    break;
  case SW:
    std::swap(xCanvasPos, zCanvasPos);
    *xPos = (map.minX >> 4) + xCanvasPos;
    *zPos = (map.maxZ >> 4) - zCanvasPos;
    break;
  case NE:
    std::swap(xCanvasPos, zCanvasPos);
    *xPos = (map.maxX >> 4) - xCanvasPos;
    *zPos = (map.minZ >> 4) + zCanvasPos;
    break;
  case SE:
    *xPos = (map.maxX >> 4) - xCanvasPos;
    *zPos = (map.maxZ >> 4) - zCanvasPos;
    break;
  }
}

void IsometricCanvas::drawTerrain(const Terrain::Data &world) {
  for (size_t xCanvasPos = 0; xCanvasPos < nXChunks; xCanvasPos++) {
    for (size_t zCanvasPos = 0; zCanvasPos < nZChunks; zCanvasPos++) {
      drawChunk(world, xCanvasPos, zCanvasPos);
    }
  }

  return;
}

void IsometricCanvas::drawChunk(const Terrain::Data &terrain,
                                const int64_t xPos, const int64_t zPos) {
  int64_t worldChunkX = 0, worldChunkZ = 0;
  translate(xPos, zPos, &worldChunkX, &worldChunkZ);

  const NBT &chunk = terrain.chunkAt(worldChunkX, worldChunkZ);
  const uint8_t height = terrain.heightAt(worldChunkX, worldChunkZ);

  if (chunk.is_end() || !chunk.contains("DataVersion"))
    return;

  const int dataVersion = *chunk["DataVersion"].get<const int *>();

  // Set the interpreter according to the type of chunk encountered
  sectionInterpreter interpreter = NULL;
  if (dataVersion < 2534)
    interpreter = blockAtPre116;
  else
    interpreter = blockAtPost116;

  const uint8_t minSection = std::max((map.minY >> 4), (height & 0x0f));
  const uint8_t maxSection = std::min((map.maxY >> 4) + 1, (height >> 4));

  for (uint8_t yPos = minSection; yPos < maxSection; yPos++) {
    drawSection(chunk["Level"]["Sections"][yPos], xPos, zPos, yPos,
                interpreter);
  }
}

void orient(uint8_t &x, uint8_t &z, Orientation o) {
  switch (o) {
  case NW:
    return;
  case NE:
    std::swap(x, z);
    x = 15 - x;
    return;
  case SW:
    std::swap(x, z);
    z = 15 - z;
    return;
  case SE:
    x = 15 - x;
    z = 15 - z;
    return;
  }
}

void IsometricCanvas::drawSection(const NBT &section, const int64_t xPos,
                                  const int64_t zPos, const uint8_t yPos,
                                  sectionInterpreter interpreter) {
  if (!interpreter) {
    fprintf(stderr, "Invalid section interpreter\n");
    return;
  }

  if (section.is_end() || !section.contains("Palette"))
    return;

  uint16_t colorIndex = 0, index = 0;
  Colors::Block *cache[256];
  Colors::Block fallback; // empty color to use in case no color is defined

  // Preload the colors in the order they appear in the palette
  for (auto &color : section["Palette"]) {
    auto defined = palette.find(*color["Name"].get<const string *>());

    if (defined == palette.end()) {
      fprintf(stderr, "Color of block %s not found\n",
              color["Name"].get<const string *>()->c_str());
      cache[colorIndex++] = &fallback;
    } else {
      cache[colorIndex++] = &defined->second;
    }
  }

  const uint64_t length =
      std::max((uint64_t)ceil(log2(section["Palette"].size())), (uint64_t)4);

  const std::vector<int64_t> *blockStates =
      section["BlockStates"].get<const std::vector<int64_t> *>();

  for (uint8_t x = 0; x < 16; x++) {
    for (uint8_t z = 0; z < 16; z++) {
      for (uint8_t y = 0; y < 16; y++) {
        uint8_t xReal = x, zReal = z;
        orient(xReal, zReal, map.orientation);
        index = interpreter(length, blockStates, xReal, zReal, y);

        if (index >= colorIndex) {
          fprintf(stderr, "Cache error in chunk %ld %ld: %d/%d\n", xPos, zPos,
                  index, colorIndex);
          continue;
        }

        if (index) {
          drawBlock(cache[index], (xPos << 4) + x, (zPos << 4) + z,
                    (yPos << 4) + y, section["Palette"][index]);
        }
      }
    }
  }

  return;
}

IsometricCanvas::drawer blockRenderers[] = {
    &IsometricCanvas::drawFull,
#define DEFINETYPE(STRING, CALLBACK) &IsometricCanvas::CALLBACK,
#include "./blocktypes.def"
#undef DEFINETYPE
};

inline void IsometricCanvas::drawBlock(const Colors::Block *color,
                                       const size_t x, const size_t z,
                                       const size_t y, const NBT &metadata) {
  if (y > map.maxY || y < map.minY)
    return;

  const size_t bmpPosX = 2 * (sizeZ - 1) + (x - z) * 2 + padding;
  const size_t bmpPosY = height - 2 + x + z - sizeX - sizeZ -
                         (y - map.minY) * heightOffset - padding;

  if (bmpPosX > width - 1)
    throw std::range_error("Invalid x: " + std::to_string(bmpPosX) + "/" +
                           std::to_string(width));

  if (bmpPosY > height - 1)
    throw std::range_error("Invalid y: " + std::to_string(bmpPosY) + "/" +
                           std::to_string(height));

  if (color->primary.empty())
    return;

  // Then call the function registered with the block's type
  (this->*blockRenderers[color->type])(bmpPosX, bmpPosY, metadata, color);
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

void IsometricCanvas::drawHead(const size_t x, const size_t y, const NBT &,
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

void IsometricCanvas::drawThin(const size_t x, const size_t y, const NBT &,
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

void IsometricCanvas::drawHidden(const size_t, const size_t, const NBT &,
                                 const Colors::Block *) {
  return;
}

void IsometricCanvas::drawTransparent(const size_t x, const size_t y,
                                      const NBT &, const Colors::Block *block) {
  // Avoid the dark/light edges for a clearer look through
  for (uint8_t i = 0; i < 4; i++)
    for (uint8_t j = 0; j < 3; j++)
      blend(pixel(x + i, y + j), (uint8_t *)&block->primary);
}

void IsometricCanvas::drawTorch(const size_t x, const size_t y, const NBT &,
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

void IsometricCanvas::drawPlant(const size_t x, const size_t y, const NBT &,
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

void IsometricCanvas::drawFire(const size_t x, const size_t y, const NBT &,
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

void IsometricCanvas::drawOre(const size_t x, const size_t y, const NBT &,
                              const Colors::Block *color) {
  /* Print a vein with the secondary in the block
   * |PPPS|
   * |DDSL|
   * |DSLS|
   * |SDLL| */

  int sub = (float(color->primary.BRIGHTNESS) / 323.0f + .21f);

  Colors::Color L(color->secondary);
  Colors::Color D(color->secondary);
  L.modColor(sub - 15);
  D.modColor(sub - 25);

  // Top row
  uint8_t *pos = pixel(x, y);
  for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL)
    memcpy(pos, (i == 3 ? &color->secondary : &color->primary), BYTESPERPIXEL);

  // Second row
  pos = pixel(x, y + 1);
  memcpy(pos, &color->dark, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &color->dark, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 2, &L, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 3, &color->light, BYTESPERPIXEL);
  // Third row
  pos = pixel(x, y + 2);
  memcpy(pos, &color->dark, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &D, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 2, &color->light, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 3, &L, BYTESPERPIXEL);
  // Last row
  pos = pixel(x, y + 3);
  memcpy(pos, &D, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &color->dark, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 2, &color->light, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL * 3, &color->light, BYTESPERPIXEL);
}

void IsometricCanvas::drawGrown(const size_t x, const size_t y, const NBT &,
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

void IsometricCanvas::drawRod(const size_t x, const size_t y, const NBT &,
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

void IsometricCanvas::drawSlab(const size_t x, const size_t y,
                               const NBT &metadata,
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
  if (metadata["Properties"].contains("type") &&
      *metadata["Properties"]["type"].get<const string *>() == "top")
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

void IsometricCanvas::drawWire(const size_t x, const size_t y, const NBT &,
                               const Colors::Block *color) {
  uint8_t *pos = pixel(x + 1, y + 2);
  memcpy(pos, &color->primary, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &color->primary, BYTESPERPIXEL);
}

/*
        void setUpStep(const size_t x, const size_t y, const uint8_t *
   const color, const uint8_t * const light, const uint8_t * const dark) {
   uint8_t *pos = pixel(x, y); for (size_t i = 0; i < 4; ++i, pos +=
   CHANSPERPIXEL) { memcpy(pos, color, BYTESPERPIXEL);
                }
                pos = pixel(x, y+1);
                for (size_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL) {
                        memcpy(pos, color, BYTESPERPIXEL);
                }
        }
*/

void IsometricCanvas::drawFull(const size_t x, const size_t y, const NBT &,
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
      // The weird check here is to get the pattern right, as the noise
      // should be stronger every other row, but take into account the
      // isometric perspective
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
  // The above two branches are almost the same, maybe one could just
  // create a function pointer and...
}

// __  __                _
//|  \/  | ___ _ __ __ _(_)_ __   __ _
//| |\/| |/ _ \ '__/ _` | | '_ \ / _` |
//| |  | |  __/ | | (_| | | | | | (_| |
//|_|  |_|\___|_|  \__, |_|_| |_|\__, |
//                 |___/         |___/
// This is the canvas merging code.

size_t IsometricCanvas::calcAnchor(const IsometricCanvas &subCanvas) {
  // Determine where in the canvas' 2D matrix is the subcanvas supposed to
  // go: the anchor is the bottom left pixel in the canvas where the
  // sub-canvas must be superimposed
  size_t anchorX = 0, anchorY = height;
  const size_t minOffset =
      subCanvas.map.minX - map.minX + subCanvas.map.minZ - map.minZ;
  const size_t maxOffset =
      map.maxX - subCanvas.map.maxX + map.maxZ - subCanvas.map.maxZ;

  switch (map.orientation) {
    // We know an image's width is relative to it's terrain size; we use
    // that property to determine where to put the subcanvas.
  case NW:
    anchorX = minOffset * 2;
    anchorY = height - maxOffset;
    break;

    // This is the opposite of NW
  case SE:
    anchorX = maxOffset * 2;
    anchorY = height - minOffset;
    break;

  case SW:
    anchorX = maxOffset * 2;
    anchorY = height - maxOffset;
    break;

  case NE:
    anchorX = minOffset * 2;
    anchorY = height - minOffset;
    break;
  }

  // Adjust the padding before translating to an offset
  anchorX = anchorX + padding - subCanvas.padding;
  anchorY = anchorY - padding + subCanvas.padding;

  // Translate those coordinates as an offset from the beginning of the
  // buffer
  return (anchorX + width * anchorY) * BYTESPERPIXEL;
}

void overLay(uint8_t *const dest, const uint8_t *const source,
             const size_t width) {
  // Render a sub-canvas above the canvas' content
  for (size_t pixel = 0; pixel < width; pixel++) {
    const uint8_t *data = source + pixel * BYTESPERPIXEL;
    // If the subCanvas is empty here, skip
    if (!data[3])
      continue;

    // If the subCanvas has a fully opaque block or the canvas has
    // nothing, just overwrite the canvas' pixel
    if (data[3] == 0xff || !(dest + pixel * BYTESPERPIXEL)[3]) {
      memcpy(dest + pixel * BYTESPERPIXEL, data, BYTESPERPIXEL);
      continue;
    }

    // Finally, blend the transparent pixel into the canvas
    blend(dest + pixel * BYTESPERPIXEL, data);
  }
}

void underLay(uint8_t *const dest, const uint8_t *const source,
              const size_t width) {
  // Render a sub-canvas under the canvas' content
  uint8_t tmpPixel[4];

  for (size_t pixel = 0; pixel < width; pixel++) {
    const uint8_t *data = source + pixel * BYTESPERPIXEL;
    // If the subCanvas is empty here, or the canvas already has a pixel
    if (!data[3] || (dest + pixel * BYTESPERPIXEL)[3] == 0xff)
      continue;

    memcpy(tmpPixel, dest + pixel * BYTESPERPIXEL, BYTESPERPIXEL);
    memcpy(dest + pixel * BYTESPERPIXEL, data, BYTESPERPIXEL);
    blend(dest + pixel * BYTESPERPIXEL, tmpPixel);
  }
}

void IsometricCanvas::merge(const IsometricCanvas &subCanvas) {
#ifdef CLOCK
  auto begin = std::chrono::high_resolution_clock::now();
#endif

  // This routine determines where the subCanvas' buffer should be
  // written, then writes it in the objects' own buffer. This results in a
  // "merge" that really is a superimposition of the subCanvas onto the
  // main canvas.
  //
  // This routine is supposed to be called multiple times with ORDERED
  // subcanvasses
  if (subCanvas.width > width || subCanvas.height > height) {
    fprintf(stderr, "Cannot merge a canvas of bigger dimensions\n");
    return;
  }

  // Determine where in the canvas' 2D matrix is the subcanvas supposed to
  // go: the anchor is the bottom left pixel in the canvas where the
  // sub-canvas must be superimposed, translated as an offset from the
  // beginning of the buffer
  const size_t anchor = calcAnchor(subCanvas);

  // For every line of the subCanvas, we create a pointer to its
  // beginning, and a pointer to where in the canvas it should be copied
  for (size_t line = 1; line < subCanvas.height + 1; line++) {
    uint8_t *subLine = subCanvas.bytesBuffer + subCanvas.size -
                       line * subCanvas.width * BYTESPERPIXEL;
    uint8_t *position = bytesBuffer + anchor - line * width * BYTESPERPIXEL;

    // Then import the line over or under the existing data, depending on
    // the orientation
    if (map.orientation == NW || map.orientation == SW)
      overLay(position, subLine, subCanvas.width);
    else
      underLay(position, subLine, subCanvas.width);
  }

#ifdef CLOCK
  auto end = std::chrono::high_resolution_clock::now();
  printf("Merged canvas in %lfms\n",
         std::chrono::duration<double, std::milli>(end - begin).count());
#endif
}
