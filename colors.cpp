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

#define SETVAR(col, vindex) do { \
    colors[col][VINDEX] = vindex; \
} while (false)

#define SETNBVAR(col, vindex) do { \
    colors[255 + colors[col][VINDEX]][VINDEX] = vindex; \
} while (false)

#define SETBLOCKTYPE(col, type) do { \
    colors[col][BLOCK_TYPE] = type; \
} while (false)

// See header for description
uint8_t colors[512][8];

void loadColors()
{
    memset(colors, 0, sizeof colors);
    SETCOLOR		(AIR,		255,	255,	255,	0);
    SETCOLORNOISE	(STONE,		128,	128,	128,	255,	16);
    SETCOLORNOISE	(GRASS,		102,	142,	62,	255,	14);
    SETCOLORNOISE	(DIRT,		134,	96,	67,	255,	22);
    SETCOLORNOISE	(COBBLESTONE,	115,	115,	115,	255,	24);
    SETCOLORNOISE	(PLANKS,	157,	128,	79,	255,	11);
    SETCOLOR		(SAPLING,	120,	120,	120,	0);
    SETCOLOR		(BEDROCK,	84,	84,	84,	255);
    SETCOLOR		(FLOWING_WATER,	38,	92,	225,	36);
    SETCOLOR		(WATER,		38,	92,	225,	36);
    SETCOLOR		(FLOWING_LAVA,	255,	90,	0,	255);
    SETCOLOR		(LAVA,		255,	90,	0,	255);
    SETCOLORNOISE	(SAND,		220,	212,	160,	255,	14);
    SETCOLORNOISE	(GRAVEL,	136,	126,	126,	255,	24);
    SETCOLOR		(GOLD_ORE,	143,	140,	125,	255);
    SETCOLOR		(IRON_ORE,	136,	130,	127,	255);
    SETCOLOR		(COAL_ORE,	115,	115,	115,	255);
    SETCOLOR		(LOG,		102,	81,	51,	255);
    SETCOLORNOISE	(LEAVES,	54,	135,	40,	180,	12);
    SETCOLOR		(SPONGE,	224,	240,	48,	255  ); //sponge
    SETCOLOR		(GLASS,		255,	255,	255,	40); //glass
    SETCOLORNOISE	(LAPIS_ORE,	102,	112,	134,	255,	10);
    SETCOLORNOISE	(LAPIS_BLOCK,	29,	71,	165,	255,	5);
    SETCOLOR		(DISPENSER,	107,	107,	107,	255);
    SETCOLORNOISE	(SANDSTONE,	218,	210,	158,	255,	7);
    SETCOLORNOISE	(NOTEBLOCK,	100,	67,	50,	255,	10);
    SETCOLOR		(BED,		175,	116,	116,	254); // Not fully opaque to prevent culling on this one
    SETCOLOR		(GOLDEN_RAIL,	160,	134,	72,	250);
    SETCOLOR		(DETECTOR_RAIL,	120,	114,	92,	250);
    SETCOLOR		(STICKY_PISTON,	106,	102,	95,	255);
    SETCOLOR		(WEB,		220,	220,	220,	190);
    SETCOLORNOISE	(TALLGRASS,	123,	79,	25,	254,	25);
    SETCOLORNOISE	(DEADBUSH,	123,	79,	25,	254,	25);
    SETCOLOR		(PISTON,	106,	102,	95,	255);
    SETCOLOR		(PISTON_HEAD,	153,	129,	89,	255);
    SETCOLOR		(WOOL,		222,	222,	222,	255); //Color(143,	143,	143,	255);
    //SETCOLOR		(36,		222,	222,	222,	255); //Non existent
    SETCOLOR		(YELLOW_FLOWER,	255,	0,	0,	254); // Not fully opaque to prevent culling on this one
    SETCOLOR		(RED_FLOWER,	255,	255,	0,	254); // Not fully opaque to prevent culling on this one
    SETCOLOR		(BROWN_MUSHROOM,	128,	100,	0,	254); // Not fully opaque to prevent culling on this one
    SETCOLOR		(RED_MUSHROOM,	140,	12,	12,	254); // Not fully opaque to prevent culling on this one
    SETCOLOR		(GOLD_BLOCK,	231,	165,	45,	255);
    SETCOLOR		(IRON_BLOCK,	191,	191,	191,	255);
    SETCOLOR		(DOUBLE_STONE_SLAB,	200,	200,	200,	255);
    SETCOLOR		(STONE_SLAB,	200,	200,	200,	254); // Not fully opaque to prevent culling on this one
    SETCOLOR		(BRICK_BLOCK,	170,	86,	62,	255);
    SETCOLOR		(TNT,		160,	83,	65,	255);
    //SETCOLOR		(BOOKSHELF,	160,	83,	65,	255); // Not set
    SETCOLORNOISE	(MOSSY_COBBLESTONE,	90,	108,	90,	255,	27);
    SETCOLOR		(OBSIDIAN,	26,	11,	43,	255);
    SETCOLOR		(TORCH,		245,	220,	50,	255);
    SETCOLOR		(FIRE,		255,	170,	30,	200);
    SETCOLOR		(MOB_SPAWNER,	20,	170,	200,	255);
    SETCOLOR		(OAK_STAIRS,	157,	128,	79,	255);
    SETCOLOR		(CHEST,		125,	91,	38,	255);
    SETCOLOR		(REDSTONE_WIRE,	200,	10,	10,	200);
    SETCOLOR		(DIAMOND_ORE,	129,	140,	143,	255);
    SETCOLOR		(DIAMOND_BLOCK,	45,	166,	152,	255);
    SETCOLOR		(CRAFTING_TABLE,	114,	88,	56,	255);
    SETCOLOR		(WHEAT,		146,	192,	0,	255);
    SETCOLOR		(FARMLAND,	95,	58,	30,	255);
    SETCOLOR		(FURNACE,	96,	96,	96,	255);
    SETCOLOR		(LIT_FURNACE,	96,	96,	96,	255);
    SETCOLOR		(STANDING_SIGN,	111,	91,	54,	255);
    SETCOLOR		(WOODEN_DOOR,	136,	109,	67,	255);
    SETCOLOR		(LADDER,	181,	140,	64,	32);
    SETCOLOR		(RAIL,		140,	134,	72,	250);
    SETCOLOR		(STONE_STAIRS,	115,	115,	115,	255);
    //SETCOLOR		(STONE_PRESSURE_PLATE,	191,	191,	191,	255); // Not set
    SETCOLOR		(IRON_DOOR,	191,	191,	191,	255);
    //SETCOLOR		(WOODEN_PRESSURE_PLATE,	191,	191,	191,	255); // Not set
    SETCOLOR		(REDSTONE_ORE,	131,	107,	107,	255);
    SETCOLOR		(LIT_REDSTONE_ORE,	131,	107,	107,	255);
    SETCOLOR		(UNLIT_REDSTONE_TORCH,	181,	100,	44,	254);
    SETCOLOR		(REDSTONE_TORCH,	255,	0,	0,	254);
    SETCOLOR		(STONE_BUTTON,	191,	191,	191,	255); // Not set
    SETCOLORNOISE	(SNOW_LAYER,	245,	246,	245,	254,	13); // Not fully opaque to prevent culling on this one
    SETCOLORNOISE	(ICE,		125,	173,	255,	159,	7);
    SETCOLOR		(SNOW,		250,	250,	250,	255);
    SETCOLOR		(CACTUS,	25,	120,	25,	255);
    SETCOLOR		(CLAY,		151,	157,	169,	255);
    SETCOLOR		(REEDS,		183,	234,	150,	255);
    SETCOLOR		(JUKEBOX,	100,	67,	50,	255);
    SETCOLOR		(FENCE,		137,	112,	65,	225); // Not fully opaque to prevent culling on this one
    SETCOLOR		(PUMPKIN,	197,	120,	23,	255);
    SETCOLORNOISE	(NETHERRACK,	110,	53,	51,	255,	16);
    SETCOLORNOISE	(SOUL_SAND,	84,	64,	51,	255,	7);
    SETCOLORNOISE	(GLOWSTONE,	137,	112,	64,	255,	11);
    SETCOLOR		(PORTAL,	0,	42,	255,	127);
    SETCOLOR		(LIT_PUMPKIN,	185,	133,	28,	255);
    SETCOLORNOISE	(CAKE,		228,	205,	206,	255,	7);
    SETCOLORNOISE	(UNPOWERED_REPEATER,	151,	147,	147,	255,	2);
    SETCOLORNOISE	(POWERED_REPEATER,	161,	147,	147,	255,	2);
    SETCOLOR		(STAINED_GLASS,	255,	255,	255,	100);
    SETCOLORNOISE	(TRAPDOOR,	126,	93,	45,	240,	5);
    SETCOLORNOISE	(MONSTER_EGG,	128,	128,	128,	255,	16);
    SETCOLORNOISE	(STONEBRICK,	122,	122,	122,	255,	7);
    SETCOLORNOISE	(BROWN_MUSHROOM_BLOCK,	141,	106,	83,	255,	0);
    SETCOLORNOISE	(RED_MUSHROOM_BLOCK,	182,	37,	36,	255,	6);
    SETCOLORNOISE	(IRON_BARS,	109,	108,	106,	254,	6);
    SETCOLOR		(GLASS_PANE,	255,	255,	255,	40);
    SETCOLORNOISE	(MELON_BLOCK,	151,	153,	36,	255,	10);
    SETCOLOR		(PUMPKIN_STEM,	115,	170,	73,	254);
    SETCOLOR		(MELON_STEM,	115,	170,	73,	254);
    SETCOLORNOISE	(VINE,		51,	130,	36,	180,	12);
    SETCOLOR		(FENCE_GATE,	137,	112,	65,	225);
    SETCOLOR		(BRICK_STAIRS,	170,	86,	62,	255);
    SETCOLORNOISE	(STONE_BRICK_STAIRS,	122,	122,	122,	255,	7);
    SETCOLORNOISE	(MYCELIUM,	140,	115,	119,	255,	14);
    SETCOLOR		(WATERLILY,	85,	124,	60,	254); 
    SETCOLORNOISE	(NETHER_BRICK,	54,	24,	30,	255,	7);
    SETCOLOR		(NETHER_BRICK_FENCE,	54,	24,	30,	225);
    SETCOLOR		(NETHER_BRICK_STAIRS,	54,	24,	30,	255);
    SETCOLOR		(NETHER_WART,	112,	8,	28,	254);
    SETCOLORNOISE	(ENCHANTING_TABLE,	103,	64,	59,	255,	6);
    SETCOLORNOISE	(BREWING_STAND,	124,	103,	81,	255,	25);
    SETCOLOR		(CAULDRON,	55,	55,	55,	255);
    SETCOLOR		(END_PORTAL,	18,	16,	27,	127);
    SETCOLORNOISE	(END_PORTAL_FRAME,	89,	117,	96,	255,	6);
    SETCOLORNOISE	(END_STONE,	221,	223,	165,	255,	3);
    SETCOLOR		(DRAGON_EGG,	20,	18,	29,	255);
    SETCOLORNOISE	(REDSTONE_LAMP,	70,	43,	26,	255,	2);
    SETCOLORNOISE	(LIT_REDSTONE_LAMP,	119,	89,	55,	255,	7);
    SETCOLORNOISE	(DOUBLE_WOODEN_SLAB,	156,	127,	78,	255,	11);
    SETCOLORNOISE	(WOODEN_SLAB,	156,	127,	78,	254,	11);
    SETCOLOR		(COCOA,		145,	80,	30,	200);
    SETCOLORNOISE	(SANDSTONE_STAIRS,	218,	210,	158,	255,	15);
    SETCOLORNOISE	(EMERALD_ORE,	109,	128,	116,	255,	18);
    SETCOLORNOISE	(ENDER_CHEST,	18,	16,	27,	255,	5);
    SETCOLORNOISE	(TRIPWIRE_HOOK,	138,	129,	113,	255,	28);
    SETCOLORNOISE	(TRIPWIRE,	129,	129,	129,	107,	25);
    SETCOLOR		(EMERALD_BLOCK,	81,	217,	117,	255);
    SETCOLORNOISE	(SPRUCE_STAIRS,	103,	77,	46,	255,	1);
    SETCOLORNOISE	(BIRCH_STAIRS,	195,	179,	123,	255,	3);
    SETCOLORNOISE	(JUNGLE_STAIRS,	154,	110,	77,	255,	2);
    SETCOLOR		(COMMAND_BLOCK,	203,	163,	136,	255 ); //command block
    SETCOLOR		(BEACON,	21,	255,	255,	255 ); //beacon
    SETCOLORNOISE	(COBBLESTONE_WALL,	   128,	  128,	  128,	  255,	  16); // cobblestone wall
    //SETCOLOR		(FLOWER_POT,	110,	110,	110,	255 ); //Not Set
    SETCOLOR		(CARROTS,	146,	192,	0,	255);
    SETCOLOR		(POTATOES,	146,	192,	0,	255);
    SETCOLOR		(WOODEN_BUTTON,	0,	0,	0,	0 ); //Not Set
    //SETCOLOR		(SKULL,		110,	110,	110,	255 ); //Not Set
    SETCOLOR		(ANVIL,		110,	110,	110,	255 ); //anvil
    SETCOLOR		(TRAPPED_CHEST,	125,	   91,	   38,	  255   ); //trapped chest
    //SETCOLOR		(LIGHT_WEIGHTED_PRESSURE_PLATE	,	125,	   91,	   38,	  255   ); //Not Set
    //SETCOLOR		(HEAVY_WEIGHTED_PRESSURE_PLATE,	125,	   91,	   38,	  255   ); //Not Set
    SETCOLOR		(UNPOWERED_COMPARATOR,	151,	   147,	   147,	  255   );
    SETCOLOR		(POWERED_COMPARATOR,	161,	   147,	   147,	  255   );
    SETCOLOR		(DAYLIGHT_DETECTOR,	187,	158,	109,	255 ); //daylight sensor
    SETCOLOR		(REDSTONE_BLOCK,	227,	 38,	 12,	255 ); //redstone block
    SETCOLOR		(QUARTZ_ORE,	168,	54,	69,	255 ); //quartz
    SETCOLOR		(HOPPER,	110,	110,	110,	255 ); //hopper
    SETCOLOR		(QUARTZ_BLOCK,	240,	238,	232,	255 ); //quartz
    SETCOLOR		(QUARTZ_STAIRS,	240,	238,	232,	255 ); //quartz stairs
    SETCOLOR		(ACTIVATOR_RAIL,	120,	114,	92,	250);
    SETCOLOR		(DROPPER,	107,	107,	107,	255);
    SETCOLOR		(STAINED_HARDENED_CLAY,	 241,	210,	192,	255   ); //White Stained Clay
    SETCOLOR		(STAINED_GLASS_PANE,	255,	255,	 255,	 100  ); //White Stained Glass pane
    SETCOLOR		(LEAVES2,	54,	135,	40,	180 ); //leaves Acacia/Dark Oak
    SETCOLOR		(LOG2,		115,	 115,	115,	255 ); //log Acacia/Dark Oak
    SETCOLOR		(ACACIA_STAIRS,	156,	127,	78,	255); // Acacia Wood Stairs
    SETCOLOR		(DARK_OAK_STAIRS, 67, 53, 10, 254); // Dark Oak
    SETCOLOR		(SLIME,		61,	255,	61,	96  ); //slime block
    SETCOLOR		(BARRIER,	0,	0,	0,	0); // Not Set
    SETCOLOR		(IRON_TRAPDOOR,	0,	0,	0,	0); // Not Set
    SETCOLOR		(PRISMARINE,	30,	151,	137,	255  ); //prismarine
    SETCOLOR		(SEA_LANTERN,	75,	196,	152,	255  ); //sea lantern
    SETCOLOR		(HAY_BLOCK,	172,	145,	18,	 255 ); //haystack
    SETCOLOR		(CARPET,	224,	224,	224,	255 ); //white carpet
    SETCOLOR		(HARDENED_CLAY,	184,	126,	99,	255 ); //hardened clay
    SETCOLOR		(COAL_BLOCK,	21,	 21,	 21,	 255 ); //coal block
    SETCOLOR		(PACKED_ICE,	159,	189,	239,	255 );
    SETCOLOR		(DOUBLE_PLANT,	255,	255,	0,	254);
    SETCOLOR		(STANDING_BANNER,	0,	0,	0,	0 );
    SETCOLOR		(WALL_BANNER,	0,	0,	0,	0 );
    SETCOLOR		(DAYLIGHT_DETECTOR_INVERTED,	187,	158,	109,	255 ); //daylight sensor
    SETCOLOR		(RED_SANDSTONE,	 225,	140,	 73,	255);
    SETCOLOR		(RED_SANDSTONE_STAIRS,	 225,	140,	 73,	255);
    SETCOLOR		(DOUBLE_STONE_SLAB2,	225,	140,	73,	255);
    SETCOLOR		(STONE_SLAB2,	225,	140,	73,	254); // Not fully opaque to prevent culling on this one
    SETCOLOR		(SPRUCE_FENCE_GATE,	137,	112,	65,	225);
    SETCOLOR		(BIRCH_FENCE_GATE,	137,	112,	65,	225);
    SETCOLOR		(JUNGLE_FENCE_GATE,	137,	112,	65,	225);
    SETCOLOR		(DARK_OAK_FENCE_GATE,	137,	112,	65,	225);
    SETCOLOR		(ACACIA_FENCE_GATE,	137,	112,	65,	225);
    SETCOLOR		(SPRUCE_FENCE,	137,	112,	65,	225);
    SETCOLOR		(BIRCH_FENCE,	137,	112,	65,	225);
    SETCOLOR		(JUNGLE_FENCE,	137,	112,	65,	225);
    SETCOLOR		(DARK_OAK_FENCE,	137,	112,	65,	225);
    SETCOLOR		(ACACIA_FENCE,	137,	112,	65,	225);
    SETCOLOR		(SPRUCE_DOOR,	136,	109,	67,	255);
    SETCOLOR		(BIRCH_DOOR,	136,	109,	67,	255);
    SETCOLOR		(JUNGLE_DOOR,	136,	109,	67,	255);
    SETCOLOR		(ACACIA_DOOR,	136,	109,	67,	255);
    SETCOLOR		(DARK_OAK_DOOR,	136,	109,	67,	255);
    SETCOLOR		(END_ROD,	224,	236,	237,	255);
    SETCOLOR		(CHORUS_PLANT,	54,	24,	30,	225);
    SETCOLOR		(CHORUS_FLOWER,	54,	24,	30,	225);
    SETCOLOR		(PURPUR_BLOCK,	155,	63,	176,	255  ); //purpur blocks
    SETCOLOR		(PURPUR_PILLAR,	155,	63,	176,	255  ); //purpur blocks
    SETCOLOR		(PURPUR_STAIRS,	155,	63,	176,	255  ); //purpur blocks
    SETCOLOR		(PURPUR_DOUBLE_SLAB,	155,	63,	176,	255  ); //purpur blocks
    SETCOLOR		(PURPUR_SLAB,	155,	63,	176,	255  ); //purpur blocks
    SETCOLORNOISE	(END_BRICKS,	221,	223,	165,	255,	3);
    SETCOLOR		(BEETROOTS,	146,	192,	0,	255);
    SETCOLOR		(GRASS_PATH,	176,	181,	0,	255  ); //grass path
    SETCOLOR		(END_GATEWAY,	26,	11,	43,	255);
    SETCOLOR		(REPEATING_COMMAND_BLOCK,	203,	163,	136,	255 ); //command block
    SETCOLOR		(CHAIN_COMMAND_BLOCK,	203,	163,	136,	255 ); //command block
    SETCOLORNOISE	(FROSTED_ICE,	125,	173,	255,	159,	7);
    SETCOLOR		(MAGMA,		255,	90,	0,	255);
    SETCOLOR		(NETHER_WART_BLOCK,	117,	26,	42,	255);
    SETCOLOR		(RED_NETHER_BRICK,	89,	26,	15,	255);
    SETCOLOR		(BONE_BLOCK,	237,	236,	233,	255);
    SETCOLOR		(STRUCTURE_VOID,	0,	0,	0,	0);
    SETCOLOR		(OBSERVER,	107,	107,	107,	255);
    SETCOLOR		(WHITE_SHULKER_BOX, 222, 222, 222, 255);
    SETCOLOR		(ORANGE_SHULKER_BOX, 244, 137, 54, 255);
    SETCOLOR		(MAGENTA_SHULKER_BOX, 200,75,210,255);
    SETCOLOR		(LIGHT_BLUE_SHULKER_BOX, 120,158,241, 255);
    SETCOLOR		(YELLOW_SHULKER_BOX, 204,200,28, 255);
    SETCOLOR		(LIME_SHULKER_BOX, 59, 210, 47, 255);
    SETCOLOR		(PINK_SHULKER_BOX, 237, 141, 164, 255);
    SETCOLOR		(GRAY_SHULKER_BOX, 76, 76, 76, 255);
    SETCOLOR		(SILVER_SHULKER_BOX, 168, 172, 172, 255);
    SETCOLOR		(CYAN_SHULKER_BOX, 39, 116, 149, 255);
    SETCOLOR		(PURPLE_SHULKER_BOX, 133, 53, 195, 255);
    SETCOLOR		(BLUE_SHULKER_BOX, 38,51,160, 255);
    SETCOLOR		(BROWN_SHULKER_BOX, 85,51,27, 255);
    SETCOLOR		(GREEN_SHULKER_BOX, 55,77,24, 255);
    SETCOLOR		(RED_SHULKER_BOX, 173,44,40, 255);
    SETCOLOR		(BLACK_SHULKER_BOX, 32,27,27, 255);
    SETCOLOR		(WHITE_GLAZED_TERRACOTTA,  241, 210, 192, 255);
    SETCOLOR		(ORANGE_GLAZED_TERRACOTTA,  194, 116,  69, 255);
    SETCOLOR		(MAGENTA_GLAZED_TERRACOTTA,  182, 120, 140, 255);
    SETCOLOR		(LIGHT_BLUE_GLAZED_TERRACOTTA,  141, 137, 167, 255);
    SETCOLOR		(YELLOW_GLAZED_TERRACOTTA,  219, 165,  66, 255);
    SETCOLOR		(LIME_GLAZED_TERRACOTTA,  137, 149,  84, 255);
    SETCOLOR		(PINK_GLAZED_TERRACOTTA,  194, 110, 110, 255);
    SETCOLOR		(GRAY_GLAZED_TERRACOTTA,   97,  82,  75, 255);
    SETCOLOR		(LIGHT_GRAY_GLAZED_TERRACOTTA,  168, 138, 128, 255);
    SETCOLOR		(CYAN_GLAZED_TERRACOTTA,  119, 122, 122, 255);
    SETCOLOR		(PURPLE_GLAZED_TERRACOTTA,  152, 102, 117, 255);
    SETCOLOR		(BLUE_GLAZED_TERRACOTTA,  103,  88, 120, 255);
    SETCOLOR		(BROWN_GLAZED_TERRACOTTA,  109,  82,  66, 255);
    SETCOLOR		(GREEN_GLAZED_TERRACOTTA,  105, 112,  70, 255);
    SETCOLOR		(RED_GLAZED_TERRACOTTA,  176,  93,  78, 255);
    SETCOLOR		(BLACK_GLAZED_TERRACOTTA,   67,  52,  46, 255);
    SETCOLOR		(CONCRETE,		208,	214,	215,	255);
    SETCOLOR		(CONCRETE_POWDER,		208,	214,	215,	255);

    // Stone Variants
    SETVAR(STONE, 1);
    SETCOLORNOISE	(256,		128,	128,	128,	255,	16); // Stone
    SETCOLOR(257, 208, 128, 128, 255); // Granite
    SETCOLOR(258, 208, 128, 128, 255); // Polished Granite
    SETCOLOR(259, 228, 228, 228, 255); // Diorite
    SETCOLOR(260, 228, 228, 228, 255); // Polished Diorite
    SETCOLOR(261, 160, 160, 160, 255); // Andesite
    SETCOLOR(262, 160, 160, 160, 255); // Polished Andesite

    // Dirt Variants
    SETVAR(DIRT, 8);
    SETCOLORNOISE	(263,		134,	96,	67,	255,	22); //Dirt
    SETCOLORNOISE	(264,		134,	96,	67,	255,	22); // Coarse Dirt
    SETCOLORNOISE	(265,		134,	96,	67,	255,	22); // Podzol

    // Plank Variants
    SETVAR(PLANKS, 11);
    SETCOLORNOISE	(266,	157,	128,	79,	255,	11); // Oak
    SETCOLORNOISE(267, 103,77,46,255, 1); // Spruce
    SETCOLORNOISE(268, 195,179,123,255, 3); // Birch
    SETCOLORNOISE(269, 154,110,77,255, 2); // Jungle
    SETCOLORNOISE	(270,172,92,0,255,11); // Acacia
    SETCOLORNOISE	(271,67,53,10,255,11); // Dark Oak

    // Sapling Variants
    SETVAR(SAPLING, 17);
    SETCOLOR		(272,	120,	120,	120,	0);
    SETCOLOR		(273,	120,	120,	120,	0);
    SETCOLOR		(274,	120,	120,	120,	0);
    SETCOLOR		(275,	120,	120,	120,	0);
    SETCOLOR		(276,	120,	120,	120,	0);
    SETCOLOR		(277,	120,	120,	120,	0);
    
    // Sand Variants
    SETVAR(SAND, 23);
    SETCOLORNOISE	(278,		220,	212,	160,	255,	14); // Sand
    SETCOLOR(279,  225, 140,  73, 255   ); // Red Sand

    // Log Variants
    SETVAR(LOG, 25);
    SETNBVAR(LOG, 4);
    SETCOLOR(280, 102, 81, 51, 255); // Oak
    SETCOLOR(281, 70,50,32, 255); // Spruce trunk
    SETCOLORNOISE(282, 206,206,201, 255, 5); // Birch trunk
    SETCOLOR(283, 122,91,51, 255); // Jungle trunk

    // Leaves Variants
    SETVAR(LEAVES, 29);
    SETNBVAR(LEAVES, 4);
    SETCOLORNOISE	(284,	54,	135,	40,	180,	12); // Oak leaves
    SETCOLORNOISE	(285,	44,	84,	44,	160,	20); // Spruce leaves
    SETCOLORNOISE	(286,	85,	124,	60,	170,	11); // Birch leaves
    SETCOLORNOISE	(287,	44,	135,	50,	175,	11); // Jungle leaves

    // Sponge Variants
    SETVAR(SPONGE, 33);
    SETCOLOR		(288,	224,	240,	48,	255  ); // Dry
    SETCOLOR		(289,	224,	240,	48,	255  ); // Wet

    // Sandstone Variants
    SETVAR(SANDSTONE, 35);
    SETCOLORNOISE	(290,	218,	210,	158,	255,	7); // Regular
    SETCOLORNOISE	(291,	218,	210,	158,	255,	7); // Chiseled
    SETCOLORNOISE	(292,	218,	210,	158,	255,	7); // Smooth

    // Grass Variants
    SETVAR(TALLGRASS, 38);
    SETCOLORNOISE(293, 123, 79, 25, 254, 25); // Dead bush
    SETCOLORNOISE(294, 110, 166, 68, 254, 12); // Grass
    SETCOLORNOISE(295, 110, 166, 68, 254, 12); // Fern

    // Wool Variants
    SETVAR(WOOL, 41);
    SETCOLOR(296, 222, 222, 222, 255); //Color(143, 143, 143, 255);
    SETCOLOR(297, 244,137,54, 255);
    SETCOLOR(298, 200,75,210,255);
    SETCOLOR(299, 120,158,241, 255);
    SETCOLOR(300, 204,200,28, 255);
    SETCOLOR(301, 59,210,47, 255);
    SETCOLOR(302, 237,141,164, 255);
    SETCOLOR(303, 76,76,76, 255);
    SETCOLOR(304, 168,172,172, 255);
    SETCOLOR(305, 39,116,149, 255);
    SETCOLOR(306, 133,53,195, 255);
    SETCOLOR(307, 38,51,160, 255);
    SETCOLOR(308, 85,51,27, 255);
    SETCOLOR(309, 55,77,24, 255);
    SETCOLOR(310, 173,44,40, 255);
    SETCOLOR(311, 32,27,27, 255);

    // Flower Variants
    SETVAR(RED_FLOWER, 57);
    SETCOLOR(312, 255, 255, 0, 254); // Not fully opaque to prevent culling on this one
    SETCOLOR(313, 120, 158, 241, 254); //blue orchid
    SETCOLOR(314, 200,  75, 210, 254); //allium
    SETCOLOR(315, 168, 172, 172, 254); //azure bluet
    SETCOLOR(316, 173,  44,  40, 254); //red tulip
    SETCOLOR(317, 244, 137,  54, 254); //orange tulip
    SETCOLOR(318, 255, 255, 255, 254); //white tulip
    SETCOLOR(319, 237, 141, 164, 254); //pink tulip
    SETCOLOR(320, 168, 172, 172, 254); //oxeye daisy

    // Double Slab Variants
    SETVAR(DOUBLE_STONE_SLAB, 66);
    SETCOLOR		(321,	200,	200,	200,	255);
    SETCOLORNOISE	(322,	218,	210,	158,	255,	7); // Double Sandstone Slab
    SETCOLORNOISE	(323,	157,	128,	79,	255,	11); // Double Wooden Slab
    SETCOLORNOISE	(324,	115,	115,	115,	255,	24); // Double Cobblestone Slab
    SETCOLOR		(325,	170,	86,	62,	255); // Double Brick
    SETCOLORNOISE	(326,	122,	122,	122,	255,	7); // Double Stone Brick
    SETCOLORNOISE	(327,	54,	24,	30,	255,	7); // Double Nether Brick
    SETCOLOR		(328,	240,	238,	232,	255 ); // Double Quartz

    // Slab variants
    SETVAR(STONE_SLAB, 74);
    SETCOLOR		(329,	200,	200,	200,	254); // Not fully opaque to prevent culling on this one
    SETCOLORNOISE	(330, 218, 210, 158, 254, 7); // Not fully opaque to prevent culling on this one
    SETCOLORNOISE	(331, 157,128,79,254, 11); // Not fully opaque to prevent culling on this one
    SETCOLORNOISE	(332, 115,115,115,254, 26); // Not fully opaque to prevent culling on this one
    SETCOLOR		(333,	170,	86,	62,	254); // Brick Slab
    SETCOLORNOISE	(334,	122,	122,	122,	254,	7); // Stone Brick Slab
    SETCOLORNOISE	(335,	54,	24,	30,	254,	7); // Nether Brick Slab
    SETCOLOR		(336,	240,	238,	232,	254 ); // Quartz Slab

    // Stained Glass Variants
    SETVAR(STAINED_GLASS, 82);
    SETCOLOR(337, 255, 255,  255,  100  ); //White Stained Glass
    SETCOLOR(338,  244, 137,  54,  40   ); //Orange Stained Glass
    SETCOLOR(339,  200,  75, 210,  40   ); //Magenta Stained Glass
    SETCOLOR(340,  120, 158, 241,  40   ); //Light Blue Stained Glass
    SETCOLOR(341,  204, 200,  28,  40   ); //Yellow Stained Glass
    SETCOLOR(342,   59, 210,  47,  40   ); //Lime Stained Glass
    SETCOLOR(343,  237, 141, 164,  40   ); //Pink Stained Glass
    SETCOLOR(344,   76,  76,  76,  40   ); //Gray Stained Glass
    SETCOLOR(345,  168, 172, 172,  40   ); //Light Gray Stained Glass
    SETCOLOR(346,   39, 116, 149,  40   ); //Cyan Stained Glass
    SETCOLOR(347,  133,  53, 195,  40   ); //Purple Stained Glass
    SETCOLOR(348,   38,  51, 160,  40   ); //Blue Stained Glass
    SETCOLOR(349,   85,  51,  27,  40   ); //Brown Stained Glass
    SETCOLOR(350,   55,  77,  24,  40   ); //Green Stained Glass
    SETCOLOR(351,  173,  44,  40,  40   ); //Red Stained Glass
    SETCOLOR(352,   32,  27,  27,  40   ); //Black Stained Glass

    // Monster Egg Variants
    SETVAR(MONSTER_EGG, 98);
    SETCOLORNOISE	(353,	128,	128,	128,	255,	16); // Stone Egg
    SETCOLORNOISE	(354,	115,	115,	115,	255,	24); // Cobblestone Egg
    SETCOLORNOISE	(355,	122,	122,	122,	255,	7); // Stone Brick Egg
    SETCOLORNOISE	(356,	90,	108,	90,	255,	27); // Mossy Egg
    SETCOLORNOISE	(357,	122,	122,	122,	255,	7); // Cracked Stone Brick Egg
    SETCOLORNOISE	(358,	122,	122,	122,	255,	7); // Chiseled Stone Brick Egg

    // Stone Brick Variants
    SETVAR(STONEBRICK, 104);
    SETCOLORNOISE	(359,	122,	122,	122,	255,	7); // Regular
    SETCOLORNOISE	(360,	122,	122,	122,	255,	7); // Mossy
    SETCOLORNOISE	(361,	122,	122,	122,	255,	7); // Cracked
    SETCOLORNOISE	(362,	122,	122,	122,	255,	7); // Chiseled

    // Double Wooden Slab Variants
    SETVAR(DOUBLE_WOODEN_SLAB, 108);
    SETNBVAR(DOUBLE_WOODEN_SLAB, 8);
    SETCOLORNOISE(363, 156, 127, 78, 255, 11); // Oak
    SETCOLORNOISE(364, 103, 77, 46, 255, 1); // Spruce
    SETCOLORNOISE(365, 195, 179, 123, 255, 3); // Birch
    SETCOLORNOISE(366, 154, 110, 77, 255, 2); // Jungle
    SETCOLORNOISE(367, 172, 92, 0, 255, 11); // Acacia
    SETCOLORNOISE(368, 67, 53, 10, 255, 11); // Dark Oak

    // Wooden Slab Variants
    SETVAR(WOODEN_SLAB, 114);
    SETNBVAR(WOODEN_SLAB, 8);
    SETCOLORNOISE(369, 156, 127, 78, 254, 11); // Oak
    SETCOLORNOISE(370, 103, 77, 46, 254, 1); // Spruce
    SETCOLORNOISE(371, 195, 179, 123, 254, 3); // Birch
    SETCOLORNOISE(372, 154, 110, 77, 254, 2); // Jungle
    SETCOLORNOISE(373, 172, 92, 0, 254, 11); // Acacia
    SETCOLORNOISE(374, 67, 53, 10, 254, 11); // Dark Oak

    SETVAR(COBBLESTONE_WALL, 120);
    SETCOLORNOISE	(375,	   128,	  128,	  128,	  255,	  16); // cobblestone wall
    SETCOLORNOISE	(376,	90,	108,	90,	255,	27); // Mossy cobblestone wall

    SETVAR(QUARTZ_BLOCK, 122);
    SETCOLOR(377, 240, 238, 232, 255); // Regular
    SETCOLOR(378, 240, 238, 232, 255); // Chiseled
    SETCOLOR(379, 240, 238, 232, 255); // Pillar

    // Stained Clay Variants
    SETVAR(STAINED_HARDENED_CLAY, 125);
    SETCOLOR(380,  241, 210, 192, 255); //White Stained Clay
    SETCOLOR(381,  194, 116,  69, 255); //Orange Stained Clay
    SETCOLOR(382,  182, 120, 140, 255); //Magenta Stained Clay
    SETCOLOR(383,  141, 137, 167, 255); //Light Blue Stained Clay
    SETCOLOR(384,  219, 165,  66, 255); //Yellow Stained Clay
    SETCOLOR(385,  137, 149,  84, 255); //Lime Stained Clay
    SETCOLOR(386,  194, 110, 110, 255); //Pink Stained Clay
    SETCOLOR(387,   97,  82,  75, 255); //Gray Stained Clay
    SETCOLOR(388,  168, 138, 128, 255); //Light Gray Stained Clay
    SETCOLOR(389,  119, 122, 122, 255); //Cyan Stained Clay
    SETCOLOR(390,  152, 102, 117, 255); //Purple Stained Clay
    SETCOLOR(391,  103,  88, 120, 255); //Blue Stained Clay
    SETCOLOR(392,  109,  82,  66, 255); //Brown Stained Clay
    SETCOLOR(393,  105, 112,  70, 255); //Green Stained Clay
    SETCOLOR(394,  176,  93,  78, 255); //Red Stained Clay
    SETCOLOR(395,   67,  52,  46, 255); //Black Stained Clay

    // Stained Glass Panes Variants
    SETVAR(STAINED_GLASS, 141);
    SETCOLOR(396, 255, 255,  255,  100); //White Stained Glass Pane
    SETCOLOR(397,  244, 137,  54,  40); //Orange Stained Glass Pane
    SETCOLOR(398,  200,  75, 210,  40); //Magenta Stained Glass Pane
    SETCOLOR(399,  120, 158, 241,  40); //Light Blue Stained Glass Pane
    SETCOLOR(400,  204, 200,  28,  40); //Yellow Stained Glass Pane
    SETCOLOR(401,   59, 210,  47,  40); //Lime Stained Glass Pane
    SETCOLOR(402,  237, 141, 164,  40); //Pink Stained Glass Pane
    SETCOLOR(403,   76,  76,  76,  40); //Gray Stained Glass Pane
    SETCOLOR(404,  168, 172, 172,  40); //Light Gray Stained Glass Pane
    SETCOLOR(405,   39, 116, 149,  40); //Cyan Stained Glass Pane
    SETCOLOR(406,  133,  53, 195,  40); //Purple Stained Glass Pane
    SETCOLOR(407,   38,  51, 160,  40); //Blue Stained Glass Pane
    SETCOLOR(408,   85,  51,  27,  40); //Brown Stained Glass Pane
    SETCOLOR(409,   55,  77,  24,  40); //Green Stained Glass Pane
    SETCOLOR(410,  173,  44,  40,  40); //Red Stained Glass Pane
    SETCOLOR(411,   32,  27,  27,  40); //Black Stained Glass Pane

    SETVAR(LEAVES2, 157);
    SETNBVAR(LEAVES2, 2);
    SETCOLOR(412, 54, 135, 40, 180); // Acacia leaves
    SETCOLOR(413, 44, 84, 44, 160); // Dark Oak leaves

    SETVAR(LOG2, 159);
    SETNBVAR(LOG2, 2); // Only 2 variants
    SETCOLOR (414, 115,	115, 115, 255); // Acacia log
    SETCOLOR (415, 58, 37, 37, 255 ); // Dark Oak log

    SETVAR(PRISMARINE, 161);
    SETCOLOR(416, 25, 146, 132, 255); // Regular
    SETCOLOR(417, 25, 146, 132, 255); // Bricks
    SETCOLOR(418, 15, 136, 122, 255); // Dark
    
    SETVAR(CARPET, 164);
    SETCOLOR(419,  255, 255, 255,  254); //White carpet
    SETCOLOR(420,  244, 137,  54,  254); //Orange carpet
    SETCOLOR(421,  200,  75, 210,  254); //Magenta carpet
    SETCOLOR(422,  120, 158, 241,  254); //Light Blue carpet
    SETCOLOR(423,  204, 200,  28,  254); //Yellow carpet
    SETCOLOR(424,   59, 210,  47,  254); //Lime carpet
    SETCOLOR(425,  237, 141, 164,  254); //Pink carpet
    SETCOLOR(426,   76,  76,  76,  254); //Gray carpet
    SETCOLOR(427,  168, 172, 172,  254); //Light Gray
    SETCOLOR(428,   39, 116, 149,  254); //Cyan carpet
    SETCOLOR(429,  133,  53, 195,  254); //Purple carpet
    SETCOLOR(430,   38,  51, 160,  254); //Blue carpet
    SETCOLOR(431,   85,  51,  27,  254); //Brown carpet
    SETCOLOR(432,   55,  77,  24,  254); //Green carpet
    SETCOLOR(433,  173,  44,  40,  254); //Red carpet
    SETCOLOR(434,   32,  27,  27,  254); //Black carpet

    SETVAR(DOUBLE_PLANT, 180);
    SETCOLOR(435, 255, 255, 0, 254); // Sunflower
    SETCOLOR(436, 200, 75, 210, 254); //Lilac
    SETCOLORNOISE(437, 110, 166, 68, 254, 25); // Grass
    SETCOLORNOISE(438, 110, 166, 68, 254, 12); // Fern
    SETCOLOR(439, 255, 255, 0, 254); // Rose Bush
    SETCOLOR(440, 237, 141, 164, 254); //Peony
    
    SETVAR(RED_SANDSTONE, 186);
    SETCOLOR(441, 225, 140, 73,	255); // Regular
    SETCOLOR(442, 225, 140, 73,	255); // Chiseled
    SETCOLOR(443, 225, 140, 73,	255); // Smooth

    SETVAR(CONCRETE, 189);
    SETCOLOR(444, 208, 214, 215, 255); // White
    SETCOLOR(445, 225, 99, 3, 255); // Orange
    SETCOLOR(446, 169, 48, 159, 255); // Magenta
    SETCOLOR(447, 36, 138, 200, 255); // Light Blue
    SETCOLOR(448, 240, 175, 21, 255); // Yellow
    SETCOLOR(449, 95, 170, 25, 255); // Lime
    SETCOLOR(450, 214, 101, 143, 255); // Pink
    SETCOLOR(451, 54, 57, 61, 255); // Gray
    SETCOLOR(452, 125, 125, 115, 255); // Silver
    SETCOLOR(453, 22, 120, 137, 255); // Cyan
    SETCOLOR(454, 100, 31, 156, 255); // Purple
    SETCOLOR(455, 45, 47, 144, 255); // Blue
    SETCOLOR(456, 96, 59, 31, 255); // Brown
    SETCOLOR(457, 74, 92, 37, 255); // Green
    SETCOLOR(458, 143, 33, 33, 255); // Red
    SETCOLOR(459, 9, 11, 16, 255); // Black
    
    SETVAR(CONCRETE_POWDER, 205);
    SETCOLOR(460, 208, 214, 215, 255); // White
    SETCOLOR(461, 225, 99, 3, 255); // Orange
    SETCOLOR(462, 169, 48, 159, 255); // Magenta
    SETCOLOR(463, 36, 138, 200, 255); // Light Blue
    SETCOLOR(464, 240, 175, 21, 255); // Yellow
    SETCOLOR(465, 95, 170, 25, 255); // Lime
    SETCOLOR(466, 214, 101, 143, 255); // Pink
    SETCOLOR(467, 54, 57, 61, 255); // Gray
    SETCOLOR(468, 125, 125, 115, 255); // Silver
    SETCOLOR(469, 22, 120, 137, 255); // Cyan
    SETCOLOR(470, 100, 31, 156, 255); // Purple
    SETCOLOR(471, 45, 47, 144, 255); // Blue
    SETCOLOR(472, 96, 59, 31, 255); // Brown
    SETCOLOR(473, 74, 92, 37, 255); // Green
    SETCOLOR(474, 143, 33, 33, 255); // Red
    SETCOLOR(475, 9, 11, 16, 255); // Black

    /* Block type definition */
    
    SETBLOCKTYPE(GRASS, Block::GROWN);

    SETBLOCKTYPE(SNOW, Block::THIN);
    SETBLOCKTYPE(TRAPDOOR, Block::THIN);
    SETBLOCKTYPE(IRON_TRAPDOOR, Block::THIN);

    SETBLOCKTYPE(CARPET, Block::THIN);
    SETBLOCKTYPE(419, Block::THIN);
    SETBLOCKTYPE(420, Block::THIN);
    SETBLOCKTYPE(421, Block::THIN);
    SETBLOCKTYPE(422, Block::THIN);
    SETBLOCKTYPE(423, Block::THIN);
    SETBLOCKTYPE(424, Block::THIN);
    SETBLOCKTYPE(425, Block::THIN);
    SETBLOCKTYPE(426, Block::THIN);
    SETBLOCKTYPE(427, Block::THIN);
    SETBLOCKTYPE(428, Block::THIN);
    SETBLOCKTYPE(429, Block::THIN);
    SETBLOCKTYPE(430, Block::THIN);
    SETBLOCKTYPE(431, Block::THIN);
    SETBLOCKTYPE(432, Block::THIN);
    SETBLOCKTYPE(433, Block::THIN);
    SETBLOCKTYPE(434, Block::THIN);

    SETBLOCKTYPE(TORCH, Block::THIN_ROD);
    SETBLOCKTYPE(REDSTONE_TORCH, Block::THIN_ROD);
    SETBLOCKTYPE(UNLIT_REDSTONE_TORCH, Block::THIN_ROD);
    SETBLOCKTYPE(END_ROD, Block::THIN_ROD);

    SETBLOCKTYPE(FENCE, Block::ROD);
    SETBLOCKTYPE(SPRUCE_FENCE, Block::ROD);
    SETBLOCKTYPE(BIRCH_FENCE, Block::ROD);
    SETBLOCKTYPE(JUNGLE_FENCE, Block::ROD);
    SETBLOCKTYPE(ACACIA_FENCE, Block::ROD);
    SETBLOCKTYPE(DARK_OAK_FENCE, Block::ROD);
    SETBLOCKTYPE(FENCE_GATE, Block::ROD);
    SETBLOCKTYPE(SPRUCE_FENCE_GATE, Block::ROD);
    SETBLOCKTYPE(BIRCH_FENCE_GATE, Block::ROD);
    SETBLOCKTYPE(JUNGLE_FENCE_GATE, Block::ROD);
    SETBLOCKTYPE(ACACIA_FENCE_GATE, Block::ROD);
    SETBLOCKTYPE(DARK_OAK_FENCE_GATE, Block::ROD);
    SETBLOCKTYPE(VINE, Block::ROD);
    SETBLOCKTYPE(IRON_BARS, Block::ROD);
    SETBLOCKTYPE(NETHER_BRICK_FENCE, Block::ROD);
    SETBLOCKTYPE(COBBLESTONE_WALL, Block::ROD);
    SETBLOCKTYPE(375, Block::ROD);
    SETBLOCKTYPE(376, Block::ROD);
    SETBLOCKTYPE(CHORUS_PLANT, Block::ROD);
    SETBLOCKTYPE(REEDS, Block::ROD);

    SETBLOCKTYPE(YELLOW_FLOWER, Block::PLANT);

    SETBLOCKTYPE(RED_FLOWER, Block::PLANT);
    SETBLOCKTYPE(312, Block::PLANT);
    SETBLOCKTYPE(313, Block::PLANT);
    SETBLOCKTYPE(314, Block::PLANT);
    SETBLOCKTYPE(315, Block::PLANT);
    SETBLOCKTYPE(316, Block::PLANT);
    SETBLOCKTYPE(317, Block::PLANT);
    SETBLOCKTYPE(318, Block::PLANT);
    SETBLOCKTYPE(319, Block::PLANT);
    SETBLOCKTYPE(320, Block::PLANT);

    SETBLOCKTYPE(RED_MUSHROOM, Block::PLANT);
    SETBLOCKTYPE(BROWN_MUSHROOM, Block::PLANT);
    SETBLOCKTYPE(WEB, Block::PLANT);
    SETBLOCKTYPE(DEADBUSH, Block::PLANT);
    SETBLOCKTYPE(MELON_STEM, Block::PLANT);
    SETBLOCKTYPE(PUMPKIN_STEM, Block::PLANT);
    SETBLOCKTYPE(DEADBUSH, Block::PLANT);
    SETBLOCKTYPE(WATERLILY, Block::PLANT);
    SETBLOCKTYPE(NETHER_WART, Block::PLANT);

    SETBLOCKTYPE(DOUBLE_PLANT, Block::PLANT);
    SETBLOCKTYPE(435, Block::PLANT);
    SETBLOCKTYPE(436, Block::PLANT);
    SETBLOCKTYPE(437, Block::PLANT);
    SETBLOCKTYPE(438, Block::PLANT);
    SETBLOCKTYPE(439, Block::PLANT);
    SETBLOCKTYPE(440, Block::PLANT);

    SETBLOCKTYPE(REDSTONE_WIRE, Block::WIRE);
    SETBLOCKTYPE(TRIPWIRE, Block::WIRE);

    SETBLOCKTYPE(RAIL, Block::RAILROAD);
    SETBLOCKTYPE(GOLDEN_RAIL, Block::RAILROAD);
    SETBLOCKTYPE(ACTIVATOR_RAIL, Block::RAILROAD);
    SETBLOCKTYPE(DETECTOR_RAIL, Block::RAILROAD);

    SETBLOCKTYPE(FIRE, Block::SPECIAL);
    SETBLOCKTYPE(COCOA, Block::SPECIAL);
    SETBLOCKTYPE(TALLGRASS, Block::SPECIAL);
    SETBLOCKTYPE(293, Block::SPECIAL);
    SETBLOCKTYPE(294, Block::SPECIAL);
    SETBLOCKTYPE(295, Block::SPECIAL);

    SETBLOCKTYPE(WOODEN_BUTTON, Block::OTHER);
    SETBLOCKTYPE(STONE_BUTTON, Block::OTHER);

    SETBLOCKTYPE(CAKE, Block::HALF);
    SETBLOCKTYPE(BED, Block::HALF);
    SETBLOCKTYPE(STONE_SLAB, Block::HALF);
    SETBLOCKTYPE(329, Block::HALF);
    SETBLOCKTYPE(330, Block::HALF);
    SETBLOCKTYPE(331, Block::HALF);
    SETBLOCKTYPE(332, Block::HALF);
    SETBLOCKTYPE(333, Block::HALF);
    SETBLOCKTYPE(334, Block::HALF);
    SETBLOCKTYPE(335, Block::HALF);
    SETBLOCKTYPE(336, Block::HALF);

    SETBLOCKTYPE(WOODEN_SLAB, Block::HALF);
    SETBLOCKTYPE(369, Block::HALF);
    SETBLOCKTYPE(370, Block::HALF);
    SETBLOCKTYPE(371, Block::HALF);
    SETBLOCKTYPE(372, Block::HALF);
    SETBLOCKTYPE(373, Block::HALF);
    SETBLOCKTYPE(374, Block::HALF);
    
    SETBLOCKTYPE(DAYLIGHT_DETECTOR, Block::HALF);
    SETBLOCKTYPE(DAYLIGHT_DETECTOR_INVERTED, Block::HALF);

    SETBLOCKTYPE(OAK_STAIRS, Block::STAIR);
    SETBLOCKTYPE(SPRUCE_STAIRS, Block::STAIR);
    SETBLOCKTYPE(BIRCH_STAIRS, Block::STAIR);
    SETBLOCKTYPE(JUNGLE_STAIRS, Block::STAIR);
    SETBLOCKTYPE(ACACIA_STAIRS, Block::STAIR);
    SETBLOCKTYPE(DARK_OAK_STAIRS, Block::STAIR);
    SETBLOCKTYPE(BRICK_STAIRS, Block::STAIR);
    SETBLOCKTYPE(STONE_STAIRS, Block::STAIR);
    SETBLOCKTYPE(STONE_BRICK_STAIRS, Block::STAIR);
    SETBLOCKTYPE(NETHER_BRICK_STAIRS, Block::STAIR);
    SETBLOCKTYPE(SANDSTONE_STAIRS, Block::STAIR);
    SETBLOCKTYPE(QUARTZ_STAIRS, Block::STAIR);
    SETBLOCKTYPE(RED_SANDSTONE_STAIRS, Block::STAIR);
    SETBLOCKTYPE(PURPUR_STAIRS, Block::STAIR);
}


bool loadColorsFromFile(const char *file)
{
    char buffer[500], *ptr;
    int id, variant;
    FILE *f = fopen(file, "r");
    uint8_t vals[5];
    bool valid;

    if (f == NULL) {
	return false;
    }
    while (!feof(f)) {
	variant = 0;
	if (fgets(buffer, 500, f) == NULL) {
	    break;
	}

	ptr = buffer;
	while (*ptr == ' ' || *ptr == '\t') {
	    ++ptr;
	}

	if (*ptr == '\0' || *ptr == '#' || *ptr == '\12') {
	    continue;   // This is a comment or empty line, skip
	}

	id = atoi(ptr);
	if (id < 0 || id > 255) {
	    printf("Skipping invalid blockid %d in colors file\n", id);
	    continue;
	}

	while (*ptr != ' ' && *ptr != '\t' && *ptr != '\0') {
	    if (*ptr == ':') {
		variant = atoi(++ptr);
	    }
	    ++ptr;
	}

	valid = true;

	for (int i = 0; i < 5; ++i) {
	    while (*ptr == ' ' || *ptr == '\t')
		++ptr;

	    if (*ptr == '\0' || *ptr == '#' || *ptr == '\12') {
		printf("Too few arguments for block %d, ignoring line.\n", id);
		valid = false;
		break;
	    }

	    vals[i] = clamp(atoi(ptr));
	    while (*ptr != ' ' && *ptr != '\t' && *ptr != '\0') {
		++ptr;
	    }
	}

	if (!valid)
	    continue;

	Block::setColor(id, variant, vals);
    }

    fclose(f);
    return true;
}

bool dumpColorsToFile(const char *file) {
    uint8_t* color;
    uint8_t toDump[256];
    FILE *f = fopen(file, "w");

    if (f == NULL) {
	printf("Error: %s\n", strerror(errno));
	return false;
    }

    memset(toDump, 1, 256*sizeof(uint8_t));

    toDump[STONE] = 7;
    toDump[DIRT] = 3;
    toDump[PLANKS] = 6;
    toDump[SAPLING] = 6;
    toDump[SAND] = 2;
    toDump[LOG] = 4;
    toDump[LEAVES] = 4;
    toDump[SPONGE] = 2;
    toDump[SANDSTONE] = 3;
    toDump[TALLGRASS] = 3;
    toDump[WOOL] = 16;
    toDump[RED_FLOWER] = 9;
    toDump[DOUBLE_STONE_SLAB] = 8;
    toDump[STONE_SLAB] = 8;
    toDump[STAINED_GLASS] = 16;
    toDump[MONSTER_EGG] = 6;
    toDump[STONEBRICK] = 4;
    toDump[DOUBLE_WOODEN_SLAB] = 6;
    toDump[WOODEN_SLAB] = 6;
    toDump[COBBLESTONE_WALL] = 2;
    toDump[QUARTZ_BLOCK] = 3;
    toDump[STAINED_HARDENED_CLAY] = 16;
    toDump[STAINED_GLASS_PANE] = 16;
    toDump[LEAVES2] = 2;
    toDump[LOG2] = 2;
    toDump[PRISMARINE] = 3;
    toDump[CARPET] = 16;
    toDump[DOUBLE_PLANT] = 6;
    toDump[RED_SANDSTONE] = 3;
    toDump[CONCRETE] = 16;
    toDump[CONCRETE_POWDER] = 16;

    fprintf(f, "# For Block IDs see http://minecraftwiki.net/wiki/Data_values\n"
	    "# and http://wrim.pl/mcmap (for blocks introduced since Minecraft 1.3.1 and mcmap 2.4)\n"
	    "# Note that noise or alpha (or both) do not work for a few blocks like snow, torches, fences, steps, ...\n"
	    "# Actually, if you see any block has an alpha value of 254 you should leave it that way to prevent black artifacts.\n"
	    "# If you want to set alpha of grass to <255, use -blendall or you won't get what you expect.\n"
	    "# Noise is supposed to look normal using -noise 10\n"
	   );

    fprintf(f, "#ID:V\tR\tG\tB\tA\tNoise\n");

    for (uint8_t i = 0; i < 255; ++i) {
	for (uint8_t j = 0; j < toDump[i]; ++j) {
	    color = Block::getColor(i, j);
	    if (toDump[i] == 1)
		fprintf(f, "%3d", i);
	    else
		fprintf(f, "%3d:%d", i, j);
	    fprintf(f, "\t%3d\t%3d\t%3d\t%3d\t%3d\n", int(color[PRED]), int(color[PGREEN]), int(color[PBLUE]), int(color[PALPHA]), int(color[NOISE]));
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
	//g_TallGrasscolor[i] = ((int)g_Grasscolor[i] * (int)colors[TALL_GRASS][BRIGHTNESS]) / 255;
	g_Grasscolor[i] = ((int)g_Grasscolor[i] * (int)colors[GRASS][BRIGHTNESS]) / 255;
    }
    const int maxT = pngLeaf->getWidth() * pngLeaf->getHeight() * g_FoliageDepth;
    for (int i = 0; i < maxT; ++i) {
	g_Leafcolor[i] = ((int)g_Leafcolor[i] * (int)colors[LEAVES][BRIGHTNESS]) / 255;
    }
    // Now re-calc brightness of those two
    colors[GRASS][BRIGHTNESS] = GETBRIGHTNESS(g_Grasscolor) - 5;
    //colors[LEAVES][BRIGHTNESS] = colors[PINELEAVES][BRIGHTNESS] = colors[BIRCHLEAVES][BRIGHTNESS] = GETBRIGHTNESS(g_Leafcolor) - 5;
    printf("Loaded biome color maps from %s\n", path);
    return true;
}
