#ifndef HELPER_H_
#define HELPER_H_

#define CHUNK(x) ((x) >> 4)
#define REGION(x) ((x) >> 5)

#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>

#define REGIONSIZE 32
#define CHUNKS_PER_BIOME_FILE 32

#define REGION_HEADER_SIZE REGIONSIZE *REGIONSIZE * 4
#define DECOMPRESSED_BUFFER 1000 * 1024
#define COMPRESSED_BUFFER 100 * 1024

// Just in case these ever change
#define CHUNKSIZE_Z 16
#define CHUNKSIZE_X 16
#define CHUNKSIZE_Y 256
#define SECTION_Y 16
#define SECTION_Y_SHIFT 4
// Some macros for easier array access
// First: Block array

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define RIGHTSTRING(x, y) (strlen(x) >= (y) ? (x) + strlen(x) - (y) : (x))

#include <string>

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

#include <stdint.h>

using std::string;

uint8_t clamp(int32_t val);
void printProgress(const size_t current, const size_t max);
bool dirExists(const char *strFilename);
bool isNumeric(char *str);

static inline uint16_t _ntohs(uint8_t *val) {
  return (uint16_t(val[0]) << 8) + (uint16_t(val[1]));
}

static inline uint32_t _ntohl(uint8_t *val) {
  return (uint32_t(val[0]) << 24) + (uint32_t(val[1]) << 16) +
         (uint32_t(val[2]) << 8) + (uint32_t(val[3]));
}

static inline uint64_t _ntohll(uint8_t *val) {
  return ((uint64_t)val[0] << 56) + ((uint64_t)val[1] << 48) +
         ((uint64_t)val[2] << 40) + ((uint64_t)val[3] << 32) +
         ((uint64_t)val[4] << 24) + ((uint64_t)val[5] << 16) +
         ((uint64_t)val[6] << 8) +
         ((uint64_t)val[7]); // Looks like crap, but should be endian-safe
}

#endif // HELPER_H_
