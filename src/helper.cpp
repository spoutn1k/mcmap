#include "helper.h"

#ifndef S_ISDIR
#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#endif

uint32_t _ntohl(uint8_t *val) {
  return (uint32_t(val[0]) << 24) + (uint32_t(val[1]) << 16) +
         (uint32_t(val[2]) << 8) + (uint32_t(val[3]));
}

uint8_t clamp(int32_t val) {
  if (val < 0) {
    return 0;
  }
  if (val > 255) {
    return 255;
  }
  return (uint8_t)val;
}

bool isNumeric(const char *str) {
  if (str[0] == '-' && str[1] != '\0') {
    ++str;
  }
  while (*str != '\0') {
    if (*str < '0' || *str > '9') {
      return false;
    }
    ++str;
  }
  return true;
}

void splitCoords(const Coordinates<int32_t> &original,
                 Coordinates<int32_t> *subCoords, const uint16_t count) {
  // Split the coordinates of the entire terrain in `count` terrain fragments
  uint32_t width = (original.sizeX() + original.sizeZ()) * 2;
  uint32_t height = original.sizeX() + original.sizeZ() +
                    (original.maxY - original.minY + 1) * 3 - 1;
  logger::debug("Rendering map {} requires a {}x{} image of {}MB\n",
                original.to_string(), width, height,
                width * height * 4 / (1024 * 1024));

  for (uint16_t index = 0; index < count; index++) {
    // Initialization with the original's values
    subCoords[index] = Coordinates(original);

    // The first fragment begins at the terrain's beginning, the other fragments
    // begin one block behind the last fragment
    subCoords[index].minX =
        (index ? subCoords[index - 1].maxX + 1 : original.minX);

    // Each fragment has a fixed size
    subCoords[index].maxX =
        subCoords[index].minX + (original.maxX - original.minX + 1) / count - 1;

    // Adjust the last terrain fragment to make sure the terrain is fully
    // covered
    if (index == count - 1)
      subCoords[index].maxX = original.maxX;
  }

  if (count > 1) {
    width = (subCoords[0].sizeX() + subCoords[0].sizeZ()) * 2;
    height = subCoords[0].sizeX() + subCoords[0].sizeZ() +
             (subCoords[0].maxY - subCoords[0].minY + 1) * 3 - 1;
    logger::debug("Splitting into {} maps requiring {} {}x{} images of {}MB\n",
                  count, count, width, height,
                  width * height * 4 / (1024 * 1024));
  }
}
