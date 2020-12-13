#include "./block_drawers.h"

#define FILL_ &fill->primary
#define DARK_ &color->dark
#define LIGHT &color->light
#define PRIME &color->primary
#define ALTER &color->secondary
#define ALT_D &secondaryDark
#define ALT_L &secondaryLight

nbt::NBT empty;

void drawHead(IsometricCanvas *canvas, const uint32_t x, const uint32_t y,
              const NBT &, const Colors::Block *block) {
  /* Small block centered
   * |    |
   * |    |
   * | PP |
   * | DL | */
  uint8_t *pos = canvas->pixel(x + 1, y + 2);
  memcpy(pos, &block->primary, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &block->primary, BYTESPERPIXEL);

  pos = canvas->pixel(x + 1, y + 3);
  memcpy(pos, &block->dark, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &block->light, BYTESPERPIXEL);
}

void drawThin(IsometricCanvas *canvas, const uint32_t x, const uint32_t y,
              const NBT &, const Colors::Block *block) {
  /* Overwrite the block below's top layer
   * |    |
   * |    |
   * |    |
   * |XXXX|
   *   XX   */
  uint8_t *pos = canvas->pixel(x, y + 3);
  for (uint8_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL)
    memcpy(pos, &block->primary, BYTESPERPIXEL);
  pos = canvas->pixel(x + 1, y + 4);
  memcpy(pos, &block->dark, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &block->light, BYTESPERPIXEL);
}

void drawHidden(IsometricCanvas *, const uint32_t, const uint32_t, const NBT &,
                const Colors::Block *) {
  return;
}

void drawTransparent(IsometricCanvas *canvas, const uint32_t x,
                     const uint32_t y, const NBT &,
                     const Colors::Block *color) {
  uint8_t top = 0;

  if (*canvas->nextBlock() == *color)
    top = 1;

  const Colors::Color *sprite[4][4] = {{PRIME, PRIME, PRIME, PRIME},
                                       {DARK_, DARK_, LIGHT, LIGHT},
                                       {DARK_, DARK_, LIGHT, LIGHT},
                                       {DARK_, DARK_, LIGHT, LIGHT}};

  // Avoid the top and dark/light edges for a clearer look through
  uint8_t *pos = canvas->pixel(x, y + top);
  for (uint8_t j = top; j < 4; ++j, pos = canvas->pixel(x, y + j))
    for (uint8_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL)
      blend(pos, (uint8_t *)sprite[j][i]);
}

void drawTorch(IsometricCanvas *canvas, const uint32_t x, const uint32_t y,
               const NBT &, const Colors::Block *block) {
  /* TODO Callback to handle the orientation
   * Print the secondary on top of two primary
   * |    |
   * |  S |
   * |  P |
   * |  P | */
  memcpy(canvas->pixel(x + 2, y + 1), &block->secondary, BYTESPERPIXEL);
  memcpy(canvas->pixel(x + 2, y + 2), &block->primary, BYTESPERPIXEL);
  memcpy(canvas->pixel(x + 2, y + 3), &block->primary, BYTESPERPIXEL);
}

void drawPlant(IsometricCanvas *canvas, const uint32_t x, const uint32_t y,
               const NBT &metadata, const Colors::Block *color) {
  /* Print a plant-like block
   * TODO Make that nicer ?
   * |    |
   * | X X|
   * |  X |
   * | X  | */
  Colors::Block *fill = &canvas->air;

  if (metadata.contains("UnderWater"))
    if (metadata["UnderWater"].get<string>() == "true")
      fill = &canvas->water;

  const Colors::Color *sprite[3][4] = {{FILL_, PRIME, FILL_, PRIME},
                                       {FILL_, FILL_, PRIME, FILL_},
                                       {FILL_, PRIME, FILL_, FILL_}};

  uint8_t *pos = canvas->pixel(x, y);
  for (uint8_t j = 0; j < 3; ++j, pos = canvas->pixel(x, y + j))
    for (uint8_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL)
      blend(pos, (uint8_t *)sprite[j][i]);
}

void drawUnderwaterPlant(IsometricCanvas *canvas, const uint32_t x,
                         const uint32_t y, const NBT &metadata,
                         const Colors::Block *block) {
  /* Print a plant-like block
   * |    |
   * |WXWX|
   * |WWXW|
   * |WXWW| */
  nbt::NBT tweaked(metadata);
  tweaked.insert({"UnderWater", nbt::NBT("true")});

  drawPlant(canvas, x, y, tweaked, block);
}

void drawFire(IsometricCanvas *canvas, const uint32_t x, const uint32_t y,
              const NBT &, const Colors::Block *const color) {
  // This basically just leaves out a few canvas->pixels
  // Top row
  uint8_t *pos = canvas->pixel(x, y);
  blend(pos, (uint8_t *)&color->light);
  blend(pos + CHANSPERPIXEL * 2, (uint8_t *)&color->dark);
  // Second and third row
  for (uint8_t i = 1; i < 3; ++i) {
    pos = canvas->pixel(x, y + i);
    blend(pos, (uint8_t *)&color->dark);
    blend(pos + (CHANSPERPIXEL * i), (uint8_t *)&color->primary);
    blend(pos + (CHANSPERPIXEL * 3), (uint8_t *)&color->light);
  }
  // Last row
  pos = canvas->pixel(x, y + 3);
  blend(pos + (CHANSPERPIXEL * 2), (uint8_t *)&color->light);
}

void drawOre(IsometricCanvas *canvas, const uint32_t x, const uint32_t y,
             const NBT &, const Colors::Block *color) {
  /* Print a vein with the secondary in the block
   * |PSPP|
   * |DDSL|
   * |DSLS|
   * |SDLL| */

  int sub = (float(color->primary.brightness()) / 323.0f + .21f);

  Colors::Color secondaryLight(color->secondary);
  Colors::Color secondaryDark(color->secondary);
  secondaryLight.modColor(sub - 15);
  secondaryDark.modColor(sub - 25);

  const Colors::Color *sprite[4][4] = {{PRIME, ALTER, PRIME, PRIME},
                                       {DARK_, DARK_, ALT_L, LIGHT},
                                       {DARK_, ALT_D, LIGHT, ALT_L},
                                       {ALT_D, DARK_, LIGHT, LIGHT}};

  uint8_t *pos = canvas->pixel(x, y);
  for (uint8_t j = 0; j < 4; ++j, pos = canvas->pixel(x, y + j))
    for (uint8_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL)
      memcpy(pos, sprite[j][i], BYTESPERPIXEL);
}

void drawGrown(IsometricCanvas *canvas, const uint32_t x, const uint32_t y,
               const NBT &, const Colors::Block *color) {
  /* Print the secondary color on top
   * |SSSS|
   * |DSSL|
   * |DDLL|
   * |DDLL| */

  // First determine how much the color has to be lightened up or darkened
  // The brighter the color, the stronger the impact
  int sub = (float(color->primary.brightness()) / 323.0f + .21f);

  Colors::Color secondaryLight(color->secondary);
  Colors::Color secondaryDark(color->secondary);
  secondaryLight.modColor(sub - 15);
  secondaryDark.modColor(sub - 25);

  const Colors::Color *sprite[4][4] = {{ALTER, ALTER, ALTER, ALTER},
                                       {DARK_, ALT_D, ALT_L, LIGHT},
                                       {DARK_, DARK_, LIGHT, LIGHT},
                                       {DARK_, DARK_, LIGHT, LIGHT}};

  uint8_t *pos = canvas->pixel(x, y);
  for (uint8_t j = 0; j < 4; ++j, pos = canvas->pixel(x, y + j))
    for (uint8_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL)
      memcpy(pos, sprite[j][i], BYTESPERPIXEL);
}

void drawRod(IsometricCanvas *canvas, const uint32_t x, const uint32_t y,
             const NBT &metadata, const Colors::Block *const color) {
  /* A full fat rod
   * | PP |
   * | DL |
   * | DL |
   * | DL | */

  Colors::Block *fill = &canvas->air;
  std::string waterlogged = "false";

  if (metadata.contains("Properties"))
    if (metadata["Properties"].contains("waterlogged"))
      waterlogged = metadata["Properties"]["waterlogged"].get<string>();

  if (waterlogged == "true")
    fill = &canvas->water;

  const Colors::Color *sprite[4][4] = {{FILL_, PRIME, PRIME, FILL_},
                                       {FILL_, DARK_, LIGHT, FILL_},
                                       {FILL_, DARK_, LIGHT, FILL_},
                                       {FILL_, DARK_, LIGHT, FILL_}};

  uint8_t *pos = canvas->pixel(x, y);
  for (uint8_t j = 0; j < 4; ++j, pos = canvas->pixel(x, y + j))
    for (uint8_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL)
      blend(pos, (uint8_t *)sprite[j][i]);
}

void drawBeam(IsometricCanvas *canvas, const uint32_t x, const uint32_t y,
              const NBT &, const Colors::Block *const color) {
  /* No top to make it look more continuous
   * |    |
   * | DL |
   * | DL |
   * | DL | */
  uint8_t *pos = canvas->pixel(x + 1, y + 1);
  for (uint8_t i = 1; i < 4; i++, pos = canvas->pixel(x + 1, y + i)) {
    blend(pos, (uint8_t *)&color->dark);
    blend(pos + CHANSPERPIXEL, (uint8_t *)&color->light);
  }
}

void drawSlab(IsometricCanvas *canvas, const uint32_t x, const uint32_t y,
              const NBT &metadata, const Colors::Block *color) {
  /* This one has a hack to make it look like a gradual step up:
   * The second layer has primary colors to make the height difference
   * less obvious.
   * |    |    |PPPP|
   * |PPPP| or |DDLL| or full
   * |DPPL|    |DDLL|
   * |DDLL|    |    | */

  Colors::Block *fill = &canvas->air;
  std::string waterlogged = "false", type = "bottom";

  if (metadata.contains("Properties")) {
    if (metadata["Properties"].contains("waterlogged"))
      waterlogged = metadata["Properties"]["waterlogged"].get<string>();

    if (metadata["Properties"].contains("type"))
      type = metadata["Properties"]["type"].get<string>();
  }

  // Draw a full block if it is a double slab
  if (type == "double") {
    drawFull(canvas, x, y, metadata, color);
    return;
  }

  if (waterlogged == "true")
    fill = &canvas->water;

  const Colors::Color *spriteTop[4][4] = {{PRIME, PRIME, PRIME, PRIME},
                                          {DARK_, DARK_, LIGHT, LIGHT},
                                          {DARK_, DARK_, LIGHT, LIGHT},
                                          {FILL_, FILL_, FILL_, FILL_}};

  const Colors::Color *spriteBottom[4][4] = {{FILL_, FILL_, FILL_, FILL_},
                                             {PRIME, PRIME, PRIME, PRIME},
                                             {DARK_, PRIME, PRIME, LIGHT},
                                             {DARK_, DARK_, LIGHT, LIGHT}};

  const Colors::Color *(*target)[4][4] = &spriteBottom;

  if (type == "top")
    target = &spriteTop;

  uint8_t *pos = canvas->pixel(x, y);
  for (uint8_t j = 0; j < 4; ++j, pos = canvas->pixel(x, y + j))
    for (uint8_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL)
      blend(pos, (uint8_t *)(*target)[j][i]);
}

void drawWire(IsometricCanvas *canvas, const uint32_t x, const uint32_t y,
              const NBT &, const Colors::Block *color) {
  uint8_t *pos = canvas->pixel(x + 1, y + 3);
  memcpy(pos, &color->primary, BYTESPERPIXEL);
  memcpy(pos + CHANSPERPIXEL, &color->primary, BYTESPERPIXEL);
}

void drawLog(IsometricCanvas *canvas, const uint32_t x, const uint32_t y,
             const NBT &metadata, const Colors::Block *color) {

  string axis = "y";
  int sub = (float(color->primary.brightness()) / 323.0f + .21f);

  Colors::Color secondaryLight(color->secondary);
  Colors::Color secondaryDark(color->secondary);
  secondaryLight.modColor(sub - 15);
  secondaryDark.modColor(sub - 25);

  const Colors::Color *spriteY[4][4] = {{ALTER, ALTER, ALTER, ALTER},
                                        {DARK_, DARK_, LIGHT, LIGHT},
                                        {DARK_, DARK_, LIGHT, LIGHT},
                                        {DARK_, DARK_, LIGHT, LIGHT}};

  const Colors::Color *spriteX[4][4] = {{PRIME, PRIME, PRIME, PRIME},
                                        {ALT_D, ALT_D, LIGHT, LIGHT},
                                        {ALT_D, ALT_D, LIGHT, LIGHT},
                                        {ALT_D, ALT_D, LIGHT, LIGHT}};

  const Colors::Color *spriteZ[4][4] = {{PRIME, PRIME, PRIME, PRIME},
                                        {DARK_, DARK_, ALT_L, ALT_L},
                                        {DARK_, DARK_, ALT_L, ALT_L},
                                        {DARK_, DARK_, ALT_L, ALT_L}};

  const Colors::Color *(*target)[4][4] = &spriteY;

  if (metadata.contains("Properties") &&
      metadata["Properties"].contains("axis")) {
    axis = metadata["Properties"]["axis"].get<string>();

    if (axis == "x") {
      if (canvas->map.orientation == NW || canvas->map.orientation == SE)
        target = &spriteZ;
      else
        target = &spriteX;
    } else if (axis == "z") {
      if (canvas->map.orientation == NW || canvas->map.orientation == SE)
        target = &spriteX;
      else
        target = &spriteZ;
    }
  }

  uint8_t *pos = canvas->pixel(x, y);
  for (uint8_t j = 0; j < 4; ++j, pos = canvas->pixel(x, y + j))
    for (uint8_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL)
      memcpy(pos, (*target)[j][i], BYTESPERPIXEL);
}

void drawStair(IsometricCanvas *canvas, const uint32_t x, const uint32_t y,
               const NBT &metadata, const Colors::Block *color) {

  string facing = "north", half = "bottom", waterlogged = "false",
         shape = "straight";
  Colors::Block *fill = &canvas->air;

  if (metadata.contains("Properties")) {
    if (metadata["Properties"].contains("facing"))
      facing = metadata["Properties"]["facing"].get<string>();

    if (metadata["Properties"].contains("half"))
      half = metadata["Properties"]["half"].get<string>();

    if (metadata["Properties"].contains("waterlogged"))
      waterlogged = metadata["Properties"]["waterlogged"].get<string>();

    if (metadata["Properties"].contains("shape"))
      shape = metadata["Properties"]["shape"].get<string>();
  }

  if (waterlogged == "true")
    fill = &canvas->water;

  const Colors::Color *spriteNorth[4][4] = {{FILL_, FILL_, PRIME, PRIME},
                                            {PRIME, PRIME, DARK_, DARK_},
                                            {DARK_, PRIME, PRIME, LIGHT},
                                            {DARK_, DARK_, DARK_, LIGHT}};

  const Colors::Color *spriteWest[4][4] = {{PRIME, PRIME, FILL_, FILL_},
                                           {LIGHT, LIGHT, PRIME, PRIME},
                                           {DARK_, PRIME, PRIME, LIGHT},
                                           {DARK_, LIGHT, LIGHT, LIGHT}};

  const Colors::Color *spriteSouth[4][4] = {{PRIME, PRIME, FILL_, FILL_},
                                            {DARK_, DARK_, PRIME, PRIME},
                                            {DARK_, DARK_, LIGHT, LIGHT},
                                            {DARK_, DARK_, LIGHT, LIGHT}};

  const Colors::Color *spriteEast[4][4] = {{FILL_, FILL_, PRIME, PRIME},
                                           {PRIME, PRIME, LIGHT, LIGHT},
                                           {DARK_, DARK_, LIGHT, LIGHT},
                                           {DARK_, DARK_, LIGHT, LIGHT}};

  const Colors::Color *spriteNorthWest[4][4] = {{PRIME, PRIME, PRIME, PRIME},
                                                {LIGHT, LIGHT, DARK_, DARK_},
                                                {PRIME, PRIME, PRIME, PRIME},
                                                {DARK_, DARK_, LIGHT, LIGHT}};

  const Colors::Color *spriteSouthEast[4][4] = {{PRIME, PRIME, PRIME, PRIME},
                                                {DARK_, DARK_, LIGHT, LIGHT},
                                                {DARK_, DARK_, LIGHT, LIGHT},
                                                {DARK_, DARK_, LIGHT, LIGHT}};

#define spriteNorthEast spriteEast
#define spriteSouthWest spriteSouth

  const void *straight[4] = {&spriteNorth, &spriteWest, &spriteSouth,
                             &spriteEast};
  const void *inner[4] = {&spriteNorthEast, &spriteNorthWest, &spriteSouthWest,
                          &spriteSouthEast};
  const Colors::Color *(*target)[4][4] = &spriteNorth;

#undef spriteNorthEast
#undef spriteSouthWest

  std::map<std::string, int> directions = {
      {"north", 0}, {"west", 1}, {"south", 2}, {"east", 3}};

  int reference = (directions[facing] + 4 - canvas->map.orientation) % 4;

  if (shape == "straight") {
    target = (const Colors::Color *(*)[4][4])straight[reference];
  } else if (shape == "inner_right") {
    target = (const Colors::Color *(*)[4][4])inner[reference];
  } else if (shape == "inner_left") {
    reference = (reference + 1) % 4;
    target = (const Colors::Color *(*)[4][4])inner[reference];
  }

  if (half == "top")
    target = &spriteSouthEast;

  uint8_t *pos = canvas->pixel(x, y);
  for (uint8_t j = 0; j < 4; ++j, pos = canvas->pixel(x, y + j))
    for (uint8_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL)
      blend(pos, (uint8_t *)(*target)[j][i]);
}

void drawFull(IsometricCanvas *canvas, const uint32_t x, const uint32_t y,
              const NBT &, const Colors::Block *color) {
  // Sets canvas->pixels around x,y where A is the anchor
  // T = given color, D = darker, L = lighter
  // A T T T
  // D D L L
  // D D L L
  // D D L L

  const Colors::Color *sprite[4][4] = {{PRIME, PRIME, PRIME, PRIME},
                                       {DARK_, DARK_, LIGHT, LIGHT},
                                       {DARK_, DARK_, LIGHT, LIGHT},
                                       {DARK_, DARK_, LIGHT, LIGHT}};

  // Top row
  uint8_t *pos = canvas->pixel(x, y);

  if (color->primary.ALPHA == 255) {
    for (uint8_t j = 0; j < 4; ++j, pos = canvas->pixel(x, y + j))
      for (uint8_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL)
        memcpy(pos, sprite[j][i], BYTESPERPIXEL);
  } else {
    for (uint8_t j = 0; j < 4; ++j, pos = canvas->pixel(x, y + j))
      for (uint8_t i = 0; i < 4; ++i, pos += CHANSPERPIXEL)
        blend(pos, (uint8_t *)sprite[j][i]);
  }
}
