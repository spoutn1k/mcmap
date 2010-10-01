#ifndef _HELPER_H_
#define _HELPER_H_

// Just in case these ever change
#define CHUNKSIZE_Z 16
#define CHUNKSIZE_X 16
#define CHUNKSIZE_Y 128
// Some macros for easier array access
// First: Block array
#define BLOCKAT(x,y,z) g_Terrain[(y) + ((z) + ((x) * MAPSIZE_Z)) * MAPSIZE_Y]
#define BLOCKEAST(x,y,z) g_Terrain[(y) + ((MAPSIZE_Z - ((x) + 1)) + ((z) * MAPSIZE_Z)) * MAPSIZE_Y]
#define BLOCKWEST(x,y,z) g_Terrain[(y) + ((x) + ((MAPSIZE_X - ((z) + 1)) * MAPSIZE_Z)) * MAPSIZE_Y]
#define BLOCKNORTH(x,y,z) g_Terrain[(y) + ((z) + ((x) * MAPSIZE_Z)) * MAPSIZE_Y]
#define BLOCKSOUTH(x,y,z) g_Terrain[(y) + ((MAPSIZE_Z - ((z) + 1)) + ((MAPSIZE_X - ((x) + 1)) * MAPSIZE_Z)) * MAPSIZE_Y]
//#define BLOCKAT(x,y,z) g_Terrain[(x) + ((z) + ((y) * MAPSIZE_Z)) * MAPSIZE_X]
//#define BLOCKEAST(x,y,z) g_Terrain[(z) + ((MAPSIZE_Z - ((x) + 1)) + ((y) * MAPSIZE_Z)) * MAPSIZE_X]
// Same for lightmap
#define GETLIGHTAT(x,y,z) ((g_Light[((y) / 2) + ((z) + ((x) * MAPSIZE_Z)) * ((MAPSIZE_Y + 1) / 2)] >> (((y) % 2) * 4)) & 0xF)
#define SETLIGHTEAST(x,y,z) g_Light[((y) / 2) + ((MAPSIZE_Z - ((x) + 1)) + ((z) * MAPSIZE_Z)) * ((MAPSIZE_Y + 1) / 2)]
#define SETLIGHTWEST(x,y,z) g_Light[((y) / 2) + ((x) + ((MAPSIZE_X - ((z) + 1)) * MAPSIZE_Z)) * ((MAPSIZE_Y + 1) / 2)]
#define SETLIGHTNORTH(x,y,z) g_Light[((y) / 2) + ((z) + ((x) * MAPSIZE_Z)) * ((MAPSIZE_Y + 1) / 2)]
#define SETLIGHTSOUTH(x,y,z) g_Light[((y) / 2) + ((MAPSIZE_Z - ((z) + 1)) + ((MAPSIZE_X - ((x) + 1)) * MAPSIZE_Z)) * ((MAPSIZE_Y + 1) / 2)]

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))


#include <string>

#if defined(_WIN32) && !defined(__GNUC__)
#	include <windows.h>
#	define usleep(x) Sleep((x) / 1000);
#else
#	include <unistd.h>
#endif

// If this is missing for you in Visual Studio: See http://en.wikipedia.org/wiki/Stdint.h#External_links
#include <stdint.h>

using std::string;

string base36(int val);
int base10(char* val);
uint8_t clamp(int32_t val);
void printProgress(const size_t current, const size_t max);
bool fileExists(const char* strFilename);
bool isNumeric(char* str);

#endif
