#ifndef _COLORS_
#define _COLORS_

#include "helper.h"
#include "json.hpp"
#include <map>
#include <list>
#include <string>
#include <filesystem>

// Byte order see below. Colors aligned to word boundaries for some speedup
// Brightness is precalculated to speed up calculations later
// Colors are stored twice since BMP and PNG need them in different order
// Noise is supposed to look normal when -noise 10 is given

// First 256 lines correspond to individual block IDs
// ie colors[0] contains color inforamtions for air
//
// Following 256 lines store variant information
// lines colors[256] to color[263] stores stone variants

using std::map;
using std::list;
using std::string;
using nlohmann::json;

typedef map<string, list<int>> colorMap;

#define PRED 0
#define PGREEN 1
#define PBLUE 2
#define PALPHA 3
#define NOISE 4
#define BRIGHTNESS 5
#define VINDEX 6	    // Use one of the unused fields to store variant info
			    // Here we store the offset from 255, the last ID
			    // ie Dirt variants begin at 263 -> VINDEX = 263 - 255 = 8
			    // This is for the info to fir on a uint8_t

#define BLOCK_TYPE 7	    // The type of block
			    // Influences the way it is drawn

#define GETBRIGHTNESS(c) (uint8_t)sqrt( \
                                        double(PRED[c]) *  double(PRED[c]) * .236 + \
                                        double(PGREEN[c]) *  double(PGREEN[c]) * .601 + \
                                        double(PBLUE[c]) *  double(PBLUE[c]) * .163)

bool loadColors(const std::filesystem::path& colorFile, colorMap&);
/*bool loadColorsFromFile(const char *file);
bool dumpColorsToFile(const char *file);
bool extractColors(const char *file);
bool loadBiomeColors(const char* path);*/

#endif
