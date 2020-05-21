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

void printProgress(const size_t current, const size_t max) {
  static float lastp = -10;
  static time_t lastt = 0;
  if (current == 0) { // Reset
    lastp = -10;
    lastt = 0;
  }
  time_t now = time(NULL);
  if (now > lastt || current == max) {
    float proc = (float(current) / float(max)) * 100.0f;
    if (proc > lastp + 0.99f || current == max) {
      // Keep user updated but don't spam the console
      printf("[%.2f%%]\r", proc);
      fflush(stdout);
      lastt = now;
      lastp = proc;
    }
  }
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
  for (size_t index = 0; index < count; index++) {
    subCoords[index] = Coordinates(original);
    subCoords[index].minX =
        (index ? subCoords[index - 1].maxX + 1 : original.minX);
    subCoords[index].maxX =
        (original.maxX - original.minX + 1) / count + subCoords[index].minX - 1;
    if (index == count - 1)
      subCoords[index].maxX = original.maxX;
  }
}
