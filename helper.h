#ifndef _HELPER_H_
#define _HELPER_H_

// Just in case these ever change
#define CHUNKSIZE_Z 16
#define CHUNKSIZE_X 16
#define CHUNKSIZE_Y 128
#define SECTION_Y 16
#define SECTION_Y_SHIFT 4
// Some macros for easier array access
// First: Block array
#define BLOCKAT(x,y,z) g_Terrain[(y) + ((z) + ((x) * g_MapsizeZ)) * g_MapsizeY]
#define BLOCKEAST(x,y,z) g_Terrain[(y) + ((g_MapsizeZ - ((x) + 1)) + ((z) * g_MapsizeZ)) * g_MapsizeY]
#define BLOCKWEST(x,y,z) g_Terrain[(y) + ((x) + ((g_MapsizeX - ((z) + 1)) * g_MapsizeZ)) * g_MapsizeY]
#define BLOCKNORTH(x,y,z) g_Terrain[(y) + ((z) + ((x) * g_MapsizeZ)) * g_MapsizeY]
#define BLOCKSOUTH(x,y,z) g_Terrain[(y) + ((g_MapsizeZ - ((z) + 1)) + ((g_MapsizeX - ((x) + 1)) * g_MapsizeZ)) * g_MapsizeY]
//#define BLOCKAT(x,y,z) g_Terrain[(x) + ((z) + ((y) * g_MapsizeZ)) * g_MapsizeX]
//#define BLOCKEAST(x,y,z) g_Terrain[(z) + ((g_MapsizeZ - ((x) + 1)) + ((y) * g_MapsizeZ)) * g_MapsizeX]
// Same for lightmap
#define GETLIGHTAT(x,y,z) ((g_Light[((y) / 2) + ((z) + ((x) * g_MapsizeZ)) * ((g_MapsizeY + 1) / 2)] >> (((y) % 2) * 4)) & 0xF)
#define SETLIGHTEAST(x,y,z) g_Light[((y) / 2) + ((g_MapsizeZ - ((x) + 1)) + ((z) * g_MapsizeZ)) * ((g_MapsizeY + 1) / 2)]
#define SETLIGHTWEST(x,y,z) g_Light[((y) / 2) + ((x) + ((g_MapsizeX - ((z) + 1)) * g_MapsizeZ)) * ((g_MapsizeY + 1) / 2)]
#define SETLIGHTNORTH(x,y,z) g_Light[((y) / 2) + ((z) + ((x) * g_MapsizeZ)) * ((g_MapsizeY + 1) / 2)]
#define SETLIGHTSOUTH(x,y,z) g_Light[((y) / 2) + ((g_MapsizeZ - ((z) + 1)) + ((g_MapsizeX - ((x) + 1)) * g_MapsizeZ)) * ((g_MapsizeY + 1) / 2)]
// Biome array
#define BIOMEAT(x,z) g_BiomeMap[(z) + ((x) * g_MapsizeZ)]
#define BIOMEEAST(x,z) g_BiomeMap[(g_MapsizeZ - ((x) + 1)) + ((z) * g_MapsizeZ)]
#define BIOMEWEST(x,z) g_BiomeMap[(x) + ((g_MapsizeX - ((z) + 1)) * g_MapsizeZ)]
#define BIOMENORTH(x,z) g_BiomeMap[(z) + ((x) * g_MapsizeZ)]
#define BIOMESOUTH(x,z) g_BiomeMap[(g_MapsizeZ - ((z) + 1)) + ((g_MapsizeX - ((x) + 1)) * g_MapsizeZ)]
// Heightmap array
#define HEIGHTAT(x,z) g_HeightMap[(z) + ((x) * g_MapsizeZ)]


#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
//#define RIGHTSTRING(x,y) ((x) + strlen(x) - ((y) > strlen(x) ? strlen(x) : (y)))
#define RIGHTSTRING(x,y) (strlen(x) >= (y) ? (x) + strlen(x) - (y) : (x))

#include <string>

// Difference between MSVC++ and gcc/others
#if defined(_WIN32) && !defined(__GNUC__)
#	include <windows.h>
#	define usleep(x) Sleep((x) / 1000);
#else
#	include <unistd.h>
#endif

// For fseek
#if defined(_WIN32) && !defined(__GNUC__)
// MSVC++
#	define fseek64 _fseeki64
#elif defined(__APPLE__)
#	define fseek64 fseeko
#elif defined(__FreeBSD__)
#   define fseek64 fseeko
#else
#	define fseek64 fseeko64
#endif

// Differently named
#if defined(_WIN32) && !defined(__GNUC__)
#	define snprintf _snprintf
#  define mkdir _mkdir
#endif


// If this is missing for you in Visual Studio: See http://en.wikipedia.org/wiki/Stdint.h#External_links
#include <stdint.h>

using std::string;

string base36(int val);
int base10(char *val);
uint8_t clamp(int32_t val);
void printProgress(const size_t current, const size_t max);
bool fileExists(const char *strFilename);
bool dirExists(const char *strFilename);
bool isNumeric(char *str);
bool isAlphaWorld(char *path);

#endif
