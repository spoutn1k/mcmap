#ifndef _COLORS_
#define _COLORS_

#include "helper.h"

// Byte order is: blue green red alpha brightness noise
// Brightness is used to speed up calculations later
extern uint8_t colors[256][6];
#define ALPHA 3
#define BRIGHTNESS 4
#define NOISE 5

void loadColors();

#define AIR 0
#define STONE 1
#define GRASS 2
#define DIRT 3
#define COBBLESTONE 4
#define WOOD 5
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
