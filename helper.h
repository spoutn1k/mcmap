#ifndef HELPER_H_
#define HELPER_H_

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <limits>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define RIGHTSTRING(x, y) (strlen(x) >= (y) ? (x) + strlen(x) - (y) : (x))

// Difference between MSVC++ and gcc/others
#if defined(_WIN32) && !defined(__GNUC__)
#include <windows.h>
#define usleep(x) Sleep((x) / 1000);
#else
#include <unistd.h>
#endif

// For fseek
#if defined(_WIN32) && !defined(__GNUC__)
// MSVC++
#define fseek64 _fseeki64
#elif defined(__APPLE__)
#define fseek64 fseeko
#elif defined(__FreeBSD__)
#define fseek64 fseeko
#else
#define fseek64 fseeko64
#endif

// Differently named
#if defined(_WIN32) && !defined(__GNUC__)
#define snprintf _snprintf
#define mkdir _mkdir
#endif

#define CHUNKSIZE 16
#define REGIONSIZE 32
#define MIN_TERRAIN_HEIGHT 0
#define MAX_TERRAIN_HEIGHT 255

#define REGION_HEADER_SIZE REGIONSIZE *REGIONSIZE * 4
#define DECOMPRESSED_BUFFER 1000 * 1024
#define COMPRESSED_BUFFER 100 * 1024

#define CHUNK(x) ((x) >> 4)
#define REGION(x) ((x) >> 5)

uint8_t clamp(int32_t val);
void printProgress(const size_t current, const size_t max);
bool dirExists(const char *strFilename);
bool isNumeric(char *str);

static inline uint32_t _ntohl(uint8_t *val) {
  return (uint32_t(val[0]) << 24) + (uint32_t(val[1]) << 16) +
         (uint32_t(val[2]) << 8) + (uint32_t(val[3]));
}

enum Orientation {
  NW,
  SW,
  NE,
  SE,
};

// A simple coordinates structure
struct Coordinates {
  int32_t minX, maxX, minZ, maxZ;
  uint8_t minY, maxY;
  Orientation orientation;

  Coordinates() {
    minX = maxX = minZ = maxZ = minY = maxY = 0;
    orientation = NW;
  }

  Coordinates(int32_t init) : Coordinates() {
    minX = maxX = minZ = maxZ = init;
  }

  void setUndefined() {
    minX = minZ = std::numeric_limits<int32_t>::max();
    maxX = maxZ = std::numeric_limits<int32_t>::min();
  }

  bool isUndefined() {
    return (minX == minZ && minX == std::numeric_limits<int32_t>::max() &&
            maxX == maxZ && maxX == std::numeric_limits<int32_t>::min());
  }

  void crop(const Coordinates &boundaries) {
    minX = std::max(minX, boundaries.minX);
    minZ = std::max(minZ, boundaries.minZ);
    maxX = std::min(maxX, boundaries.maxX);
    maxZ = std::min(maxZ, boundaries.maxZ);
  }
};

void splitCoords(const Coordinates &original, Coordinates *&subCoords,
                 const size_t count);
#endif // HELPER_H_
