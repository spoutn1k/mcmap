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
    //SETCOLOR		(UNPOWERED_COMPARATOR,	125,	   91,	   38,	  255   ); //Not Set
    //SETCOLOR		(POWERED_COMPARATOR,	125,	   91,	   38,	  255   ); //Not Set
    SETCOLOR		(DAYLIGHT_DETECTOR,	187,	158,	109,	255 ); //daylight sensor
    SETCOLOR		(REDSTONE_BLOCK,	227,	 38,	 12,	255 ); //redstone block
    //SETCOLOR		(QUARTZ_ORE,	227,	 38,	 12,	255 ); //Not Set
    SETCOLOR		(HOPPER,	110,	110,	110,	255 ); //hopper
    SETCOLOR		(QUARTZ_BLOCK,	240,	238,	232,	255 ); //quartz
    SETCOLOR		(QUARTZ_STAIRS,	240,	238,	232,	255 ); //quartz stairs
    SETCOLOR		(ACTIVATOR_RAIL,	120,	114,	92,	250);
    SETCOLOR		(DROPPER,	107,	107,	107,	255);
    SETCOLOR		(STAINED_HARDENED_CLAY,	 241,	210,	192,	255   ); //White Stained Clay
    SETCOLOR		(STAINED_GLASS_PANE,	255,	255,	 255,	 100  ); //White Stained Glass pane
    SETCOLOR		(LEAVES2,	54,	135,	40,	180 ); //leaves Acacia/Dark Oak
    SETCOLOR		(LOG2,		72,	 72,	72,	255 ); //log Acacia/Dark Oak
    SETCOLOR		(ACACIA_STAIRS,	156,	127,	78,	255); // Acacia Wood Stairs
    SETCOLOR		(DARK_OAK_STAIRS,	156,	127,	78,	255); // Dark Oak Wood Stairs
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
    SETCOLOR		(DOUBLE_STONE_SLAB2,	200,	200,	200,	255);
    SETCOLOR		(STONE_SLAB2,	200,	200,	200,	254); // Not fully opaque to prevent culling on this one
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
    SETCOLOR		(END_ROD,	0,	0,	0,	0);
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
    SETCOLOR		(NETHER_WART_BLOCK,	0,	0,	0,	0);
    SETCOLOR		(RED_NETHER_BRICK,	0,	0,	0,	0);
    SETCOLOR		(BONE_BLOCK,	0,	0,	0,	0);
    SETCOLOR		(STRUCTURE_VOID,	0,	0,	0,	0);
    SETCOLOR		(OBSERVER,	0,	0,	0,	0);
    SETCOLOR		(WHITE_SHULKER_BOX,	0,	0,	0,	0);
    SETCOLOR		(ORANGE_SHULKER_BOX,	0,	0,	0,	0);
    SETCOLOR		(MAGENTA_SHULKER_BOX,	0,	0,	0,	0);
    SETCOLOR		(LIGHT_BLUE_SHULKER_BOX,	0,	0,	0,	0);
    SETCOLOR		(YELLOW_SHULKER_BOX,	0,	0,	0,	0);
    SETCOLOR		(LIME_SHULKER_BOX,	0,	0,	0,	0);
    SETCOLOR		(PINK_SHULKER_BOX,	0,	0,	0,	0);
    SETCOLOR		(GRAY_SHULKER_BOX,	0,	0,	0,	0);
    SETCOLOR		(SILVER_SHULKER_BOX,	0,	0,	0,	0);
    SETCOLOR		(CYAN_SHULKER_BOX,	0,	0,	0,	0);
    SETCOLOR		(PURPLE_SHULKER_BOX,	0,	0,	0,	0);
    SETCOLOR		(BLUE_SHULKER_BOX,	0,	0,	0,	0);
    SETCOLOR		(BROWN_SHULKER_BOX,	0,	0,	0,	0);
    SETCOLOR		(GREEN_SHULKER_BOX,	0,	0,	0,	0);
    SETCOLOR		(RED_SHULKER_BOX,	0,	0,	0,	0);
    SETCOLOR		(BLACK_SHULKER_BOX,	0,	0,	0,	0);
    SETCOLOR		(WHITE_GLAZED_TERRACOTTA,	0,	0,	0,	0);
    SETCOLOR		(ORANGE_GLAZED_TERRACOTTA,	0,	0,	0,	0);
    SETCOLOR		(MAGENTA_GLAZED_TERRACOTTA,	0,	0,	0,	0);
    SETCOLOR		(LIGHT_BLUE_GLAZED_TERRACOTTA,	0,	0,	0,	0);
    SETCOLOR		(YELLOW_GLAZED_TERRACOTTA,	0,	0,	0,	0);
    SETCOLOR		(LIME_GLAZED_TERRACOTTA,	0,	0,	0,	0);
    SETCOLOR		(PINK_GLAZED_TERRACOTTA,	0,	0,	0,	0);
    SETCOLOR		(GRAY_GLAZED_TERRACOTTA,	0,	0,	0,	0);
    SETCOLOR		(LIGHT_GRAY_GLAZED_TERRACOTTA,	0,	0,	0,	0);
    SETCOLOR		(CYAN_GLAZED_TERRACOTTA,	0,	0,	0,	0);
    SETCOLOR		(PURPLE_GLAZED_TERRACOTTA,	0,	0,	0,	0);
    SETCOLOR		(BLUE_GLAZED_TERRACOTTA,	0,	0,	0,	0);
    SETCOLOR		(BROWN_GLAZED_TERRACOTTA,	0,	0,	0,	0);
    SETCOLOR		(GREEN_GLAZED_TERRACOTTA,	0,	0,	0,	0);
    SETCOLOR		(RED_GLAZED_TERRACOTTA,	0,	0,	0,	0);
    SETCOLOR		(BLACK_GLAZED_TERRACOTTA,	0,	0,	0,	0);
    SETCOLOR		(CONCRETE,	0,	0,	0,	0);
    SETCOLOR		(CONCRETE_POWDER,	0,	0,	0,	0);

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
    SETCOLORNOISE	(270,	172,	92,	0,	255,	11); // Acacia
    SETCOLORNOISE	(271,	57,	43,	0,	255,	11); // Dark Oak

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
    SETCOLOR(280, 102,	81,	51,	255); // Oak
    SETCOLOR(281, 70,50,32, 255); // Spruce trunk
    SETCOLORNOISE(282, 206,206,201, 255, 5); // Birch trunk
    SETCOLOR(283, 122,91,51, 255); // Jungle trunk

    // Leaves Variants
    SETVAR(LEAVES, 29);
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
    SETCOLOR(313, 120, 158, 241, 254   ); //blue orchid
    SETCOLOR(314, 200,  75, 210, 254 ); //allium
    SETCOLOR(315, 168, 172, 172, 254 ); //azure bluet
    SETCOLOR(316, 173,  44,  40, 254 ); //red tulip
    SETCOLOR(317, 244, 137,  54, 254 ); //orange tulip
    SETCOLOR(318, 255, 255, 255, 254 ); //white tulip
    SETCOLOR(319, 237, 141, 164, 254 ); //pink tulip
    SETCOLOR(320, 168, 172, 172, 254 ); //oxeye daisy

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

    // Wooden Slab Variants
    SETVAR(WOODEN_SLAB, 108);
    SETNBVAR(WOODEN_SLAB, 8);
    SETCOLORNOISE(363,	156,	127,	78,	254,	11); // Oak
    SETCOLORNOISE(364, 103,77,46,254, 1); // Spruce
    SETCOLORNOISE(365, 195,179,123,254, 3); // Birch
    SETCOLORNOISE(366, 154,110,77,254, 2); // Jungle
    SETCOLORNOISE(367,	172,	92,	0,	254,	11); // Acacia
    SETCOLORNOISE(368,	57,	43,	0,	254,	11); // Dark Oak

    SETVAR(COBBLESTONE_WALL, 114);
    SETCOLORNOISE	(369,	   128,	  128,	  128,	  255,	  16); // cobblestone wall
    SETCOLORNOISE	(370,	90,	108,	90,	255,	27); // Mossy cobblestone wall

    SETVAR(QUARTZ_BLOCK, 116);
    SETCOLOR		(371,	240,	238,	232,	255 ); // Regular
    SETCOLOR		(372,	240,	238,	232,	255 ); // Chiseled
    SETCOLOR		(373,	240,	238,	232,	255 ); // Pillar

    // Stained Clay Variants
    SETVAR(STAINED_HARDENED_CLAY, 119);
    SETCOLOR(374,  241, 210, 192, 255   ); //White Stained Clay
    SETCOLOR(375,  194, 116,  69, 255   ); //Orange Stained Clay
    SETCOLOR(376,  182, 120, 140, 255   ); //Magenta Stained Clay
    SETCOLOR(377,  141, 137, 167, 255   ); //Light Blue Stained Clay
    SETCOLOR(378,  219, 165,  66, 255   ); //Yellow Stained Clay
    SETCOLOR(379,  137, 149,  84, 255   ); //Lime Stained Clay
    SETCOLOR(380,  194, 110, 110, 255   ); //Pink Stained Clay
    SETCOLOR(381,   97,  82,  75, 255   ); //Gray Stained Clay
    SETCOLOR(382,  168, 138, 128, 255   ); //Light Gray Stained Clay
    SETCOLOR(383,  119, 122, 122, 255   ); //Cyan Stained Clay
    SETCOLOR(384,  152, 102, 117, 255   ); //Purple Stained Clay
    SETCOLOR(385,  103,  88, 120, 255   ); //Blue Stained Clay
    SETCOLOR(386,  109,  82,  66, 255   ); //Brown Stained Clay
    SETCOLOR(387,  105, 112,  70, 255   ); //Green Stained Clay
    SETCOLOR(388,  176,  93,  78, 255   ); //Red Stained Clay
    SETCOLOR(389,   67,  52,  46, 255   ); //Black Stained Clay

    // Stained Glass Panes Variants
    SETVAR(STAINED_GLASS, 135);
    SETCOLOR(390, 255, 255,  255,  100  ); //White Stained Glass Pane
    SETCOLOR(391,  244, 137,  54,  40   ); //Orange Stained Glass Pane
    SETCOLOR(392,  200,  75, 210,  40   ); //Magenta Stained Glass Pane
    SETCOLOR(393,  120, 158, 241,  40   ); //Light Blue Stained Glass Pane
    SETCOLOR(394,  204, 200,  28,  40   ); //Yellow Stained Glass Pane
    SETCOLOR(395,   59, 210,  47,  40   ); //Lime Stained Glass Pane
    SETCOLOR(396,  237, 141, 164,  40   ); //Pink Stained Glass Pane
    SETCOLOR(397,   76,  76,  76,  40   ); //Gray Stained Glass Pane
    SETCOLOR(398,  168, 172, 172,  40   ); //Light Gray Stained Glass Pane
    SETCOLOR(399,   39, 116, 149,  40   ); //Cyan Stained Glass Pane
    SETCOLOR(400,  133,  53, 195,  40   ); //Purple Stained Glass Pane
    SETCOLOR(401,   38,  51, 160,  40   ); //Blue Stained Glass Pane
    SETCOLOR(402,   85,  51,  27,  40   ); //Brown Stained Glass Pane
    SETCOLOR(403,   55,  77,  24,  40   ); //Green Stained Glass Pane
    SETCOLOR(404,  173,  44,  40,  40   ); //Red Stained Glass Pane
    SETCOLOR(405,   32,  27,  27,  40   ); //Black Stained Glass Pane

    SETVAR(LEAVES2, 151);
    SETCOLOR	(406,	54,	135,	40,	180 ); // Acacia leaves
    SETCOLOR	(407,	44,	84,	44,	160); // Dark Oak leaves

    SETVAR(LOG2, 153);
    SETNBVAR(LOG2, 2); // Only 2 variants
    SETCOLOR (408, 115,	115, 115, 255); // Acacia log
    SETCOLOR (409, 72, 72, 72, 255 ); // Dark Oak log

    SETVAR(PRISMARINE, 155);
    SETCOLOR		(410,	25,	146,	132,	255  ); // Regular
    SETCOLOR		(411,	25,	146,	132,	255  ); // Bricks
    SETCOLOR		(412,	15,	136,	122,	255  ); // Dark
    
    SETVAR(CARPET, 158);
    SETCOLOR(413,  255, 255, 255,  254   ); //White carpet
    SETCOLOR(414,  244, 137,  54,  254   ); //Orange carpet
    SETCOLOR(415,  200,  75, 210,  254   ); //Magenta carpet
    SETCOLOR(416,  120, 158, 241,  254   ); //Light Blue carpet
    SETCOLOR(417,  204, 200,  28,  254   ); //Yellow carpet
    SETCOLOR(418,   59, 210,  47,  254   ); //Lime carpet
    SETCOLOR(419,  237, 141, 164,  254   ); //Pink carpet
    SETCOLOR(420,   76,  76,  76,  254   ); //Gray carpet
    SETCOLOR(421,  168, 172, 172,  254   ); //Light Gray
    SETCOLOR(422,   39, 116, 149,  254   ); //Cyan carpet
    SETCOLOR(423,  133,  53, 195,  254   ); //Purple carpet
    SETCOLOR(424,   38,  51, 160,  254   ); //Blue carpet
    SETCOLOR(425,   85,  51,  27,  254   ); //Brown carpet
    SETCOLOR(426,   55,  77,  24,  254   ); //Green carpet
    SETCOLOR(427,  173,  44,  40,  254   ); //Red carpet
    SETCOLOR(428,   32,  27,  27,  254   ); //Black carpet

    SETVAR(DOUBLE_PLANT, 174);
    SETCOLOR(429, 255, 255,  0, 254); // Sunflower
    SETCOLOR(430, 200,  75, 210, 254 ); //Lilac
    SETCOLORNOISE(431,	123,	79,	25,	254,	25); // Grass
    SETCOLORNOISE(432,	110,	166,	68,	254,	12); // Fern
    SETCOLOR(433,	255,	255,	0,	254); // Rose Bush
    SETCOLOR(434, 237, 141, 164, 254 ); //Peony
    
    SETVAR(RED_SANDSTONE, 180);
    SETCOLOR(435, 225, 140, 73,	255); // Regular
    SETCOLOR(436, 225, 140, 73,	255); // Chiseled
    SETCOLOR(437, 225, 140, 73,	255); // Smooth

    SETVAR(CONCRETE, 183);
    SETCOLOR(438, 208, 214, 215, 255); // White
    SETCOLOR(439, 225, 99, 3, 255); // Orange
    SETCOLOR(440, 169, 48, 159, 255); // Magenta
    SETCOLOR(441, 36, 138, 200, 255); // Light Blue
    SETCOLOR(442, 240, 175, 21, 255); // Yellow
    SETCOLOR(443, 95, 170, 25, 255); // Lime
    SETCOLOR(444, 214, 101, 143, 255); // Pink
    SETCOLOR(445, 54, 57, 61, 255); // Gray
    SETCOLOR(446, 125, 125, 115, 255); // Silver
    SETCOLOR(447, 22, 120, 137, 255); // Cyan
    SETCOLOR(448, 100, 31, 156, 255); // Purple
    SETCOLOR(449, 45, 47, 144, 255); // Blue
    SETCOLOR(450, 96, 59, 31, 255); // Brown
    SETCOLOR(451, 74, 92, 37, 255); // Green
    SETCOLOR(452, 143, 33, 33, 255); // Red
    SETCOLOR(453, 9, 11, 16, 255); // Black
    
    SETVAR(CONCRETE_POWDER, 199);
    SETCOLOR(454, 208, 214, 215, 255); // White
    SETCOLOR(455, 225, 99, 3, 255); // Orange
    SETCOLOR(456, 169, 48, 159, 255); // Magenta
    SETCOLOR(457, 36, 138, 200, 255); // Light Blue
    SETCOLOR(458, 240, 175, 21, 255); // Yellow
    SETCOLOR(459, 95, 170, 25, 255); // Lime
    SETCOLOR(460, 214, 101, 143, 255); // Pink
    SETCOLOR(461, 54, 57, 61, 255); // Gray
    SETCOLOR(462, 125, 125, 115, 255); // Silver
    SETCOLOR(463, 22, 120, 137, 255); // Cyan
    SETCOLOR(464, 100, 31, 156, 255); // Purple
    SETCOLOR(465, 45, 47, 144, 255); // Blue
    SETCOLOR(466, 96, 59, 31, 255); // Brown
    SETCOLOR(467, 74, 92, 37, 255); // Green
    SETCOLOR(468, 143, 33, 33, 255); // Red
    SETCOLOR(469, 9, 11, 16, 255); // Black

    /* Block type definition */
    
    SETBLOCKTYPE(GRASS, Blocks::GROWN);

    SETBLOCKTYPE(SNOW, Blocks::THIN);
    SETBLOCKTYPE(TRAPDOOR, Blocks::THIN);
    SETBLOCKTYPE(IRON_TRAPDOOR, Blocks::THIN);

    SETBLOCKTYPE(CARPET, Blocks::THIN);
    SETBLOCKTYPE(413, Blocks::THIN);
    SETBLOCKTYPE(414, Blocks::THIN);
    SETBLOCKTYPE(415, Blocks::THIN);
    SETBLOCKTYPE(416, Blocks::THIN);
    SETBLOCKTYPE(417, Blocks::THIN);
    SETBLOCKTYPE(418, Blocks::THIN);
    SETBLOCKTYPE(419, Blocks::THIN);
    SETBLOCKTYPE(420, Blocks::THIN);
    SETBLOCKTYPE(421, Blocks::THIN);
    SETBLOCKTYPE(422, Blocks::THIN);
    SETBLOCKTYPE(423, Blocks::THIN);
    SETBLOCKTYPE(424, Blocks::THIN);
    SETBLOCKTYPE(425, Blocks::THIN);
    SETBLOCKTYPE(426, Blocks::THIN);
    SETBLOCKTYPE(427, Blocks::THIN);
    SETBLOCKTYPE(428, Blocks::THIN);

    SETBLOCKTYPE(TORCH, Blocks::THIN_ROD);
    SETBLOCKTYPE(REDSTONE_TORCH, Blocks::THIN_ROD);
    SETBLOCKTYPE(UNLIT_REDSTONE_TORCH, Blocks::THIN_ROD);
    SETBLOCKTYPE(END_ROD, Blocks::THIN_ROD);

    SETBLOCKTYPE(FENCE, Blocks::ROD);
    SETBLOCKTYPE(SPRUCE_FENCE, Blocks::ROD);
    SETBLOCKTYPE(BIRCH_FENCE, Blocks::ROD);
    SETBLOCKTYPE(JUNGLE_FENCE, Blocks::ROD);
    SETBLOCKTYPE(ACACIA_FENCE, Blocks::ROD);
    SETBLOCKTYPE(DARK_OAK_FENCE, Blocks::ROD);
    SETBLOCKTYPE(FENCE_GATE, Blocks::ROD);
    SETBLOCKTYPE(SPRUCE_FENCE_GATE, Blocks::ROD);
    SETBLOCKTYPE(BIRCH_FENCE_GATE, Blocks::ROD);
    SETBLOCKTYPE(JUNGLE_FENCE_GATE, Blocks::ROD);
    SETBLOCKTYPE(ACACIA_FENCE_GATE, Blocks::ROD);
    SETBLOCKTYPE(DARK_OAK_FENCE_GATE, Blocks::ROD);
    SETBLOCKTYPE(VINE, Blocks::ROD);
    SETBLOCKTYPE(IRON_BARS, Blocks::ROD);
    SETBLOCKTYPE(NETHER_BRICK_FENCE, Blocks::ROD);
    SETBLOCKTYPE(COBBLESTONE_WALL, Blocks::ROD);
    SETBLOCKTYPE(369, Blocks::ROD);
    SETBLOCKTYPE(370, Blocks::ROD);
    SETBLOCKTYPE(CHORUS_PLANT, Blocks::ROD);

    SETBLOCKTYPE(YELLOW_FLOWER, Blocks::PLANT);

    SETBLOCKTYPE(RED_FLOWER, Blocks::PLANT);
    SETBLOCKTYPE(312, Blocks::PLANT);
    SETBLOCKTYPE(313, Blocks::PLANT);
    SETBLOCKTYPE(314, Blocks::PLANT);
    SETBLOCKTYPE(315, Blocks::PLANT);
    SETBLOCKTYPE(316, Blocks::PLANT);
    SETBLOCKTYPE(317, Blocks::PLANT);
    SETBLOCKTYPE(318, Blocks::PLANT);
    SETBLOCKTYPE(319, Blocks::PLANT);
    SETBLOCKTYPE(320, Blocks::PLANT);

    SETBLOCKTYPE(RED_MUSHROOM, Blocks::PLANT);
    SETBLOCKTYPE(BROWN_MUSHROOM, Blocks::PLANT);
    SETBLOCKTYPE(WEB, Blocks::PLANT);
    SETBLOCKTYPE(DEADBUSH, Blocks::PLANT);
    SETBLOCKTYPE(MELON_STEM, Blocks::PLANT);
    SETBLOCKTYPE(PUMPKIN_STEM, Blocks::PLANT);
    SETBLOCKTYPE(DEADBUSH, Blocks::PLANT);
    SETBLOCKTYPE(WATERLILY, Blocks::PLANT);
    SETBLOCKTYPE(NETHER_WART, Blocks::PLANT);

    SETBLOCKTYPE(DOUBLE_PLANT, Blocks::PLANT);
    SETBLOCKTYPE(429, Blocks::PLANT);
    SETBLOCKTYPE(430, Blocks::PLANT);
    SETBLOCKTYPE(431, Blocks::PLANT);
    SETBLOCKTYPE(432, Blocks::PLANT);
    SETBLOCKTYPE(433, Blocks::PLANT);
    SETBLOCKTYPE(434, Blocks::PLANT);

    SETBLOCKTYPE(REDSTONE_WIRE, Blocks::WIRE);
    SETBLOCKTYPE(TRIPWIRE, Blocks::WIRE);

    SETBLOCKTYPE(RAIL, Blocks::RAILROAD);
    SETBLOCKTYPE(GOLDEN_RAIL, Blocks::RAILROAD);
    SETBLOCKTYPE(ACTIVATOR_RAIL, Blocks::RAILROAD);
    SETBLOCKTYPE(DETECTOR_RAIL, Blocks::RAILROAD);

    SETBLOCKTYPE(FIRE, Blocks::SPECIAL);
    SETBLOCKTYPE(COCOA, Blocks::SPECIAL);
    SETBLOCKTYPE(TALLGRASS, Blocks::SPECIAL);
    SETBLOCKTYPE(293, Blocks::SPECIAL);
    SETBLOCKTYPE(294, Blocks::SPECIAL);
    SETBLOCKTYPE(295, Blocks::SPECIAL);

    SETBLOCKTYPE(WOODEN_BUTTON, Blocks::OTHER);
    SETBLOCKTYPE(STONE_BUTTON, Blocks::OTHER);

    SETBLOCKTYPE(STONE_SLAB, Blocks::HALF);
    SETBLOCKTYPE(329, Blocks::HALF);
    SETBLOCKTYPE(330, Blocks::HALF);
    SETBLOCKTYPE(331, Blocks::HALF);
    SETBLOCKTYPE(332, Blocks::HALF);
    SETBLOCKTYPE(333, Blocks::HALF);
    SETBLOCKTYPE(334, Blocks::HALF);
    SETBLOCKTYPE(335, Blocks::HALF);
    SETBLOCKTYPE(336, Blocks::HALF);

    SETBLOCKTYPE(WOODEN_SLAB, Blocks::HALF);
    SETBLOCKTYPE(363, Blocks::HALF);
    SETBLOCKTYPE(364, Blocks::HALF);
    SETBLOCKTYPE(365, Blocks::HALF);
    SETBLOCKTYPE(366, Blocks::HALF);
    SETBLOCKTYPE(367, Blocks::HALF);
    SETBLOCKTYPE(368, Blocks::HALF);
    
    SETBLOCKTYPE(DAYLIGHT_DETECTOR, Blocks::HALF);
    SETBLOCKTYPE(DAYLIGHT_DETECTOR_INVERTED, Blocks::HALF);

    SETBLOCKTYPE(OAK_STAIRS, Blocks::STAIR);
    SETBLOCKTYPE(SPRUCE_STAIRS, Blocks::STAIR);
    SETBLOCKTYPE(BIRCH_STAIRS, Blocks::STAIR);
    SETBLOCKTYPE(JUNGLE_STAIRS, Blocks::STAIR);
    SETBLOCKTYPE(ACACIA_STAIRS, Blocks::STAIR);
    SETBLOCKTYPE(DARK_OAK_STAIRS, Blocks::STAIR);
    SETBLOCKTYPE(BRICK_STAIRS, Blocks::STAIR);
    SETBLOCKTYPE(STONE_STAIRS, Blocks::STAIR);
    SETBLOCKTYPE(STONE_BRICK_STAIRS, Blocks::STAIR);
    SETBLOCKTYPE(NETHER_BRICK_STAIRS, Blocks::STAIR);
    SETBLOCKTYPE(SANDSTONE_STAIRS, Blocks::STAIR);
    SETBLOCKTYPE(QUARTZ_STAIRS, Blocks::STAIR);
    SETBLOCKTYPE(RED_SANDSTONE_STAIRS, Blocks::STAIR);
    SETBLOCKTYPE(PURPUR_STAIRS, Blocks::STAIR);
    /*

       SETCOLORNOISE	(PINELEAVES,	44,	84,	44,	160,	20); // Pine leaves
       SETCOLORNOISE	(BIRCHLEAVES,	85,	124,	60,	170,	11); // Birch leaves
       SETCOLORNOISE	(JUNGLELEAVES,	44,	135,	50,	175,	11); // Birch leaves
       SETCOLORNOISE	(SANDSTEP, 218, 210, 158, 254, 7); // Not fully opaque to prevent culling on this one
       SETCOLORNOISE(UP_SANDSTEP, 218, 210, 158, 254, 7); // Not fully opaque to prevent culling on this one
       SETCOLORNOISE(WOODSTEP, 157,128,79,254, 11); // Not fully opaque to prevent culling on this one
       SETCOLORNOISE(UP_WOODSTEP, 157,128,79,254, 11); // Not fully opaque to prevent culling on this one
       SETCOLORNOISE(COBBLESTEP, 115,115,115,254, 26); // Not fully opaque to prevent culling on this one
       SETCOLORNOISE(UP_COBBLESTEP, 115,115,115,254, 26); // Not fully opaque to prevent culling on this one

       SETCOLORNOISE(PINESTEP, 103,77,46,254, 1);
       SETCOLORNOISE(BIRCHSTEP, 195,179,123,254, 3);
       SETCOLORNOISE(JUNGLESTEP, 154,110,77,254, 2);

       SETCOLORNOISE(UP_WOODSTEP2, 157,128,79,254, 11);
       SETCOLORNOISE(UP_PINESTEP, 103,77,46,255, 1);
       SETCOLORNOISE(UP_BIRCHSTEP, 195,179,123,255, 3);
       SETCOLORNOISE(UP_JUNGLESTEP, 154,110,77,255, 2);

       SETCOLORNOISE(226, 103,77,46,255, 1);
       SETCOLORNOISE(227, 195,179,123,255, 3);
       SETCOLORNOISE(228, 154,110,77,255, 2);

       SETCOLOR(237, 70,50,32, 255); // Pine trunk
       SETCOLORNOISE(238, 206,206,201, 255, 5); // Birch trunk
       SETCOLOR(239, 122,91,51, 255); // Jungle trunk
       SETCOLOR(240, 244,137,54, 255); // Dyed wool
       SETCOLOR(241, 200,75,210,255);
       SETCOLOR(242, 120,158,241, 255);
       SETCOLOR(243, 204,200,28, 255);
       SETCOLOR(244, 59,210,47, 255);
       SETCOLOR(245, 237,141,164, 255);
       SETCOLOR(246, 76,76,76, 255);
       SETCOLOR(247, 168,172,172, 255);
       SETCOLOR(248, 39,116,149, 255);
       SETCOLOR(249, 133,53,195, 255);
       SETCOLOR(250, 38,51,160, 255);
       SETCOLOR(251, 85,51,27, 255);
       SETCOLOR(252, 55,77,24, 255);
       SETCOLOR(253, 173,44,40, 255);
       SETCOLOR(254, 32,27,27, 255);

    //1.3.1+ various
    SETCOLOR(133, 61, 255, 61, 255  ); //emerald
    SETCOLOR(207, 240, 238, 232, 255 ); //quartz slab
    //SETCOLOR(159, 209, 177, 160, 255 ); //white stained clay !
    SETCOLOR(206, 54, 24, 30, 255 ); //nether bricks slab

    //1.8+
    SETCOLOR(140, 208, 128, 128, 255  ); //granite
    SETCOLOR(144, 228, 228, 228, 255  ); //diorite
    SETCOLOR(157, 160, 160, 160, 255  ); //andesite
    SETCOLOR(209, 155, 63, 176, 255  ); //purpur blocks

    // carpets
    SETCOLOR(36 ,  255, 255, 255,  254   ); //White carpet
    SETCOLOR(68 ,  244, 137,  54,  254   ); //Orange carpet
    SETCOLOR(69 ,  200,  75, 210,  254   ); //Magenta carpet
    SETCOLOR(70 ,  120, 158, 241,  254   ); //Light Blue carpet
    SETCOLOR(72 ,  204, 200,  28,  254   ); //Yellow carpet
    SETCOLOR(77 ,   59, 210,  47,  254   ); //Lime carpet
    SETCOLOR(131,  237, 141, 164,  254   ); //Pink carpet
    SETCOLOR(132,   76,  76,  76,  254   ); //Gray carpet
    SETCOLOR(141,  168, 172, 172,  254   ); //Light Gray
    SETCOLOR(142,   39, 116, 149,  254   ); //Cyan carpet
    SETCOLOR(143,  133,  53, 195,  254   ); //Purple carpet
    SETCOLOR(147,   38,  51, 160,  254   ); //Blue carpet
    SETCOLOR(148,   85,  51,  27,  254   ); //Brown carpet
    SETCOLOR(149,   55,  77,  24,  254   ); //Green carpet
    SETCOLOR(150,  173,  44,  40,  254   ); //Red carpet
    SETCOLOR(158,   32,  27,  27,  254   ); //Black carpet

    // clays
    SETCOLOR(159,  241, 210, 192, 255   ); //White Stained Clay
    SETCOLOR(186,  194, 116,  69, 255   ); //Orange Stained Clay
    SETCOLOR(187,  182, 120, 140, 255   ); //Magenta Stained Clay
    SETCOLOR(188,  141, 137, 167, 255   ); //Light Blue Stained Clay
    SETCOLOR(189,  219, 165,  66, 255   ); //Yellow Stained Clay
    SETCOLOR(190,  137, 149,  84, 255   ); //Lime Stained Clay
    SETCOLOR(191,  194, 110, 110, 255   ); //Pink Stained Clay
    SETCOLOR(192,   97,  82,  75, 255   ); //Gray Stained Clay
    SETCOLOR(193,  168, 138, 128, 255   ); //Light Gray Stained Clay
    SETCOLOR(194,  119, 122, 122, 255   ); //Cyan Stained Clay
    SETCOLOR(195,  152, 102, 117, 255   ); //Purple Stained Clay
    SETCOLOR(196,  103,  88, 120, 255   ); //Blue Stained Clay
    SETCOLOR(197,  109,  82,  66, 255   ); //Brown Stained Clay
    SETCOLOR(198,  105, 112,  70, 255   ); //Green Stained Clay
    SETCOLOR(199,  176,  93,  78, 255   ); //Red Stained Clay
    SETCOLOR(200,   67,  52,  46, 255   ); //Black Stained Clay


    // glass
    SETCOLOR(95 , 255, 255,  255,  100  ); //White Stained Glass
    SETCOLOR(160, 255, 255,  255,  100  ); //White Stained Glass pane
    SETCOLOR(234,  244, 137,  54,  40   ); //Orange Stained Glass [pane]
    SETCOLOR(225,  200,  75, 210,  40   ); //Magenta Stained Glass [pane]
    SETCOLOR(255,  120, 158, 241,  40   ); //Light Blue Stained Glass [pane]
    SETCOLOR(166,  204, 200,  28,  40   ); //Yellow Stained Glass [pane]
    SETCOLOR(167,   59, 210,  47,  40   ); //Lime Stained Glass [pane]
    SETCOLOR(168,  237, 141, 164,  40   ); //Pink Stained Glass [pane]
    SETCOLOR(169,   76,  76,  76,  40   ); //Gray Stained Glass [pane]
    SETCOLOR(178,  168, 172, 172,  40   ); //Light Gray Stained Glass [pane]
    SETCOLOR(179,   39, 116, 149,  40   ); //Cyan Stained Glass [pane]
    SETCOLOR(180,  133,  53, 195,  40   ); //Purple Stained Glass [pane]
    SETCOLOR(181,   38,  51, 160,  40   ); //Blue Stained Glass [pane]
    SETCOLOR(182,   85,  51,  27,  40   ); //Brown Stained Glass [pane]
    SETCOLOR(183,   55,  77,  24,  40   ); //Green Stained Glass [pane]
    SETCOLOR(184,  173,  44,  40,  40   ); //Red Stained Glass [pane]
    SETCOLOR(185,   32,  27,  27,  40   ); //Black Stained Glass [pane]

    // flowers
    //	SETCOLOR(165, 120, 158, 241, 254   ); //BLUE_ORCHID 165
    SETCOLOR(176, 200,  75, 210, 254 ); //ALLIUM 176
    SETCOLOR(235, 168, 172, 172, 254 ); //AZURE_BLUET 235
    // 38, 173  44  40 254, ); //RED_TULIP 38
    SETCOLOR(217, 244, 137,  54, 254 ); //ORANGE_TULIP 217
    SETCOLOR(218, 255, 255, 255, 254 ); //WHITE_TULIP 218
    SETCOLOR(219, 237, 141, 164, 254 ); //PINK_TULIP 219
    SETCOLOR(220, 168, 172, 172, 254 ); //OXEYE_DAISY 220
    // 37, 255 255  0 254, ); //SUNFLOWER 37
    SETCOLOR(233, 200,  75, 210, 254 ); //LILAC 233
    SETCOLOR(177, 237, 141, 164, 254 ); //PEONY 177

    // nether
    //SETCOLORNOISE(238,  206, 206, 201,  255,   5); // Birch Wood / quartz slab (sic!)
    */

	//SETCOLOR(UP_STEP, 200,200,200,254); // Not fully opaque to prevent culling on this one // WHAT ? 208
	//SETCOLOR(BRICKSTEP, 170,86,62,254); // What ? 
	//SETCOLOR(UP_BRICKSTEP, 170,86,62,254); // What ? 
	//SETCOLORNOISE(STONEBRICKSTEP, 122,122,122,254, 7);
	//SETCOLORNOISE(UP_STONEBRICKSTEP, 122,122,122,254, 7);

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
	char *ptr = buffer;
	while (*ptr == ' ' || *ptr == '\t') {
	    ++ptr;
	}
	if (*ptr == '\0' || *ptr == '#' || *ptr == '\12') {
	    continue;   // This is a comment or empty line, skip
	}
	int blockid = atoi(ptr);
	if (blockid < 1 || blockid > 255) {
	    printf("Skipping invalid blockid %d in colors file\n", blockid);
	    continue;
	}
	while (*ptr != ' ' && *ptr != '\t' && *ptr != '\0') {
	    ++ptr;
	}
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
    for (size_t i = 1; i < 255; ++i) {
	uint8_t *c = colors[i];
	if (i % 15 == 1) {
	    fprintf(f, "#ID    R   G   B    A  Noise\n");
	}
	fprintf(f, "%3d  %3d %3d %3d  %3d  %3d\n", int(i), int(c[PRED]), int(c[PGREEN]), int(c[PBLUE]), int(c[PALPHA]), int(c[NOISE]));
	fprintf(f, "%3d %3d %3d %3d %3d %3d %3d %3d\n", int(c[0]), int(c[1]), int(c[2]), int(c[3]), int(c[4]), int(c[5]), int(c[6]), int(c[7]));
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

uint8_t* getColor(Block b) {
    uint8_t var = getVariant(b);
    uint8_t* ret = colors[getId(b)];

    //printf("Block: %3d, variant: %3d [%3d, %3d, %3d, %3d, %3d, %3d, %3d, %3d]\n", b - 256*var, var, ret[0], ret[1], ret[2], ret[3], ret[4], ret[5], ret[6], ret[7]);

    // For blocks that can have several states (eg logs axis, half-slabs if on top or bottom)
    // get the max number of variants (set with SETNBVAR) and get the real variant of the block
    // by taking the modulo of the variant and max variant
    if (ret[VINDEX] && var && colors[255 + ret[VINDEX]][VINDEX]) {
	var = var % colors[255 + ret[VINDEX]][VINDEX];
	//printf("Max variants set (%d): new variant: %d\n", colors[255 + ret[VINDEX]][VINDEX], var);
    }

    // The above method works for everything but quartz pillars, so check manually
    if (b - 256*var == QUARTZ_BLOCK && var > 2)
	var = 2;

    // Double height flowers have a fixed variant value when on top
    // I use the g_LastDoubleFlower to get the right color
    if (b - 256*var == DOUBLE_PLANT) {
	if (var == 10)
	    var = g_LastDoubleFlower;
	else
	    g_LastDoubleFlower = var;
    }

    if (var && ret[VINDEX])
	ret = colors[255 + ret[VINDEX] + var];

    return ret;
}

uint8_t getId(Block b) {return b % 256;}
uint8_t getVariant(Block b) {return (b - getId(b))/256;}
