#ifndef HELPER_H_
#define HELPER_H_

#include <filesystem>

namespace fs = std::filesystem;

#if defined(__clang__) || defined(__GNUC__)
#define NOINLINE __attribute__((noinline))
#else
#define NOINLINE
#endif

#if defined(_OPENMP) && defined(_WINDOWS)
#define OMP_FOR_INDEX int
#else
#define OMP_FOR_INDEX std::vector<World::Coordinates>::size_type
#endif

#ifdef _WINDOWS
#define FSEEK fseek
#else
#define FSEEK fseeko
#endif

#define CHUNKSIZE 16
#define REGIONSIZE 32

namespace mcmap {
namespace constants {
const int16_t min_y = 0;
const int16_t max_y = 255;
const uint16_t terrain_height = max_y - min_y + 1;

const int8_t color_offset_left = -27;
const int8_t color_offset_right = -17;

const int8_t lighting_dark = -75;
const int8_t lighting_bright = 100;
const int8_t lighting_delta = (lighting_bright - lighting_dark) >> 4;
} // namespace constants
} // namespace mcmap

#define REGION_HEADER_SIZE REGIONSIZE *REGIONSIZE * 4
#define DECOMPRESSED_BUFFER 1000 * 1024
#define COMPRESSED_BUFFER 500 * 1024

#define CHUNK(x) ((x) >> 4)
#define REGION(x) ((x) >> 5)

uint8_t clamp(int32_t val);
bool isNumeric(const char *str);

uint32_t _ntohl(uint8_t *val);

size_t memory_capacity(size_t, size_t, size_t, size_t);
bool prepare_cache(const fs::path &);

fs::path getHome();
fs::path getSaveDir();

#endif // HELPER_H_
