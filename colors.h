#ifndef _COLORS_
#define _COLORS_

#include "helper.h"
#include <cmath>

// Byte order see below. Colors aligned to word boundaries for some speedup
// Brightness is precalculated to speed up calculations later
// Colors are stored twice since BMP and PNG need them in different order
// Noise is supposed to look normal when -noise 10 is given
extern uint8_t colors[65536][8];
extern int16_t biomes[256][4];
extern uint8_t colorsToMap[65536];
extern uint16_t colorsToID[256];

#define PRED 0
#define PGREEN 1
#define PBLUE 2
#define PALPHA 3
#define NOISE 4
#define BRIGHTNESS 5
#define BLOCKTYPE 6

#define GETBRIGHTNESS(c) (uint8_t)sqrt( \
                                        double(PRED[c]) *  double(PRED[c]) * .236 + \
                                        double(PGREEN[c]) *  double(PGREEN[c]) * .601 + \
                                        double(PBLUE[c]) *  double(PBLUE[c]) * .163)

void SET_COLORNOISE(uint16_t col, uint16_t r, uint16_t g, uint16_t b, uint16_t a, uint16_t n);
void SET_COLOR(uint16_t col, uint16_t r, uint16_t g, uint16_t b, uint16_t a);
void SET_COLOR_W(uint16_t col, uint16_t r, uint16_t g, uint16_t b, uint16_t a);
void SET_COLOR_C(uint16_t col, uint16_t r, uint16_t g, uint16_t b, uint16_t a);
void SET_COLOR1(uint16_t col, uint16_t r, uint16_t g, uint16_t b, uint16_t a);
void loadColors();
bool loadColorsFromFile(const char *file);
bool dumpColorsToFile(const char *file);
bool extractColors(const char *file);
bool loadBiomeColors(const char* path);

#define AIR 0
#define STONE 1
#define GRASS 2
#define DIRT 3
#define GRASSBOTTOM 3
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
#define LEAVES2 161

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

#define CARPET 171

#define BLUE_ORCHID 165
#define ALLIUM 176
#define AZURE_BLUET 235
#define RED_TULIP 38
#define ORANGE_TULIP 217
#define WHITE_TULIP 218
#define PINK_TULIP 219
#define OXEYE_DAISY 220
#define SUNFLOWER 37
#define LILAC 233
#define PEONY 177

#define REDSAND 153

#define BLOCKSOLID 0
#define BLOCKFLAT 1
#define BLOCKTORCH 2
#define BLOCKFLOWER 3
#define BLOCKFENCE 4
#define BLOCKWIRE 5
#define BLOCKRAILROAD 6
#define BLOCKGRASS 7
#define BLOCKFIRE 8
#define BLOCKSTEP 9
#define BLOCKUPSTEP 10
#define BLOCKBIOME 128

/*
115 	SOLID 0	
	FLAT 1		108 CARPET	116-
	FIRE 8		105 BLAZE	102
	FLOWER 3	108 PLANT	114-
	FENCE 4		102 ???		102
116 	TORCH 2	
119 	WIRE 5	
114 	RAILROAD 6	
103 	GRASS 7	
115 	STEP 9	
117 	UPSTEP 10	

*/

#endif
