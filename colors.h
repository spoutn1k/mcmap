#ifndef _COLORS_
#define _COLORS_

#include "helper.h"
#include <cmath>

#define GETBRIGHTNESS(c) (uint8_t)sqrt( \
			double(2[c] * 2[c]) * .236 + \
			double(1[c] * 1[c]) * .601 + \
			double(0[c] * 0[c]) * .163)

// Byte order is: blue green red alpha noise brightness
// Brightness is used to speed up calculations later
extern uint8_t colors[256][6];
#define BLUE 0
#define GREEN 1
#define RED 2
#define ALPHA 3
#define NOISE 4
#define BRIGHTNESS 5

void loadColors();
bool loadColorsFromFile(const char* file);
bool dumpColorsToFile(const char* file);

#define AIR 0
#define STONE 1
#define GRASS 2
#define DIRT 3
#define COBBLESTONE 4
#define WOOD 5
#define WATER 8
#define STAT_WATER 9
#define SAND 12
#define GRAVEL 13
#define LOG 17
#define LEAVES 18
#define FLOWERY 37
#define FLOWERR 38
#define MUSHROOMB 39
#define MUSHROOMR 40
#define DOUBLESTEP 43
#define STEP 44
#define TORCH 50
#define FIRE 51
#define REDTORCH_OFF 75
#define REDTORCH_ON 76
#define SNOW 78
#define FENCE 85

#endif
