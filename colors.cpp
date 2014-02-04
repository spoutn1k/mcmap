#include "colors.h"
#include "extractcolors.h"
#include "pngreader.h"
#include "globals.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>

#define SETCOLOR(col,r,g,b,a) do { \
		colors[col][PBLUE]		= b; \
		colors[col][PGREEN] 		= g; \
		colors[col][PRED] 		= r; \
		colors[col][PALPHA] 		= a; \
		colors[col][BRIGHTNESS]	= (uint8_t)sqrt( \
		                          double(r) *  double(r) * .236 + \
		                          double(g) *  double(g) * .601 + \
		                          double(b) *  double(b) * .163); \
	} while (false)

#define SETCOLORNOISE(col,r,g,b,a,n) do { \
		colors[col][PBLUE]		= b; \
		colors[col][PGREEN] 		= g; \
		colors[col][PRED] 		= r; \
		colors[col][PALPHA] 		= a; \
		colors[col][NOISE]		= n; \
		colors[col][BRIGHTNESS]	= (uint8_t)sqrt( \
		                          double(r) *  double(r) * .236 + \
		                          double(g) *  double(g) * .601 + \
		                          double(b) *  double(b) * .163); \
	} while (false)

// See header for description
uint8_t colors[65536][8];


void SET_COLORNOISE(uint16_t col, uint16_t r, uint16_t g, uint16_t b, uint16_t a, uint16_t n)
{
	col %= 4096;
	for (int i = 0; i < 16; i++)
	{
		int x = col + (i << 12);
		SETCOLORNOISE(x, r, g, b, a, n);
	}
}
void SET_COLOR(uint16_t col, uint16_t r, uint16_t g, uint16_t b, uint16_t a)
{
	SET_COLORNOISE(col,r,g,b,a,0);
}
void SET_COLOR1(uint16_t col, uint16_t r, uint16_t g, uint16_t b, uint16_t a)
{
	SETCOLORNOISE(col,r,g,b,a,0);
}
void SET_COLOR_W(uint16_t col, uint16_t r, uint16_t g, uint16_t b, uint16_t a)
{
	col -= 239;
	//uint16_t col2 = CARPET + (col<<12);
	col = 35 + (col<<12);
	SETCOLORNOISE(col,r,g,b,a,0);
	//SETCOLORNOISE(col2,r,g,b,a,0);
}
void SET_COLOR_C(uint16_t col, uint16_t r, uint16_t g, uint16_t b, uint16_t a)
{
	col -= 185;
	col = 159 + (col<<12);
	SETCOLORNOISE(col,r,g,b,a,0);
}

void COLOR_COPY(uint16_t from, uint16_t to)
{
	for (int i = 0; i < 16; i++)
	{
		memcpy(colors[(i<<12)+to], colors[(i<<12)+from], 6);
	}
}
void SET_BLOCK(uint8_t type, uint8_t block)
{
	for (int i = 0; i < 16; i++)
	{
		colors[(i<<12)+block][BLOCKTYPE] = type;
	}
}

void loadColors()
{
	memset(colors, 0, sizeof colors);
	//uint8_t x[10] = {SNOW, TRAPDOOR, 171, 0, 0, 0, 0, 0, 0, 0};
	//SET_BLOCK(BLOCKFLAT, x);

	//colors[171][BLOCKTYPE] = BLOCKTORCH;
	SET_BLOCK(BLOCKFLAT, SNOW);
	SET_BLOCK(BLOCKFLAT, TRAPDOOR);
	SET_BLOCK(BLOCKFLAT, CARPET);

	SET_BLOCK(BLOCKTORCH, TORCH);
	SET_BLOCK(BLOCKTORCH, REDTORCH_ON);
	SET_BLOCK(BLOCKTORCH, REDTORCH_OFF);

	SET_BLOCK(BLOCKFLOWER, FLOWERR);
	SET_BLOCK(BLOCKFLOWER, FLOWERY);
	SET_BLOCK(BLOCKFLOWER, MUSHROOMB);
	SET_BLOCK(BLOCKFLOWER, MUSHROOMR);
	SET_BLOCK(BLOCKFLOWER, MELON_STEM);
	SET_BLOCK(BLOCKFLOWER, PUMPKIN_STEM);
	SET_BLOCK(BLOCKFLOWER, SHRUB);
	SET_BLOCK(BLOCKFLOWER, COBWEB);
	SET_BLOCK(BLOCKFLOWER, LILYPAD);
	SET_BLOCK(BLOCKFLOWER, NETHER_WART);
	SET_BLOCK(BLOCKFLOWER, 175);

	SET_BLOCK(BLOCKFENCE, FENCE);
	SET_BLOCK(BLOCKFENCE, FENCE_GATE);
	SET_BLOCK(BLOCKFENCE, VINES);
	SET_BLOCK(BLOCKFENCE, IRON_BARS);
	SET_BLOCK(BLOCKFENCE, NETHER_BRICK_FENCE);
	SET_BLOCK(BLOCKFENCE, 139);
	SET_BLOCK(BLOCKFENCE, 160);

	SET_BLOCK(BLOCKWIRE, REDWIRE);
	SET_BLOCK(BLOCKWIRE, TRIPWIRE);

	SET_BLOCK(BLOCKRAILROAD, RAILROAD);
	SET_BLOCK(BLOCKRAILROAD, POW_RAILROAD);
	SET_BLOCK(BLOCKRAILROAD, DET_RAILROAD);
	SET_BLOCK(BLOCKRAILROAD, 157);

	SET_BLOCK(BLOCKGRASS, GRASS);
	//SET_BLOCK(BLOCKGRASS, SAND);

	SET_BLOCK(BLOCKFIRE, FIRE);
	SET_BLOCK(BLOCKFIRE, TALL_GRASS);
	SET_BLOCK(BLOCKFIRE, COCOA_PLANT);

	SET_BLOCK(BLOCKSTEP, STEP);
	SET_BLOCK(BLOCKSTEP, CAKE);
	SET_BLOCK(BLOCKSTEP, BED);
	SET_BLOCK(BLOCKSTEP, SANDSTEP);
	SET_BLOCK(BLOCKSTEP, WOODSTEP);
	SET_BLOCK(BLOCKSTEP, COBBLESTEP);
	SET_BLOCK(BLOCKSTEP, BRICKSTEP);
	SET_BLOCK(BLOCKSTEP, STONEBRICKSTEP);
	SET_BLOCK(BLOCKSTEP, PINESTEP);
	SET_BLOCK(BLOCKSTEP, BIRCHSTEP);
	SET_BLOCK(BLOCKSTEP, JUNGLESTEP);

	SET_BLOCK(BLOCKUPSTEP, UP_STEP);
	SET_BLOCK(BLOCKUPSTEP, UP_SANDSTEP);
	SET_BLOCK(BLOCKUPSTEP, UP_WOODSTEP);
	SET_BLOCK(BLOCKUPSTEP, UP_WOODSTEP2);
	SET_BLOCK(BLOCKUPSTEP, UP_COBBLESTEP);
	SET_BLOCK(BLOCKUPSTEP, UP_BRICKSTEP);
	SET_BLOCK(BLOCKUPSTEP, UP_STONEBRICKSTEP);
	SET_BLOCK(BLOCKUPSTEP, UP_PINESTEP);
	SET_BLOCK(BLOCKUPSTEP, UP_BIRCHSTEP);
	SET_BLOCK(BLOCKUPSTEP, UP_JUNGLESTEP);


	SET_COLOR(AIR, 255,255,255,0);
	SET_COLORNOISE(STONE, 128,128,128,255, 16);
	SET_COLORNOISE(GRASS, 102,142,62,255, 14);
	SET_COLORNOISE(DIRT, 134,96,67,255, 22);
	SET_COLORNOISE(COBBLESTONE, 115,115,115,255, 24);
	SET_COLORNOISE(WOOD, 157,128,79,255, 11);
	SET_COLOR(6, 120,120,120,0);
	SET_COLOR(7, 84,84,84,255);
	SET_COLOR(WATER, 38,92,225,36);
	SET_COLOR(STAT_WATER, 38,92,225,36);
	SET_COLOR(10, 255,90,0,255);
	SET_COLOR(11, 255,90,0,255);
	SET_COLORNOISE(SAND, 220,212,160,255, 14);
	SET_COLORNOISE(GRAVEL, 136,126,126,255, 24);
	SET_COLOR(14, 143,140,125,255);
	SET_COLOR(15, 136,130,127,255);
	SET_COLOR(16, 115,115,115,255);
	SET_COLOR(LOG, 102,81,51,255);
	SET_COLORNOISE(LEAVES, 54,135,40,180, 12);
	SET_COLOR(20, 255,255,255,40); //glass
	SET_COLORNOISE(21, 102, 112, 134, 255, 10);
	SET_COLORNOISE(22, 29, 71, 165, 255, 5);
	SET_COLOR(23, 107, 107, 107, 255);
	SET_COLORNOISE(SANDSTONE, 218, 210, 158, 255, 7);
	SET_COLORNOISE(25, 100, 67, 50, 255, 10);
	SET_COLOR(BED, 175,116,116, 254); // Not fully opaque to prevent culling on this one
	SET_COLOR(POW_RAILROAD, 160,134,72,250);
	SET_COLOR(DET_RAILROAD, 120,114,92,250);
	SET_COLOR(29, 106,102,95,255);
	SET_COLOR(COBWEB, 220,220,220,190);
	SET_COLORNOISE(TALL_GRASS, 110,166,68,254, 12);
	SET_COLORNOISE(SHRUB, 123,79,25,254, 25);
	SET_COLOR(33, 106,102,95,255);
	SET_COLOR(34, 153,129,89,255);
	SET_COLOR(WOOL, 222,222,222,255); //Color(143,143,143,255);
	//SET_COLOR(36, 222,222,222,255);
	SET_COLOR(FLOWERR, 255,0,0,254); // Not fully opaque to prevent culling on this one
	SET_COLOR(FLOWERY, 255,255,0,254); // Not fully opaque to prevent culling on this one
	SET_COLOR(MUSHROOMB, 128,100,0,254); // Not fully opaque to prevent culling on this one
	SET_COLOR(MUSHROOMR, 140,12,12,254); // Not fully opaque to prevent culling on this one
	SET_COLOR(41, 231,165,45,255);
	SET_COLOR(42, 191,191,191,255);
	SET_COLOR(DOUBLESTEP, 200,200,200,255);
	SET_COLOR(STEP, 200,200,200,254); // Not fully opaque to prevent culling on this one
	SET_COLOR(UP_STEP, 200,200,200,254); // Not fully opaque to prevent culling on this one
	SET_COLOR(45, 170,86,62,255);
	SET_COLOR(BRICKSTEP, 170,86,62,254);
	SET_COLOR(UP_BRICKSTEP, 170,86,62,254);
	SET_COLOR(46, 160,83,65,255);
	SET_COLORNOISE(48, 90,108,90,255, 27);
	SET_COLOR(49, 26,11,43,255);
	SET_COLOR(TORCH, 245,220,50,200);
	SET_COLOR(FIRE, 255,170,30,200);
	SET_COLOR(52, 20,170,200,255);
	SET_COLOR(53, 157,128,79,255);
	SET_COLOR(54, 125,91,38,255);
	SET_COLOR(REDWIRE, 200,10,10,200);
	SET_COLOR(56, 129,140,143,255);
	SET_COLOR(57, 45,166,152,255);
	SET_COLOR(58, 114,88,56,255);
	SET_COLOR(59, 146,192,0,255);
	SET_COLOR(60, 95,58,30,255);
	SET_COLOR(61, 96,96,96,255);
	SET_COLOR(62, 96,96,96,255);
	SET_COLOR(63, 111,91,54,255);
	SET_COLOR(64, 136,109,67,255);
	SET_COLOR(65, 181,140,64,32);
	SET_COLOR(RAILROAD, 140,134,72,250);
	SET_COLOR(67, 115,115,115,255);
	SET_COLOR(71, 191,191,191,255);
	SET_COLOR(73, 131,107,107,255);
	SET_COLOR(74, 131,107,107,255);
	SET_COLOR(REDTORCH_OFF, 181,100,44,254);
	SET_COLOR(REDTORCH_ON, 255,0,0,254);
	SET_COLORNOISE(SNOW, 245,246,245,254, 13); // Not fully opaque to prevent culling on this one
	SET_COLORNOISE(79, 125,173,255,159, 7);
	SET_COLOR(80, 250,250,250,255);
	SET_COLOR(81, 25,120,25,255);
	SET_COLOR(82, 151,157,169,255);
	SET_COLOR(83, 183,234,150,255);
	SET_COLOR(84, 100,67,50,255);
	SET_COLOR(FENCE, 137,112,65,225); // Not fully opaque to prevent culling on this one
	SET_COLOR(86, 197,120,23,255);
	SET_COLORNOISE(87, 110,53,51,255, 16);
	SET_COLORNOISE(88, 84,64,51,255, 7);
	SET_COLORNOISE(89, 137,112,64,255, 11);
	SET_COLOR(90, 0,42,255,127);
	SET_COLOR(91, 185,133,28,255);
	SET_COLORNOISE(CAKE, 228, 205, 206, 255, 7);
	SET_COLORNOISE(93, 151,147,147, 255, 2);
	SET_COLORNOISE(94, 161,147,147, 255, 2);
	SET_COLOR(95, 125,91,38,255);
	SET_COLORNOISE(TRAPDOOR, 126,93,45,240, 5);
	SET_COLORNOISE(97, 128,128,128,255, 16);
	SET_COLORNOISE(98, 122,122,122,255, 7);
	SET_COLORNOISE(STONEBRICKSTEP, 122,122,122,254, 7);
	SET_COLORNOISE(UP_STONEBRICKSTEP, 122,122,122,254, 7);
	SET_COLORNOISE(99, 141,106,83,255, 0);
	SET_COLORNOISE(100, 182,37,36,255, 6);
	SET_COLORNOISE(IRON_BARS, 109,108,106,254, 6);
	SET_COLOR(102, 255,255,255,40);
	SET_COLORNOISE(103, 151,153,36,255, 10);
	SET_COLOR(PUMPKIN_STEM, 115,170,73,254);
	SET_COLOR(MELON_STEM, 115,170,73,254);
	SET_COLORNOISE(VINES, 51,130,36,180, 12);
	SET_COLOR(FENCE_GATE, 137,112,65,225);
	SET_COLOR(108, 170,86,62,255);
	SET_COLORNOISE(109, 122,122,122,255, 7);
	SET_COLORNOISE(MYCELIUM, 140,115,119,255, 14);
	SET_COLOR(LILYPAD, 85,124,60,254); 
	SET_COLORNOISE(NETHER_BRICK, 54,24,30,255, 7);
	SET_COLOR(NETHER_BRICK_FENCE, 54,24,30,225);
	SET_COLOR(NETHER_BRICK_STAIRS, 54,24,30,255);
	SET_COLOR(NETHER_WART, 112,8,28,254);
	SET_COLORNOISE(116, 103,64,59,255, 6);
	SET_COLORNOISE(117, 124,103,81,255, 25);
	SET_COLOR(118, 55,55,55,255);
	SET_COLOR(119, 18,16,27,127);
	SET_COLORNOISE(120, 89,117,96,255, 6);
	SET_COLORNOISE(121, 221,223,165,255, 3);
	SET_COLOR(122, 20,18,29,255);
	SET_COLORNOISE(123, 70,43,26,255, 2);
	SET_COLORNOISE(124, 119,89,55,255, 7);
	SET_COLORNOISE(WOODEN_DOUBLE_STEP, 156,127,78,255, 11);
	SET_COLORNOISE(WOODEN_STEP, 156,127,78,254, 11);
	SET_COLOR(COCOA_PLANT, 145,80,30,200);
	SET_COLORNOISE(128, 218,210,158,255, 15);
	SET_COLORNOISE(129, 109,128,116,255, 18);
	SET_COLORNOISE(130, 18,16,27,255, 5);
	SET_COLORNOISE(131, 138,129,113,255, 28);
	SET_COLORNOISE(132, 129,129,129,107, 25);
	SET_COLOR(133, 81,217,117,255);
	SET_COLORNOISE(134, 103,77,46,255, 1);
	SET_COLORNOISE(135, 195,179,123,255, 3);
	SET_COLORNOISE(136, 154,110,77,255, 2);
	
	SET_COLORNOISE(PINELEAVES, 44,84,44,160, 20); // Pine leaves
	SET_COLORNOISE(BIRCHLEAVES, 85,124,60,170, 11); // Birch leaves
	SET_COLORNOISE(JUNGLELEAVES, 44,135,50,175, 11); // Birch leaves
	SET_COLORNOISE(SANDSTEP, 218, 210, 158, 254, 7); // Not fully opaque to prevent culling on this one
	SET_COLORNOISE(UP_SANDSTEP, 218, 210, 158, 254, 7); // Not fully opaque to prevent culling on this one
	SET_COLORNOISE(WOODSTEP, 157,128,79,254, 11); // Not fully opaque to prevent culling on this one
	SET_COLORNOISE(UP_WOODSTEP, 157,128,79,254, 11); // Not fully opaque to prevent culling on this one
	SET_COLORNOISE(COBBLESTEP, 115,115,115,254, 26); // Not fully opaque to prevent culling on this one
	SET_COLORNOISE(UP_COBBLESTEP, 115,115,115,254, 26); // Not fully opaque to prevent culling on this one

	SET_COLORNOISE(PINESTEP, 103,77,46,254, 1);
	SET_COLORNOISE(BIRCHSTEP, 195,179,123,254, 3);
	SET_COLORNOISE(JUNGLESTEP, 154,110,77,254, 2);
	
	SET_COLORNOISE(UP_WOODSTEP2, 157,128,79,254, 11);
	SET_COLORNOISE(UP_PINESTEP, 103,77,46,255, 1);
	SET_COLORNOISE(UP_BIRCHSTEP, 195,179,123,255, 3);
	SET_COLORNOISE(UP_JUNGLESTEP, 154,110,77,255, 2);
	
	SET_COLORNOISE(226, 103,77,46,255, 1);
	SET_COLORNOISE(227, 195,179,123,255, 3);
	SET_COLORNOISE(228, 154,110,77,255, 2);
	
	SET_COLOR(237, 70,50,32, 255); // Pine trunk
	SET_COLORNOISE(238, 206,206,201, 255, 5); // Birch trunk
	SET_COLOR(239, 122,91,51, 255); // Jungle trunk
	SET_COLOR_W(240, 244,137,54, 255); // Dyed wool
	SET_COLOR_W(241, 200,75,210,255);
	SET_COLOR_W(242, 120,158,241, 255);
	SET_COLOR_W(243, 204,200,28, 255);
	SET_COLOR_W(244, 59,210,47, 255);
	SET_COLOR_W(245, 237,141,164, 255);
	SET_COLOR_W(246, 76,76,76, 255);
	SET_COLOR_W(247, 168,172,172, 255);
	SET_COLOR_W(248, 39,116,149, 255);
	SET_COLOR_W(249, 133,53,195, 255);
	SET_COLOR_W(250, 38,51,160, 255);
	SET_COLOR_W(251, 85,51,27, 255);
	SET_COLOR_W(252, 55,77,24, 255);
	SET_COLOR_W(253, 173,44,40, 255);
	SET_COLOR_W(254, 32,27,27, 255);
	
	//1.3.1+ various
	SET_COLOR(133, 61, 255, 61, 255  ); //emerald
	SET_COLOR(137, 203, 163, 136, 255 ); //command block
	SET_COLOR(138, 21, 255, 255, 255 ); //beacon
	SET_COLORNOISE(139,    128,   128,   128,   255,   16); // cobblestone wall
	SET_COLOR(145, 110, 110, 110, 255 ); //anvil
	SET_COLOR(146, 125,    91,    38,   255   ); //trapped chest
	SET_COLOR(151, 187, 158, 109, 255 ); //daylight sensor
	SET_COLOR(152, 227,  38,  12, 255 ); //redstone block
	SET_COLOR(154, 110, 110, 110, 255 ); //hopper
	SET_COLOR(155, 240, 238, 232, 255 ); //quartz
	SET_COLOR(156, 240, 238, 232, 255 ); //quartz stairs
	SET_COLOR(207, 240, 238, 232, 255 ); //quartz slab
	//SET_COLOR(159, 209, 177, 160, 255 ); //white stained clay !
	SET_COLOR(161, 54, 135, 40, 180 ); //leaves Acacia/Dark Oak
	SET_COLOR(162, 72,  72, 72, 255 ); //log Acacia/Dark Oak
	SET_COLORNOISE(163, 154, 110, 77, 255, 2); // Acacia Wood Stairs
	SET_COLORNOISE(164, 106, 127, 98, 255, 11); // Dark Oak Wood Stairs
	SET_COLOR(170, 172, 145, 18,  255 ); //haystack
	//SET_COLOR(171, 224, 224, 224, 255 ); //white carpet
	SET_COLOR(172, 184, 126, 99, 255 ); //hardened clay
	SET_COLOR(173, 21,  21,  21,  255 ); //coal block
	SET_COLOR(174, 159, 189, 239, 255 ); //packed ice
	SET_COLOR(175, 0, 255, 0, 254 ); //Large flower
	SET_COLOR(206, 54, 24, 30, 255 ); //nether bricks slab

	COLOR_COPY(WOOL, CARPET);
	//COLOR_COPY(WOOL, 95);
	//COLOR_COPY(WOOL, 160);

	// carpets
	SET_COLOR(36 ,  255, 255, 255,  254   ); //White carpet
	SET_COLOR(68 ,  244, 137,  54,  254   ); //Orange carpet
	SET_COLOR(69 ,  200,  75, 210,  254   ); //Magenta carpet
	SET_COLOR(70 ,  120, 158, 241,  254   ); //Light Blue carpet
	SET_COLOR(72 ,  204, 200,  28,  254   ); //Yellow carpet
	SET_COLOR(77 ,   59, 210,  47,  254   ); //Lime carpet
	SET_COLOR(131,  237, 141, 164,  254   ); //Pink carpet
	SET_COLOR(132,   76,  76,  76,  254   ); //Gray carpet
	SET_COLOR(141,  168, 172, 172,  254   ); //Light Gray
	SET_COLOR(142,   39, 116, 149,  254   ); //Cyan carpet
	SET_COLOR(143,  133,  53, 195,  254   ); //Purple carpet
	SET_COLOR(147,   38,  51, 160,  254   ); //Blue carpet
	SET_COLOR(148,   85,  51,  27,  254   ); //Brown carpet
	SET_COLOR(149,   55,  77,  24,  254   ); //Green carpet
	SET_COLOR(150,  173,  44,  40,  254   ); //Red carpet
	SET_COLOR(158,   32,  27,  27,  254   ); //Black carpet

	// clays
	SET_COLOR(159,  241, 210, 192, 255   ); //White Stained Clay

	SET_COLOR_C(186,  194, 116,  69, 255   ); //Orange Stained Clay
	SET_COLOR_C(187,  182, 120, 140, 255   ); //Magenta Stained Clay
	SET_COLOR_C(188,  141, 137, 167, 255   ); //Light Blue Stained Clay
	SET_COLOR_C(189,  219, 165,  66, 255   ); //Yellow Stained Clay
	SET_COLOR_C(190,  137, 149,  84, 255   ); //Lime Stained Clay
	SET_COLOR_C(191,  194, 110, 110, 255   ); //Pink Stained Clay
	SET_COLOR_C(192,   97,  82,  75, 255   ); //Gray Stained Clay
	SET_COLOR_C(193,  168, 138, 128, 255   ); //Light Gray Stained Clay
	SET_COLOR_C(194,  119, 122, 122, 255   ); //Cyan Stained Clay
	SET_COLOR_C(195,  152, 102, 117, 255   ); //Purple Stained Clay
	SET_COLOR_C(196,  103,  88, 120, 255   ); //Blue Stained Clay
	SET_COLOR_C(197,  109,  82,  66, 255   ); //Brown Stained Clay
	SET_COLOR_C(198,  105, 112,  70, 255   ); //Green Stained Clay
	SET_COLOR_C(199,  176,  93,  78, 255   ); //Red Stained Clay
	SET_COLOR_C(200,   67,  52,  46, 255   ); //Black Stained Clay

	SET_COLOR1(12+4096,  225, 140,  73, 255   ); //Red Sand

	// glass
	SET_COLOR1(95 , 255, 255,  255,  100  ); //White Stained Glass
	SET_COLOR1(95+4096*0, 255, 255,  255,  100  ); //White Stained Glass pane
	SET_COLOR1(95+4096*1,  244, 137,  54,  40   ); //Orange Stained Glass [pane]
	SET_COLOR1(95+4096*2,  200,  75, 210,  40   ); //Magenta Stained Glass [pane]
	SET_COLOR1(95+4096*3,  120, 158, 241,  40   ); //Light Blue Stained Glass [pane]
	SET_COLOR1(95+4096*4,  204, 200,  28,  40   ); //Yellow Stained Glass [pane]
	SET_COLOR1(95+4096*5,   59, 210,  47,  40   ); //Lime Stained Glass [pane]
	SET_COLOR1(95+4096*6,  237, 141, 164,  40   ); //Pink Stained Glass [pane]
	SET_COLOR1(95+4096*7,   76,  76,  76,  40   ); //Gray Stained Glass [pane]
	SET_COLOR1(95+4096*8,  168, 172, 172,  40   ); //Light Gray Stained Glass [pane]
	SET_COLOR1(95+4096*9,   39, 116, 149,  40   ); //Cyan Stained Glass [pane]
	SET_COLOR1(95+4096*10,  133,  53, 195,  40   ); //Purple Stained Glass [pane]
	SET_COLOR1(95+4096*11,   38,  51, 160,  40   ); //Blue Stained Glass [pane]
	SET_COLOR1(95+4096*12,   85,  51,  27,  40   ); //Brown Stained Glass [pane]
	SET_COLOR1(95+4096*13,   55,  77,  24,  40   ); //Green Stained Glass [pane]
	SET_COLOR1(95+4096*14,  173,  44,  40,  40   ); //Red Stained Glass [pane]
	SET_COLOR1(95+4096*15,   32,  27,  27,  40   ); //Black Stained Glass [pane]

	COLOR_COPY(95, 160);

	// flowers
	SET_COLOR(165, 120, 158, 241, 254   ); //BLUE_ORCHID 165
	SET_COLOR(176, 200,  75, 210, 254 ); //ALLIUM 176
	SET_COLOR(235, 168, 172, 172, 254 ); //AZURE_BLUET 235
	// 38, 173  44  40 254, ); //RED_TULIP 38
	SET_COLOR(217, 244, 137,  54, 254 ); //ORANGE_TULIP 217
	SET_COLOR(218, 255, 255, 255, 254 ); //WHITE_TULIP 218
	SET_COLOR(219, 237, 141, 164, 254 ); //PINK_TULIP 219
	SET_COLOR(220, 168, 172, 172, 254 ); //OXEYE_DAISY 220
	// 37, 255 255  0 254, ); //SUNFLOWER 37
	SET_COLOR(233, 200,  75, 210, 254 ); //LILAC 233
	SET_COLOR(177, 237, 141, 164, 254 ); //PEONY 177

	// nether
	//SET_COLORNOISE(238,  206, 206, 201,  255,   5); // Birch Wood / quartz slab (sic!)

	SET_COLOR1(203,0,0,255,255);
	SET_COLOR1(592,255,0,0,255);
	SET_COLOR(606,128,128,120,255);
	SET_COLOR(517,250,250,0,255);
	//SET_COLOR1(80,255,0,0,255);

}


bool loadColorsFromFile(const char *file)
{
	FILE *f = fopen(file, "r");
	if (f == NULL) {
		return false;
	}
	while (!feof(f)) {
		char buffer[500];
		if (fgets(buffer, 500, f) == NULL) {
			break;
		}
		//printf("%s %d %d.\n",buffer,*buffer,*(buffer+1));
		char *ptr = buffer;
		while (*ptr == ' ' || *ptr == '\t') {
			++ptr;
		}
		if (*ptr == '\0' || *ptr == '#' || *ptr == '\12') {
			continue;   // This is a comment or empty line, skip
		}
		float blockid2 = atof(ptr);
		int blockid = (int)blockid2;
		if (blockid < 1 || blockid > 255) {
			printf("Skipping invalid blockid %.4f in colors file\n", blockid2);
			continue;
		}
		
		int suffix = 0;
		while (*ptr != ' ' && *ptr != '\t' && *ptr != '\0') {
			if (*ptr == ':')
			{
				int blockid3 = atoi(++ptr);
				if (blockid3 < 0 || blockid3 > 15) {
					printf("Skipping invalid blockid %d:%d in colors file\n", blockid, blockid3);
					suffix = -1;
				}
				printf("Second value %d:%d in colors file\n", blockid, blockid3); //wrim
				suffix = 1;
			}
			++ptr;
		}
		if (suffix < 0) continue;

		//int blockid3 = atoi(ptr);
			//printf("Druga wartoœæ %d in colors file\n", blockid3);

		uint8_t vals[5];
		bool valid = true;
		for (int i = 0; i < 5; ++i) {
			while (*ptr == ' ' || *ptr == '\t') {
				++ptr;
			}
			if (*ptr == '\0' || *ptr == '#' || *ptr == '\12') {
				printf("Too few arguments for block %d, ignoring line.\n", blockid);
				valid = false;
				break;
			}
			vals[i] = clamp(atoi(ptr));
			while (*ptr != ' ' && *ptr != '\t' && *ptr != '\0') {
				++ptr;
			}
		}
		if (!valid) {
			continue;
		}
		memcpy(colors[blockid], vals, 5);
		colors[blockid][BRIGHTNESS] = GETBRIGHTNESS(colors[blockid]);
	}
	fclose(f);
	return true;
}

bool dumpColorsToFile(const char *file)
{
	FILE *f = fopen(file, "w");
	if (f == NULL) {
		return false;
	}
	fprintf(f, "# For Block IDs see http://minecraftwiki.net/wiki/Data_values\n"
				"# and http://wrim.pl/mcmap (for blocks introduced since Minecraft 1.3.1 and mcmap 2.4)\n"
				"# Note that noise or alpha (or both) do not work for a few blocks like snow, torches, fences, steps, ...\n"
				"# Actually, if you see any block has an alpha value of 254 you should leave it that way to prevent black artifacts.\n"
				"# If you want to set alpha of grass to <255, use -blendall or you won't get what you expect.\n"
				"# Noise is supposed to look normal using -noise 10\n"
				"# Dyed wool ranges from ID 240 to 254, it's orange to black in the order described at http://www.minecraftwiki.net/wiki/Data_values#Wool\n"
				"# half-steps of sand, wood and cobblestone are 232 to 236\n\n");
	uint16_t j = 0;
	for (size_t i = 1; i < 4096; ++i) {
		uint8_t *c = colors[i];
		if (c[PALPHA] == 0) continue; // if color doesn't exist, don't dump it
		if (++j % 15 == 1) { //wrim - 3 cases - color only for :0, common for : and different for every :x
			fprintf(f, "#ID	R	G	B	A	Noise\n");//wrim - block types dump
		}
		fprintf(f, "%3d\t%3d\t%3d\t%3d\t%3d\t%3d\n", int(i), int(c[PRED]), int(c[PGREEN]), int(c[PBLUE]), int(c[PALPHA]), int(c[NOISE]));
		for (int j = 0; j < 16; j++)
		{
			uint8_t *c2 = colors[i+(j<<12)];
			if (c2[PALPHA] != 0 && (c[PRED] != c2[PRED] || c[PGREEN] != c2[PGREEN] || c[PBLUE] != c2[PBLUE] || c[PALPHA] != c2[PALPHA] || c[NOISE] != c2[NOISE]))
			fprintf(f, "%3d:%d\t%3d\t%3d\t%3d\t%3d\t%3d\n", int(i), int(j), int(c2[PRED]), int(c2[PGREEN]), int(c2[PBLUE]), int(c2[PALPHA]), int(c2[NOISE]));
		}
	}
	fprintf(f, "\n"
				"# Block types:\n"
				"\n");
	for (size_t i = 1; i < 4096; ++i) {
		uint8_t *c = colors[i];
		if (c[BLOCKTYPE] == 0) continue; // if type is default
		fprintf(f, "B\t%3d\t%3d\n", int(i), int(c[BLOCKTYPE]));
		for (int j = 0; j < 16; j++)
		{
			uint8_t *c2 = colors[i+(j<<12)];
			if (c2[BLOCKTYPE] != 0 && (c[BLOCKTYPE] != c2[BLOCKTYPE]))
			fprintf(f, "B\t%3d:%d\t%3d\n", int(i), int(j), int(c2[BLOCKTYPE]));
		}
	}
	fclose(f);
	return true;
}

/**
 * Extract block colors from given terrain.png file
 */
bool extractColors(const char* file)
{
	PngReader png(file);
	if (!png.isValidImage()) return false;
	if (png.getWidth() != png.getHeight() // Quadratic
			|| (png.getWidth() / 16) * 16 != png.getWidth() // Has to be multiple of 16
			|| png.getBitsPerChannel() != 8 // 8 bits per channel, 32bpp in total
			|| png.getColorType() != PngReader::RGBA) return false;
	uint8_t *imgData = png.getImageData();
	// Load em up
	for (int i = 0; i < 256; i++) {
		if (i == TORCH) {
			continue;   // Keep those yellow for now
		}
		int r, g, b, a, n; // i i s g t u o l v n
		if (getTileRGBA(imgData, png.getWidth() / 16, i, r, g, b, a, n)) {
			const bool flag = (colors[i][PALPHA] == 254);
			if (i == FENCE) {
				r = clamp(r + 10);
				g = clamp(g + 10);
				b = clamp(b + 10);
			}
			SETCOLORNOISE(i, r, g, b, a, n);
			if (flag) {
				colors[i][PALPHA] = 254;   // If you don't like this, dump texture pack to txt file and modify that one
			}
		}
	}
	return true;
}

static PngReader *pngGrass = NULL;
static PngReader *pngLeaf = NULL;
/**
 * This loads grasscolor.png and foliagecolor.png where the
 * biome colors will be read from upon rendering
 */
bool loadBiomeColors(const char* path)
{
	size_t len = strlen(path) + 21;
	char *grass = new char[len], *foliage = new char[len];
	snprintf(grass, len + 20, "%s/grasscolor.png", path);
	snprintf(foliage, len + 20, "%s/foliagecolor.png", path);
	pngGrass = new PngReader(grass);
	pngLeaf = new PngReader(foliage);
	if (!pngGrass->isValidImage() || !pngLeaf->isValidImage()) {
		delete pngGrass;
		delete pngLeaf;
		printf("Could not load %s and %s: no valid PNG files. Biomes disabled.\n", grass, foliage);
		return false;
	}
	if ((pngGrass->getColorType() != PngReader::RGBA && pngGrass->getColorType() != PngReader::RGB) || pngGrass->getBitsPerChannel() != 8
			|| (pngLeaf->getColorType() != PngReader::RGBA && pngLeaf->getColorType() != PngReader::RGB) || pngLeaf->getBitsPerChannel() != 8
			|| pngGrass->getWidth() != 256 || pngGrass->getHeight() != 256 || pngLeaf->getWidth() != 256 || pngLeaf->getHeight() != 256) {
		delete pngGrass;
		delete pngLeaf;
		printf("Could not load %s and %s; Expecting RGB(A), 8 bits per channel.\n", grass, foliage);
		return false;
	}
	g_GrasscolorDepth = pngGrass->getBytesPerPixel();
	g_FoliageDepth = pngLeaf->getBytesPerPixel();
	g_Grasscolor = pngGrass->getImageData();
	g_TallGrasscolor = new uint8_t[pngGrass->getBytesPerPixel() * 256 * 256];
	g_Leafcolor = pngLeaf->getImageData();
	// Adjust brightness to what colors.txt says
	const int maxG = pngGrass->getWidth() * pngGrass->getHeight() * g_GrasscolorDepth;
	for (int i = 0; i < maxG; ++i) {
		g_TallGrasscolor[i] = ((int)g_Grasscolor[i] * (int)colors[TALL_GRASS][BRIGHTNESS]) / 255;
		g_Grasscolor[i] = ((int)g_Grasscolor[i] * (int)colors[GRASS][BRIGHTNESS]) / 255;
	}
	const int maxT = pngLeaf->getWidth() * pngLeaf->getHeight() * g_FoliageDepth;
	for (int i = 0; i < maxT; ++i) {
		g_Leafcolor[i] = ((int)g_Leafcolor[i] * (int)colors[LEAVES][BRIGHTNESS]) / 255;
	}
	// Now re-calc brightness of those two
	colors[GRASS][BRIGHTNESS] = GETBRIGHTNESS(g_Grasscolor) - 5;
	colors[LEAVES][BRIGHTNESS] = colors[PINELEAVES][BRIGHTNESS] = colors[BIRCHLEAVES][BRIGHTNESS] = GETBRIGHTNESS(g_Leafcolor) - 5;
	printf("Loaded biome color maps from %s\n", path);
	return true;
}
