#include "helper.h"

#ifndef S_ISDIR
#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#endif

uint8_t clamp(int32_t val) {
  if (val < 0) {
    return 0;
  }
  if (val > 255) {
    return 255;
  }
  return (uint8_t)val;
}

bool dirExists(const char *strFilename) {
  struct stat stFileInfo;
  int ret;
  ret = stat(strFilename, &stFileInfo);
  if (ret == 0) {
    return S_ISDIR(stFileInfo.st_mode);
  }
  return false;
}

bool isNumeric(char *str) {
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

void splitCoords(const Coordinates &original, Coordinates *&subCoords,
                 const size_t count) {
  // Split the coordinates of the entire terrain in `count` terrain fragments

  for (size_t index = 0; index < count; index++) {
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
}
