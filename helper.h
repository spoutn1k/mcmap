#ifndef _HELPER_H_
#define _HELPER_H_

// Just in case these ever change
#define CHUNKSIZE_Z 16
#define CHUNKSIZE_X 16
#define SLICESIZE_Y 128
// Some macros for easier array access
// First: Block array
//#define BLOCKAT(x,y,z) g_Terrain[(y) + ((z) * MAPSIZE_Y + ((x) * MAPSIZE_Y * MAPSIZE_Z))]
//#define BLOCKROTATED(x,y,z) g_Terrain[(y) + ((MAPSIZE_Z - ((x) + 1)) * MAPSIZE_Y + ((z) * MAPSIZE_Y * MAPSIZE_Z))]
// I changed the order in which the bytes are stored in the array for a little speedup.
// The way this array is accessed, this makes much better use of the CPU's cache.
// If this looks confusing to you: Yes it is. The data needs to be flipped/rotated to draw the map
// properly, that's why there are two macros. One for writing, one for reading
#define BLOCKAT(x,y,z) g_Terrain[(x) + ((z) + ((y) * MAPSIZE_Z))  * MAPSIZE_X]
#define BLOCKROTATED(x,y,z) g_Terrain[(z) + ((MAPSIZE_Z - ((x) + 1)) + ((y) * MAPSIZE_Z)) * MAPSIZE_X]
// Same for lightmap
#define GETLIGHTAT(x,y,z) ((g_Light[(x) + ((z) + (((y) / 2) * MAPSIZE_Z)) * MAPSIZE_X] >> (((y) % 2) * 4)) & 0xF)
#define SETLIGHTROTATED(x,y,z) g_Light[(z) + ((MAPSIZE_Z - ((x) + 1)) + (((y) / 2) * MAPSIZE_Z)) * MAPSIZE_X]



#include <cstdio>
#include <string>
#include <ctime>

#if defined(_WIN32) && !defined(__GNUC__)
#include <windows.h>
#define usleep(x) Sleep((x) / 1000);
// See http://en.wikipedia.org/wiki/Stdint.h#External_links
#include <stdint.h>
#else
#include <unistd.h>
#endif

using std::string;

string base36(int val);
int base10(char* val);
uint8_t clamp(int32_t val);
void printProgress(const int current, const int max);

#endif
