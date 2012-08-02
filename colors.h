#ifndef _COLORS_
#define _COLORS_

#include "helper.h"
#include <cmath>

// Byte order see below. Colors aligned to word boundaries for some speedup
// Brightness is precalculated to speed up calculations later
// Colors are stored twice since BMP and PNG need them in different order
// Noise is supposed to look normal when -noise 10 is given
extern uint8_t colors[256][8];
#define PRED 0
#define PGREEN 1
#define PBLUE 2
#define PALPHA 3
#define NOISE 4
#define BRIGHTNESS 5

#define GETBRIGHTNESS(c) (uint8_t)sqrt( \
                                        double(PRED[c]) *  double(PRED[c]) * .236 + \
                                        double(PGREEN[c]) *  double(PGREEN[c]) * .601 + \
                                        double(PBLUE[c]) *  double(PBLUE[c]) * .163)

void loadColors();
bool loadColorsFromFile(const char *file);
bool dumpColorsToFile(const char *file);
bool extractColors(const char *file);
bool loadBiomeColors(const char* path);

#define AIR 0
#define STONE 1
#define GRASS 2
#define DIRT 3
#define COBBLESTONE 4
#define WOOD 5
#define WATER 8
#define STAT_WATER 9
#define LAVA 10
#define STAT_LAVA 11
#define SAND 12
#define GRAVEL 13
#define LOG 17
#define LEAVES 18
#define SANDSTONE 24
#define BED 26
#define POW_RAILROAD 27
#define DET_RAILROAD 28
#define COBWEB 30
#define TALL_GRASS 31
#define SHRUB 32
#define WOOL 35
#define FLOWERY 37
#define FLOWERR 38
#define MUSHROOMB 39
#define MUSHROOMR 40
#define DOUBLESTEP 43
#define STEP 44
#define TORCH 50
#define FIRE 51
#define REDWIRE 55
#define RAILROAD 66
#define REDTORCH_OFF 75
#define REDTORCH_ON 76
#define SNOW 78
#define FENCE 85
#define CAKE 92
#define TRAPDOOR 96
#define IRON_BARS 101
#define PUMPKIN_STEM 104
#define MELON_STEM 105
#define VINES 106
#define FENCE_GATE 107
#define MYCELIUM 110
#define LILYPAD 111
#define NETHER_BRICK 112
#define NETHER_BRICK_FENCE 113
#define NETHER_BRICK_STAIRS 114
#define NETHER_WART 115
#define WOODEN_DOUBLE_STEP 125
#define WOODEN_STEP 126
#define COCOA_PLANT 127
#define TRIPWIRE_HOOK 131
#define TRIPWIRE 132

#define SANDSTEP 201
#define WOODSTEP 202
#define COBBLESTEP 203
#define BRICKSTEP 204
#define STONEBRICKSTEP 205

#define UP_STEP 208
#define UP_SANDSTEP 209
#define UP_WOODSTEP 210
#define UP_COBBLESTEP 211
#define UP_BRICKSTEP 212
#define UP_STONEBRICKSTEP 213

#define PINESTEP 214
#define BIRCHSTEP 215
#define JUNGLESTEP 216

#define UP_WOODSTEP2 221
#define UP_PINESTEP 222
#define UP_BIRCHSTEP 223
#define UP_JUNGLESTEP 224

#define PINELEAVES 229
#define BIRCHLEAVES 230
#define JUNGLELEAVES 231

#endif
